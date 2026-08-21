#ifndef IOURING_INTERFACE_H
#define IOURING_INTERFACE_H
#include "tmio.h"
#if ENABLE_IOURING_TRACE == 1
IOtraceIOuring &get_iouring_iotrace(); // allows to access iotrace in application code

#if BW_LIMIT_POSIX_AIO == 1
// Untraced access to the real io_uring_queue_init, for TMIO's own internal
// io_uring usage (POSIX AIO emulation in libc_interface.cxx). That ring must
// never be handed to Register_Ring: it doesn't represent an application-level
// io_uring the app is directly responsible for, and registering it would make
// the io_uring tracer double-count transactions already reported by
// get_libc_iotrace() as POSIX Async I/O.
int tmio_real_io_uring_queue_init(unsigned entries, struct io_uring *ring, unsigned flags);
#endif
#endif
#endif // IOURING_INTERFACE_H