#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mmio.h"
#include "nic.h"
#include "encoding.h"
#include "reduce.h"

#define NPACKETS ((NBYTES - 1) / PACKET_BYTES + 1)
#define PAUSE_CYCLES (PAUSE_MS * 3200000L)

static inline void even_reduce(uint64_t srcmac, uint64_t leftmac, uint64_t rightmac)
{
	for (int i = 0; i < NCOMPUTE; i++) {
		recv_data_loop(srcmac, 24, 1);
		send_data_loop(srcmac, rightmac, NBYTES);

		send_data_loop(srcmac, leftmac, 24);
		recv_data_loop(srcmac, NBYTES, NPACKETS);
	}
}

static inline void odd_reduce(uint64_t srcmac, uint64_t leftmac, uint64_t rightmac)
{
	for (int i = 0; i < NCOMPUTE; i++) {
		send_data_loop(srcmac, leftmac, 24);
		recv_data_loop(srcmac, NBYTES, NPACKETS);

		recv_data_loop(srcmac, 24, 1);
		send_data_loop(srcmac, rightmac, NBYTES);
	}
}

int main(void)
{
	uint64_t srcmac, reversed, leftmac, rightmac;
	long end_cycle;

	srcmac = nic_macaddr();
	reversed = macaddr_reverse(srcmac);

	leftmac = ((reversed & 0xffff) == 0x0002) ?
		macaddr_reverse(reversed + NCOMPUTE - 1) :
		macaddr_reverse(reversed - 1);
	rightmac = ((reversed & 0xffff) == (0x0002 + NCOMPUTE - 1)) ?
		macaddr_reverse((reversed & ~0xFFFF) | 0x0002) :
		macaddr_reverse(reversed + 1);

	printf("start test leftmac: %012lx, rightmac: %012lx\n",
			leftmac, rightmac);

	for (int i = 0; i < NROUNDS; i++) {
		end_cycle = rdcycle() + PAUSE_CYCLES;
		while (rdcycle() < end_cycle) {}

		if ((reversed & 1) == 0)
			even_reduce(srcmac, leftmac, rightmac);
		else
			odd_reduce(srcmac, leftmac, rightmac);
	}

	printf("finished\n");

	end_cycle = rdcycle() + PAUSE_CYCLES;
	while (rdcycle() < end_cycle) {}

	return 0;
}
