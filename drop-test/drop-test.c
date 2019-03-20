#include "mmio.h"
#include "nic.h"
#include "encoding.h"

#define NFLITS 180
#define NSEND 70
#define END_CYCLE 500000L

uint64_t packet[NFLITS];

void complete_sends(int navail)
{
	for (int i = 0; i < navail; i++)
		reg_read16(NIC_SEND_COMP);
}

int main(void)
{
	int i, nsent = 0, ncomp = 0;
	long cycle;

	packet[0] = nic_macaddr() << 16;
	packet[1] = nic_macaddr();

	for (i = 2; i < NFLITS; i++)
		packet[i] = i << 8;

	while (nsent < NSEND) {
		int navail;
		uint64_t pkt_size = NFLITS * sizeof(uint64_t);
		uint64_t src_addr = (uint64_t) packet;
		uint64_t send_packet = (pkt_size << 48) | src_addr;
		uint32_t counts = nic_counts();

		navail = (counts >> NIC_COUNT_SEND_COMP) & 0xff;
		asm volatile ("fence");
		complete_sends(navail);
		ncomp += navail;

		navail = (counts >> NIC_COUNT_SEND_REQ) & 0xff;
		asm volatile ("fence");
		for (i = 0; i < navail && nsent < NSEND; i++) {
			reg_write64(NIC_SEND_REQ, send_packet);
			nsent++;
		}
	}

	while (ncomp < NSEND) {
		int navail = (nic_counts() >> NIC_COUNT_SEND_COMP) & 0xff;
		asm volatile ("fence");
		complete_sends(navail);
		ncomp += navail;
	}

	do {
		cycle = rdcycle();
	} while (cycle < END_CYCLE);

	return 0;
}
