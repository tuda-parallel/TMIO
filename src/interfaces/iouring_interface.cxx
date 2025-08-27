#include "tmio.h"
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <liburing.h>

#if ENABLE_IOURING_TRACE == 1
IOtraceIOuring& get_iouring_iotrace() {
    static thread_local IOtraceIOuring instance;
    return instance;
}
#endif