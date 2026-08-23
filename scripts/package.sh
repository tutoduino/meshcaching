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
packaged=0

# Cibles ESP32 : tout environnement compilé qui a produit un bootloader.
# Deux fichiers par carte, aux suffixes que flasher.meshcore.io interprète :
#  - "-merged.bin"  : image complète, flashée à 0x0 avec effacement —
#    première installation ou récupération ;
#  - "-update.bin"  : application seule, flashée à 0x10000 — mise à jour
#    qui conserve la configuration persistée.
# (`merge_bin` : alias accepté par esptool v4 — repli PlatformIO local —
# comme v5, épinglée en CI, où le nom canonique est `merge-bin`.)
for bootloader in "$BUILD"/*/bootloader.bin; do
  [ -e "$bootloader" ] || continue
  target=$(basename "$(dirname "$bootloader")")
  esptool --chip esp32s3 merge_bin \
    -o "$DIST/meshcaching-$target-$VERSION-merged.bin" \
    0x0 "$bootloader" \
    0x8000 "$BUILD/$target/partitions.bin" \
    0xe000 "$BOOT_APP0" \
    0x10000 "$BUILD/$target/firmware.bin"
  cp "$BUILD/$target/firmware.bin" \
    "$DIST/meshcaching-$target-$VERSION-update.bin"
  packaged=$((packaged + 1))
done

# Cibles nRF52 : tout environnement qui a produit un firmware.hex.
# Deux formats : le paquet DFU (firmware.zip, généré par la plateforme
# nordicnrf52) attendu par flasher.meshcore.io et adafruit-nrfutil, et
# l'UF2 pour le glisser-déposer sur le disque du bootloader.
for hex in "$BUILD"/*/firmware.hex; do
  [ -e "$hex" ] || continue
  target=$(basename "$(dirname "$hex")")
  cp "$BUILD/$target/firmware.zip" "$DIST/meshcaching-$target-$VERSION.zip"
  python3 scripts/hex2uf2.py "$hex" \
    "$DIST/meshcaching-$target-$VERSION.uf2" \
    0xADA52840
  packaged=$((packaged + 1))
done

[ "$packaged" -gt 0 ] || { echo "aucune cible compilée dans $BUILD" >&2; exit 1; }
(cd "$DIST" && sha256sum meshcaching-* > SHA256SUMS)
ls -l "$DIST"
