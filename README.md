### 基于Boost fcontext_t和io_uring的C++高性能协程异步框架，相比传统的epoll框架，具有更高的性能，支持更多的系统调用

- 基于Boost fcontext_t实现非对称有栈协程，实现纳秒级协程切换
- 基于io_uring实现异步系统调用框架，支持存储文件和网络文件IO操作，也支持更多的异步系统调用 （accept/openat/stat/...）
- 基于io_uring_linked_timeout 实现任意操作的超时取消机制
- 基于时间堆实现纳秒级定时器，支持定时事件的管理
- 基于Futex和原子变量实现协程级的锁、条件变量和信号量等同步机制
- 支持hook相关系统调用库函数，实现无感协程
- 基于面对对象思想，封装File、Socket相关操作，实现SocketAddressResolver，仿照iostream实现输入输出流
- 使用本框架实现简易服务器，使用wrk进行压力测试，并发连接数量1000，单线程RPS 30w+，多线程RPS 130w+
