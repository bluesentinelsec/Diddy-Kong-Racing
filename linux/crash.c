// Crash reporting for the host build: on a fatal signal print the game frame and
// a backtrace to stderr and to dkracing-crash.log in the working directory,
// then re-raise the signal so the shell (or a debugger) still sees the fault.
//
// The backtrace comes from execinfo, which exists on macOS and glibc. Frames
// show function names from the symbol table; for file:line, run the same
// binary under `make run-debugger`, or feed the addresses to atos (macOS) or
// addr2line (Linux). MinGW has no execinfo, so Windows only gets the frame line.

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gfx.h"

#if defined(__APPLE__) || defined(__GLIBC__)
#define CRASH_HAVE_BACKTRACE 1
#include <execinfo.h>
#else
#define CRASH_HAVE_BACKTRACE 0
#endif

#define CRASH_LOG_FILE "dkracing-crash.log"

#if !defined(_WIN32)
static char sAltStack[64 * 1024]; // so a stack overflow can still report
#endif

static const char *signal_name(int sig) {
    switch (sig) {
        case SIGSEGV:
            return "SIGSEGV (segmentation fault)";
#if !defined(_WIN32)
        case SIGBUS:
            return "SIGBUS (bus error)";
#endif
        case SIGFPE:
            return "SIGFPE (arithmetic fault)";
        case SIGILL:
            return "SIGILL (illegal instruction)";
        case SIGABRT:
            return "SIGABRT (abort)";
        default:
            return "fatal signal";
    }
}

// Only async-signal-safe calls below: write(2) and backtrace_symbols_fd.
static void write_str(int fd, const char *s) {
    write(fd, s, strlen(s));
}

static void write_uint(int fd, unsigned long v) {
    char buf[24];
    int n = sizeof(buf);

    buf[--n] = '\0';
    do {
        buf[--n] = (char) ('0' + v % 10);
        v /= 10;
    } while (v != 0);
    write_str(fd, buf + n);
}

static void report(int fd, int sig) {
    write_str(fd, "\n=== DKR PC crash: ");
    write_str(fd, signal_name(sig));
    write_str(fd, " at gfx frame ");
    write_uint(fd, gfx_frame_count());
    write_str(fd, " ===\n");
#if CRASH_HAVE_BACKTRACE
    {
        void *frames[64];
        int count = backtrace(frames, (int) (sizeof(frames) / sizeof(frames[0])));
        backtrace_symbols_fd(frames, count, fd);
    }
#else
    write_str(fd, "(no backtrace support on this platform; run under make run-debugger)\n");
#endif
    write_str(fd, "=== end of crash report (also in " CRASH_LOG_FILE ") ===\n");
}

static void crash_handler(int sig) {
    int fd;

    report(STDERR_FILENO, sig);
    fd = open(CRASH_LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        report(fd, sig);
        close(fd);
    }
    signal(sig, SIG_DFL);
    raise(sig);
}

void pc_crash_handler_install(void) {
#if defined(_WIN32)
    static const int sigs[] = { SIGSEGV, SIGFPE, SIGILL, SIGABRT };
#else
    static const int sigs[] = { SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT };
#endif
    size_t i;

    if (getenv("DKR_NO_CRASH_HANDLER") != NULL) {
        return; // leave the fault to the debugger or the OS crash reporter
    }
#if !defined(_WIN32)
    {
        stack_t ss;
        struct sigaction sa;

        ss.ss_sp = sAltStack;
        ss.ss_size = sizeof(sAltStack);
        ss.ss_flags = 0;
        sigaltstack(&ss, NULL);

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = crash_handler;
        sa.sa_flags = SA_ONSTACK | SA_RESETHAND;
        sigemptyset(&sa.sa_mask);
        for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++) {
            sigaction(sigs[i], &sa, NULL);
        }
    }
#else
    for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++) {
        signal(sigs[i], crash_handler);
    }
#endif
}
