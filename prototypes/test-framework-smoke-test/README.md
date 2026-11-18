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
| snitch, `SNITCH_APPEND_TO_CHARS=0`, `-flto` | 26,384 B | 3,620 B | ~1,936 B | ~5.4 KB | none | **fails size** |
| own runner, `-flto` | 3,472 B | 116 B | ~344 B | ~0.5 KB | none | passes |
| CppUTest | 36,124 B | 1,652 B* | ~1,044 B | ~2.7 KB | **static pool** | **fails size, fails no-heap** |
| CppUTest, `-flto` | 34,136 B | 1,572 B* | ~1,036 B | ~2.6 KB | **static pool** | **fails size, fails no-heap** |

\* CppUTest's static RAM minus the 8 KB pool I reserved, plus the 648 B of it actually used.

`-flto` rows are measured against a `-flto` baseline (2,636 B flash).

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

## CppUTest (added for comparison)

Pinned at `42feb87` (master, 2026-11-03). Built without memory-leak detection or the C++
standard library, and run through `TestRegistry` directly (`CommandLineTestRunner` needs argv
and allocates its outputs). `cpputest/platform.cpp` fills every `PlatformSpecific*` hook:
- UART output;
- newlib `setjmp`/`longjmp` for fatal checks;
- `vsnprintf` from eyalroz/printf (`22318ed`): newlib's version mallocs when it prints floats;
- malloc/free and global `operator new`/`delete` on a static 8 KB pool.

Findings:
- **It needs an allocator.** Every `TEST` is created with `new` (`Utest.cpp`), and `SimpleString`
  allocates through `PlatformSpecificMalloc`. A static pool works (648 B peak for these tests),
  but that breaks the "no allocator linked" rule (decision 1).
- **Size:** of the 36 KB on M0, eyalroz/printf is 4.7 KB and extra libgcc is 10.4 KB
  (double-precision soft float and 64-bit division). CppUTest's own code is ~21 KB, mostly
  virtual methods that `--gc-sections` can't drop (`cpputest-split.sh`).
- **The tests had to be rewritten** (`cpputest/tests.cpp`): names must be identifiers, a plain
  `CHECK(a <= b)` prints no values (so `CHECK_COMPARE`), and every check is fatal via `longjmp`.
  The house style (decision 3) is non-fatal checks with no `longjmp`.
- libgcc's 64-bit division routines carry unwind tables, so the linker script now defines
  `__exidx_start`/`__exidx_end` (as vendor scripts do). That makes no difference to the other images.
