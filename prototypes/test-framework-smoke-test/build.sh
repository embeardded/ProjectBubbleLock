#!/usr/bin/env bash
# PROTOTYPE (issue #18): build every candidate for Cortex-M0 and Cortex-M4, measure the
# framework's flash/RAM share against a no-framework baseline, check that no allocator is
# linked, and run each image on an emulated STM32F405 in Renode. Run setup-tools.sh first.
set -euo pipefail
cd "$(dirname "$0")"
TOOLS="${TOOLS:-$HOME/proto-tools}"
BIN="$TOOLS/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin"
SNITCH="$TOOLS/snitch"
RENODE="$TOOLS/renode/renode"
CXX="$BIN/arm-none-eabi-g++"

# The Firmware's flags (C++20, no exceptions, no RTTI) plus usual size/bare-metal flags.
COMMON_FLAGS="-std=c++20 -fno-exceptions -fno-rtti -fno-threadsafe-statics -Os -g
  -ffunction-sections -fdata-sections -Wall -Wextra -Icommon"
LDFLAGS="-nostartfiles --specs=nano.specs -Wl,--gc-sections -Tcommon/stm32f405.ld"

declare -A CPU_FLAGS=(
    [m0]="-mcpu=cortex-m0 -mthumb"
    [m4]="-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16"
)
# variant -> "include dir | extra sources | extra flags"
declare -A VARIANTS=(
    [baseline]="baseline||"
    [snitch]="snitch|$SNITCH/src/snitch.cpp|-Isnitch/config -I$SNITCH/include"
    [snitch-no-to_chars]="snitch|$SNITCH/src/snitch.cpp|-Isnitch/config -I$SNITCH/include -DSNITCH_APPEND_TO_CHARS=0"
    [snitch-16-tests]="snitch|$SNITCH/src/snitch.cpp|-Isnitch/config -I$SNITCH/include -DSNITCH_APPEND_TO_CHARS=0 -DSNITCH_MAX_TEST_CASES=16"
    [own]="own|own/minitest.cpp|"
)
ORDER="baseline snitch snitch-no-to_chars snitch-16-tests own"

rm -rf build && mkdir -p build
for cpu in m0 m4; do
    for v in $ORDER; do
        IFS='|' read -r dir extra_src extra_flags <<<"${VARIANTS[$v]}"
        out="build/$cpu/$v" && mkdir -p "$out"
        # shellcheck disable=SC2086
        "$CXX" ${CPU_FLAGS[$cpu]} $COMMON_FLAGS -I"$dir" $extra_flags \
            common/board.cpp common/tests.cpp "$dir/main.cpp" $extra_src \
            $LDFLAGS -Wl,-Map="$out/image.map" -o "$out/image.elf" 2>"$out/compile.log" \
            || { cat "$out/compile.log"; exit 1; }
        read -r text data bss _ < <("$BIN/arm-none-eabi-size" -B "$out/image.elf" | tail -1)
        echo "$((text + data)) $((data + bss))" >"$out/size.txt"
        "$BIN/arm-none-eabi-nm" "$out/image.elf" \
            | grep -E ' (_?malloc|_malloc_r|_?free|_free_r|_?realloc|_realloc_r|calloc|_calloc_r|_sbrk|_sbrk_r|_Zn[wa][jm].*)$' \
            >"$out/allocator-symbols.txt" || true
    done
done

run_renode() { # elf out
    "$RENODE" --disable-gui --plain --console -e "
        mach create \"stm32f405\"
        machine LoadPlatformDescription @platforms/cpus/stm32f4.repl
        sysbus LoadELF @$PWD/$1
        sysbus.usart2 CreateFileBackend @$PWD/$2 true
        emulation RunFor \"0.5\"
        quit" >"$2.renode.log" 2>&1 || true
}

echo
echo "| cpu | image | flash (B) | RAM .data+.bss (B) | framework flash | framework RAM | stack high-water | allocator linked |"
echo "|---|---|---|---|---|---|---|---|"
for cpu in m0 m4; do
    read -r base_flash base_ram <"build/$cpu/baseline/size.txt"
    for v in $ORDER; do
        out="build/$cpu/$v"
        read -r flash ram <"$out/size.txt"
        run_renode "$out/image.elf" "$out/uart.txt"
        stack=$(grep -o 'stack high-water: [0-9]*' "$out/uart.txt" 2>/dev/null | grep -o '[0-9]*$' || echo '?')
        alloc=$([ -s "$out/allocator-symbols.txt" ] && tr '\n' ' ' <"$out/allocator-symbols.txt" || echo no)
        echo "| $cpu | $v | $flash | $ram | $((flash - base_flash)) | $((ram - base_ram)) | $stack | $alloc |"
    done
done

for cpu in m0 m4; do
    for v in snitch own; do
        echo
        echo "---- UART output: $cpu / $v ----"
        cat "build/$cpu/$v/uart.txt" 2>/dev/null || echo "(no output, see build/$cpu/$v/uart.txt.renode.log)"
    done
done
