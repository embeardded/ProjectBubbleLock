#!/usr/bin/env bash
# PROTOTYPE (issue #18): which of common/board.cpp's runtime stubs each M0 image actually links.
cd "$(dirname "$0")"
NM="${TOOLS:-$HOME/proto-tools}/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-nm"
for v in baseline snitch-16-tests own; do
    printf '%s: ' "$v"
    "$NM" -C "build/m0/$v/image.elf" \
        | grep -E ' (abort|__assert_func|std::terminate\(\)|std::__throw_out_of_range_fmt.*|__aeabi_atexit|__cxa_atexit)$' \
        | cut -d' ' -f3- | tr '\n' ',' ; echo
done
