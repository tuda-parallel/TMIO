#include "tmio.h"
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

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

TMIO_FORWARD_DECL(read, ssize_t, (int fd, void *buf, size_t count));

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

//! ----------------------- Sync Read ------------------------------

ssize_t TMIO_DECL(read)(int fd, void *buf, size_t count) {
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;
    int aligned_flag = 0;

	MAP_OR_FAIL(read);

	libc_iotrace.Read_Sync_Start(count, aligned_flag);
	ret = __real_read(fd, buf, count);
	libc_iotrace.Read_Sync_End();

	return(ret);
}

//! ----------------------- Wait and Test ------------------------------



#endif // ENABLE_LIBC_TRACE