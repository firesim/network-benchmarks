# Minimalistic makefile to compile stuff
# Works for two nodes communicating with each other

RISCV_PFX=riscv64-unknown-elf-
RISCV_CC=$(RISCV_PFX)gcc

BINARY_OPT=-static -specs=htif_nano.specs


CLIENT=client
CLIENT_RISCV=$(CLIENT).riscv
SERVER=server
SERVER_RISCV=$(SERVER).riscv

all: $(CLIENT_RISCV) $(SERVER_RISCV)

INCLUDE_DIR=../common

NPACKETS=100
PACKET_WORDS=180


SERVER_MACROS=-DCLIENT_MACADDR=0x0200006D1200 -DNPACKETS=$(NPACKETS) -DPACKET_WORDS=$(PACKET_WORDS) -DEND_CYCLE=3000000L
CLIENT_MACROS=-DSERVER_MACADDR=0x0300006D1200 -DNPACKETS=$(NPACKETS) -DPACKET_WORDS=$(PACKET_WORDS) -DSTART_CYCLE=1000000L -DEND_CYCLE=2000000L -DWAIT_CYCLES=3000000L

all: $(CLIENT_RISCV) $(SERVER_RISCV)

$(CLIENT_RISCV): $(CLIENT).c
	$(RISCV_CC) $(BINARY_OPT) -I$(INCLUDE_DIR) -o $@ $^ $(CLIENT_MACROS)

$(SERVER_RISCV): $(SERVER).c
	$(RISCV_CC) $(BINARY_OPT) -I$(INCLUDE_DIR) -o $@ $^ $(SERVER_MACROS)


.PHONY: clean
clean:
	rm -f $(CLIENT_RISCV) $(SERVER_RISCV)
