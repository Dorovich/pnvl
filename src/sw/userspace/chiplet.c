/* chiplet.c - Userspace program to test the device (compute operation)
 *
 * Author: David Cañadas López <dcanadas@bsc.es>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include "hw/pnvl_hw.h"
#include "sw/module/pnvl_ioctl.h"
#include "pnvl_util.h"

static int handle_work(int fd, void *addr, size_t len)
{
	struct timeval t1, t2, t3;
	struct pnvl_data data = {
		.addr = (unsigned long)addr,
		.len = (unsigned long)len,
	};

	puts("Waiting for data...");

	gettimeofday(&t1, NULL);
	if (ioctl(fd, PNVL_IOCTL_RECV, &data) < 0) {
		perror("PNVL_IOCTL_RECV failed!");
		return -1;
	}
	gettimeofday(&t2, NULL);

	printf("PNVL_IOCTL_RECV: %ld us elapsed\n", calc_time(&t1, &t2));

	for (int i = 0; i < data.len/sizeof(int); ++i)
		((int *)data.addr)[i] = i;

	gettimeofday(&t2, NULL);
	if (ioctl(fd, PNVL_IOCTL_RETURN) < 0) {
		perror("PNVL_IOCTL_RETURN failed!");
		return -1;
	}
	gettimeofday(&t3, NULL);

	printf("PNVL_IOCTL_RETURN: %ld us elapsed\n", calc_time(&t2, &t3));
	printf("TOTAL: %ld us elapsed\n", calc_time(&t1, &t3));

	puts("Results sent.");

	return 0;
}

int main(int argc, char **argv)
{
	struct context ctx = parse_args(argc, argv);
	int *data;

	if (!ctx.vec_len)
		return -1;

	if (open_pnvl_dev(&ctx) < 0)
		return -2;

	data = calloc(ctx.vec_len, sizeof(int));
	if (!data) {
		close(ctx.fd);
		return -3;
	}

	if (handle_work(ctx.fd, data, ctx.vec_len * sizeof(int))) {
		close(ctx.fd);
		return -4;
	}

	close(ctx.fd);
	return 0;
}
