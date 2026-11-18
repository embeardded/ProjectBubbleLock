#!/usr/bin/env bash
# PROTOTYPE (issue #18): largest RAM and flash symbols of one image. Usage: top-symbols.sh build/m0/snitch
set -euo pipefail
NM="${TOOLS:-$HOME/proto-tools}/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-nm"
elf="$1/image.elf"
echo "== RAM (.data/.bss) =="
"$NM" -C --size-sort -S "$elf" | grep -E ' [bBdD] ' | tail -"${2:-8}" | cut -c1-170
echo "== flash (code/rodata) =="
"$NM" -C --size-sort -S "$elf" | grep -E ' [tTrRW] ' | tail -"${3:-25}" | cut -c1-170
