#!/usr/bin/env bash
# Assemble les binaires de release dans dist/ après un `pio run` :
#  - cibles ESP32 : image "factory" fusionnée (bootloader + table de
#    partitions + boot_app0 + application), flashable à l'adresse 0x0 —
#    c'est le format attendu par flasher.meshcore.io et esptool ;
#  - Wio Tracker L1 : UF2 à glisser-déposer sur le disque du bootloader.
# Usage : scripts/package.sh <version> [dossier]
set -euo pipefail

VERSION="${1:?usage : package.sh <version> [dossier]}"
DIST="${2:-dist}"
BUILD=.pio/build

# esptool : module pip en CI, sinon celui du paquet PlatformIO
if python3 -c 'import esptool' 2>/dev/null; then
  esptool() { python3 -m esptool "$@"; }
else
  ESPTOOL_PY=$(ls "$HOME"/.platformio/packages/tool-esptoolpy*/esptool.py 2>/dev/null | head -1)
  [ -n "$ESPTOOL_PY" ] || { echo "esptool introuvable (pip install esptool)" >&2; exit 1; }
  esptool() { python3 "$ESPTOOL_PY" "$@"; }
fi

BOOT_APP0=$(ls "$HOME"/.platformio/packages/framework-arduinoespressif32*/tools/partitions/boot_app0.bin 2>/dev/null | head -1)
[ -n "$BOOT_APP0" ] || { echo "boot_app0.bin introuvable" >&2; exit 1; }

mkdir -p "$DIST"

for target in heltec_v3 heltec_v4_3 heltec_v4_r8; do
  esptool --chip esp32s3 merge_bin \
    -o "$DIST/meshcaching-$target-$VERSION.bin" \
    0x0 "$BUILD/$target/bootloader.bin" \
    0x8000 "$BUILD/$target/partitions.bin" \
    0xe000 "$BOOT_APP0" \
    0x10000 "$BUILD/$target/firmware.bin"
done

python3 scripts/hex2uf2.py \
  "$BUILD/wio_tracker_l1/firmware.hex" \
  "$DIST/meshcaching-wio_tracker_l1-$VERSION.uf2" \
  0xADA52840

(cd "$DIST" && sha256sum meshcaching-*-"$VERSION".* > SHA256SUMS)
ls -l "$DIST"
