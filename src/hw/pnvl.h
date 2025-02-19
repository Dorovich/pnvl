/* pnvl.h
 *
 * Author: David Cañadas López <dcanadas@bsc.es>
 *
 */

#ifndef PNVL_H
#define PNVL_H

#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_device.h"
#include "pnvl_hw.h"
#include "dma.h"
#include "irq.h"
#include "proxy.h"

#define TYPE_PNVL_DEVICE "pnvl"
#define PNVL_DEVICE_DESC "Proto-NVLink Device"

#define PNVL_SUCCESS 0
#define PNVL_FAILURE -1

OBJECT_DECLARE_TYPE(PNVLDevice, PNVLDeviceClass, PNVL_DEVICE);
DECLARE_INSTANCE_CHECKER(PNVLDevice, PNVL, TYPE_PNVL_DEVICE);

typedef struct PNVLDeviceClass {
	PCIDeviceClass parent_class;
} PNVLDeviceClass;

typedef struct PNVLDevice {
	PCIDevice pci_dev;
	IRQStatus irq;
	DMAEngine dma;
	MemoryRegion mmio;
	PNVLProxy proxy;
} PNVLDevice;


/* ============================================================================
 * Public
 * ============================================================================
 */

void pnvl_execute(PNVLDevice *dev);

#endif /* PNVL_H */
