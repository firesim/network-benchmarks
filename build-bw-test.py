import argparse
import subprocess
import os
import shutil

CC=["riscv64-unknown-elf-gcc"]
CFLAGS=["-mcmodel=medany", "-Wall", "-O2", "-fno-common", "-fno-builtin-printf", "-Icommon/"]
LDFLAGS=["-T", "common/link.ld", "-static", "-nostdlib", "-nostartfiles", "-lgcc"]

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

MAC_TEMPLATE = "00:12:6D:00:00:{:02x}"

def main():
    if os.path.isdir("testbuild"):
        shutil.rmtree("testbuild")
    os.makedirs("testbuild")

    parser = argparse.ArgumentParser(description="Build client and server binaries")
    parser.add_argument(
            "-n", "--num-pairs", type=int, default=8)
    parser.add_argument(
            "-c", "--num-packets", type=int, default=100)
    parser.add_argument(
            "-w", "--packet-words", type=int, default=180)
    parser.add_argument(
            "-s", "--cycle-step", type=int, default=100000000)
    args = parser.parse_args()

    all_macs = [MAC_TEMPLATE.format(i) for i in range(2, 2 + 2 * args.num_pairs)]
    server_macs = all_macs[args.num_pairs:]
    client_macs = all_macs[:args.num_pairs]

    compile("common/crt.S", "testbuild/crt.o", {})
    compile("common/syscalls.c", "testbuild/syscalls.o", {})

    end_cycle = (args.num_pairs + 1) * args.cycle_step
    server_wait = args.num_pairs * args.cycle_step
    client_wait = server_wait + 2 * args.cycle_step

    for (i, (server_mac, client_mac)) in enumerate(zip(server_macs, client_macs)):
        SERVER_BASE = "testbuild/bw-test-server-{}".format(i + args.num_pairs)
        CLIENT_BASE = "testbuild/bw-test-client-{}".format(i)
        compile(
            "bw-test/server.c",
            SERVER_BASE + ".o",
            {"CLIENT_MACADDR": mac_to_hex(client_mac),
             "NPACKETS": args.num_packets,
             "PACKET_WORDS": args.packet_words,
             "END_CYCLE": ltoa(end_cycle + server_wait)})
        compile(
            "bw-test/client.c",
            CLIENT_BASE + ".o",
            {"SERVER_MACADDR": mac_to_hex(server_mac),
             "NPACKETS": args.num_packets,
             "PACKET_WORDS": args.packet_words,
             "START_CYCLE": ltoa((i + 1) * args.cycle_step),
             "END_CYCLE": ltoa(end_cycle),
             "WAIT_CYCLES": ltoa(client_wait)})

        link([SERVER_BASE + ".o", "testbuild/crt.o", "testbuild/syscalls.o"], SERVER_BASE + ".riscv")
        link([CLIENT_BASE + ".o", "testbuild/crt.o", "testbuild/syscalls.o"], CLIENT_BASE + ".riscv")

if __name__ == "__main__":
    main()
