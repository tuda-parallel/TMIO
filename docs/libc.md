# POSIX I/O Tracing with TMIO

> [Test README](../test/README.md)

This guide explains how TMIO intercepts and analyzes POSIX I/O calls (libc) to provide detailed performance insights. It covers the supported interfaces, how to run the examples, and how to configure the tracing behavior using compile-time flags.

## Overview

TMIO's POSIX module uses `LD_PRELOAD` (or dynamic linking) to transparently intercept standard C library I/O calls. This allows you to trace I/O behavior in applications without modifying their source code. It supports:

*   **Standard I/O:** `read`, `write`, `open`, `close`.
*   **Positional I/O:** `pread`, `pwrite` (random access).
*   **Vector I/O:** `readv`, `writev` (scatter/gather).
*   **Asynchronous I/O (POSIX AIO):** `aio_read`, `aio_write`, `lio_listio`.

## How It Works

When you preload the TMIO library, it sits between your application and the operating system.
1.  **Interception:** When your app calls `write()`, TMIO catches it.
2.  **Measurement:** TMIO records the start time, data size, and file offset.
3.  **Execution:** TMIO calls the *real* `write()` function to perform the actual I/O.
4.  **Completion:** Upon return, TMIO records the end time and calculates bandwidth.

## Running the POSIX Example

> [POSIX example README](../examples/libc/README.md)

We provide a standard example to demonstrate how TMIO captures different POSIX I/O patterns.

### 1. Compile the Library and Example
First, ensure the TMIO library is built with libc tracing enabled.

```bash
# In the root TMIO directory
make clean
make library ENABLE_LIBC_TRACE=1
cd examples/libc
mpicxx -o POSIX_example POSIX_example.cxx
```

### 2. Run with Tracing
You do not need to change the example code. Simply run it while preloading the TMIO library.

```bash
# Run the example
mpirun -n 1 -x LD_PRELOAD=./build/libtmio.so ./POSIX_example
```

### 3. Understanding the Output

The example would use pthread read to read from the file.

TMIO will generate a summary at the end (and potentially a JSON log file depending on flags), showing:
*   Total I/O time vs. Application time.
*   Bandwidth for Read and Write phases.
*   Overhead introduced by tracing.

---

## Supported Interfaces

TMIO intercepts the following POSIX functions:

### Synchronous I/O
*   `open`, `open64`, `creat`, `creat64`
*   `close`
*   `write`, `read`
*   `pwrite`, `pwrite64`, `pread`, `pread64` (Positional)
*   `writev`, `readv` (Vector/Scatter-Gather)

### Asynchronous I/O (AIO)
*   `aio_write`, `aio_write64`
*   `aio_read`, `aio_read64`
*   `lio_listio`, `lio_listio64` (Batch)
*   `aio_error`, `aio_return` (Completion monitoring)
*   `aio_suspend` (Waiting)

---

## Troubleshooting

**Q: I don't see any output.**
*   Check if `ENABLE_LIBC_TRACE` is set to `1` in ioflags.h.
*   Ensure you are using `LD_PRELOAD` correctly pointing to `libtmio.so`.

**Q: The application crashes with `lio_listio`.**
*   If using `BATCH_LIO=0` (Individual tracing), ensure your application handles `EINTR` correctly, as tracing individual items adds slight latency that might expose race conditions in the application's signal handling.

**Q: My log file is huge.**
*   Reduce `IOTRACE_VERBOSE` to `0` or `1`.
*   Set `ALL_SAMPLES` to a lower value (e.g., `0`) to aggregate data instead of logging every byte.