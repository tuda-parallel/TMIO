似乎不允许对同一个fd启动多个异步IO操作。不过其实是可以的，但是IO操作会被入列
      /* The current file descriptor is worked on.  It makes no sense
	 to start another thread since this new thread would fight
	 with the running thread for the resources.  But we also cannot
	 say that the thread processing this desriptor shall immediately
	 after finishing the current job process this request if there
	 are other threads in the running queue which have a higher
	 priority.  */
     
Glibc's POSIX AIO can't have more than one I/O in flight on a single file descriptor whereas io_uring most certainly can!

No. You can almost assume nothing on their completion order

而且似乎可以同时排队的AIO有限制


POSIX AIO 是一个用户级实现，它在多个线程中执行正常的阻塞 I/O，因此给人一种 I/O 是异步的错觉。用户空间线程池

不过既然AIO是用户空间实现，底层依靠的又是原来的read/write等阻塞IO，这些IO接口又是reentrant，所以AIO应该也是reentrant的。

现在就直接如果发现IO失败，也不一个个检测了。毕竟都不能明确到底有没有被初始化，最好还是直接给用户进行提示好了
