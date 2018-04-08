import subprocess
import os

CC=["riscv64-unknown-elf-gcc"]
CFLAGS=["-mcmodel=medany", "-Wall", "-O2", "-fno-common", "-fno-builtin-printf"]
LDFLAGS=["-T", "link.ld", "-static", "-nostdlib", "-nostartfiles", "-lgcc"]

def compile(source_name, target_name, macros):
    macro_flags = ["-D{}={}".format(name, macros[name]) for name in macros]
    command = CC + CFLAGS + macro_flags + ["-c", source_name, "-o", target_name]
    print(' '.join(command))
    return subprocess.call(command)

def link(source_names, target_name):
    command = CC + LDFLAGS + source_names + ["-o", target_name]
    print(' '.join(command))
    return subprocess.call(command)

def mac_to_hex(macaddr):
    return '0x' + ''.join(reversed(macaddr.split(":")))

def ltoa(i):
    return str(i) + "L"

SERVER_MACS = ["00:12:6D:00:00:{:02x}".format(i) for i in range(10, 18)]
CLIENT_MACS = ["00:12:6D:00:00:{:02x}".format(i) for i in range(2, 10)]
NPACKETS = 100
PACKET_WORDS = 180
END_CYCLE = 800 * 1000 * 1000 #* 1000
CYCLE_STEP = END_CYCLE / (len(CLIENT_MACS))
SERVER_WAIT = 12 * CYCLE_STEP
CLIENT_WAIT = SERVER_WAIT + 2 * CYCLE_STEP

def main():
    if not os.path.isdir("testbuild"):
        os.makedirs("testbuild")

    compile("crt.S", "testbuild/crt.o", {})
    compile("syscalls.c", "testbuild/syscalls.o", {})

    for (i, (server_mac, client_mac)) in enumerate(zip(SERVER_MACS, CLIENT_MACS)):
        SERVER_BASE = "testbuild/bw-test-server-{}".format(i + len(CLIENT_MACS))
        CLIENT_BASE = "testbuild/bw-test-client-{}".format(i)
        compile(
            "bw-test-server.c",
            SERVER_BASE + ".o",
            {"CLIENT_MACADDR": mac_to_hex(client_mac),
             "NPACKETS": NPACKETS,
             "PACKET_WORDS": PACKET_WORDS,
             "END_CYCLE": ltoa(END_CYCLE + SERVER_WAIT)})
        compile(
            "bw-test-client.c",
            CLIENT_BASE + ".o",
            {"SERVER_MACADDR": mac_to_hex(server_mac),
             "NPACKETS": NPACKETS,
             "PACKET_WORDS": PACKET_WORDS,
             "START_CYCLE": ltoa(i * CYCLE_STEP),
             "END_CYCLE": ltoa(END_CYCLE),
             "WAIT_CYCLES": ltoa(CLIENT_WAIT)})
        link([SERVER_BASE + ".o", "testbuild/crt.o", "testbuild/syscalls.o"], SERVER_BASE + ".riscv")
        link([CLIENT_BASE + ".o", "testbuild/crt.o", "testbuild/syscalls.o"], CLIENT_BASE + ".riscv")

    compile("latency-test.c", "testbuild/latency-test.o", {})
    link(["testbuild/latency-test.o",
          "testbuild/crt.o",
          "testbuild/syscalls.o"],
          "testbuild/latency-test.riscv")

if __name__ == "__main__":
    main()
