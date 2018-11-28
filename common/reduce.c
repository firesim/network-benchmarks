#include <stdint.h>

#include "reduce.h"
#include "mmio.h"
#include "nic.h"

static uint64_t out_packets[MAX_OUTSTANDING][PACKET_WORDS];
static uint64_t in_packets[MAX_OUTSTANDING][PACKET_WORDS];
static long send_id, recv_id;
static int noutstanding;
static int inflight[MAX_OUTSTANDING];

static int send_acks(uint64_t srcmac, int n)
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

static int complete_acks(int n)
{
	for (int i = 0; i < n; i++)
		nic_complete_send();
	noutstanding -= n;
}

static inline void recv_packets(uint64_t srcmac, int n)
{
	long nbytes = 0;
	int nout = n;

	for (int i = 0; i < n; i++) {
		int j = recv_id % MAX_OUTSTANDING;
		nic_post_recv((uint64_t) in_packets[j]);
		recv_id++;
	}

	noutstanding += n;
}

long send_packets(int npackets, long bytes_left)
{
	long nbytes = 0;

	for (int i = 0; i < npackets; i++) {
		int j = send_id % MAX_OUTSTANDING;
		long to_send = (bytes_left < PACKET_BYTES) ? bytes_left : PACKET_BYTES;

		if (inflight[j] || noutstanding >= MAX_OUTSTANDING)
			break;

		nic_post_recv((uint64_t) in_packets[j]);
		nic_post_send((uint64_t) out_packets[j], to_send);
		inflight[j] = 1;
		noutstanding++;
		bytes_left -= to_send;
		send_id++;
	}

	return bytes_left;
}

uint64_t recv_data_loop(uint64_t srcmac, long nbytes)
{
	long bytes_left = nbytes;
	long npackets = (nbytes - 1) / PACKET_BYTES + 1;
	uint64_t dstmac;
	int j;

	send_id = 0;
	recv_id = 0;
	noutstanding = 0;

	while (recv_id < npackets) {
		int counts = nic_counts();
		int send_req = (counts >> NIC_COUNT_SEND_REQ) & NIC_COUNT_MASK;
		int recv_req = (counts >> NIC_COUNT_RECV_REQ) & NIC_COUNT_MASK;
		int recv_comp = (counts >> NIC_COUNT_RECV_COMP) & NIC_COUNT_MASK;
		int send_comp = (counts >> NIC_COUNT_SEND_COMP) & NIC_COUNT_MASK;
		int pkts_left = npackets - recv_id;
		int can_post = MAX_OUTSTANDING - noutstanding;

		int to_post = (pkts_left < recv_req) ? pkts_left : recv_req;
		int to_ack = (send_req < recv_comp) ? send_req : recv_comp;

		if (can_post < to_post)
			to_post = can_post;

		bytes_left -= send_acks(srcmac, to_ack);
		complete_acks(send_comp);
		recv_packets(srcmac, to_post);
	}

	while (noutstanding > 0) {
		int counts = nic_counts();
		int send_req = (counts >> NIC_COUNT_SEND_REQ) & NIC_COUNT_MASK;
		int recv_comp = (counts >> NIC_COUNT_RECV_COMP) & NIC_COUNT_MASK;
		int send_comp = (counts >> NIC_COUNT_SEND_COMP) & NIC_COUNT_MASK;
		int to_ack = (send_req < recv_comp) ? send_req : recv_comp;

		bytes_left -= send_acks(srcmac, to_ack);
		complete_acks(send_comp);
	}

	j = (recv_id - 1) % MAX_OUTSTANDING;
	dstmac = in_packets[j][1] & 0xffffffffffffL;

	return dstmac;
}

static void process_ack(int n)
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
	long npackets = (nbytes - 1) / PACKET_BYTES + 1;

	send_id = 0;
	recv_id = 0;
	noutstanding = 0;

	for (int i = 0; i < MAX_OUTSTANDING; i++) {
		out_packets[i][0] = dstmac << 16;
		out_packets[i][1] = srcmac | (ETHTYPE << 48);
		out_packets[i][2] = i;
	}

	asm volatile ("fence");

	while (send_id < npackets) {
		int counts = nic_counts();
		int send_req = (counts >> NIC_COUNT_SEND_REQ) & NIC_COUNT_MASK;
		int send_comp = (counts >> NIC_COUNT_SEND_COMP) & NIC_COUNT_MASK;
		int recv_req = (counts >> NIC_COUNT_RECV_REQ) & NIC_COUNT_MASK;
		int recv_comp = (counts >> NIC_COUNT_RECV_COMP) & NIC_COUNT_MASK;

		int req_avail = (send_req < recv_req) ? send_req : recv_req;
		int pkts_left = npackets - send_id;
		int npkt_to_send = (pkts_left < req_avail) ? pkts_left : req_avail;

		for (int j = 0; j < send_comp; j++)
			nic_complete_send();

		if (recv_comp > 0)
			process_ack(recv_comp);

		bytes_left = send_packets(npkt_to_send, bytes_left);
	}

	while (noutstanding > 0) {
		int counts = nic_counts();
		int send_comp = (counts >> NIC_COUNT_SEND_COMP) & NIC_COUNT_MASK;
		int recv_comp = (counts >> NIC_COUNT_RECV_COMP) & NIC_COUNT_MASK;

		for (int j = 0; j < send_comp; j++)
			nic_complete_send();

		if (recv_comp > 0)
			process_ack(recv_comp);
	}
}

uint64_t macaddr_add(uint64_t macaddr, int inc)
{
	uint64_t temp, result;

	temp = macaddr_reverse(macaddr);
	temp += inc;
	return macaddr_reverse(temp);
}


