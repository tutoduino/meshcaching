#!/usr/bin/env python3
# Convertit un firmware Intel HEX en UF2, à glisser-déposer sur le disque
# exposé par le bootloader UF2 (double appui sur RESET sur le Wio Tracker).
# Format UF2 : https://github.com/microsoft/uf2 — blocs de 512 octets dont
# 256 de charge utile, famille passée en argument (nRF52840 : 0xADA52840).
#
# Usage : hex2uf2.py <entrée.hex> <sortie.uf2> <famille>
import struct
import sys

UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30
UF2_FLAG_FAMILY_ID = 0x00002000
PAYLOAD_SIZE = 256


def parse_hex(path):
    """Renvoie la liste triée des (adresse, octet) du fichier Intel HEX."""
    memory = {}
    base = 0
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line.startswith(":"):
                continue
            raw = bytes.fromhex(line[1:])
            count, addr, rectype = raw[0], (raw[1] << 8) | raw[2], raw[3]
            data = raw[4 : 4 + count]
            if sum(raw) & 0xFF != 0:
                raise ValueError("somme de contrôle HEX invalide : " + line)
            if rectype == 0x00:  # données
                for i, byte in enumerate(data):
                    memory[base + addr + i] = byte
            elif rectype == 0x01:  # fin de fichier
                break
            elif rectype == 0x02:  # adresse de segment étendue
                base = ((data[0] << 8) | data[1]) << 4
            elif rectype == 0x04:  # adresse linéaire étendue
                base = ((data[0] << 8) | data[1]) << 16
            # 0x03/0x05 (adresses de démarrage) : sans objet pour un UF2
    return sorted(memory.items())


def to_blocks(memory):
    """Regroupe les octets en blocs de PAYLOAD_SIZE alignés."""
    blocks = {}
    for addr, byte in memory:
        block_addr = addr - (addr % PAYLOAD_SIZE)
        block = blocks.setdefault(block_addr, bytearray([0xFF] * PAYLOAD_SIZE))
        block[addr - block_addr] = byte
    return sorted(blocks.items())


def main():
    if len(sys.argv) != 4:
        sys.exit("usage : hex2uf2.py <entrée.hex> <sortie.uf2> <famille>")
    src, dst, family = sys.argv[1], sys.argv[2], int(sys.argv[3], 0)

    blocks = to_blocks(parse_hex(src))
    with open(dst, "wb") as out:
        for number, (addr, payload) in enumerate(blocks):
            header = struct.pack(
                "<IIIIIIII",
                UF2_MAGIC_START0, UF2_MAGIC_START1, UF2_FLAG_FAMILY_ID,
                addr, PAYLOAD_SIZE, number, len(blocks), family,
            )
            out.write(header + payload + bytes(476 - PAYLOAD_SIZE) +
                      struct.pack("<I", UF2_MAGIC_END))
    first = blocks[0][0]
    last = blocks[-1][0] + PAYLOAD_SIZE
    print("%s : %d blocs, 0x%05X-0x%05X" % (dst, len(blocks), first, last))


if __name__ == "__main__":
    main()
