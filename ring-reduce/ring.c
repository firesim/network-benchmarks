#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mmio.h"
#include "nic.h"
#include "encoding.h"
#include "common.h"

#define NPACKETS ((NBYTES - 1) / PACKET_BYTES + 1)
#define PAUSE_CYCLES (PAUSE_MS * 3200000L)

static inline void even_reduce(uint64_t srcmac, uint64_t dstmac)
{
	for (int i = 0; i < NCOMPUTE; i++) {
		send_data_loop(srcmac, dstmac, NBYTES);
		recv_data_loop(srcmac, NBYTES, NPACKETS);
	}
}

static inline void odd_reduce(uint64_t srcmac, uint64_t dstmac)
{
	for (int i = 0; i < NCOMPUTE; i++) {
		recv_data_loop(srcmac, NBYTES, NPACKETS);
		send_data_loop(srcmac, dstmac, NBYTES);
	}
}

int main(void)
{
	uint64_t srcmac, reversed, dstmac;
	long end_cycle;

	srcmac = nic_macaddr();
	reversed = macaddr_reverse(srcmac);
	dstmac = ((reversed & 0xffff) == (0x0002 + NCOMPUTE - 1)) ?
		macaddr_reverse((reversed & ~0xFFFF) | 0x0002) :
		macaddr_reverse(reversed + 1);

	printf("start test dstmac %012lx\n", dstmac);

	for (int i = 0; i < NROUNDS; i++) {
		end_cycle = rdcycle() + PAUSE_CYCLES;
		while (rdcycle() < end_cycle) {}

		if ((reversed & 1) == 0)
			even_reduce(srcmac, dstmac);
		else
			odd_reduce(srcmac, dstmac);
	}

	printf("finished\n");

	end_cycle = rdcycle() + PAUSE_CYCLES;
	while (rdcycle() < end_cycle) {}

	return 0;
}
