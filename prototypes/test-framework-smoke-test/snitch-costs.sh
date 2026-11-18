#!/usr/bin/env bash
# PROTOTYPE (issue #18): what each snitch feature and limit costs. Cortex-M0, -flto, to_chars off.
# Run build.sh first (the core breakdown reads build/m0/snitch-no-to_chars).
set -euo pipefail
cd "$(dirname "$0")"
TOOLS="${TOOLS:-$HOME/proto-tools}"
BIN="$TOOLS/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin"
SNITCH="$TOOLS/snitch"
RENODE="$TOOLS/renode/renode"
FLAGS="-mcpu=cortex-m0 -mthumb -std=c++20 -fno-exceptions -fno-rtti -fno-threadsafe-statics -Os -g
  -ffunction-sections -fdata-sections -flto -Icommon -Isnitch -Isnitch/config -I$SNITCH/include
  -DSNITCH_APPEND_TO_CHARS=0"
LDFLAGS="-nostartfiles --specs=nano.specs -Wl,--gc-sections -Tcommon/stm32f405.ld"
OUT=build/costs
rm -rf "$OUT" && mkdir -p "$OUT"

declare -A FLASH RAM STACK
measure() { # name, extra flags...
    local name=$1 dir="$OUT/$1"
    shift
    mkdir -p "$dir"
    # shellcheck disable=SC2086
    "$BIN/arm-none-eabi-g++" $FLAGS "$@" common/board.cpp common/tests.cpp snitch/main.cpp \
        snitch/features.cpp "$SNITCH/src/snitch.cpp" $LDFLAGS -o "$dir/image.elf" 2>"$dir/compile.log" \
        || { echo "build failed: $name"; tail -5 "$dir/compile.log"; return; }
    read -r text data bss _ < <("$BIN/arm-none-eabi-size" -B "$dir/image.elf" | tail -1)
    FLASH[$name]=$((text + data))
    RAM[$name]=$((data + bss))
    "$RENODE" --disable-gui --plain --console -e "
        mach create \"stm32f405\"
        machine LoadPlatformDescription @platforms/cpus/stm32f4.repl
        sysbus LoadELF @$PWD/$dir/image.elf
        sysbus.usart2 CreateFileBackend @$PWD/$dir/uart.txt true
        emulation RunFor \"0.5\"
        quit" >"$dir/renode.log" 2>&1 || true
    STACK[$name]=$(grep -o 'stack high-water: [0-9]*' "$dir/uart.txt" 2>/dev/null | grep -o '[0-9]*$' || echo 0)
    grep -q 'END OF RUN' "$dir/uart.txt" 2>/dev/null || echo "WARNING: $name did not finish its run"
}
row() { # label, name, reference, divisor
    local d=${4:-1}
    printf '| %s | %d | %d | %d |\n' "$1" $(((FLASH[$2] - FLASH[$3]) / d)) \
        $(((RAM[$2] - RAM[$3]) / d)) $(((STACK[$2] - STACK[$3]) / d))
}

measure reference
measure plain -DFEATURE_PLAIN
features="INT_VALUES TAGS SECTION CAPTURE INFO CHECK_THAT CHECK_FALSE REQUIRE FAIL_CHECK SKIP_CHECK TEMPLATE_TEST_CASE TEST_CASE_METHOD CONSTEXPR_CHECK"
for f in $features; do measure "$f" "-DFEATURE_$f"; done

limits="MAX_TEST_CASES:64:128 MAX_EXPR_LENGTH:96:192 MAX_MESSAGE_LENGTH:96:192 MAX_TEST_NAME_LENGTH:64:128
        MAX_CAPTURES:2:4 MAX_CAPTURE_LENGTH:32:64 MAX_NESTED_SECTIONS:2:4 MAX_UNIQUE_TAGS:4:8 MAX_TAG_LENGTH:16:32"
for l in $limits; do
    IFS=: read -r macro _ raised <<<"$l"
    measure "$macro" "-DSNITCH_$macro=$raised"
done
measure DECOMPOSE_SUCCESSFUL_ASSERTIONS -DSNITCH_DECOMPOSE_SUCCESSFUL_ASSERTIONS=1
measure NO_BITCAST -DSNITCH_CONSTEXPR_FLOAT_USE_BITCAST=0

echo "Reference image (the two tests): flash ${FLASH[reference]} B, RAM ${RAM[reference]} B, stack ${STACK[reference]} B"
echo
echo "## Using a feature (one extra test; relative to one extra plain test)"
echo "| feature | flash (B) | static RAM (B) | stack (B) |"
echo "|---|---|---|---|"
row "one more plain TEST_CASE with a float CHECK (relative to the reference)" plain reference
for f in $features; do row "$f" "$f" plain; done
echo
echo "## Limits (cost per unit, from raising each limit; relative to the reference)"
echo "| limit (current -> raised) | flash per unit | static RAM per unit | stack per unit |"
echo "|---|---|---|---|"
for l in $limits; do
    IFS=: read -r macro now raised <<<"$l"
    row "SNITCH_$macro ($now -> $raised)" "$macro" reference $((raised - now))
done
echo
echo "## Switches (relative to the reference)"
echo "| switch | flash (B) | static RAM (B) | stack (B) |"
echo "|---|---|---|---|"
row "SNITCH_DECOMPOSE_SUCCESSFUL_ASSERTIONS=1" DECOMPOSE_SUCCESSFUL_ASSERTIONS reference
row "SNITCH_CONSTEXPR_FLOAT_USE_BITCAST=0" NO_BITCAST reference

echo
echo "## Core: snitch's flash by area (build/m0/snitch-no-to_chars, no LTO, symbol sizes)"
"$BIN/arm-none-eabi-nm" -C -S --size-sort build/m0/snitch-no-to_chars/image.elf | awk '
    NF >= 4 && $3 ~ /^[tTrRwW]$/ && !seen[$1]++ {
        size = strtonum("0x" $2); name = $0; sub(/^[^ ]+ [^ ]+ [^ ]+ /, "", name)
        if      (name ~ /reporter::console/)                                         a = "console reporter (formatting the output)"
        else if (name ~ /(append|fixed|float|set_precision|binary_table|to_chars)/)    a = "number and float formatting"
        else if (name ~ /(filter|tag|list_|wildcard)/)                               a = "test filtering, tags, listing"
        else if (name ~ /snitch::cli/)                                               a = "command-line parsing"
        else if (name ~ /(section|capture|info)/)                                    a = "sections, captures, info"
        else if (name ~ /registry::(run|report|add_impl)|test_case|registry::print|registry::add/) a = "registry: registration, run loop, reporting"
        else if (name ~ /__variant/)                                                 a = "std::variant visitors (event dispatch)"
        else if (name ~ /snitch/)                                                    a = "other snitch"
        else if (name ~ /^__(aeabi|.*(si|di|sf|df)[0-9]|gnu_|clz|ctz|udiv|div|mul|lsh|ash)/) a = "libgcc (soft float, division)"
        else                                                                          a = "startup, UART, tests, libc"
        total[a] += size
    }
    END { for (a in total) printf "| %s | %d |\n", a, total[a] }' | sort -t'|' -k3 -rn
