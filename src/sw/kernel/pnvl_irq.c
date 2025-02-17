/* pnvl_irq.c - pnvl virtual device IRQ operations
 *
 * Author: David Cañadas López <dcanadas@bsc.es>
 *
 */

#include "hw/pnvl_hw.h"
#include "pnvl_module.h"
#include <linux/pci.h>

static inline void pnvl_irq_ack(struct pnvl_dev *pnvl_dev)
{
	iowrite32(1, pnvl_dev->irq.mmio_ack_irq);
}

static irqreturn_t pnvl_irq_handler(int irq, void *data)
{
	struct pnvl_dev *pnvl_dev = data;

	pnvl_irq_ack(pnvl_dev);
	if (pnvl_dma_finished(pnvl_dev)) {
		pnvl_dma_wake(pnvl_dev);
	} else {
		pnvl_dma_update_handles(pnvl_dev);
		pnvl_dma_doorbell_ring(pnvl_dev);
	}

	dev_dbg(&pnvl_dev->pdev->dev, "irq_handler irq = %d dev = %d\n", irq,
		pnvl_dev->major);

	printk(KERN_DEBUG "pnvl interrupt received - handling\n");

	return IRQ_HANDLED;
}

static int pnvl_irq_enable_vectors(struct pnvl_dev *pnvl_dev)
{
	int irq_vecs_req, irq_vecs, err = 0;

	irq_vecs_req = min_t(int, pci_msi_vec_count(pnvl_dev->pdev),
			     num_online_cpus() + 1);
	irq_vecs = pci_alloc_irq_vectors(pnvl_dev->pdev, 1,
					 irq_vecs_req, PCI_IRQ_ALL_TYPES);

	if (irq_vecs < 0)
		return -ENOSPC;
	if (irq_vecs != irq_vecs_req) {
		err = -ENOSPC;
		goto err_clean_irqs;
	}

	pnvl_dev->irq.irq_num = pci_irq_vector(pnvl_dev->pdev,
			PNVL_HW_IRQ_WORK_ENDED_VECTOR);
	if (pnvl_dev->irq.irq_num < 0) {
		err = -EINVAL;
		goto err_clean_irqs;
	}

	err = request_irq(pnvl_dev->irq.irq_num, pnvl_irq_handler,
			  PNVL_HW_IRQ_WORK_ENDED_VECTOR,
			  "pnvl_irq_dma_ended", pnvl_dev);
	if (err)
		goto err_clean_irqs;

	pnvl_dev->irq.mmio_ack_irq =
		pnvl_dev->bar.mmio + PNVL_HW_IRQ_WORK_ENDED_ACK_ADDR;
	return 0;

err_clean_irqs:
	pci_free_irq_vectors(pnvl_dev->pdev);
	return err;
}

int pnvl_irq_enable(struct pnvl_dev *pnvl_dev)
{
	if (!pci_msi_enabled())
		return -1;
	return pnvl_irq_enable_vectors(pnvl_dev);
}
