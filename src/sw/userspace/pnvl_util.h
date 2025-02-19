/* pnvl_util.h - Utility functions for userspace programs
 *
 * Author: David Cañadas López <dcanadas@bsc.es>
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

struct context {
	int fd;			/* file descriptor of dev file */
	uint32_t pci_domain_nb;	/* PCI domain number of device (16 bits) */
	uint16_t pci_bus_nb;	/* PCI bus number of device (8 bits)  */
	uint8_t pci_device_nb;	/* PCI device number of device (5 bits) */
	uint8_t pci_func_nb;	/* PCI function number of device (3 bits) */
	size_t vec_len;		/* number of integers to transmit */
};

void usage(FILE * fd, char **argv);
int open_pnvl_dev(struct context *ctx);
struct context parse_args(int argc, char **argv);
long int calc_time(struct timeval *t1, struct timeval *t2);
