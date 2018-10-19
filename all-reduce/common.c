#include <stdint.h>

#include "common.h"
#include "mmio.h"
#include "nic.h"

static uint64_t out_packets[MAX_OUTSTANDING][PACKET_WORDS];
static uint64_t in_packets[MAX_OUTSTANDING][PACKET_WORDS];
static long send_id, recv_id;
static int noutstanding;
static int inflight[MAX_OUTSTANDING];

static int process_recv(uint64_t srcmac, int n)
{
	long nbytes = 0;
	uint64_t dstmac;

	asm volatile ("fence");

	for (int i = 0; i < n; i++) {
		int k = send_id % MAX_OUTSTANDING;
		long pkt_id;

		nbytes += nic_complete_recv();
		dstmac = in_packets[k][1] & 0xFFFFFFFFFFFFL;
		pkt_id = in_packets[k][2];

		out_packets[k][0] = dstmac << 16;
		out_packets[k][1] = srcmac | (ETHTYPE << 48);
		out_packets[k][2] = pkt_id;
		asm volatile ("fence");

		nic_post_send((uint64_t) out_packets[k], 24);
		send_id++;
	}

	return nbytes;
}

static inline long recv_packets(uint64_t srcmac, int n)
{
	long nbytes = 0;
	int nout = n;

	for (int i = 0; i < n; i++) {
		int j = recv_id % MAX_OUTSTANDING;
		nic_post_recv((uint64_t) in_packets[j]);
		recv_id++;
	}

	while (nout > 0) {
		int counts = nic_counts();
		int recv_comp = (counts >> NIC_COUNT_RECV_COMP) & 0xf;
		int to_process = (recv_comp < nout) ? recv_comp : nout;

		nbytes += process_recv(srcmac, to_process);
		nout -= to_process;
	}

	nout = n;
	while (nout > 0) {
		int counts = nic_counts();
		int send_comp = (counts >> NIC_COUNT_SEND_COMP) & 0xf;

		for (int i = 0; i < send_comp; i++) {
			nic_complete_send();
			nout--;
		}


	}

	return nbytes;
}

uint64_t recv_data_loop(uint64_t srcmac, long nbytes, long npackets)
{
	long bytes_left = nbytes;
	uint64_t dstmac;
	int j;

	send_id = 0;
	recv_id = 0;

	while (recv_id < npackets) {
		int counts = nic_counts();
		int send_req = (counts >> NIC_COUNT_SEND_REQ) & 0xf;
		int recv_req = (counts >> NIC_COUNT_RECV_REQ) & 0xf;
		int pkts_left = npackets - recv_id;
		int n = MAX_OUTSTANDING;

		if (n > send_req)
			n = send_req;
		if (n > recv_req)
			n = recv_req;
		if (n > pkts_left)
			n = pkts_left;

		bytes_left -= recv_packets(srcmac, n);
	}

	j = (recv_id - 1) % MAX_OUTSTANDING;
	dstmac = in_packets[j][1] & 0xffffffffffffL;

	return dstmac;
}

static void process_recv_comp(int n)
{
	asm volatile ("fence");
	for (int j = 0; j < n; j++) {
		long pkt_id;
		int k = recv_id % MAX_OUTSTANDING;

		nic_complete_recv();
		pkt_id = in_packets[k][2];
		inflight[pkt_id] = 0;
		recv_id++;
		noutstanding--;
	}
}

void send_data_loop(uint64_t srcmac, uint64_t dstmac, long nbytes)
{
	long bytes_left = nbytes;

	send_id = 0;
	recv_id = 0;

	while (bytes_left > 0) {
		int counts = nic_counts();
		int send_req = (counts >> NIC_COUNT_SEND_REQ) & 0xf;
		int send_comp = (counts >> NIC_COUNT_SEND_COMP) & 0xf;
		int recv_req = (counts >> NIC_COUNT_RECV_REQ) & 0xf;
		int recv_comp = (counts >> NIC_COUNT_RECV_COMP) & 0xf;
		int i = send_id % MAX_OUTSTANDING;
		long to_send = (bytes_left < PACKET_BYTES) ? bytes_left : PACKET_BYTES;

		for (int j = 0; j < send_comp; j++)
			nic_complete_send();

		if (recv_comp > 0)
			process_recv_comp(recv_comp);

		if (send_req == 0 || recv_req == 0)
			continue;

		if (inflight[i] || noutstanding >= MAX_OUTSTANDING)
			continue;

		out_packets[i][0] = dstmac << 16;
		out_packets[i][1] = srcmac | (ETHTYPE << 48);
		out_packets[i][2] = send_id;
		asm volatile ("fence");

		nic_post_recv((uint64_t) in_packets[i]);
		nic_post_send((uint64_t) out_packets[i], to_send);
		inflight[i] = 1;
		noutstanding++;
		bytes_left -= to_send;
	}

	while (noutstanding > 0) {
		int counts = nic_counts();
		int recv_comp = (counts >> NIC_COUNT_RECV_COMP) & 0xf;

		if (recv_comp > 0)
			process_recv_comp(recv_comp);
	}
}

uint64_t macaddr_add(uint64_t macaddr, int inc)
{
	uint64_t temp, result;

	temp = ((macaddr >> 40) & 0xffL) |
	       ((macaddr >> 24) & 0xff00L) |
	       ((macaddr >> 8)  & 0xff0000L) |
	       ((macaddr << 8)  & 0xff000000L) |
	       ((macaddr << 24) & 0xff00000000L) |
	       ((macaddr << 40) & 0xff0000000000L);
	temp += inc;

	result = ((temp >> 40) & 0xffL) |
		 ((temp >> 24) & 0xff00L) |
		 ((temp >> 8)  & 0xff0000L) |
		 ((temp << 8)  & 0xff000000L) |
		 ((temp << 24) & 0xff00000000L) |
		 ((temp << 40) & 0xff0000000000L);
	return result;
}


