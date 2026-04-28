#ifndef IOURING_INTERFACE_H
#define IOURING_INTERFACE_H
#include "tmio.h"
#if ENABLE_IOURING_TRACE == 1
IOtraceIOuring &get_iouring_iotrace(); // allows to access iotrace in application code
#endif
#endif // IOURING_INTERFACE_H