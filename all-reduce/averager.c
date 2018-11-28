#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mmio.h"
#include "nic.h"
#include "encoding.h"
#include "reduce.h"

//#define NBYTES (2 * 1024L * 1024L * 1024L)

static void distribute_data(uint64_t srcmac)
{
	send_data_loop(srcmac, BROADCAST_MAC, NBYTES);
}

static void collect_data(uint64_t srcmac)
{
	uint64_t dstmac;

	for (int i = 0; i < NCOMPUTE; i++) {
		dstmac = macaddr_add(srcmac, i+1);
		send_data_loop(srcmac, dstmac, 24);
		recv_data_loop(srcmac, NBYTES, 1);
	}
}

static void finish_test(uint64_t srcmac)
{
	uint64_t dstmac;

	for (int i = 0; i < NCOMPUTE; i++) {
		dstmac = macaddr_add(srcmac, i+1);
		send_data_loop(srcmac, dstmac, 24);
	}
}

int main(void)
{
	uint64_t srcmac;

	srcmac = nic_macaddr();

	printf("start test\n");

	for (int i = 0; i < NROUNDS; i++) {
		distribute_data(srcmac);
		collect_data(srcmac);
	}

	finish_test(srcmac);

	printf("finished\n");

	return 0;
}
