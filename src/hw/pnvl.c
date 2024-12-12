/* pnvl.c - PCIe offload device emulation.
 *
 * Author: David Cañadas López <dcanadas@bsc.es>
 *
 */

#include "dma.h"
#include "irq.h"
#include "mmio.h"
#include "pnvl.h"
#include "pnvl_hw.h"
#include "proxy.h"
#include "qom/object.h"

/* ============================================================================
 * Object
 * ============================================================================
 */

static void pnvl_device_init(PCIDevice *pci_dev, Error **errp)
{
	PNVLDevice *dev = PNVL_DEVICE(pci_dev);
	pnvl_irq_init(dev);
	pnvl_dma_init(dev);
	pnvl_mmio_init(dev);
	pnvl_proxy_init(dev);
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
	.interfaces = (InferfaceInfo[]){
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
 * Public
 * ============================================================================
 */

/*
 * TODO: dma will use dma->current, some things will be obsolete
 */
void pnvl_transfer_pages(PNVLDevice *dev)
{
	dma_addr_t addr;
	size_t len_total = dev->dma.config.len;
	size_t len;
	int ret;
	int pos = 0;

	pnvl_dma_init_current(dev);

	do {
		addr = dev->dma.handles[pos];
		len = pnvl_dma_rx_page(dev, addr, &pos, len_total);
		ret = pnvl_proxy_tx_page(dev, dev->dma.buffer, len);
		if (ret == PNVL_FAILURE)
			return;
		len_total -= len;
	} while (len_total > 0);
}

/*
 * TODO: dma will use dma->current, some things will be obsolete
 */
void pnvl_receive_pages(PNVLDevice *dev)
{
	dma_addr_t addr;
	size_t len_total = dev->dma.config.len;
	size_t len;
	int ret;

	pnvl_dma_init_current(dev);

	do {
		len = pnvl_proxy_rx_page(dev, dev->dma.buffer);
		addr = dev->dma.handles[pos];
		ret = pnvl_dma_tx_page(dev, addr, &pos, len, len_total,
				page_size);
		if (ret == PNVL_FAILURE)
			return;
		len_total -= len;
	} while (len_total > 0);
}
