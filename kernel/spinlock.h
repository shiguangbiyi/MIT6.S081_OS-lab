// spinlock.h 定义了互斥锁（spinlock）的数据结构和相关字段。
// 互斥锁用于保护临界区，确保在多线程或多处理器环境下，同一时刻只有一个线程可以进入临界区。
// struct spinlock 结构体表示互斥锁，包含了以下字段：
struct spinlock {
  uint locked;       // 表示锁是否被持有，为 0 表示未被持有，为 1 表示被持有。

  // 用于调试和记录锁的信息
  char *name;        // 锁的名称，用于标识锁的用途。
  struct cpu *cpu;   // 持有该锁的 CPU，用于调试和追踪锁的持有情况。
};
// 在多线程或多处理器环境中，使用互斥锁可以防止多个线程同时访问临界区，保证了临界区的互斥性。
