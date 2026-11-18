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
CPPUTEST="$TOOLS/cpputest"
PRINTF="$TOOLS/printf"
CPPUTEST_SRC="$(echo "$CPPUTEST"/src/CppUTest/*.cpp)"
# No memory-leak detection (it replaces global new/malloc) and no C++ standard library.
CPPUTEST_FLAGS="-I$CPPUTEST/include -I$PRINTF/src -DCPPUTEST_USE_OWN_CONFIGURATION -DCPPUTEST_MEM_LEAK_DETECTION_DISABLED -DCPPUTEST_STD_CPP_LIB_DISABLED"
PRINTF_FLAGS="-I$PRINTF/src -std=c11 -Os -g -ffunction-sections -fdata-sections -DPRINTF_SUPPORT_WRITEBACK_SPECIFIER=0"

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
    # Link-time optimization. Each "-lto" image is measured against baseline-lto.
    [baseline-lto]="baseline||-flto"
    [snitch-no-to_chars-lto]="snitch|$SNITCH/src/snitch.cpp|-Isnitch/config -I$SNITCH/include -DSNITCH_APPEND_TO_CHARS=0 -flto"
    [own-lto]="own|own/minitest.cpp|-flto"
    # CppUTest: its own tests.cpp (different syntax), all of src/CppUTest, our platform layer,
    # and eyalroz/printf (compiled as C below) for the vsnprintf CppUTest needs.
    [cpputest]="cpputest|$CPPUTEST_SRC cpputest/platform.cpp build/@CPU@/printf.o|$CPPUTEST_FLAGS|cpputest/tests.cpp"
    [cpputest-lto]="cpputest|$CPPUTEST_SRC cpputest/platform.cpp build/@CPU@/printf-lto.o|$CPPUTEST_FLAGS -flto|cpputest/tests.cpp"
)
ORDER="baseline snitch snitch-no-to_chars snitch-16-tests own cpputest baseline-lto snitch-no-to_chars-lto own-lto cpputest-lto"

rm -rf build && mkdir -p build
for cpu in m0 m4; do
    mkdir -p "build/$cpu"
    # shellcheck disable=SC2086
    "$BIN/arm-none-eabi-gcc" ${CPU_FLAGS[$cpu]} $PRINTF_FLAGS -c "$PRINTF/src/printf/printf.c" -o "build/$cpu/printf.o"
    # shellcheck disable=SC2086
    "$BIN/arm-none-eabi-gcc" ${CPU_FLAGS[$cpu]} $PRINTF_FLAGS -flto -c "$PRINTF/src/printf/printf.c" -o "build/$cpu/printf-lto.o"
    for v in $ORDER; do
        IFS='|' read -r dir extra_src extra_flags tests <<<"${VARIANTS[$v]}"
        extra_src="${extra_src//@CPU@/$cpu}"
        tests="${tests:-common/tests.cpp}"
        out="build/$cpu/$v" && mkdir -p "$out"
        # shellcheck disable=SC2086
        "$CXX" ${CPU_FLAGS[$cpu]} $COMMON_FLAGS -I"$dir" $extra_flags \
            common/board.cpp "$tests" "$dir/main.cpp" $extra_src \
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
    for v in $ORDER; do
        base=baseline && [[ $v == *-lto ]] && base=baseline-lto
        read -r base_flash base_ram <"build/$cpu/$base/size.txt"
        out="build/$cpu/$v"
        read -r flash ram <"$out/size.txt"
        run_renode "$out/image.elf" "$out/uart.txt"
        stack=$(grep -o 'stack high-water: [0-9]*' "$out/uart.txt" 2>/dev/null | grep -o '[0-9]*$' || echo '?')
        alloc=$([ -s "$out/allocator-symbols.txt" ] && tr '\n' ' ' <"$out/allocator-symbols.txt" || echo no)
        # With LTO our operator new is inlined away, so also report use of CppUTest's pool.
        pool=$(grep -o 'allocator pool high-water: [0-9]*' "$out/uart.txt" 2>/dev/null | grep -o '[0-9]*$' || true)
        [ -n "$pool" ] && alloc="$alloc; 8192 B static pool, $pool B used"
        echo "| $cpu | $v | $flash | $ram | $((flash - base_flash)) | $((ram - base_ram)) | $stack | $alloc |"
    done
done

for cpu in m0 m4; do
    for v in snitch own cpputest snitch-no-to_chars-lto own-lto cpputest-lto; do
        echo
        echo "---- UART output: $cpu / $v ----"
        cat "build/$cpu/$v/uart.txt" 2>/dev/null || echo "(no output, see build/$cpu/$v/uart.txt.renode.log)"
    done
done
