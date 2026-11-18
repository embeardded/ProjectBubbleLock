#!/usr/bin/env bash
# PROTOTYPE (issue #18): split CppUTest's M0 flash share into printf, soft-float/libgcc and the rest.
cd "$(dirname "$0")"
BIN="${TOOLS:-$HOME/proto-tools}/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin"
img=build/m0/cpputest/image.elf
sum_syms() { # elf, list of symbol names on stdin -> total size of those symbols in elf
    local names; names=$(cat)
    "$BIN/arm-none-eabi-nm" -S "$1" | awk -v list="$names" '
        BEGIN { n = split(list, a, "\n"); for (i = 1; i <= n; i++) want[a[i]] = 1 }
        NF == 4 && ($4 in want) && !seen[$1]++ { total += strtonum("0x" $2) }
        END { print total + 0 }'
}
printf_bytes=$("$BIN/arm-none-eabi-nm" build/m0/printf.o | awk 'NF == 3 { print $3 }' | sum_syms "$img")
libgcc_syms=$("$BIN/arm-none-eabi-nm" "$BIN/../lib/gcc/arm-none-eabi/14.2.1/thumb/v6-m/nofp/libgcc.a" 2>/dev/null \
    | awk 'NF == 3 && $2 == "T" { print $3 }' | sort -u)
libgcc_img=$(echo "$libgcc_syms" | sum_syms "$img")
libgcc_base=$(echo "$libgcc_syms" | sum_syms build/m0/baseline/image.elf)
echo "printf (eyalroz) in image:      $printf_bytes B"
echo "libgcc in image / in baseline:  $libgcc_img B / $libgcc_base B (extra: $((libgcc_img - libgcc_base)) B)"
