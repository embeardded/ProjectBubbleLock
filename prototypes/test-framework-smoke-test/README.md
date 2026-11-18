# PROTOTYPE: test-framework smoke test (issue #18). Throwaway, never merged.

Answers one question for #16: do **snitch** and **our own minimal C++20 runner** meet the
test-framework constraints on a small MCU? Pass criteria: framework share ≤ 16 KB flash and
≤ 2 KB RAM on Cortex-M0, no allocator linked, readable output over the UART in Renode.

## Run it (WSL / Linux, no sudo)

```sh
bash setup-tools.sh   # Arm GNU Toolchain 14.2.rel1, Renode (portable), snitch @ c29ce5b
bash build.sh         # builds, measures, runs every image in Renode, prints the table + UART logs
bash top-symbols.sh build/m0/snitch-16-tests   # where the bytes go
bash stubs-used.sh    # which runtime stubs each image needs
```

`build-output.txt` is the recorded output of `build.sh` (results below come from it).

## Layout

- `common/tests.cpp`: the **same two tests, unchanged source**, compiled against each candidate
  through a per-candidate `test_framework.hpp`.
- `common/board.cpp`: startup, USART2 output, stack painting, bare-metal runtime stubs. Linked into every image.
- `baseline/`: the same tests with no framework (CHECK just evaluates). The framework's
  share = candidate image − baseline image.
- `snitch/`: freestanding config (`snitch/config/snitch/snitch_config.hpp`) with lowered limits.
- `own/minitest.{hpp,cpp}`: the own runner, 285 lines.

Flags: `-std=c++20 -fno-exceptions -fno-rtti -fno-threadsafe-statics -Os`,
`-ffunction-sections -fdata-sections -Wl,--gc-sections`, newlib-nano, no startfiles.
M0 = `-mcpu=cortex-m0` (soft float). M4 = `-mcpu=cortex-m4 -mfloat-abi=hard`.
Both run on Renode's `platforms/cpus/stm32f4.repl` (ARMv6-M code runs on the M4 core).
The linker script has no heap and no `end` symbol, so anything that pulls in `_sbrk` fails to link.

## Results (Cortex-M0, framework share over baseline)

| candidate | flash | static RAM | extra stack | total RAM | allocator | verdict |
|---|---|---|---|---|---|---|
| snitch, default `to_chars` | 137,636 B | 3,624 B | ~2,120 B | ~5.7 KB | none | **fails size** |
| snitch, `SNITCH_APPEND_TO_CHARS=0` | 28,556 B | 3,624 B | ~2,120 B | ~5.7 KB | none | **fails size** |
| snitch, also `MAX_TEST_CASES=16` | 25,980 B | 1,128 B | ~2,120 B | ~3.2 KB | none | **fails size** |
| own runner | **4,056 B** | **120 B** | ~424 B | **~0.5 KB** | none | **passes** |

M4 is similar: snitch 26.5–138 KB flash, own runner 2.5 KB (smaller because floats are in hardware).
Full table: `build-output.txt`.

Notes:
- snitch's flash is spread over the console reporter, `std::variant` visitors, test filtering
  and its float formatter. No single limit removes it. Its static RAM is ~52 B per
  `SNITCH_MAX_TEST_CASES` slot, and `snitch::tests` is in `.data`, so it costs flash too.
- The own runner's per-test cost is a 20 B `test_case` node, with no fixed cap.
- snitch needs these bare-metal stubs; the own runner needs none: `std::terminate`
  (its default handler pulls in the demangler, which calls malloc), `abort`,
  `std::__throw_out_of_range_fmt` (from `string_view::substr`), `__aeabi_atexit`/`__cxa_atexit`.
- snitch freestanding gotcha, not in its README: besides `snitch::cli::console_print`, you must
  set `snitch::tests.print_callback`. Otherwise the first print calls `std::terminate`.
  `snitch::main()` hides this by calling `configure(argv)`.
- `SNITCH_MAX_COMMAND_LINE_ARGS` can't go below 32: it also sizes the CLI option table.
