// PROTOTYPE (issue #18): CppUTest platform layer for bare metal, modelled on
// src/Platforms/GccNoStdC (which leaves every hook NULL for the user to fill in).
// Here every hook is filled: output to USART2, setjmp/longjmp from newlib, vsnprintf
// from eyalroz/printf (heap-free), and malloc/free plus global operator new/delete on a
// fixed static pool. CppUTest can't run without that allocator: every TEST is created
// with `new`, and SimpleString allocates through PlatformSpecificMalloc.
#include "CppUTest/PlatformSpecificFunctions.h"
#include "CppUTest/TestHarness.h"

#include <setjmp.h>
#include <string.h>

#include "board.hpp"
#include "printf/printf.h"

// ---- static-pool allocator (first fit, no coalescing: enough for a smoke test) ----
namespace {
constexpr size_t pool_size = 8192;
alignas(8) unsigned char pool[pool_size];
size_t pool_top = 0; // bytes of the pool ever touched (high-water)

struct block {
    size_t size;
    block* next; // only meaningful while the block is on the free list
};
block* free_list = nullptr;

void* pool_alloc(size_t n) {
    n = (n + 7u) & ~size_t{7};
    for (block** p = &free_list; *p != nullptr; p = &(*p)->next) {
        if ((*p)->size >= n) {
            block* b = *p;
            *p = b->next;
            return reinterpret_cast<unsigned char*>(b) + sizeof(block);
        }
    }
    if (pool_top + sizeof(block) + n > pool_size) {
        uart_write("\n!! allocator pool exhausted\n");
        for (;;) {}
    }
    auto* b = reinterpret_cast<block*>(pool + pool_top);
    b->size = n;
    pool_top += sizeof(block) + n;
    return reinterpret_cast<unsigned char*>(b) + sizeof(block);
}

void pool_free(void* p) {
    if (p == nullptr) return;
    auto* b = reinterpret_cast<block*>(static_cast<unsigned char*>(p) - sizeof(block));
    b->next = free_list;
    free_list = b;
}

void* pool_realloc(void* p, size_t n) {
    if (p == nullptr) return pool_alloc(n);
    auto* b = reinterpret_cast<block*>(static_cast<unsigned char*>(p) - sizeof(block));
    if (b->size >= n) return p;
    void* q = pool_alloc(n);
    memcpy(q, p, b->size);
    pool_free(p);
    return q;
}
} // namespace

std::size_t cpputest_pool_high_water() noexcept { return pool_top; }

void* operator new(size_t n) { return pool_alloc(n); }
void* operator new[](size_t n) { return pool_alloc(n); }
void operator delete(void* p) noexcept { pool_free(p); }
void operator delete[](void* p) noexcept { pool_free(p); }
void operator delete(void* p, size_t) noexcept { pool_free(p); }
void operator delete[](void* p, size_t) noexcept { pool_free(p); }

// ---- fatal-check exit: setjmp/longjmp, as in CppUTest's Gcc platform ----
namespace {
jmp_buf test_exit_jmp_buf[4]; // upstream uses 10; nesting here is 1-2 deep
int jmp_buf_index = 0;

int set_jmp(void (*function)(void*), void* data) {
    if (setjmp(test_exit_jmp_buf[jmp_buf_index]) == 0) {
        ++jmp_buf_index;
        function(data);
        --jmp_buf_index;
        return 1;
    }
    return 0;
}
void long_jmp() {
    --jmp_buf_index;
    longjmp(test_exit_jmp_buf[jmp_buf_index], 1);
}
void restore_jump_buffer() { --jmp_buf_index; }

void fputs_uart(const char* s, PlatformSpecificFile) { uart_write(s); }
void no_op() {}
unsigned long time_zero() { return 0; }
const char* time_string() { return ""; }
double fabs_(double d) { return __builtin_fabs(d); }
int isnan_(double d) { return __builtin_isnan(d); }
int isinf_(double d) { return __builtin_isinf(d); }
int atexit_(void (*)()) { return 0; }
PlatformSpecificMutex mutex_create() { return nullptr; }
void mutex_no_op(PlatformSpecificMutex) {}
void srand_(unsigned int) {}
int rand_() { return 4; }
void abort_() {
    uart_write("\n!! CppUTest abort\n");
    for (;;) {}
}
void* memcpy_(void* d, const void* s, size_t n) { return memcpy(d, s, n); }
void* memset_(void* m, int c, size_t n) { return memset(m, c, n); }
} // namespace

TestOutput::WorkingEnvironment PlatformSpecificGetWorkingEnvironment() { return TestOutput::eclipse; }

void (*PlatformSpecificRunTestInASeperateProcess)(UtestShell*, TestPlugin*, TestResult*) = NULLPTR;
int (*PlatformSpecificFork)() = NULLPTR;
int (*PlatformSpecificWaitPid)(int, int*, int) = NULLPTR;

void (*PlatformSpecificLongJmp)() = long_jmp;
int (*PlatformSpecificSetJmp)(void (*)(void*), void*) = set_jmp;
void (*PlatformSpecificRestoreJumpBuffer)() = restore_jump_buffer;

unsigned long (*GetPlatformSpecificTimeInMillis)() = time_zero;
const char* (*GetPlatformSpecificTimeString)() = time_string;

PlatformSpecificFile PlatformSpecificStdOut = reinterpret_cast<PlatformSpecificFile>(1);
PlatformSpecificFile (*PlatformSpecificFOpen)(const char*, const char*) = NULLPTR;
void (*PlatformSpecificFPuts)(const char*, PlatformSpecificFile) = fputs_uart;
void (*PlatformSpecificFClose)(PlatformSpecificFile) = NULLPTR;
void (*PlatformSpecificFlush)() = no_op;

int (*PlatformSpecificVSNprintf)(char*, size_t, const char*, va_list) = vsnprintf_;

void* (*PlatformSpecificMalloc)(size_t) = pool_alloc;
void* (*PlatformSpecificRealloc)(void*, size_t) = pool_realloc;
void (*PlatformSpecificFree)(void*) = pool_free;
void* (*PlatformSpecificMemCpy)(void*, const void*, size_t) = memcpy_;
void* (*PlatformSpecificMemset)(void*, int, size_t) = memset_;

double (*PlatformSpecificFabs)(double) = fabs_;
int (*PlatformSpecificIsNan)(double) = isnan_;
int (*PlatformSpecificIsInf)(double) = isinf_;
int (*PlatformSpecificAtExit)(void (*)()) = atexit_;

PlatformSpecificMutex (*PlatformSpecificMutexCreate)() = mutex_create;
void (*PlatformSpecificMutexLock)(PlatformSpecificMutex) = mutex_no_op;
void (*PlatformSpecificMutexUnlock)(PlatformSpecificMutex) = mutex_no_op;
void (*PlatformSpecificMutexDestroy)(PlatformSpecificMutex) = mutex_no_op;

void (*PlatformSpecificSrand)(unsigned int) = srand_;
int (*PlatformSpecificRand)() = rand_;
void (*PlatformSpecificAbort)() = abort_;
