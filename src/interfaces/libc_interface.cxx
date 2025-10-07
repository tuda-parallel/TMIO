#include "tmio.h"
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <aio.h>

#ifdef __linux__
#define _GNU_SOURCE // For preadv2, must be defined before including any headers
#include <sys/uio.h>
#endif

// Check for the prototype
#if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 10))
// preadv is available in glibc since 2.10
#define HAVE_PREADV
#endif
#if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 26))
// preadv2 is available in glibc since 2.26
#define HAVE_PREADV2
#endif

#if ENABLE_LIBC_TRACE == 1
IOtraceLibc& get_libc_iotrace() {
	// Must use static IOtraceLibc instance to ensure a single instance is initialized when first used.
	// TODO: Might need to use a mutex to protect the instance creation if multiple threads might access it simultaneously.
	// `thread_local` is for some MPI implementations that has a separate MPI progress thread, 
	// which differs from the main thread, and both the main thread and the MPI progress thread
	// might access the IOtraceLibc instance simultaneously.  So we use thread_local to ensure
	// that each thread has its own instance of IOtraceLibc.
    static thread_local IOtraceLibc instance;
    return instance;
}

TMIO_FORWARD_DECL(open, int, (const char *path, int flags, ...));
// See：https://github.com/darshan-hpc/darshan/issues/253 for `__open_2`
TMIO_FORWARD_DECL(__open_2, int, (const char *path, int oflag));
TMIO_FORWARD_DECL(open64, int, (const char *path, int flags, ...));
TMIO_FORWARD_DECL(openat, int, (int dirfd, const char *pathname, int flags, ...));
TMIO_FORWARD_DECL(openat64, int, (int dirfd, const char *pathname, int flags, ...));
// TMIO_FORWARD_DECL(creat, int, (const char* path, mode_t mode));
// TMIO_FORWARD_DECL(creat64, int, (const char* path, mode_t mode));
TMIO_FORWARD_DECL(close, int, (int fd));

TMIO_FORWARD_DECL(aio_read, int, (struct aiocb * aiocbp));
TMIO_FORWARD_DECL(aio_read64, int, (struct aiocb64 * aiocbp));
TMIO_FORWARD_DECL(aio_write, int, (struct aiocb * aiocbp));
TMIO_FORWARD_DECL(aio_write64, int, (struct aiocb64 * aiocbp));
TMIO_FORWARD_DECL(aio_error, int, (const struct aiocb *aiocbp));
// `restrict` said cannot be multiple pointer to the same object, but C++ does not support `restrict` keyword, so we use `__restrict__`
TMIO_FORWARD_DECL(aio_suspend, int, (const struct aiocb *const aiocb_list[], int n, const struct timespec *__restrict__ timeout));
TMIO_FORWARD_DECL(aio_return, ssize_t, (struct aiocb * aiocbp));
TMIO_FORWARD_DECL(aio_return64, ssize_t, (struct aiocb64 * aiocbp));
TMIO_FORWARD_DECL(lio_listio, int, (int mode, struct aiocb *const aiocb_list[], int nitems, struct sigevent *sevp));
TMIO_FORWARD_DECL(lio_listio64, int, (int mode, struct aiocb64 *const aiocb_list[], int nitems, struct sigevent *sevp));

TMIO_FORWARD_DECL(read, ssize_t, (int fd, void *buf, size_t count));
TMIO_FORWARD_DECL(write, ssize_t, (int fd, const void *buf, size_t count));
TMIO_FORWARD_DECL(pread, ssize_t, (int fd, void *buf, size_t count, off_t offset));
TMIO_FORWARD_DECL(pread64, ssize_t, (int fd, void *buf, size_t count, off64_t offset));
TMIO_FORWARD_DECL(pwrite, ssize_t, (int fd, const void *buf, size_t count, off_t offset));
TMIO_FORWARD_DECL(pwrite64, ssize_t, (int fd, const void *buf, size_t count, off64_t offset));
TMIO_FORWARD_DECL(readv, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off_t offset));
#ifdef HAVE_PREADV
TMIO_FORWARD_DECL(preadv, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off_t offset));
TMIO_FORWARD_DECL(preadv64, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off64_t offset));
#endif // HAVE_PREADV
#ifdef HAVE_PREADV2
TMIO_FORWARD_DECL(preadv2, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags));
TMIO_FORWARD_DECL(preadv64v2, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags));
#endif // HAVE_PREADV2
TMIO_FORWARD_DECL(writev, ssize_t, (int fd, const struct iovec *iov, int iovcnt));
#ifdef HAVE_PREADV
TMIO_FORWARD_DECL(pwritev, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off_t offset));
TMIO_FORWARD_DECL(pwritev64, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off64_t offset));
#endif // HAVE_PREADV
#ifdef HAVE_PREADV2
TMIO_FORWARD_DECL(pwritev2, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags));
TMIO_FORWARD_DECL(pwritev64v2, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags));
#endif // HAVE_PREADV2

// //! ----------------------- Open, Close, and Create ------------------------------

int TMIO_DECL(open)(const char *path, int flags, ...)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int mode = 0;
	int ret;

	MAP_OR_FAIL(open);

	if (flags & O_CREAT)
	{
		va_list arg;
		va_start(arg, flags);
		mode = va_arg(arg, int);
		va_end(arg);

		ret = __real_open(path, flags, mode);
	}
	else
	{
		ret = __real_open(path, flags);
	}

	get_libc_iotrace().Open();
	return (ret);
}

int TMIO_DECL(__open_2)(const char *path, int oflag)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

	MAP_OR_FAIL(__open_2);

	ret = __real_open(path, oflag);

	get_libc_iotrace().Open();

	return (ret);
}

int TMIO_DECL(open64)(const char *path, int flags, ...)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int mode = 0;
	int ret;

	MAP_OR_FAIL(open64);

	if (flags & O_CREAT)
	{
		va_list arg;
		va_start(arg, flags);
		mode = va_arg(arg, int);
		va_end(arg);

		ret = __real_open64(path, flags, mode);
	}
	else
	{
		ret = __real_open64(path, flags);
	}

	get_libc_iotrace().Open();
	return (ret);
}

int TMIO_DECL(openat)(int dirfd, const char *pathname, int flags, ...)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int mode = 0;
	int ret;

	MAP_OR_FAIL(openat);

	if (flags & O_CREAT)
	{
		va_list arg;
		va_start(arg, flags);
		mode = va_arg(arg, int);
		va_end(arg);

		ret = __real_openat(dirfd, pathname, flags, mode);
	}
	else
	{
		ret = __real_openat(dirfd, pathname, flags);
	}

	get_libc_iotrace().Open();
	return (ret);
}

int TMIO_DECL(openat64)(int dirfd, const char *pathname, int flags, ...)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int mode = 0;
	int ret;

	MAP_OR_FAIL(openat64);

	if (flags & O_CREAT)
	{
		va_list arg;
		va_start(arg, flags);
		mode = va_arg(arg, int);
		va_end(arg);

		ret = __real_openat64(dirfd, pathname, flags, mode);
	}
	else
	{
		ret = __real_openat64(dirfd, pathname, flags);
	}

	get_libc_iotrace().Open();
	return (ret);
}

int TMIO_DECL(close)(int fd)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

	MAP_OR_FAIL(close);

	ret = __real_close(fd);

	get_libc_iotrace().Close();

	// std::cout << "TMIO > close(" << fd << ")" << std::endl;

	return (ret);
}

//! ----------------------- Async Write ------------------------------
int TMIO_DECL(aio_write)(struct aiocb *aiocbp)
{
	std::string function_name = __PRETTY_FUNCTION__;
	// Also append the fd to the function name for better debugging
	function_name += " (fd: " + std::to_string(aiocbp->aio_fildes) + ")";
	Function_Debug(function_name);
	int ret;

	MAP_OR_FAIL(aio_write);

	get_libc_iotrace().Write_Async_Start(aiocbp);
	ret = __real_aio_write(aiocbp);
	return (ret);
}

int TMIO_DECL(aio_write64)(struct aiocb64 *aiocbp)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

	MAP_OR_FAIL(aio_write64);

	get_libc_iotrace().Write_Async_Start(aiocbp);
	ret = __real_aio_write64(aiocbp);
	return (ret);
}

//! ----------------------- Async Read ------------------------------
int TMIO_DECL(aio_read)(struct aiocb *aiocbp)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

	MAP_OR_FAIL(aio_read);

	get_libc_iotrace().Read_Async_Start(aiocbp);
	ret = __real_aio_read(aiocbp);
	return (ret);
}

int TMIO_DECL(aio_read64)(struct aiocb64 *aiocbp)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

	MAP_OR_FAIL(aio_read64);

	get_libc_iotrace().Read_Async_Start(aiocbp);
	ret = __real_aio_read64(aiocbp);
	return (ret);
}

//! ----------------------- Wait and Test ------------------------------
/**
 * @brief Trace the only actual completion of an asynchronous I/O operation.
 * @param aiocbp Pointer to the aiocb structure representing the I/O operation.
 * @return The number of bytes read or written, or -1 on error.
 */
int TMIO_DECL(aio_error)(const struct aiocb *aiocbp)
{
	std::string function_name = __PRETTY_FUNCTION__;
	// Also append the fd to the function name for better debugging
	function_name += " (fd: " + std::to_string(aiocbp->aio_fildes) + ")";
	Function_Debug(function_name);

	int ret;

	MAP_OR_FAIL(aio_error);

	ret = __real_aio_error(aiocbp);

    switch (ret)
    {
        case EINPROGRESS:
            // Request has not been completed yet; do not trace
            break;
        case ECANCELED:
            // Request was canceled; trace and print error
            get_libc_iotrace().Read_Async_End(aiocbp);
            get_libc_iotrace().Write_Async_End(aiocbp);
            fprintf(stderr, "TMIO > aio_error with aio_fildes: %d was canceled (ECANCELED: %s)\n", aiocbp->aio_fildes, strerror(ECANCELED));
            break;
        case 0:
            // Request completed successfully; trace
            get_libc_iotrace().Read_Async_End(aiocbp);
            get_libc_iotrace().Write_Async_End(aiocbp);
            break;
        default:
            if (ret > 0) {
                // A positive error number: operation failed; trace and print error
                get_libc_iotrace().Read_Async_End(aiocbp);
                get_libc_iotrace().Write_Async_End(aiocbp);
                fprintf(stderr, "TMIO > aio_error with aio_fildes: %d failed with error code: %s\n", aiocbp->aio_fildes, strerror(ret));
            }
            break;
    }

	return (ret);
}

/**
 * @brief Record the required finished time for those AIO requests.
 * @note TMIO should treat a call to aio_suspend as an explicit indication from the user that
 * 		 they are waiting for the specified AIO requests to complete.
 *
 * 		 Therefore, the point at which aio_suspend are called (the relevant request(s) should be
 * 		 later confirmed complete via aio_error) would be used to record the required finished time for those AIO requests.
 * @param aiocbp Pointer to the aiocb structure representing the I/O operation.
 * @return The number of bytes read or written, or -1 on error.
 */
int TMIO_DECL(aio_suspend)(const struct aiocb *const aiocb_list[], int nitems, const struct timespec *__restrict__ timeout)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

	MAP_OR_FAIL(aio_suspend);

	for (int i = 0; i < nitems; ++i)
	{
		if (aiocb_list[i] != nullptr)
		{
			get_libc_iotrace().Read_Async_Required(aiocb_list[i]);
			get_libc_iotrace().Write_Async_Required(aiocb_list[i]);
		}
	}
	ret = __real_aio_suspend(aiocb_list, nitems, timeout);
	return (ret);
}

int TMIO_DECL(aio_suspend64)(const struct aiocb64 *const aiocb_list[], int nitems, const struct timespec *__restrict__ timeout)
{
	Function_Debug(__PRETTY_FUNCTION__);

	return aio_suspend(reinterpret_cast<const struct aiocb *const *>(aiocb_list), nitems, timeout); // Cast aiocb64 to aiocb
}

/**
 * @brief Fallback for `aio_suspend`
 *
 * @note For actual time: aio_return further confirms the completion and
 * 		could be a point to finalize the recording of the "actual" finished time
 * 		if not already definitively captured.
 *
 * 		For required time: If TMIO hasn't already established a "required" finish time
 * 		for an AIO request (e.g., through an encompassing aio_suspend),
 * 		could the call to aio_return for that request serve as a fallback
 * 		to record its "required" finished time. This bhv is configurable by
 * 		preprocessing flags to disable this option if needed.
 */
ssize_t TMIO_DECL(aio_return)(struct aiocb *aiocbp)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(aio_return);

	ret = __real_aio_return(aiocbp);

	if (ret != -1) // If success, trace
	{
		// Further confirm the completion of actual time
		get_libc_iotrace().Read_Async_End(aiocbp);
		get_libc_iotrace().Write_Async_End(aiocbp);

		// Fallback for required time
		get_libc_iotrace().Read_Async_Required(aiocbp);
		get_libc_iotrace().Write_Async_Required(aiocbp);
	}

	return (ret);
}

ssize_t TMIO_DECL(aio_return64)(struct aiocb64 *aiocbp)
{
	Function_Debug(__PRETTY_FUNCTION__);

	return aio_return(reinterpret_cast<struct aiocb *>(aiocbp)); // Cast aiocb64 to aiocb
}
// //! ----------------------- List I/O ------------------------------
int TMIO_DECL(lio_listio)(int mode, struct aiocb *const aiocb_list[], int nitems, struct sigevent *sevp)
{
	Function_Debug(__PRETTY_FUNCTION__);

	ssize_t ret;
	MAP_OR_FAIL(lio_listio);

	if (mode == LIO_WAIT) // Sync
	{
#if BATCH_LIO == 1
		// Merge all aiocb into one overarching request, but only for tracing
		int batch_read_iovcnt = 0;
		int batch_write_iovcnt = 0;
		const static int OFFSET = INT32_MAX; // Use a large offset to avoid conflicts

		for (int i = 0; i < nitems; ++i)
		{
			if (aiocb_list[i] != nullptr)
			{
				if (aiocb_list[i]->aio_lio_opcode == LIO_READ)
					batch_read_iovcnt += aiocb_list[i]->aio_nbytes;
				else if (aiocb_list[i]->aio_lio_opcode == LIO_WRITE)
					batch_write_iovcnt += aiocb_list[i]->aio_nbytes;
				else
					continue; // LIO_NOP
			}
		}

		if (batch_read_iovcnt > 0)
			get_libc_iotrace().Read_Sync_Start(batch_read_iovcnt, OFFSET);

		if (batch_write_iovcnt > 0)
			get_libc_iotrace().Write_Sync_Start(batch_write_iovcnt, OFFSET);
#else
		for (int i = 0; i < nitems; ++i)
		{
			if (aiocb_list[i] != nullptr)
			{
				if (aiocb_list[i]->aio_lio_opcode == LIO_READ)
					get_libc_iotrace().Batch_Read_Sync_Start(aiocb_list[i]->aio_nbytes, aiocb_list[i]->aio_offset);
				else if (aiocb_list[i]->aio_lio_opcode == LIO_WRITE)
					get_libc_iotrace().Batch_Write_Sync_Start(aiocb_list[i]->aio_nbytes, aiocb_list[i]->aio_offset);
				else
					continue; // LIO_NOP
			}
		}

#endif

		ret = __real_lio_listio(mode, aiocb_list, nitems, sevp);

		if (ret != 0)
			std::cerr << "TMIO > lio_listio(LIO_WAIT) failed or interrupted: " << strerror(errno) << " (errno=" << errno << "). TMIO tracing result might be inaccurate. Better to call the lio_list again" << std::endl;

// End the sync tracing
#if BATCH_LIO == 1
		if (batch_read_iovcnt > 0)
			get_libc_iotrace().Read_Sync_End();
		if (batch_write_iovcnt > 0)
			get_libc_iotrace().Write_Sync_End();
#else
		for (int i = 0; i < nitems; ++i)
		{
			if (aiocb_list[i] != nullptr)
			{
				if (aiocb_list[i]->aio_lio_opcode == LIO_READ)
					get_libc_iotrace().Batch_Read_Sync_End();
				else if (aiocb_list[i]->aio_lio_opcode == LIO_WRITE)
					get_libc_iotrace().Batch_Write_Sync_End();
				else
					continue; // LIO_NOP
			}
		}
#endif
	}
	else if (mode == LIO_NOWAIT)
	{
		for (int i = 0; i < nitems; ++i)
		{
			if (aiocb_list[i] != nullptr)
			{
				get_libc_iotrace().Read_Async_Start(aiocb_list[i]);
				get_libc_iotrace().Write_Async_Start(aiocb_list[i]);
			}
		}

		ret = __real_lio_listio(mode, aiocb_list, nitems, sevp);

		// We cannot use aio_err to check each request.
		// If we use it to check a non-initialized aiocb, it would return the same result as success call.
		if (ret != 0)
			std::cerr << "TMIO > lio_listio(LIO_NOWAIT) failed: " << strerror(errno) << " (errno=" << errno << "). TMIO tracing result might be inaccurate. Better to call the lio_list again" << std::endl;
	} else
	{
		std::cerr << "TMIO > lio_listio: unsupported mode: " << mode << ". Only LIO_WAIT and LIO_NOWAIT are supported." << std::endl;
	}
	return (ret);
}

int TMIO_DECL(lio_listio64)(int mode, struct aiocb64 *const aiocb_list[], int nitems, struct sigevent *sevp)
{
	Function_Debug(__PRETTY_FUNCTION__);

	ssize_t ret;
	MAP_OR_FAIL(lio_listio);

	if (mode == LIO_WAIT) // Sync
	{
#if BATCH_LIO == 1
		// Merge all aiocb into one overarching request, but only for tracing
		int batch_read_iovcnt = 0;
		int batch_write_iovcnt = 0;
		const static int OFFSET = INT32_MAX; // Use a large offset to avoid conflicts

		for (int i = 0; i < nitems; ++i)
		{
			if (aiocb_list[i] != nullptr)
			{
				if (aiocb_list[i]->aio_lio_opcode == LIO_READ)
					batch_read_iovcnt += aiocb_list[i]->aio_nbytes;
				else if (aiocb_list[i]->aio_lio_opcode == LIO_WRITE)
					batch_write_iovcnt += aiocb_list[i]->aio_nbytes;
				else
					continue; // LIO_NOP
			}
		}

		if (batch_read_iovcnt > 0)
			get_libc_iotrace().Read_Sync_Start(batch_read_iovcnt, OFFSET);

		if (batch_write_iovcnt > 0)
			get_libc_iotrace().Write_Sync_Start(batch_write_iovcnt, OFFSET);
#else
		for (int i = 0; i < nitems; ++i)
		{
			if (aiocb_list[i] != nullptr)
			{
				if (aiocb_list[i]->aio_lio_opcode == LIO_READ)
					get_libc_iotrace().Read_Sync_Start(aiocb_list[i]->aio_nbytes, aiocb_list[i]->aio_offset);
				else if (aiocb_list[i]->aio_lio_opcode == LIO_WRITE)
					get_libc_iotrace().Write_Sync_Start(aiocb_list[i]->aio_nbytes, aiocb_list[i]->aio_offset);
				else
					continue; // LIO_NOP
			}
		}

#endif

		ret = __real_lio_listio64(mode, aiocb_list, nitems, sevp);

		if (ret != 0)
			std::cerr << "TMIO > lio_listio(LIO_WAIT) failed or interrupted: " << strerror(errno) << " (errno=" << errno << "). TMIO tracing result might be inaccurate. Better to call the lio_list again" << std::endl;

// End the sync tracing
#if BATCH_LIO == 1
		if (batch_read_iovcnt > 0)
			get_libc_iotrace().Read_Sync_End();
		if (batch_write_iovcnt > 0)
			get_libc_iotrace().Write_Sync_End();
#else
		for (int i = 0; i < nitems; ++i)
		{
			if (aiocb_list[i] != nullptr)
			{
				if (aiocb_list[i]->aio_lio_opcode == LIO_READ)
					get_libc_iotrace().Read_Sync_End();
				else if (aiocb_list[i]->aio_lio_opcode == LIO_WRITE)
					get_libc_iotrace().Write_Sync_End();
				else
					continue; // LIO_NOP
			}
		}
#endif
	}
	else if (mode == LIO_NOWAIT)
	{
		for (int i = 0; i < nitems; ++i)
		{
			if (aiocb_list[i] != nullptr)
			{
				get_libc_iotrace().Read_Async_Start(aiocb_list[i]);
				get_libc_iotrace().Write_Async_Start(aiocb_list[i]);
			}
		}

		ret = __real_lio_listio64(mode, aiocb_list, nitems, sevp);

		// We cannot use aio_err to check each request.
		// If we use it to check a non-initialized aiocb, it would return the same result as success call.
		if (ret != 0)
			std::cerr << "TMIO > lio_listio(LIO_NOWAIT) failed: " << strerror(errno) << " (errno=" << errno << "). TMIO tracing result might be inaccurate. Better to call the lio_list again" << std::endl;
	} else
	{
		std::cerr << "TMIO > lio_listio: unsupported mode: " << mode << ". Only LIO_WAIT and LIO_NOWAIT are supported." << std::endl;
	}
	return (ret);
}
//! ----------------------- Sync Read & Write ------------------------------

ssize_t TMIO_DECL(read)(int fd, void *buf, size_t count)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(read);

	get_libc_iotrace().Read_Sync_Start(count);
	ret = __real_read(fd, buf, count);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}

ssize_t TMIO_DECL(write)(int fd, const void *buf, size_t count)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(write);

	get_libc_iotrace().Write_Sync_Start(count);
	ret = __real_write(fd, buf, count);
	get_libc_iotrace().Write_Sync_End();

	return (ret);
}

ssize_t TMIO_DECL(pread)(int fd, void *buf, size_t count, off_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pread);

	get_libc_iotrace().Read_Sync_Start(count, static_cast<off64_t>(offset));
	ret = __real_pread(fd, buf, count, offset);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(pread64)(int fd, void *buf, size_t count, off64_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pread64);

	get_libc_iotrace().Read_Sync_Start(count, offset);
	ret = __real_pread64(fd, buf, count, offset);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(pwrite)(int fd, const void *buf, size_t count, off_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwrite);

	get_libc_iotrace().Write_Sync_Start(count, static_cast<off64_t>(offset));
	ret = __real_pwrite(fd, buf, count, offset);
	get_libc_iotrace().Write_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(pwrite64)(int fd, const void *buf, size_t count, off64_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwrite64);

	get_libc_iotrace().Write_Sync_Start(count, offset);
	ret = __real_pwrite64(fd, buf, count, offset);
	get_libc_iotrace().Write_Sync_End();
	return (ret);
}

//! ----------------------- Readv and Writev ------------------------------
ssize_t TMIO_DECL(readv)(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(readv);

	get_libc_iotrace().Read_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_readv(fd, iov, iovcnt, offset);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(writev)(int fd, const struct iovec *iov, int iovcnt)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(writev);

	get_libc_iotrace().Write_Sync_Start(iovcnt);
	ret = __real_writev(fd, iov, iovcnt);
	get_libc_iotrace().Write_Sync_End();

	return (ret);
}
#ifdef HAVE_PREADV
ssize_t TMIO_DECL(preadv)(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(preadv);

	get_libc_iotrace().Read_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_preadv(fd, iov, iovcnt, offset);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(preadv64)(int fd, const struct iovec *iov, int iovcnt, off64_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(preadv64);

	get_libc_iotrace().Read_Sync_Start(iovcnt, offset);
	ret = __real_preadv64(fd, iov, iovcnt, offset);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(pwritev)(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwritev);

	get_libc_iotrace().Write_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_pwritev(fd, iov, iovcnt, offset);
	get_libc_iotrace().Write_Sync_End();
	return (ret);
}
ssize_t TMIO_DECL(pwritev64)(int fd, const struct iovec *iov, int iovcnt, off64_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwritev64);

	get_libc_iotrace().Write_Sync_Start(iovcnt, offset);
	ret = __real_pwritev64(fd, iov, iovcnt, offset);
	get_libc_iotrace().Write_Sync_End();
	return (ret);
}
#ifdef HAVE_PREADV2
ssize_t TMIO_DECL(preadv2)(int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(preadv2);

	get_libc_iotrace().Read_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_preadv2(fd, iov, iovcnt, offset, flags);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(preadv64v2)(int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(preadv64v2);

	get_libc_iotrace().Read_Sync_Start(iovcnt, offset);
	ret = __real_preadv64v2(fd, iov, iovcnt, offset, flags);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(pwritev2)(int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwritev2);

	get_libc_iotrace().Write_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_pwritev2(fd, iov, iovcnt, offset, flags);
	get_libc_iotrace().Write_Sync_End();
	return (ret);
}
ssize_t TMIO_DECL(pwritev64v2)(int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwritev64v2);

	get_libc_iotrace().Write_Sync_Start(iovcnt, offset);
	ret = __real_pwritev64v2(fd, iov, iovcnt, offset, flags);
	get_libc_iotrace().Write_Sync_End();
	return (ret);
}
#endif // HAVE_PREADV2
#endif // HAVE_PREADV

#endif // ENABLE_LIBC_TRACE