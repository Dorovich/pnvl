/* pnvl_irq.c - pnvl virtual device DMA operations
 *
 * Author: David Cañadas López <dcanadas@bsc.es>
 *
 */

#include "hw/pnvl_hw.h"
#include "pnvl_module.h"
#include <linux/dma-mapping.h>

/* Beware of double evaluation of arguments */
#define CMIN(a, b) ((a)<(b)?(a):(b))

static dma_addr_t pnvl_dma_mru_handle(struct pnvl_dev *pnvl_dev)
{
	void __iomem *mmio = pnvl_dev->bar.mmio;
	return (dma_addr_t)ioread32(mmio + PNVL_HW_BAR0_DMA_CFG_MRU);
}

bool pnvl_dma_finished(struct pnvl_dev *pnvl_dev)
{
	void __iomem *mmio = pnvl_dev->bar.mmio;
	return ioread32(mmio + PNVL_HW_BAR0_DMA_FINI);
}

void pnvl_dma_update_handles(struct pnvl_dev *pnvl_dev)
{
	struct pnvl_dma *dma = &pnvl_dev->dma;
	void __iomem *mmio = pnvl_dev->bar.mmio;
	size_t rem_hnds, ofs = sizeof(u32);
	int last_pos_hnd = dma->pos_hnd;

	iowrite32((u32)pnvl_dma_mru_handle(pnvl_dev),
			mmio + PNVL_HW_BAR0_DMA_HANDLES);

	rem_hnds = dma->npages - dma->pos_hnd;
	dma->pos_hnd += CMIN(rem_hnds, PNVL_HW_BAR0_DMA_MAX_PAGES);
	for (int i = last_pos_hnd; i < dma->pos_hnd; ++i) {
		iowrite32((u32)dma->dma_handles[i],
			mmio + PNVL_HW_BAR0_DMA_HANDLES + ofs);
		ofs += sizeof(u32);
	}
}

int pnvl_dma_pin_pages(struct pnvl_dev *pnvl_dev)
{
	int first_page, last_page, npages, pinned = 0;
	struct pnvl_data *data = &pnvl_dev->data;
	struct page **pages;

	first_page = (data->addr & PAGE_MASK) >> PAGE_SHIFT;
	last_page = ((data->addr + data->len - 1) & PAGE_MASK) >> PAGE_SHIFT;
	npages = last_page - first_page + 1;
	pages = kmalloc(npages * sizeof(*pages), GFP_KERNEL);
	pinned = pin_user_pages_fast(data->addr, npages, FOLL_LONGTERM, pages);
	if (pinned < npages)
		goto err_dma_pin;

	pnvl_dev->dma.pages = pages;
	pnvl_dev->dma.npages = npages;
	return 0;

err_dma_pin:
	kfree(pages);
	return -1;
}

int pnvl_dma_get_handles(struct pnvl_dev *pnvl_dev)
{
	struct pci_dev *pdev = pnvl_dev->pdev;
	struct pnvl_dma *dma = &pnvl_dev->dma;
	dma_addr_t *handles;
	size_t len, len_map, ofs;

	handles = kmalloc(dma->npages * sizeof(*handles), GFP_KERNEL);

	len = pnvl_dev->data.len;
	ofs = pnvl_dev->data.addr & ~PAGE_MASK;
	len_map = PAGE_SIZE - ofs;
	if (len_map > len)
		len_map = len;
	len -= len_map;
	handles[0] = dma_map_page(&pdev->dev, dma->pages[0], ofs, len_map,
			dma->direction);
	if (dma_mapping_error(&pdev->dev, handles[0]))
		goto err_dma_map_mem;

	for (int i = 1; i < dma->npages; ++i) {
		len_map = len > PAGE_SIZE ?  PAGE_SIZE : len;
		len -= len_map;
		handles[i] = dma_map_page(&pdev->dev, dma->pages[i], 0,
				len_map, dma->direction);
		if (dma_mapping_error(&pdev->dev, handles[i]))
			goto err_dma_map_mem;
	}

	dma->dma_handles = handles;
	dma->pos_hnd = 0;
	return 0;

err_dma_map_mem:
	kfree(handles);
	return -ENOMEM;
}

void pnvl_dma_write_params(struct pnvl_dev *pnvl_dev)
{
	struct pnvl_dma *dma = &pnvl_dev->dma;
	void __iomem *mmio = pnvl_dev->bar.mmio;
	size_t ofs = 0;

	if (dma->mode == PNVL_MODE_PASSIVE) {
		iowrite32((u32)pnvl_dev->data.len,
				mmio + PNVL_HW_BAR0_DMA_CFG_LEN_AVAIL);
	}

	iowrite32((u32)pnvl_dev->data.len, mmio + PNVL_HW_BAR0_DMA_CFG_LEN);
	iowrite32((u32)dma->npages, mmio + PNVL_HW_BAR0_DMA_CFG_PGS);
	iowrite32((u32)dma->mode, mmio + PNVL_HW_BAR0_DMA_CFG_MOD);

	dma->pos_hnd = CMIN(dma->npages, PNVL_HW_BAR0_DMA_MAX_PAGES);
	for (int i = 0; i < dma->pos_hnd; ++i) {
		iowrite32((u32)dma->dma_handles[i],
			mmio + PNVL_HW_BAR0_DMA_HANDLES + ofs);
		ofs += sizeof(u32);
	}
}

void pnvl_dma_doorbell_ring(struct pnvl_dev *pnvl_dev)
{
	void __iomem *mmio = pnvl_dev->bar.mmio;
	iowrite32(1, mmio + PNVL_HW_BAR0_DMA_DOORBELL_RING);
}

void pnvl_dma_dismantle(struct pnvl_dev *pnvl_dev)
{
	struct pnvl_dma *dma = &pnvl_dev->dma;

	for (int i = 0; i < dma->npages; ++i) {
		dma_unmap_page(&pnvl_dev->pdev->dev, dma->dma_handles[i],
			pnvl_dev->data.len, dma->direction);
		unpin_user_page(dma->pages[i]);
	}

	kfree(dma->dma_handles);
	kfree(dma->pages);
}

void pnvl_dma_wait(struct pnvl_dev *pnvl_dev)
{
	if (!pnvl_dev->wq_flag)
		wait_event(pnvl_dev->wq, pnvl_dev->wq_flag == 1);
	pnvl_dev->wq_flag = 0;
}

void pnvl_dma_wake(struct pnvl_dev *pnvl_dev)
{
	pnvl_dev->wq_flag = 1;
	wake_up(&pnvl_dev->wq);
}
