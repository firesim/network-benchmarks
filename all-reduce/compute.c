#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mmio.h"
#include "nic.h"
#include "encoding.h"
#include "reduce.h"

//#define NBYTES (2 * 1024L * 1024L * 1024L)
#define PAUSE_CYCLES (PAUSE_MS * 3200000L)

int main(void)
{
	uint64_t srcmac, dstmac, revmac;
	long end_cycle;
	int is_last;

	srcmac = nic_macaddr();
	revmac = macaddr_reverse(srcmac);
	is_last = (revmac & 0xffff) == (2 + NCOMPUTE);

	printf("start test\n");

	for (int i = 0; i < NROUNDS; i++) {
		recv_data_loop(srcmac, NBYTES, is_last);

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
