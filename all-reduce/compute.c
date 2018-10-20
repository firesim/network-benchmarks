#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mmio.h"
#include "nic.h"
#include "encoding.h"
#include "reduce.h"

//#define NBYTES (2 * 1024L * 1024L * 1024L)
#define NPACKETS ((NBYTES - 1) / PACKET_BYTES + 1)
#define PAUSE_CYCLES (PAUSE_MS * 3200000L)

int main(void)
{
	uint64_t srcmac, dstmac;
	long end_cycle;

	srcmac = nic_macaddr();

	printf("start test\n");

	for (int i = 0; i < NROUNDS; i++) {
		printf("Round %d\n", i);
		recv_data_loop(srcmac, NBYTES, NPACKETS);

		end_cycle = rdcycle() + PAUSE_CYCLES;
		while (rdcycle() < end_cycle) {}

		dstmac = recv_data_loop(srcmac, 24, 1);
		send_data_loop(srcmac, dstmac, NBYTES);
	}

	recv_data_loop(srcmac, 24, 1);

	printf("finished\n");

	end_cycle = rdcycle() + PAUSE_CYCLES;
	while (rdcycle() < end_cycle) {}

	return 0;
}
