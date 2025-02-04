/* pnvl_module.h - Definitions for the kernel module
 *
 * Author: David Cañadas López <dcanadas@bsc.es>
 *
 */

#ifndef _PNVL_MODULE_H_
#define _PNVL_MODULE_H_

#include "hw/pnvl_hw.h"
#include "sw/module/pnvl_ioctl.h"
#include <linux/cdev.h>
#include <linux/pci.h>
#include <linux/wait.h>

#define PNVL_MODE_ACTIVE 1
#define PNVL_MODE_PASSIVE 0

/* forward declaration */
struct pnvl_dev;

struct pnvl_bar {
	u64 start;
	u64 end;
	u64 len;
	void __iomem *mmio;
};

struct pnvl_dma {
	dma_addr_t *dma_handles;
	size_t offset;
	size_t len;
	size_t npages;
	int mode;
	enum dma_data_direction direction;
	struct page *pages[PNVL_HW_BAR0_DMA_HANDLES_CNT];
};

struct pnvl_irq {
	void __iomem *mmio_ack_irq;
	int irq_num;
};

struct pnvl_dev {
	bool sending; // PNVL_IOCTL_SEND in progress
	bool recving; // PNVL_IOCTL_RECV in progress
	int wq_flag;
	wait_queue_head_t wq;
	struct pci_dev *pdev;
	struct pnvl_bar bar;
	struct pnvl_irq irq;
	struct pnvl_dma dma;
	struct pnvl_data data;
	dev_t minor;
	dev_t major;
	struct cdev cdev;
};

bool pnvl_dma_check_size_avail(struct pnvl_dev *pnvl_dev);
int pnvl_dma_pin_pages(struct pnvl_dev *pnvl_dev);
int pnvl_dma_get_handles(struct pnvl_dev *pnvl_dev);
void pnvl_dma_write_params(struct pnvl_dev *pnvl_dev);
void pnvl_dma_doorbell_ring(struct pnvl_dev *pnvl_dev);
void pnvl_dma_mode_active(struct pnvl_dev *pnvl_dev);
void pnvl_dma_mode_passive(struct pnvl_dev *pnvl_dev);
void pnvl_dma_mode_off(struct pnvl_dev *pnvl_dev);
void pnvl_dma_dismantle(struct pnvl_dev *pnvl_dev);
void pnvl_dma_wait(struct pnvl_dev *pnvl_dev);
void pnvl_dma_wake(struct pnvl_dev *pnvl_dev);

int pnvl_irq_enable(struct pnvl_dev *pnvl_dev);

#endif /* _PNVL_MODULE_H_ */
