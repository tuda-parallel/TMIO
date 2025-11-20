#ifndef IOTRACE_TRAITS_H
#define IOTRACE_TRAITS_H

#include "ioflags.h" // For ENABLE flags

// Include necessary headers for the types used in specializations
#include <mpi.h>
#include <aio.h>
#include <liburing.h>

// * @brief Dynamic tag dispatching of IOtrace
// * @details This is a template class that uses a tag to determine the type of IOtrace to use.
// ! NOTE: Add explicit specialization (in the end of `iotrace_base.cxx`) for each tag to avoid linker errors.

// Primary template declaration (forward declaration)
template <typename T>
struct IOtraceTraits;

// --- Specializations ---

struct MPI_Tag
{
};
template <>
struct IOtraceTraits<MPI_Tag>
{
    using RequestType = MPI_Request;
    using RequestIDType = MPI_Request *;
    static constexpr const char *Name = "MPI";
};

struct Libc_Tag
{
};
template <>
struct IOtraceTraits<Libc_Tag>
{
    using RequestType = struct aiocb;
    using RequestIDType = struct aiocb *;
    static constexpr const char *Name = "Libc";
};

struct IOuring_Tag
{
};
template <>
struct IOtraceTraits<IOuring_Tag>
{
    using RequestType = __u64;
    using RequestIDType = __u64;
    static constexpr const char *Name = "IOuring";
};

#endif // IOTRACE_TRAITS_H