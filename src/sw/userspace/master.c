/* master.c - Userspace program to test the device (offload operation)
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

#define VEC_LEN 2000000

static int offload_work(int fd, void *addr, size_t len)
{
	struct timeval t1, t2, t3;
	struct pnvl_data data = {
		.addr = (unsigned long)addr,
		.len = (unsigned long)len,
	};

	puts("Sending data...");

	gettimeofday(&t1, NULL);
	if (ioctl(fd, PNVL_IOCTL_SEND, &data) < 0) {
		perror("PNVL_IOCTL_SEND failed!");
		return -1;
	}
	gettimeofday(&t2, NULL);

	printf("PNVL_IOCTL_SEND: %ld us elapsed\n", calc_time(&t1, &t2));

	gettimeofday(&t2, NULL);
	if (ioctl(fd, PNVL_IOCTL_WAIT) < 0) {
		perror("PNVL_IOCTL_WAIT failed!");
		return -1;
	}
	gettimeofday(&t3, NULL);

	printf("PNVL_IOCTL_WAIT: %ld us elapsed\n", calc_time(&t2, &t3));
	printf("TOTAL: %ld us elapsed\n", calc_time(&t1, &t3));

	puts("Checking results...");

	int errcnt = 0;
	for (int i = 0; i < data.len/sizeof(int); ++i) {
		if (((int *)data.addr)[i] != i)
			errcnt++;
	}

	printf("%d errors counted.\n", errcnt);

	return 0;
}

int main(int argc, char **argv)
{
	struct context ctx = parse_args(argc, argv);
	int *data;
	size_t data_len;

	if (open_pnvl_dev(&ctx) < 0)
		return -1;

	data_len = VEC_LEN * sizeof(int);
	data = malloc(data_len);
	memset(data, 0, data_len);

	if (offload_work(ctx.fd, data, data_len)) {
		close(ctx.fd);
		return -1;
	}

	close(ctx.fd);
	return 0;
}
