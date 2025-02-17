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
 * MMIO
 * ============================================================================
 */

#define PNVL_HW_BAR0_IRQ_0_RAISE 0x00
#define PNVL_HW_BAR0_IRQ_0_LOWER 0x08
#define PNVL_HW_BAR0_DMA_CFG_LEN 0x10
#define PNVL_HW_BAR0_DMA_CFG_PGS 0x18
#define PNVL_HW_BAR0_DMA_CFG_MOD 0x20
#define PNVL_HW_BAR0_DMA_CFG_LEN_AVAIL 0x28
#define PNVL_HW_BAR0_DMA_CFG_MRU 0x30
#define PNVL_HW_BAR0_DMA_DOORBELL_RING 0x38
#define PNVL_HW_BAR0_DMA_FINI 0x40
#define PNVL_HW_BAR0_DMA_HANDLES 0x48

#define PNVL_HW_BAR0_DMA_MAX_PAGES 512
#define PNVL_HW_BAR0_DMA_MAX_LEN (PNVL_HW_BAR0_DMA_MAX_PAGES * PAGE_SIZE)
/* 512 for a space of 2MB (if PAGE_SIZE is 4KB); 1 more if offset exists */
#define PNVL_HW_BAR0_DMA_HANDLES_CNT (PNVL_HW_BAR0_DMA_MAX_PAGES+1)

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
