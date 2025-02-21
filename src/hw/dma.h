/* dma.h - Direct Memory Access (DMA) operations
 *
 * Author: David Cañadas López <dcanadas@bsc.es>
 *
 */

#ifndef PNVL_DMA_H
#define PNVL_DMA_H

#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "pnvl_hw.h"

#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : ((1ULL << (n)) - 1))

/* forward declaration */
typedef struct PNVLDevice PNVLDevice;

typedef dma_addr_t dma_size_t;
typedef uint64_t dma_mask_t;

typedef struct DMAConfig {
	dma_size_t npages;
	dma_size_t len;
	dma_size_t len_avail;
	dma_mask_t mask;
	size_t page_size;
	dma_addr_t handles[PNVL_HW_BAR0_DMA_HANDLES_CNT];
} DMAConfig;

typedef struct DMACurrent {
	dma_size_t len_left;
	dma_addr_t addr_next;
	int hnd_pos;
} DMACurrent;

typedef enum DMAStatus {
	DMA_STATUS_IDLE,
	DMA_STATUS_EXECUTING,
	DMA_STATUS_OFF,
} DMAStatus;

typedef enum DMAMode {
	DMA_MODE_ACTIVE,
	DMA_MODE_PASSIVE,
} DMAMode;

typedef struct DMAEngine {
	DMAConfig config;
	DMACurrent current;
	DMAStatus status;
	DMAMode mode;
	uint8_t buff[PNVL_HW_DMA_AREA_SIZE];
} DMAEngine;

/* ============================================================================
 * Public
 * ============================================================================
 */

int pnvl_dma_rx_page(PNVLDevice *dev);
int pnvl_dma_tx_page(PNVLDevice *dev, int len_want);

int pnvl_dma_begin_run(PNVLDevice *dev);
void pnvl_dma_end_run(PNVLDevice *dev);
void pnvl_dma_add_handle(PNVLDevice *dev, dma_addr_t handle);
bool pnvl_dma_is_idle(PNVLDevice *dev);
bool pnvl_dma_need_handles(PNVLDevice *dev);
bool pnvl_dma_is_finished(PNVLDevice *dev);

void pnvl_dma_reset(PNVLDevice *dev);
void pnvl_dma_init(PNVLDevice *dev, Error **errp);
void pnvl_dma_fini(PNVLDevice *dev);

#endif /* PNVL_DMA_H */
