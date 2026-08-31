#! /usr/bin/env python3
#
# Stand-in for the subset of srec_cat the GigaDevice post-build scripts use.
#
# The Windows scripts run scripts/imgtool/srec_cat.exe, vendored next to this
# file. Linux has no such binary: srec_cat comes from the srecord package, which
# is not installed by default, so the build would stop at image-all.bin unless
# every user apt-installs it. The scripts prefer the real srec_cat when it is on
# PATH and fall back here otherwise.
#
# Implemented, because that is all the scripts ask for:
#   <infile> -Binary -offset <addr>     place a binary blob at an address
#   -fill <byte> <start> <end>          fill the still-unset bytes of [start, end)
#   -o <outfile> -Binary | -Intel       write a flat binary or an Intel hex file
#
# Addresses are file offsets in the binary output and gaps read back as 0x00,
# matching srec_cat. Anything outside the subset is rejected loudly rather than
# quietly producing a different image.

import sys


class Memory:
    """Sparse byte map: address -> value, exactly what srec_cat builds up."""

    def __init__(self):
        self.data = {}

    def load(self, path, offset):
        with open(path, "rb") as f:
            blob = f.read()
        for i, b in enumerate(blob):
            self.data[offset + i] = b

    def fill(self, value, start, end):
        for addr in range(start, end):
            self.data.setdefault(addr, value)

    def to_binary(self):
        if not self.data:
            return b""
        return bytes(self.data.get(a, 0) for a in range(0, max(self.data) + 1))

    def to_ihex(self):
        lines = []
        upper = None
        for base in range(0, (max(self.data) if self.data else 0) + 1, 16):
            chunk = [(base + i, self.data[base + i])
                     for i in range(16) if base + i in self.data]
            if not chunk:
                continue
            # a record may not straddle a 16-bit boundary shift, so emit an
            # extended linear address record whenever the upper half changes
            if (base >> 16) != upper:
                upper = base >> 16
                lines.append(_ihex_record(0, 4, upper.to_bytes(2, "big")))
            # holes inside the 16 bytes: split into runs so they stay unset
            run_start, run = None, []
            for addr, value in chunk:
                if run and addr != run_start + len(run):
                    lines.append(_ihex_record(run_start & 0xFFFF, 0, bytes(run)))
                    run_start, run = addr, []
                if not run:
                    run_start = addr
                run.append(value)
            if run:
                lines.append(_ihex_record(run_start & 0xFFFF, 0, bytes(run)))
        lines.append(_ihex_record(0, 1, b""))
        return "".join(line + "\n" for line in lines).encode("ascii")


def _ihex_record(addr, rectype, payload):
    body = bytes([len(payload), (addr >> 8) & 0xFF, addr & 0xFF, rectype]) + payload
    checksum = (-sum(body)) & 0xFF
    return ":" + (body + bytes([checksum])).hex().upper()


def die(msg):
    print(f"srec_cat_lite: {msg}", file=sys.stderr)
    sys.exit(1)


def parse_int(text):
    try:
        return int(text, 0)
    except ValueError:
        die(f"not a number: {text}")


def main(argv):
    mem = Memory()
    outfile = None
    out_format = None
    fills = []  # applied after every input, as srec_cat only fills unset bytes

    i = 0
    while i < len(argv):
        arg = argv[i]
        if arg == "-o":
            outfile = argv[i + 1]
            i += 2
        elif arg in ("-Binary", "-binary", "-Intel", "-intel"):
            # after -o it names the output format; the input case is consumed
            # together with the file name below
            out_format = arg.lower()
            i += 1
        elif arg == "-fill":
            fills.append((parse_int(argv[i + 1]),
                          parse_int(argv[i + 2]),
                          parse_int(argv[i + 3])))
            i += 4
        elif arg.startswith("-"):
            die(f"unsupported option: {arg}")
        else:
            if argv[i + 1:i + 2] != ["-Binary"]:
                die(f"input {arg} must be followed by -Binary")
            offset = 0
            i += 2
            if argv[i:i + 1] == ["-offset"]:
                offset = parse_int(argv[i + 1])
                i += 2
            mem.load(arg, offset)

    if not outfile:
        die("no -o output given")

    for value, start, end in fills:
        mem.fill(value, start, end)

    with open(outfile, "wb") as f:
        f.write(mem.to_ihex() if out_format in ("-intel",) else mem.to_binary())
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
