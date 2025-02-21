/* pnvl_hw.h - Hardware resources description
 *
 * Author: David Cañadas López <dcanadas@bsc.es>
 *
 */

#pragma once

/* ============================================================================
 * Device info
 * ============================================================================
 */

#define PNVL_HW_VENDOR_ID 0x1b36
#define PNVL_HW_DEVICE_ID 0x1100
#define PNVL_HW_REVISION 0x01

#define PNVL_HW_BAR0 0
#define PNVL_HW_BAR_CNT 1

/* ============================================================================
 * Flags for kernel module
 * ============================================================================
 */

#define PNVL_FLAG_CLR 0
#define PNVL_FLAG_FIN (1 << 0) /* transaction finished */
#define PNVL_FLAG_SPC (1 << 1) /* not enough space for handles */
#define PNVL_FLAG_UPD (1 << 2) /* handles update required */
#define PNVL_FLAG_RST (1 << 3) /* handles reset required */
#define PNVL_FLAG_RET (1 << 4) /* awaiting return */
#define PNVL_FLAG_SKP (1 << 5) /* skip synchronization */

#define PNVL_FLAGS_ALL (PNVL_FLAG_FIN|PNVL_FLAG_SPC|PNVL_FLAG_UPD| \
		PNVL_FLAG_RST|PNVL_FLAG_RET|PNVL_FLAG_SKP)
/* when to stop transaction */
#define PNVL_FLAGS_EXIT (PNVL_FLAG_FIN|PNVL_FLAG_UPD)
/* request for handle update */
#define PNVL_FLAGS_NEED (PNVL_FLAG_UPD|PNVL_FLAG_SPC)

/* ============================================================================
 * MMIO
 * ============================================================================
 */

#define PNVL_HW_BAR0_IRQ_0_RAISE	0x00
#define PNVL_HW_BAR0_IRQ_0_LOWER	0x08
#define PNVL_HW_BAR0_FLAGS		0x10
#define PNVL_HW_BAR0_DMA_CFG_LEN	0x18
#define PNVL_HW_BAR0_DMA_CFG_PGS	0x20
#define PNVL_HW_BAR0_DMA_CFG_MOD	0x28
#define PNVL_HW_BAR0_DMA_CFG_LEN_AVAIL	0x30
#define PNVL_HW_BAR0_DMA_DOORBELL_RING	0x38
#define PNVL_HW_BAR0_DMA_HANDLES	0x40

/* 512 for a space of 2MB (if host PAGE_SIZE is 4KB) */
#define PNVL_HW_BAR0_DMA_MAX_PAGES 512
#define PNVL_HW_BAR0_DMA_MAX_LEN (PNVL_HW_BAR0_DMA_MAX_PAGES * PAGE_SIZE)
#define PNVL_HW_BAR0_DMA_HANDLES_CNT PNVL_HW_BAR0_DMA_MAX_PAGES

#define PNVL_HW_BAR0_START PNVL_HW_BAR0_IRQ_0_RAISE
#define PNVL_HW_BAR0_END \
	(PNVL_HW_BAR0_DMA_HANDLES + 64 * PNVL_HW_BAR0_DMA_HANDLES_CNT)

#define PNVL_HW_DMA_ADDR_CAPABILITY 32
#define PNVL_HW_DMA_AREA_START (PNVL_HW_BAR0_END + 0x1000)
#define PNVL_HW_DMA_AREA_SIZE 0x1000

/* ============================================================================
 * IRQs
 * ============================================================================
 */

#define PNVL_HW_IRQ_CNT 1
#define PNVL_HW_IRQ_VECTOR_START 0
#define PNVL_HW_IRQ_VECTOR_END 0
#define PNVL_HW_IRQ_INTX 0

#define PNVL_HW_IRQ_WORK_ENDED_VECTOR 0
#define PNVL_HW_IRQ_WORK_ENDED_ADDR PNVL_HW_BAR0_IRQ_0_RAISE
#define PNVL_HW_IRQ_WORK_ENDED_ACK_ADDR PNVL_HW_BAR0_IRQ_0_LOWER
