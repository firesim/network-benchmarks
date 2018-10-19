#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mmio.h"
#include "nic.h"
#include "encoding.h"
#include "common.h"

//#define NBYTES (2 * 1024L * 1024L * 1024L)
#define NPACKETS ((NBYTES - 1) / PACKET_BYTES + 1)

static void distribute_data(uint64_t srcmac)
{
	uint64_t dstmac;

	for (int i = 0; i < NCOMPUTE; i++) {
		dstmac = macaddr_add(srcmac, i+1);
		send_data_loop(srcmac, dstmac, NBYTES);
	}
}

int main(void)
{
	uint64_t srcmac;

	srcmac = nic_macaddr();

	printf("start test\n");

	for (int i = 0; i < NROUNDS; i++) {
		distribute_data(srcmac);

		recv_data_loop(srcmac, NBYTES * NCOMPUTE, NPACKETS * NCOMPUTE);
	}

	printf("finished\n");

	return 0;
}
