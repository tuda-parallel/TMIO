#include "tmio.h"
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

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
IOtraceLibc libc_iotrace;

TMIO_FORWARD_DECL(open, int, (const char *path, int flags, ...));
// See：https://github.com/darshan-hpc/darshan/issues/253 for `__open_2`
TMIO_FORWARD_DECL(__open_2, int, (const char *path, int oflag));
TMIO_FORWARD_DECL(open64, int, (const char *path, int flags, ...));
TMIO_FORWARD_DECL(openat, int, (int dirfd, const char *pathname, int flags, ...));
TMIO_FORWARD_DECL(openat64, int, (int dirfd, const char *pathname, int flags, ...));
// TMIO_FORWARD_DECL(creat, int, (const char* path, mode_t mode));
// TMIO_FORWARD_DECL(creat64, int, (const char* path, mode_t mode));
TMIO_FORWARD_DECL(close, int, (int fd));

TMIO_FORWARD_DECL(aio_read, int, (struct aiocb *aiocbp));
TMIO_FORWARD_DECL(aio_read64, int, (struct aiocb64 *aiocbp));
TMIO_FORWARD_DECL(aio_write, int, (struct aiocb *aiocbp));
TMIO_FORWARD_DECL(aio_write64, int, (struct aiocb64 *aiocbp));
TMIO_FORWARD_DECL(aio_error, int, (const struct aiocb *aiocbp));
TMIO_FORWARD_DECL(aio_return, ssize_t, (struct aiocb *aiocbp));
TMIO_FORWARD_DECL(aio_return64, ssize_t, (struct aiocb64 *aiocbp));
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

	libc_iotrace.Open();
	return (ret);
}

int TMIO_DECL(__open_2)(const char *path, int oflag)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

	MAP_OR_FAIL(__open_2);

	ret = __real_open(path, oflag);

	libc_iotrace.Open();

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

	libc_iotrace.Open();
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

	libc_iotrace.Open();
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

	libc_iotrace.Open();
	return (ret);
}

int TMIO_DECL(close)(int fd) {
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

	MAP_OR_FAIL(close);

	ret = __real_close(fd);

	libc_iotrace.Close();

	// std::cout << "TMIO > close(" << fd << ")" << std::endl;

	return(ret);
}

//! ----------------------- Async Write ------------------------------

//! ----------------------- Async Read ------------------------------

//! ----------------------- Sync Read & Write ------------------------------

ssize_t TMIO_DECL(read)(int fd, void *buf, size_t count) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(read);

	libc_iotrace.Read_Sync_Start(count);
	ret = __real_read(fd, buf, count);
	libc_iotrace.Read_Sync_End();

	return(ret);
}

ssize_t TMIO_DECL(write)(int fd, const void *buf, size_t count) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(write);

	libc_iotrace.Write_Sync_Start(count);
	ret = __real_write(fd, buf, count);
	libc_iotrace.Write_Sync_End();

	return(ret);
}

ssize_t TMIO_DECL(pread)(int fd, void *buf, size_t count, off_t offset) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pread);

	libc_iotrace.Read_Sync_Start(count, static_cast<off64_t>(offset));
	ret = __real_pread(fd, buf, count, offset);
	libc_iotrace.Read_Sync_End();

	return(ret);
}
ssize_t TMIO_DECL(pread64)(int fd, void *buf, size_t count, off64_t offset) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pread64);

	libc_iotrace.Read_Sync_Start(count, offset);
	ret = __real_pread64(fd, buf, count, offset);
	libc_iotrace.Read_Sync_End();

	return(ret);
}
ssize_t TMIO_DECL(pwrite)(int fd, const void *buf, size_t count, off_t offset) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwrite);

	libc_iotrace.Write_Sync_Start(count, static_cast<off64_t>(offset));
	ret = __real_pwrite(fd, buf, count, offset);
	libc_iotrace.Write_Sync_End();

	return(ret);
}
ssize_t TMIO_DECL(pwrite64)(int fd, const void *buf, size_t count, off64_t offset) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwrite64);

	libc_iotrace.Write_Sync_Start(count, offset);
	ret = __real_pwrite64(fd, buf, count, offset);
	libc_iotrace.Write_Sync_End();
	return(ret);
}

//! ----------------------- Readv and Writev ------------------------------
ssize_t TMIO_DECL(readv)(int fd, const struct iovec *iov, int iovcnt, off_t offset) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(readv);

	libc_iotrace.Read_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_readv(fd, iov, iovcnt, offset);
	libc_iotrace.Read_Sync_End();

	return(ret);
}
ssize_t TMIO_DECL(writev)(int fd, const struct iovec *iov, int iovcnt) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(writev);

	libc_iotrace.Write_Sync_Start(iovcnt);
	ret = __real_writev(fd, iov, iovcnt);
	libc_iotrace.Write_Sync_End();

	return(ret);
}
#ifdef HAVE_PREADV
ssize_t TMIO_DECL(preadv)(int fd, const struct iovec *iov, int iovcnt, off_t offset) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(preadv);

	libc_iotrace.Read_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_preadv(fd, iov, iovcnt, offset);
	libc_iotrace.Read_Sync_End();

	return(ret);
}
ssize_t TMIO_DECL(preadv64)(int fd, const struct iovec *iov, int iovcnt, off64_t offset) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(preadv64);

	libc_iotrace.Read_Sync_Start(iovcnt, offset);
	ret = __real_preadv64(fd, iov, iovcnt, offset);
	libc_iotrace.Read_Sync_End();

	return(ret);
}
ssize_t TMIO_DECL(pwritev)(int fd, const struct iovec *iov, int iovcnt, off_t offset) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwritev);

	libc_iotrace.Write_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_pwritev(fd, iov, iovcnt, offset);
	libc_iotrace.Write_Sync_End();
	return(ret);
}
ssize_t TMIO_DECL(pwritev64)(int fd, const struct iovec *iov, int iovcnt, off64_t offset) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwritev64);

	libc_iotrace.Write_Sync_Start(iovcnt, offset);
	ret = __real_pwritev64(fd, iov, iovcnt, offset);
	libc_iotrace.Write_Sync_End();
	return(ret);
}
#ifdef HAVE_PREADV2
ssize_t TMIO_DECL(preadv2)(int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(preadv2);

	libc_iotrace.Read_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_preadv2(fd, iov, iovcnt, offset, flags);
	libc_iotrace.Read_Sync_End();

	return(ret);
}
ssize_t TMIO_DECL(preadv64v2)(int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(preadv64v2);

	libc_iotrace.Read_Sync_Start(iovcnt, offset);
	ret = __real_preadv64v2(fd, iov, iovcnt, offset, flags);
	libc_iotrace.Read_Sync_End();

	return(ret);
}
ssize_t TMIO_DECL(pwritev2)(int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwritev2);

	libc_iotrace.Write_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_pwritev2(fd, iov, iovcnt, offset, flags);
	libc_iotrace.Write_Sync_End();
	return(ret);
}
ssize_t TMIO_DECL(pwritev64v2)(int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwritev64v2);

	libc_iotrace.Write_Sync_Start(iovcnt, offset);
	ret = __real_pwritev64v2(fd, iov, iovcnt, offset, flags);
	libc_iotrace.Write_Sync_End();
	return(ret);
}
#endif // HAVE_PREADV2
#endif // HAVE_PREADV

//! ----------------------- Wait and Test ------------------------------



#endif // ENABLE_LIBC_TRACE