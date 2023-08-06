// sleeplock.h 定义了进程的长期锁（sleeplock）的数据结构和相关字段。
// 进程的长期锁用于保护长时间运行的资源，如文件系统中的文件、进程表等。
// struct sleeplock 结构体表示长期锁，包含了以下字段：
struct sleeplock {
  uint locked;       // 表示锁是否被持有，为 0 表示未被持有，为 1 表示被持有。
  struct spinlock lk; // 用于保护此 sleep lock 的自旋锁。

  // 用于调试和记录锁的信息
  char *name;        // 锁的名称，用于标识锁的用途。
  int pid;           // 持有该锁的进程的 PID，用于调试和追踪锁的持有情况。
};

// 进程的长期锁相较于普通的自旋锁（spinlock），适用于对于一些需要长时间保持的资源的保护，因为自旋锁会占用 CPU 时间，
// 在长时间等待资源时，如果使用自旋锁，会导致浪费大量的 CPU 资源。而进程的长期锁则允许资源持有者在资源不可用时休眠，
// 并且在资源可用时唤醒等待的进程，减少了 CPU 时间的浪费。
