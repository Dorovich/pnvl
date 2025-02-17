/* pnvl.c - PCIe offload device emulation.
 *
 * Author: David Cañadas López <dcanadas@bsc.es>
 *
 */

#include "pnvl.h"
#include "pnvl_hw.h"
#include "dma.h"
#include "irq.h"
#include "mmio.h"
#include "proxy.h"
#include "qom/object.h"

/* ============================================================================
 * Object
 * ============================================================================
 */

static void pnvl_device_init(PCIDevice *pci_dev, Error **errp)
{
	PNVLDevice *dev = PNVL_DEVICE(pci_dev);
	pnvl_irq_init(dev, errp);
	pnvl_dma_init(dev, errp);
	pnvl_mmio_init(dev, errp);
	pnvl_proxy_init(dev, errp);
	dev->ret = false;
}

static void pnvl_device_fini(PCIDevice *pci_dev)
{
	PNVLDevice *dev = PNVL_DEVICE(pci_dev);
	pnvl_irq_fini(dev);
	pnvl_dma_fini(dev);
	pnvl_mmio_fini(dev);
	pnvl_proxy_fini(dev);
}

static void pnvl_device_reset(DeviceState *dev_st)
{
	PNVLDevice *dev = PNVL_DEVICE(dev_st);
	pnvl_irq_reset(dev);
	pnvl_dma_reset(dev);
	pnvl_mmio_reset(dev);
	pnvl_proxy_reset(dev);
	dev->ret = false;
}

/* ============================================================================
 * Class
 * ============================================================================
 */

static void pnvl_class_init(ObjectClass *klass, void *class_data)
{
	DeviceClass *dev_class = DEVICE_CLASS(klass);
	PCIDeviceClass *pci_dev_class = PCI_DEVICE_CLASS(klass);

	pci_dev_class->realize = pnvl_device_init;
	pci_dev_class->exit = pnvl_device_fini;
	pci_dev_class->vendor_id = PNVL_HW_VENDOR_ID;
	pci_dev_class->device_id = PNVL_HW_DEVICE_ID;
	pci_dev_class->revision = PNVL_HW_REVISION;
	pci_dev_class->class_id = PCI_CLASS_OTHERS;

	set_bit(DEVICE_CATEGORY_MISC, dev_class->categories);
	dev_class->desc = PNVL_DEVICE_DESC;
	dev_class->reset = pnvl_device_reset;
}

static void pnvl_instance_init(Object *obj)
{
	PNVLDevice *dev = PNVL(obj);

	dev->proxy.server_mode = true;
	object_property_add_bool(obj, "server_mode", pnvl_proxy_get_mode,
				pnvl_proxy_set_mode);

	dev->proxy.port = PNVL_PROXY_PORT;
	object_property_add_uint16_ptr(obj, "port", &dev->proxy.port,
				OBJ_PROP_FLAG_READWRITE);
}

/* ============================================================================
 * Type information
 * ============================================================================
 */

static const TypeInfo pnvl_info = {
	.name = TYPE_PNVL_DEVICE,
	.parent = TYPE_PCI_DEVICE,
	.instance_size = sizeof(PNVLDevice),
	.instance_init = pnvl_instance_init,
	.class_init = pnvl_class_init,
	.interfaces = (InterfaceInfo[]){
		{ INTERFACE_PCIE_DEVICE },
		{},
	},
};

static void pnvl_register_types(void)
{
	type_register_static(&pnvl_info);
}

type_init(pnvl_register_types)

/* ============================================================================
 * Private
 * ============================================================================
 */

static void pnvl_transfer_pages(PNVLDevice *dev)
{
	int ret;
	size_t len;

	if (pnvl_dma_begin_run(dev) < 0)
		return;
	printf("BEGIN pnvl_transfer_pages (len=%lu)\n",
			dev->dma.current.len_left);

	do {
		len = pnvl_dma_rx_page(dev);
		ret = pnvl_proxy_tx_page(dev, dev->dma.buff, len);
		if (ret == PNVL_FAILURE)
			break;
	} while (!pnvl_dma_is_finished(dev) && !pnvl_dma_need_handles(dev));

	pnvl_dma_end_run(dev);
	printf("DONE pnvl_transfer_pages (ret=%d)\n", ret);
}

static void pnvl_receive_pages(PNVLDevice *dev)
{
	int ret;
	size_t len;

	if (pnvl_dma_begin_run(dev) < 0)
		return;
	printf("BEGIN pnvl_receive_pages (len=%lu)\n",
			dev->dma.current.len_left);

	do {
		len = pnvl_proxy_rx_page(dev, dev->dma.buff);
		ret = pnvl_dma_tx_page(dev, len);
		if (ret == PNVL_FAILURE)
			break;
	} while (!pnvl_dma_is_finished(dev) && !pnvl_dma_need_handles(dev));

	pnvl_dma_end_run(dev);
	printf("DONE pnvl_receive_pages (ret=%d)\n", ret);
}

/* ============================================================================
 * Public
 * ============================================================================
 */

void pnvl_execute(PNVLDevice *dev)
{
	switch(dev->dma.mode) {
	case DMA_MODE_ACTIVE:
		pnvl_transfer_pages(dev);
		pnvl_receive_pages(dev);
		if (dev->ret)
			dev->ret = false;
		break;
	case DMA_MODE_PASSIVE:
		if (!dev->ret) {
			pnvl_proxy_await_req(dev, PNVL_REQ_SLN);
			pnvl_receive_pages(dev);
			dev->ret = true;
		} else {
			pnvl_transfer_pages(dev);
			dev->ret = false;
		}
		break;
	}
	pnvl_irq_raise(dev, PNVL_HW_IRQ_WORK_ENDED_VECTOR);
}
