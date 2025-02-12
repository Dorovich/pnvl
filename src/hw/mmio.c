/* mmio.c - Memory Mapped IO operations
 *
 * Author: David Cañadas López <dcanadas@bsc.es>
 *
 */

#include "qemu/osdep.h"
#include "exec/target_page.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "qemu/units.h"
#include "mmio.h"
#include "irq.h"
#include "pnvl_hw.h"

/* ============================================================================
 * Private
 * ============================================================================
 */

static inline bool pnvl_mmio_valid_access(hwaddr addr, unsigned int size)
{
	return (PNVL_HW_BAR0_START <= addr && addr <= PNVL_HW_BAR0_END);
}

static inline int pnvl_mmio_handle_pos(hwaddr addr)
{
	return ((addr - PNVL_HW_BAR0_DMA_HANDLES) / sizeof(uint32_t));
}

static void pnvl_mmio_write_handle(DMAEngine *dma, hwaddr addr, uint64_t hnd)
{
	int pos = pnvl_mmio_handle_pos(addr);

	/* if (pos >= dma->config.npages || !dma->config.handles) */
	if (pos >= dma->config.npages)
		return;

	dma->config.handles[pos] = hnd;
	printf("New handle: %#010lx @ %#010lx (pos=%d)\n", hnd, addr, pos);
}

/*
static void pnvl_mmio_alloc_handles(DMAEngine *dma, uint64_t npages)
{
	if (dma->config.npages >= npages)
		return;
	if (dma->config.handles)
		free(dma->config.handles);
	dma->config.handles = malloc(npages);
}
*/

static uint64_t pnvl_mmio_read(void *opaque, hwaddr addr, unsigned int size)
{
	PNVLDevice *dev = opaque;
	uint64_t val = ~0ULL;

	if (!pnvl_mmio_valid_access(addr, size))
		goto mmio_read_end;

	switch(addr) {
	case PNVL_HW_BAR0_DMA_CFG_LEN:
		val = dev->dma.config.len;
		break;
	case PNVL_HW_BAR0_DMA_CFG_PGS:
		val = dev->dma.config.npages;
		break;
	case PNVL_HW_BAR0_DMA_CFG_MOD:
		val = dev->dma.mode;
		break;
	case PNVL_HW_BAR0_DMA_CFG_LEN_AVAIL:
		if (dev->dma.mode == DMA_MODE_ACTIVE) {
			pnvl_proxy_issue_req(dev, PNVL_REQ_SLN);
			pnvl_proxy_wait_and_handle_req(dev);
		}
		val = dev->dma.config.len_avail;
		break;
	}

mmio_read_end:
	return val;
}

static void pnvl_mmio_write(void *opaque, hwaddr addr, uint64_t val,
				unsigned int size)
{
	PNVLDevice *dev = opaque;
	DMAEngine *dma = &dev->dma;

	if (!pnvl_mmio_valid_access(addr, size))
		return;

	if (!pnvl_dma_is_idle(dev))
		return;

	switch(addr) {
	case PNVL_HW_BAR0_IRQ_0_RAISE:
		pnvl_irq_raise(dev, 0);
		break;
	case PNVL_HW_BAR0_IRQ_0_LOWER:
		pnvl_irq_lower(dev, 0);
		break;
	case PNVL_HW_BAR0_DMA_CFG_LEN:
		dma->config.len = val;
		break;
	case PNVL_HW_BAR0_DMA_CFG_PGS:
		/* pnvl_mmio_alloc_handles(dma, val); */
		dma->config.npages = val;
		break;
	case PNVL_HW_BAR0_DMA_CFG_MOD:
		dma->mode = val > 0 ? DMA_MODE_ACTIVE : DMA_MODE_PASSIVE;
		break;
	case PNVL_HW_BAR0_DMA_CFG_LEN_AVAIL:
		dma->config.len_avail = val;
		break;
	case PNVL_HW_BAR0_DMA_DOORBELL_RING:
		pnvl_execute(dev);
		break;
	default: /* DMA handles area */
		pnvl_mmio_write_handle(dma, addr, val);
		break;
	}
}

/* ============================================================================
 * Public
 * ============================================================================
 */

void pnvl_mmio_reset(PNVLDevice *dev)
{
	return;
}

void pnvl_mmio_init(PNVLDevice *dev, Error **errp)
{
	memory_region_init_io(&dev->mmio, OBJECT(dev), &pnvl_mmio_ops, dev,
			"pnvl-mmio", qemu_target_page_size());

	pci_register_bar(&dev->pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY,
			&dev->mmio);
}

void pnvl_mmio_fini(PNVLDevice *dev)
{
	pnvl_mmio_reset(dev);
}

const MemoryRegionOps pnvl_mmio_ops = {
	.read = pnvl_mmio_read,
	.write = pnvl_mmio_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
	.valid = {
		.min_access_size = 4,
		.max_access_size = 8,
	},
	.impl = {
		.min_access_size = 4,
		.max_access_size = 8,
	},
};
