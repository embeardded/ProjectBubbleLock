# Research: C++ test framework for Host Tests and Emulated Tests

Research for issue [#16](https://github.com/embeardded/ProjectBubbleLock/issues/16). This file gathers checked facts. It does **not** make the decision; that happens with the human.

Checked on 2026-11-17, mostly by reading each project's source code at the version given. Every claim links to the file or doc that owns it. Statements marked *(estimate)* or *(inference)* are my own reasoning, not something the source says.

## Hard constraints used

From issue #16 and [ADR-0003](../adr/0003-deep-tests-only.md):

- **H1**: builds with `-fno-exceptions -fno-rtti` (C++20 firmware rules).
- **H2**: no heap use *during the test run*. Allocation at startup (static initialisation, before `main`) is acceptable. A framework that allocates can still pass if those allocations can be sent to a static pool.
- **H3**: results go out through a function we supply (for example a UART `putchar`), with no stdio or file system, and each failure names the failing test.
- **H4**: runs bare metal on the STM32F405 (Cortex-M4F, no OS) in Renode. The part has 1 MB flash and 192 KB RAM (128 KB SRAM + 64 KB CCM) ([ST product page](https://www.st.com/en/microcontrollers-microprocessors/stm32f405-415.html)).
- **H5**: the same test sources compile as C++20 for both Host Tests and Emulated Tests.

## Comparison table

| | CppUTest | Unity (+CMock) | doctest | boost.ut | snitch | utest.h | GoogleTest | Catch2 |
|---|---|---|---|---|---|---|---|---|
| Version checked | master `42feb87` (2026-11-03); last tag v4.0 (2020-05-27) | master `54f864c` (2.7.2 in header); release v2.7.0 (2026-07-16) | v2.5.3 (2026-07-06) | v2.3.1 (2025-04-02); master `872b8b9` (2026-10-24) | v1.3.2 (2025-03-22); master `c29ce5b` (2026-10-07) | master `fc4715d` (2026-11-04); no version tags | v1.18.0 (2026-08-10) | v3.16.1 (2026-10-08) |
| H1 no-exc / no-RTTI | ✅ CI builds both | ✅ (it is C) | ✅ but `REQUIRE` removed | ✅ | ✅ | ✅ | ✅ | ⚠️ "experimental" |
| What a fatal assert does without exceptions | `longjmp` out, next test runs | `longjmp` out, next test runs | no fatal asserts (`CHECK` only) | `std::exit(-1)`, **whole run ends** | `std::terminate()`, **whole run ends** | `return` from the current function | `return` from the current function | `std::terminate()`, **whole run ends** |
| H2 heap during run | ⚠️ yes: `new` per test plus `SimpleString`. Can be sent to a pool through its own allocator hooks | ✅ none in the core | ⚠️ yes (`std::ostringstream`, `std::vector`, …). Only fix: replace global `operator new` | ⚠️ yes (`std::string`, `std::vector`, `std::ostringstream`). Only fix: global `operator new` | ✅ none at all (fixed-capacity buffers) | ❌ `realloc` per failure, hard-wired | ❌ `new` per test, `std::string` | ❌ heavy `std::string` use |
| H3 output through our function | ✅ `TestOutput` subclass or platform `FPuts` hook | ✅ `UNITY_OUTPUT_CHAR(c)` | ⚠️ through a `std::ostream*` (needs iostream on the MCU) | ⚠️ custom printer, but the runner also writes to `std::cout` directly | ✅ `console_print` function | ❌ `printf` hard-wired | ⚠️ event listener (needs iostream/`std::string`) | ⚠️ `Catch::cout()` returning a `std::ostream&` |
| Failure shows test name + file:line | ✅ | ✅ `file:line:test:FAIL:msg` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Machine-readable format | JUnit (to files), TeamCity (to console) | text lines; Ruby/Python scripts turn them into JUnit | JUnit, XML (to the stream) | JUnit (to `std::cout`) | TeamCity, Catch2-XML (to the console callback) | xUnit XML, to a file only | XML/JSON, to files only | JUnit, TAP, XML, JSON, TeamCity, … |
| H4 bare metal | ✅ "GccNoStdC" platform; CI builds for Cortex-M4 | ✅ its main target | ⚠️ needs `<iostream>`, `<sstream>`, `<set>`. Got running on STM32F7 only with source patches (2021) | ⚠️ needs `<iostream>`, `<sstream>`. One user report on an STM32F769 under Mbed OS | ✅ "freestanding" build in CI (x86); no Cortex-M report found | ❌ `#error Unsupported platform!` | ❌ no bare-metal port | ❌ no bare-metal port |
| H5 C++20 | ✅ C++20 and C++23 in CI | ✅ (C library, `extern "C"` headers) | ✅ | ✅ (requires C++20) | ✅ (requires C++20) | ✅ | ✅ (needs ≥ C++17) | ✅ (needs ≥ C++14) |
| Fixtures | `TEST_GROUP` with `setup()`/`teardown()` | `setUp()`/`tearDown()` per file; `unity_fixture` groups | `TEST_CASE_FIXTURE`, `SUBCASE` | plain lambdas/scopes | `TEST_CASE_METHOD`, `SECTION` | `UTEST_F` with `UTEST_F_SETUP`/`TEARDOWN` | `TEST_F` | `TEST_CASE_METHOD`, `SECTION` |
| Mocking | CppUMock (built in) | CMock (C functions only) | none | none | none | none | GoogleMock | none |
| Test registration | automatic (static objects) | **manual list** or a generated runner (Ruby) | automatic | automatic | automatic | automatic | automatic | automatic |

## Which candidates fit all hard constraints

- **Fits all hard constraints without changing the framework: Unity, snitch, CppUTest.**
  - **Unity**: no heap and output through one character macro. Its costs are that tests are written in a C API (typed assert macros such as `TEST_ASSERT_EQUAL_INT`), and tests are not registered automatically.
  - **snitch**: no heap by design, C++ API in the Catch2 style, and an output function we supply. Its costs are that a failing `REQUIRE` without exceptions ends the whole run, it has a small community, and it has no reported Cortex-M use.
  - **CppUTest**: it allocates during the run, but through its own hooks, so those allocations can go to a static pool. Its output, a no-stdlib platform and a Cortex-M4 CI build are all built in, and it comes with a mocking library.
- **Fits only with workarounds: doctest, boost.ut.** Both allocate during the run through the standard library. The only way to meet H2 is to replace the global `operator new` with a static pool. Both also need `<iostream>` linked on the MCU, which carries a flash cost.
  - doctest without exceptions has no fatal assertions at all.
  - boost.ut ends the whole run on a fatal assertion.
- **Fails:**
  - **utest.h** does not compile for a bare-metal target (`#error`), has `printf` hard-wired and calls `realloc` on every failure.
  - **GoogleTest** and **Catch2** have no bare-metal port and depend on the heap. They remain usable as Host-Test-only baselines, but that breaks "same tests unchanged".

### Key differentiators for the decision

1. **What a fatal assertion does without exceptions.** This determines whether one failing test hides all the others in an Emulated Test run.
   - Only CppUTest and Unity keep running the remaining tests. They do it with `setjmp`/`longjmp`.
   - The C++ standard says a `setjmp`/`longjmp` pair has undefined behaviour "if replacing the setjmp and longjmp by catch and throw would invoke any non-trivial destructors for any objects with automatic storage duration" ([\[csetjmp.syn\]](https://eel.is/c++draft/csetjmp.syn)). So a fatal assertion fired while a test-double object with a destructor is alive on the test's stack is technically undefined behaviour.
   - doctest, snitch and GoogleTest avoid this by having no fatal asserts, or by using `CHECK` followed by `return`.
   - boost.ut, snitch's `REQUIRE` and Catch2 end the whole process instead.
2. **Heap.** Only snitch and Unity are heap-free. CppUTest routes its allocations through its own hooks. The others need a global `operator new` replacement.
3. **C++ ergonomics versus C heritage.**
   - Unity's C asserts don't print C++ types.
   - CppUTest is C++98-style but has CppUMock.
   - snitch and doctest have expression-decomposing asserts in the Catch2 style.
4. **Maturity on Cortex-M.**
   - Unity and CppUTest are proven there.
   - Only one patched or OS-based report each was found for doctest and boost.ut.
   - None was found for snitch.

## Per-candidate findings

### CppUTest (master `42feb87`, 2026-11-03; last tagged release v4.0, 2020-05-27)

1. **No exceptions / no RTTI.**
   - The config header detects both (`CPPUTEST_HAVE_EXCEPTIONS`, `CPPUTEST_HAVE_RTTI`, via `__cpp_exceptions`/`__GXX_RTTI`) ([CppUTestConfig.h L180–211](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/include/CppUTest/CppUTestConfig.h#L180-L211)).
   - CI has `no-rtti` and `no-exceptions` jobs ([basic.yml L201–206](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/.github/workflows/basic.yml#L201-L206)).
   - Without exceptions, a failing assertion leaves the test through `TestTerminatorWithoutExceptions::exitCurrentTest()`, which calls `PlatformSpecificLongJmp()` ([Utest.cpp L768–771](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/src/CppUTest/Utest.cpp#L768-L771)). The GCC platform implements this with `setjmp`/`longjmp` ([Gcc/UtestPlatform.cpp L164–197](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/src/Platforms/Gcc/UtestPlatform.cpp#L164-L197)). See the destructor caveat above.
2. **Heap.**
   - Each test object is created with `new` when it runs (`createTest() { return new TEST_…_Test; }`) ([UtestMacros.h L67](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/include/CppUTest/UtestMacros.h#L67)).
   - Messages use `SimpleString`, which allocates through a replaceable `TestMemoryAllocator` (`SimpleString::setStringAllocator`) ([SimpleString.cpp L107–120](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/src/CppUTest/SimpleString.cpp#L107-L120)).
   - All raw allocation in the no-stdlib platform goes through the function pointers `PlatformSpecificMalloc/Realloc/Free`, which we fill in ([GccNoStdC/UtestPlatform.cpp L65–67](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/src/Platforms/GccNoStdC/UtestPlatform.cpp#L65-L67)).
   - So the heap can be a static pool *(inference: a bump or pool allocator reset between tests)*.
   - Memory-leak detection is optional (`CPPUTEST_USE_MEM_LEAK_DETECTION`) ([CppUTestConfig.h L89–93](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/include/CppUTest/CppUTestConfig.h#L89-L93)).
3. **Output.**
   - All output goes through the abstract `TestOutput::printBuffer` ([TestOutput.h L64](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/include/CppUTest/TestOutput.h#L64)), or through the platform `PlatformSpecificFPuts`/`PlatformSpecificStdOut` hooks.
   - A failure prints the file, the line and the test name ([TestOutput.cpp L203–223](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/src/CppUTest/TestOutput.cpp#L203-L223)).
   - Formats are `-onormal|eclipse|junit|teamcity` ([CommandLineArguments.cpp L106–132](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/src/CppUTest/CommandLineArguments.cpp#L106-L132)).
   - JUnit opens one file per group ([JUnitTestOutput.cpp L270, L312](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/src/CppUTest/JUnitTestOutput.cpp#L270)), so on the MCU it would need the `FOpen`/`FPuts` hooks mapped to the UART.
   - TeamCity prints `##teamcity[testFailed …]` service messages through the normal console path ([TeamCityTestOutput.cpp L82](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/src/CppUTest/TeamCityTestOutput.cpp#L82)), so it is parseable over serial as-is.
4. **Bare metal.**
   - CMake options `CPPUTEST_STD_C_LIB_DISABLED` and `CPPUTEST_STD_CPP_LIB_DISABLED` exist ([CMakeLists.txt L30–35](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/CMakeLists.txt#L30-L35)).
   - The `GccNoStdC` platform leaves every OS function as a null pointer for the user to fill in (file, time, malloc, setjmp, mutex) ([GccNoStdC/UtestPlatform.cpp](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/src/Platforms/GccNoStdC/UtestPlatform.cpp)). `setjmp`/`longjmp` must therefore still be supplied (newlib has them).
   - CI has a "GCC ARM embedded" job using an `arm-none-eabi-gcc` toolchain file with `-mcpu=cortex-m4` and `rdimon.specs`, run under qemu ([basic.yml L217–219](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/.github/workflows/basic.yml#L217-L219), [arm-none-eabi-gcc.toolchain.cmake](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/cmake/Toolchains/arm-none-eabi-gcc.toolchain.cmake)). That job uses semihosting and the standard library, not the no-stdlib platform.
   - Footprint: not documented.
5. **C++20 and maintenance.**
   - CI builds C++98 through C++23, including GCC 20 and Clang 20 jobs ([basic.yml L120–192](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/.github/workflows/basic.yml#L120-L192)).
   - Master is active (last commit 2026-11-03), but the newest tagged release is v4.0 from 2020 ([releases](https://github.com/cpputest/cpputest/releases)). In practice you would pin a commit.
6. **Deep-test style.**
   - `TEST_GROUP` gives fixtures with `setup()`/`teardown()` ([UtestMacros.h L41–50](https://github.com/cpputest/cpputest/blob/42feb8740bea189667528bda8a2fd0eb745da637/include/CppUTest/UtestMacros.h#L41-L50)).
   - CppUTestExt includes CppUMock (`MockSupport.h`) ([include/CppUTestExt](https://github.com/cpputest/cpputest/tree/42feb8740bea189667528bda8a2fd0eb745da637/include/CppUTestExt)). It is string-keyed and uses the same allocator, so hand-written fakes at ports work equally well.

### Unity (ThrowTheSwitch) + CMock (Unity master `54f864c`; release v2.7.0, 2026-07-16)

1. **No exceptions / no RTTI.**
   - Unity is a C library and uses neither. Its headers are wrapped in `extern "C"` ([unity.h L17](https://github.com/ThrowTheSwitch/Unity/blob/54f864cf6027a50f4cc5424616e9e01e1bb729da/src/unity.h#L17)).
   - A test is aborted with `setjmp`/`longjmp` by default ([unity_internals.h L929–945](https://github.com/ThrowTheSwitch/Unity/blob/54f864cf6027a50f4cc5424616e9e01e1bb729da/src/unity_internals.h#L929-L945)).
   - `UNITY_EXCLUDE_SETJMP` turns the abort into a plain `return`, at the cost of custom helper functions and CMock. You can also supply your own `UNITY_TEST_PROTECT`/`UNITY_TEST_ABORT` ([UnityConfigurationGuide.md L400–420](https://github.com/ThrowTheSwitch/Unity/blob/54f864cf6027a50f4cc5424616e9e01e1bb729da/docs/UnityConfigurationGuide.md#unity_exclude_setjmp)).
2. **Heap.**
   - The core never calls `malloc` (no hits in `src/*.c`).
   - The optional `extras/memory` add-on can serve its own `unity_malloc` from a static buffer (`UNITY_EXCLUDE_STDLIB_MALLOC`) ([unity_memory.h L18–47](https://github.com/ThrowTheSwitch/Unity/blob/54f864cf6027a50f4cc5424616e9e01e1bb729da/extras/memory/src/unity_memory.h#L18-L47)).
3. **Output.**
   - Everything goes through `UNITY_OUTPUT_CHAR(a)`; the docs' own example is `RS232_putc(a)` ([UnityConfigurationGuide.md L302–325](https://github.com/ThrowTheSwitch/Unity/blob/54f864cf6027a50f4cc5424616e9e01e1bb729da/docs/UnityConfigurationGuide.md#unity_output_chara)).
   - A failure line is `file:line:testname:FAIL: message` ([unity.c `UnityTestResultsBegin`](https://github.com/ThrowTheSwitch/Unity/blob/54f864cf6027a50f4cc5424616e9e01e1bb729da/src/unity.c#L536)). The test name comes from `#func` in `RUN_TEST` ([unity_internals.h L984–986](https://github.com/ThrowTheSwitch/Unity/blob/54f864cf6027a50f4cc5424616e9e01e1bb729da/src/unity_internals.h#L984-L986)).
   - There is no XML format on the target. Host-side scripts convert the text output: `auto/stylize_as_junit.py` and `.rb`, and `auto/parse_output.rb` ([auto/](https://github.com/ThrowTheSwitch/Unity/tree/54f864cf6027a50f4cc5424616e9e01e1bb729da/auto)).
4. **Bare metal.**
   - Unity calls itself "built for C, with a focus on working with embedded toolchains" ([README.md L6–8](https://github.com/ThrowTheSwitch/Unity/blob/54f864cf6027a50f4cc5424616e9e01e1bb729da/README.md)).
   - It includes `<stdio.h>` only when no `UNITY_OUTPUT_CHAR` is given ([unity_internals.h L340–352](https://github.com/ThrowTheSwitch/Unity/blob/54f864cf6027a50f4cc5424616e9e01e1bb729da/src/unity_internals.h#L340)).
   - Footprint: not documented.
5. **C++20 and maintenance.** It is C, so it is called from C++20 test files. Release v2.7.0 is from 2026-07-16 and master is active (2026-10-31).
6. **Deep-test style.**
   - Tests are `void f(void)` functions with per-file `setUp`/`tearDown`, and must be listed in a runner. That is done by hand (`RUN_TEST`) or by generating the runner with `auto/generate_test_runner.rb`, which needs Ruby ([UnityHelperScriptsGuide.md](https://github.com/ThrowTheSwitch/Unity/blob/54f864cf6027a50f4cc5424616e9e01e1bb729da/docs/UnityHelperScriptsGuide.md)).
   - `extras/fixture` adds `TEST_GROUP`/`TEST` in the CppUTest style, but groups still have to be listed in `TEST_GROUP_RUNNER`. It "makes it more difficult to integrate with other testing tools like Ceedling and CMock" ([extras/fixture/readme.md](https://github.com/ThrowTheSwitch/Unity/blob/54f864cf6027a50f4cc5424616e9e01e1bb729da/extras/fixture/readme.md)).
   - Asserts are typed C macros, so there is no generic printing of C++ values.
   - CMock "does NOT mock non-static members. For those, you'll need an actual C++ mocking framework" ([CMock_Summary.md, C++ Support](https://github.com/ThrowTheSwitch/CMock/blob/34c26e540301bd434602aad5b6680ef51855bba1/docs/CMock_Summary.md#c-support)). It is not useful for C++ port interfaces, so port doubles would be hand-written.

### doctest (v2.5.3, 2026-07-06)

1. **No exceptions / no RTTI.**
   - `DOCTEST_CONFIG_NO_EXCEPTIONS` removes the `REQUIRE` and exception macros, and `abort-after` "won't be fully working" ([configuration.md](https://github.com/doctest/doctest/blob/v2.5.3/doc/markdown/configuration.md#doctest_config_no_exceptions)).
   - `…_BUT_WITH_ALL_ASSERTS` keeps `REQUIRE` but makes it behave like `CHECK` ([configuration.md](https://github.com/doctest/doctest/blob/v2.5.3/doc/markdown/configuration.md#doctest_config_no_exceptions_but_with_all_asserts)). So there is no fatal assertion that stops just the current test.
   - No `typeid`/`dynamic_cast` appears in `doctest.h`.
2. **Heap.**
   - Tests register into a `std::set<TestCase>` during static initialisation, which is startup.
   - During the run it uses `std::ostringstream`, `std::stringstream` and `std::vector` (for example the reporters' `std::vector<IReporter*>`, and the stringification streams) ([doctest.h](https://github.com/doctest/doctest/blob/v2.5.3/doctest/doctest.h)).
   - There is no allocator hook. The only way to meet H2 would be a project-wide replacement of `operator new`/`delete` *(inference)*.
3. **Output.**
   - Output goes through `Context::setCout(std::ostream*)` ([doctest.h `setCout`](https://github.com/doctest/doctest/blob/v2.5.3/doctest/doctest.h)). On the MCU that means an `std::ostream` over a custom UART `streambuf` *(inference)*.
   - The reporters are `console`, `junit` and `xml`.
4. **Bare metal.**
   - The header includes `<iostream>`, `<sstream>`, `<fstream>`, `<set>`, `<map>`, `<mutex>` and `<csignal>`. Config macros `DOCTEST_CONFIG_NO_POSIX_SIGNALS`, `DOCTEST_CONFIG_NO_MULTITHREADING` and `DOCTEST_CONFIG_NO_INCLUDE_IOSTREAM` exist ([configuration.md](https://github.com/doctest/doctest/blob/v2.5.3/doc/markdown/configuration.md)).
   - Issue [#506](https://github.com/doctest/doctest/issues/506) (2021, an older version, STM32F723) got it compiling only after removing the XML/JUnit reporters and the `std::mutex` uses by hand. One `CHECK` then used **191 KB flash / 7 KB RAM**. Output to a UART was not done in that report.
5. **C++20 and maintenance.** Active; the v2.5.x release line runs from 2026-03 to 2026-07 ([releases](https://github.com/doctest/doctest/releases)).
6. **Deep-test style.** `TEST_CASE_FIXTURE` and `SUBCASE`. No mocking.

### boost.ut (v2.3.1, 2025-04-02; master `872b8b9`)

1. **No exceptions / no RTTI.**
   - `try`/`catch` is compiled only `#if defined(__cpp_exceptions)` ([ut.hpp L96, L2118](https://github.com/boost-ext/ut/blob/872b8b9ef8e6d7ccd6e702f32a91b643ffa1caa7/include/boost/ut.hpp#L2118)). The repo's own tests build some suites with `-fno-exceptions` ([test/ft/CMakeLists.txt L20–25](https://github.com/boost-ext/ut/blob/872b8b9ef8e6d7ccd6e702f32a91b643ffa1caa7/test/ft/CMakeLists.txt#L20-L25)).
   - A fatal assertion makes the default runner call `std::exit(-1)` ([ut.hpp L2166–2169](https://github.com/boost-ext/ut/blob/872b8b9ef8e6d7ccd6e702f32a91b643ffa1caa7/include/boost/ut.hpp#L2166-L2169)), which ends the whole run.
   - No `typeid` found.
2. **Heap.** It uses `std::string`, `std::vector` and `std::ostringstream` in the runner and printer (for example [L578–579, L1478, L1618–1619](https://github.com/boost-ext/ut/blob/872b8b9ef8e6d7ccd6e702f32a91b643ffa1caa7/include/boost/ut.hpp#L1478)), so it allocates during the run. There is no allocator hook.
3. **Output.**
   - A custom printer can be plugged in through `ut::cfg<ut::override>` ([example/cfg/printer.cpp](https://github.com/boost-ext/ut/blob/872b8b9ef8e6d7ccd6e702f32a91b643ffa1caa7/example/cfg/printer.cpp); the maintainer's answer in issue [#527](https://github.com/boost-ext/ut/issues/527)).
   - The default runner still writes some lines to `std::cout` directly (summary, dry-run) ([L2031](https://github.com/boost-ext/ut/blob/872b8b9ef8e6d7ccd6e702f32a91b643ffa1caa7/include/boost/ut.hpp#L2031)).
   - A JUnit reporter exists (`reporter_junit`) and writes to `std::cout` ([L1593, L1683](https://github.com/boost-ext/ut/blob/872b8b9ef8e6d7ccd6e702f32a91b643ffa1caa7/include/boost/ut.hpp#L1593)).
4. **Bare metal.**
   - It includes `<iostream>`, `<fstream>` and `<sstream>`; `<unistd.h>` only when it is present ([L73–104](https://github.com/boost-ext/ut/blob/872b8b9ef8e6d7ccd6e702f32a91b643ffa1caa7/include/boost/ut.hpp#L73-L104)).
   - Issue [#527](https://github.com/boost-ext/ut/issues/527) reports it running on an STM32F769, but under Mbed OS with `std::cout` available, not bare metal.
5. **C++20 and maintenance.** It requires C++20. The last release is v2.3.1 (2025-04); commits continue (2026-10).
6. **Deep-test style.** Lambdas and suites; no fixture classes, no mocking.

### snitch (v1.3.2, 2025-03-22; master `c29ce5b`, 2026-10-07) — added candidate

1. **No exceptions / no RTTI.**
   - "Works with exceptions disabled". Without exceptions, `REQUIRE*()`, `FAIL()` and `SKIP()` "use `std::terminate()` … the whole test application stops". The README's advice is to use `CHECK*()` and return from the test yourself ([README, Exceptions](https://github.com/snitch-org/snitch/blob/c29ce5b465fb02250afb0cb35338488c0e6b1534/README.md#exceptions)).
   - No `typeid` found.
2. **Heap.**
   - "No heap allocation from the testing framework" ([README L59](https://github.com/snitch-org/snitch/blob/c29ce5b465fb02250afb0cb35338488c0e6b1534/README.md)).
   - Every container has a fixed capacity set at build time (`SNITCH_MAX_TEST_CASES` = 5000, `SNITCH_MAX_MESSAGE_LENGTH` = 1024, …) ([CMakeLists.txt L6–19](https://github.com/snitch-org/snitch/blob/c29ce5b465fb02250afb0cb35338488c0e6b1534/CMakeLists.txt#L6-L19)).
   - The registry holds a `small_vector<impl::test_case, max_test_cases>` ([snitch_registry.hpp L119](https://github.com/snitch-org/snitch/blob/c29ce5b465fb02250afb0cb35338488c0e6b1534/include/snitch/snitch_registry.hpp#L119)). Each `test_case` holds an id, a location, a function pointer and a state ([snitch_test_data.hpp L301–306](https://github.com/snitch-org/snitch/blob/c29ce5b465fb02250afb0cb35338488c0e6b1534/include/snitch/snitch_test_data.hpp#L301-L306)).
   - *(Estimate)* That is roughly 40+ bytes each on 32-bit, so the defaults reserve about 200 KB of static RAM. That is more than the F405's 128 KB SRAM, so the limits must be lowered for Emulated Tests.
3. **Output.**
   - With `SNITCH_WITH_STDOUT=OFF` you supply `snitch::cli::console_print` (a `std::string_view` function) ([README, Freestanding](https://github.com/snitch-org/snitch/blob/c29ce5b465fb02250afb0cb35338488c0e6b1534/README.md#freestanding)). Registry output goes through `print_callback` ([snitch_registry.hpp L159](https://github.com/snitch-org/snitch/blob/c29ce5b465fb02250afb0cb35338488c0e6b1534/include/snitch/snitch_registry.hpp#L159)).
   - The failure event carries the test id and the assertion location ([snitch_test_data.hpp L222–228](https://github.com/snitch-org/snitch/blob/c29ce5b465fb02250afb0cb35338488c0e6b1534/include/snitch/snitch_test_data.hpp#L222-L228)).
   - Reporters are TeamCity and Catch2-XML, plus custom reporters through callbacks ([README, Reporters](https://github.com/snitch-org/snitch/blob/c29ce5b465fb02250afb0cb35338488c0e6b1534/README.md#reporters)).
4. **Bare metal.**
   - CI has a "freestdg" job: `-fno-exceptions -ffreestanding` with timings, multithreading, stdout and file I/O all off ([cmake.yml L97](https://github.com/snitch-org/snitch/blob/c29ce5b465fb02250afb0cb35338488c0e6b1534/.github/workflows/cmake.yml#L97)). It runs on x86 Linux, not ARM.
   - No issue or report of Cortex-M use was found.
   - It still needs parts of the STL (`std::string_view`, `<charconv>`-style formatting). Footprint: not documented for MCUs.
5. **C++20 and maintenance.** It requires C++20. One main maintainer, 318 stars, last release March 2025, commits through October 2026.
6. **Deep-test style.** `TEST_CASE_METHOD(FIXTURE, …)` and `SECTION`, in the Catch2 style ([README L318](https://github.com/snitch-org/snitch/blob/c29ce5b465fb02250afb0cb35338488c0e6b1534/README.md)). No mocking.

### utest.h (master `fc4715d`, 2026-11-04; no version tags beyond `pre-ai`)

- **Bare metal (fails).** `utest_ns()` handles only Windows, Linux/BSD, Apple, AIX and Emscripten, and otherwise hits `#error Unsupported platform!` ([utest.h L380–414](https://github.com/sheredom/utest.h/blob/fc4715da3b3e3bfa936cf69850f2cfe969ed85b7/utest.h#L380-L414)). It does not compile for `arm-none-eabi` without patching.
- **Output (fails H3).** `UTEST_PRINTF` is hard-wired to `printf` plus an optional `fprintf` to the XML file ([L466–470](https://github.com/sheredom/utest.h/blob/fc4715da3b3e3bfa936cf69850f2cfe969ed85b7/utest.h#L466-L470)). The xUnit XML goes to a file only (`--output=`, [L1747–1774](https://github.com/sheredom/utest.h/blob/fc4715da3b3e3bfa936cf69850f2cfe969ed85b7/utest.h#L1747-L1774)).
- **Heap (fails H2).** Registration uses `realloc` at static initialisation, and the runner calls `realloc` again for every failed or skipped test during the run ([L1893–1902](https://github.com/sheredom/utest.h/blob/fc4715da3b3e3bfa936cf69850f2cfe969ed85b7/utest.h#L1893-L1902)).
- **No exceptions.** Works: it detects `__EXCEPTIONS` ([L104–113](https://github.com/sheredom/utest.h/blob/fc4715da3b3e3bfa936cf69850f2cfe969ed85b7/utest.h#L104-L113)), and `ASSERT_*` does a plain `return`.
- **Fixtures.** `UTEST_F` with `UTEST_F_SETUP`/`UTEST_F_TEARDOWN`.

### GoogleTest (v1.18.0, baseline)

- No exceptions and no RTTI are both supported through `GTEST_HAS_EXCEPTIONS`/`GTEST_HAS_RTTI` ([gtest-port.h L75–91, L453–501](https://github.com/google/googletest/blob/v1.18.0/googletest/include/gtest/internal/gtest-port.h#L453-L501)).
- `ASSERT_*` returns from the current function.
- **Heap.** Registration does `new TestInfo` ([gtest.cc L2853](https://github.com/google/googletest/blob/v1.18.0/googletest/src/gtest.cc#L2853)), and every test run does `new TestClass` ([gtest-internal.h L455](https://github.com/google/googletest/blob/v1.18.0/googletest/include/gtest/internal/gtest-internal.h#L455)). The public header pulls in `<string>`, `<sstream>`, `<vector>` and `<set>` ([gtest.h L52–62](https://github.com/google/googletest/blob/v1.18.0/googletest/include/gtest/gtest.h#L52-L62)).
- **Output.** Custom output goes through a `TestEventListener` ([advanced.md, Defining Event Listeners](https://github.com/google/googletest/blob/v1.18.0/docs/advanced.md#defining-event-listeners)). XML/JSON go to files. `GTEST_HAS_FILE_SYSTEM` can be set to 0.
- **Bare metal.** The only MCU-like ports are ESP32/ESP8266/NRF52/QN9090 special cases ([gtest-port-arch.h L111–121](https://github.com/google/googletest/blob/v1.18.0/googletest/include/gtest/internal/gtest-port-arch.h#L111-L121)). There is no generic bare-metal Cortex-M port; see open issue [#2739](https://github.com/google/googletest/issues/2739) on cross-compiling.
- Requires at least C++17 ([README L17](https://github.com/google/googletest/blob/v1.18.0/README.md)).
- Brings GoogleMock.

### Catch2 (v3.16.1, baseline)

- **No exceptions.** "Experimental". A failing `REQUIRE` calls `std::terminate` after the reporter returns, and internal errors also terminate ([configuration.md, disabling exceptions](https://github.com/catchorg/Catch2/blob/v3.16.1/docs/configuration.md#disabling-exceptions)).
- **Output.** `CATCH_CONFIG_NOSTDOUT` lets you supply `Catch::cout()/cerr()/clog()` as `std::ostream&` ([configuration.md, stdout](https://github.com/catchorg/Catch2/blob/v3.16.1/docs/configuration.md#stdout)). There are many reporters (JUnit, TAP, XML, JSON, TeamCity, SonarQube, Automake; [src/catch2/reporters](https://github.com/catchorg/Catch2/tree/v3.16.1/src/catch2/reporters)).
- **Heap.** Heavy `std::string` use during the run. The snitch README describes it as noisy, platform-dependent heap use ([snitch README L284](https://github.com/snitch-org/snitch/blob/c29ce5b465fb02250afb0cb35338488c0e6b1534/README.md)); that is a third-party but specific claim.
- **Bare metal.** No port. Open issues such as [#2877](https://github.com/catchorg/Catch2/issues/2877) and [#977](https://github.com/catchorg/Catch2/issues/977).
- Requires C++14 ([migrate-v2-to-v3.md](https://github.com/catchorg/Catch2/blob/v3.16.1/docs/migrate-v2-to-v3.md)).

### Dropped without a full check

- **Boost.Test** and **lest**: built around exceptions *(not verified in depth)*.

## Notes that apply to every candidate

- **Test doubles at ports are framework-independent.** Under ADR-0003, a port double is a hand-written class implementing the port's interface, injected where the adapter would be. That works the same in every framework above. Only CppUTest (CppUMock) and GoogleTest (GoogleMock) ship a mocking library. CMock cannot mock C++ member functions.
- **"Same tests unchanged" and exceptions.** If Host Tests were built *with* exceptions, doctest, snitch and Catch2 would get real per-test `REQUIRE` on the host but not on the MCU. Test behaviour would then differ between Host Tests and Emulated Tests. Whether Host Tests use the firmware's `-fno-exceptions` flags is part of the decision *(inference)*.
- **Flash and RAM.** The only measured MCU footprint found is doctest's 191 KB flash from issue #506. No project documents its own Cortex-M footprint. Measuring a minimal Emulated Test image is cheap once the toolchain exists.
