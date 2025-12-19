# Flags and Configuration Options

### 1. Core Tracing Switches
These flags control which I/O interfaces are intercepted by TMIO.

| Flag Name | Default | Description |
| :--- | :---: | :--- |
| `ENABLE_MPI_TRACE` | `1` | **MPI Interception.** Set to `1` to enable tracing of MPI I/O calls. |
| `ENABLE_LIBC_TRACE` | `1` | **POSIX/Libc Interception.** Set to `1` to enable tracing of standard C library I/O (read, write, open, etc.). |
| `ENABLE_IOURING_TRACE` | `0` | **io_uring Interception.** Set to `1` to enable tracing of Linux `io_uring` interfaces. |
| `IO_BEFORE_MAIN` | `0` | **Static Initialization.** <br>`0`: Start tracing when `main()` begins.<br>`1`: Trace I/O occurring in static constructors before `main()`. |

### 2. Logging & Verbosity
Controls the runtime output printed to `stdout`.

| Flag Name | Default | Description |
| :--- | :---: | :--- |
| `IOTRACE_VERBOSE` | `0` | **Runtime Log Level.**<br>`0` (NONE): Silent.<br>`1` (BASIC): Errors and high-level info.<br>`2` (DETAILED): Start/End of operations.<br>`3` (DEBUG): Internal state changes.<br>`4` (TRACE): Raw event stream. |
| `FUNCTION_INFO` | `0` | **Function Call Tracing.**<br>`0`: Disabled.<br>`1`: Print function calls to stdout.<br>`2`: Enhanced (PID/TID tagged, process-safe, printed at Finalize).<br>`3`: Main thread only. |
| `COLOR_OUTPUT` | *Defined* | If defined, enables colored text output in the terminal for better readability. |

### 3. Output Data & Granularity
Controls the format and detail level of the generated trace files (e.g., JSON).

| Flag Name | Default | Description |
| :--- | :---: | :--- |
| `ALL_SAMPLES` | `5` | **JSON Output Detail Level.**<br>`0`: No file output.<br>`1`: Overlapping phase bandwidths only.<br>`2`: Adds active/request times for phase changes.<br>`3`: Adds bandwidth/throughput for all ranks.<br>`4`: Adds start/act/req timestamps.<br>`5`: **Full Detail.** Logs every single I/O operation (Start, End, Size, Offset). |
| `FILE_FORMAT` | `0` | **Output Format.**<br>`0`: JSON Lines (`.jsonl`).<br>`1`: Binary.<br>`2`: MessagePack.<br>`3`: ZeroMQ (Streaming). |
| `OVERLAP_GRAPH` | `0` | Set to `1` to print a text-based bandwidth overlap graph to stdout. |

### 4. Calculation & Analysis Logic
Controls how TMIO calculates metrics like bandwidth and overhead.

| Flag Name | Default | Description |
| :--- | :---: | :--- |
| `ONLINE` | `1` | **Analysis Mode.**<br>`1`: Calculate bandwidth statistics during runtime (Online).<br>`0`: Defer calculation to post-processing (Offline). |
| `BATCH_LIO` | `1` | **Batch I/O Strategy (`lio_listio`).**<br>`1`: Aggregate entire batch into one trace event (Low Overhead).<br>`0`: Trace each request in the batch individually (High Detail). |
| `OVERHEAD` | `1` | Set to `1` to calculate and report the runtime overhead introduced by TMIO. |
| `SYNC_MODE` | `0` | **Sync Phase Logic.**<br>`0`: Phase starts/ends per sync I/O operation.<br>`1`: Phase spans from first sync I/O until file close. |
| `SKIP_*` | `0` | `SKIP_FIRST_WRITE`, `SKIP_LAST_READ`, etc. Number of operations to exclude from calculation (useful for warmup/teardown). |

### 5. Advanced / Experimental Features
Flags for specialized use cases (Bandwidth Limiting, DFT, etc.).

| Flag Name | Default | Description |
| :--- | :---: | :--- |
| `BW_LIMIT` | *Undef* | If defined, enables Bandwidth Limiting (requires custom MPI). |
| `BW_LIMIT_STRATEGY`| `2` | `0`: Always limit. `1`: Increase only. `2`: Limit downside. |
| `DFT` | `0` | **Discrete Fourier Transform.** (Deprecated) Performs frequency analysis on I/O phases. |
| `DFT_TIME_WINDOW` | `1` | `0`: Standard window. `1`: Adaptive window based on frequency. |