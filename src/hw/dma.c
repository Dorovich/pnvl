/* dma.c - Direct Memory Access (DMA) operations
 *
 * Author: David Cañadas López <dcanadas@bsc.es>
 *
 */

#include "qemu/osdep.h"
#include "exec/target_page.h"
#include "qemu/log.h"
#include "pnvl.h"
#include "dma.h"

/* ============================================================================
 * Private
 * ============================================================================
 */

static inline dma_addr_t pnvl_dma_mask(DMAEngine *dma, dma_addr_t addr)
{
	dma_addr_t masked_addr = addr & dma->config.mask;
	if (masked_addr != addr) {
		qemu_log_mask(LOG_GUEST_ERROR,
				"masked_addr (%" PRIx64 ") != addr (%" PRIx64 ")",
				masked_addr, addr);
	}
	return masked_addr;
}

static inline bool pnvl_dma_inside_dev_boundaries(dma_addr_t addr)
{
	return (PNVL_HW_DMA_AREA_START <= addr &&
			addr <= PNVL_HW_DMA_AREA_START + PNVL_HW_DMA_AREA_SIZE);
}

static inline void pnvl_dma_init_current(DMAEngine *dma)
{
	dma->current.len_left = dma->config.len;
	dma->current.addr = dma->config.handles[0];
	dma->current.hnd_pos = 0;
}

static int pnvl_dma_read(PNVLDevice *dev, dma_addr_t addr, int len, int ofs)
{
	//printf("DMA RD: %d bytes @ %#010lx\n", len, addr);
	return pci_dma_read(&dev->pci_dev, addr, dev->dma.buff + ofs, len);
}

static int pnvl_dma_write(PNVLDevice *dev, dma_addr_t addr, int len, int ofs)
{
	//printf("DMA WR: %d bytes @ %#010lx\n", len, addr);
	return pci_dma_write(&dev->pci_dev, addr, dev->dma.buff + ofs, len);
}


/* ============================================================================
 * Public
 * ============================================================================
 */

/*
 * Receive page: DMA buffer <-- RAM
 */
int pnvl_dma_rx_page(PNVLDevice *dev)
{
	DMAEngine *dma = &dev->dma;
	unsigned long mask = pnvl_dma_mask(dma, dma->config.page_size - 1);
	dma_addr_t addr = dma->current.addr;
	size_t ofs, len_want, len_have;

	ofs = addr & mask;
	len_have = dma->config.page_size - ofs;
	len_want = MIN(dma->config.page_size, dma->current.len_left);

	if (len_want <= len_have) {
		if (pnvl_dma_read(dev, addr, len_want, 0) < 0)
			return PNVL_FAILURE;
		addr += len_want;
	} else {
		if (pnvl_dma_read(dev, addr, len_have, 0) < 0)
			return PNVL_FAILURE;
		addr = dma->config.handles[++dma->current.hnd_pos];

		if (pnvl_dma_read(dev, addr, len_want-len_have, len_have) < 0)
			return PNVL_FAILURE;
		addr += len_want - len_have;
	}

	dma->current.addr = addr;
	dma->current.len_left -= len_want;
	return len_want;
}

/*
 * Transmit page: DMA buffer --> RAM
 */
int pnvl_dma_tx_page(PNVLDevice *dev, int len_want)
{
	DMAEngine *dma = &dev->dma;
	unsigned long mask = pnvl_dma_mask(dma, dma->config.page_size - 1);
	dma_addr_t addr = dma->current.addr;
	size_t ofs, len_have;

	ofs = addr & mask;
	len_have = dma->config.page_size - ofs;

	if (len_want == PNVL_FAILURE)
		return PNVL_FAILURE;

	if (len_want <= len_have) {
		if (pnvl_dma_write(dev, addr, len_want, 0) < 0)
			return PNVL_FAILURE;
		addr += len_want;
	} else {
		if (pnvl_dma_write(dev, addr, len_have, 0) < 0)
			return PNVL_FAILURE;
		addr = dma->config.handles[++dma->current.hnd_pos];

		if (pnvl_dma_write(dev, addr, len_want-len_have, len_have) < 0)
			return PNVL_FAILURE;
		addr += len_want - len_have;
	}

	dma->current.addr = addr;
	dma->current.len_left -= len_want;
	return PNVL_SUCCESS;
}

int pnvl_dma_begin_run(PNVLDevice *dev)
{
	DMAStatus status;

	pnvl_dma_init_current(&dev->dma);
	status = qatomic_cmpxchg(&dev->dma.status, DMA_STATUS_IDLE,
			DMA_STATUS_EXECUTING);
	if (status == DMA_STATUS_EXECUTING)
		return PNVL_FAILURE;

	return PNVL_SUCCESS;
}

void pnvl_dma_end_run(PNVLDevice *dev)
{
	qatomic_set(&dev->dma.status, DMA_STATUS_IDLE);
}

void pnvl_dma_add_handle(PNVLDevice *dev, dma_addr_t handle)
{
	DMAEngine *dma = &dev->dma;
	dma_addr_t new_hnd = pnvl_dma_mask(dma, handle);
	dma->config.handles[dma->config.npages++] = new_hnd;
}

bool pnvl_dma_is_idle(PNVLDevice *dev)
{
	return qatomic_read(&dev->dma.status) == DMA_STATUS_IDLE;
}

bool pnvl_dma_is_finished(PNVLDevice *dev)
{
	return !dev->dma.current.len_left;
}

void pnvl_dma_reset(PNVLDevice *dev)
{
	DMAEngine *dma = &dev->dma;
	dma->status = DMA_STATUS_IDLE;
	dma->config.npages = 0;
	dma->config.len = 0;
	dma->config.page_size = qemu_target_page_size();
	memset(dma->buff, 0, PNVL_HW_DMA_AREA_SIZE);
	memset(dma->config.handles, 0,
			sizeof(dma_addr_t) * PNVL_HW_BAR0_DMA_HANDLES_CNT);
}

void pnvl_dma_init(PNVLDevice *dev, Error **errp)
{
	pnvl_dma_reset(dev);
	dev->dma.config.mask = DMA_BIT_MASK(PNVL_HW_DMA_ADDR_CAPABILITY);
}

void pnvl_dma_fini(PNVLDevice *dev)
{
	pnvl_dma_reset(dev);
	dev->dma.status = DMA_STATUS_OFF;
}
