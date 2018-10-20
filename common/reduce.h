#define PACKET_WORDS 180
#define PACKET_BYTES (PACKET_WORDS * 8)
#define MAX_OUTSTANDING 64
#define ETHTYPE 0x1008L

static inline uint64_t macaddr_reverse(uint64_t macaddr)
{
	return ((macaddr >> 40) & 0xffL) |
	       ((macaddr >> 24) & 0xff00L) |
	       ((macaddr >> 8)  & 0xff0000L) |
	       ((macaddr << 8)  & 0xff000000L) |
	       ((macaddr << 24) & 0xff00000000L) |
	       ((macaddr << 40) & 0xff0000000000L);
}

uint64_t macaddr_add(uint64_t macaddr, int inc);
void send_data_loop(uint64_t srcmac, uint64_t dstmac, long nbytes);
uint64_t recv_data_loop(uint64_t srcmac, long nbytes, long npackets);
