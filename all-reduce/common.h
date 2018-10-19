#define PACKET_WORDS 180
#define PACKET_BYTES (PACKET_WORDS * 8)
#define MAX_OUTSTANDING 10
#define ETHTYPE 0x1008L

uint64_t macaddr_add(uint64_t macaddr, int inc);
void send_data_loop(uint64_t srcmac, uint64_t dstmac, long nbytes);
uint64_t recv_data_loop(uint64_t srcmac, long nbytes, long npackets);
