// 互斥自旋锁

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"

// 初始化自旋锁，分别设置锁的名称name、锁状态locked和持有锁的CPU编号cpu。
void
initlock(struct spinlock *lk, char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

// 获取锁。
// 循环等待（自旋）直到锁被获取。
void
acquire(struct spinlock *lk)
{
  push_off(); // 禁用中断，以避免死锁。
  if(holding(lk))
    panic("acquire");

  // 在RISC-V中，sync_lock_test_and_set转换为原子交换指令：
  //   a5 = 1
  //   s1 = &lk->locked
  //   amoswap.w.aq a5, a5, (s1)
  while(__sync_lock_test_and_set(&lk->locked, 1) != 0)
    ;

  // 告知C编译器和处理器在此点之后不要移动加载或存储指令，
  // 确保临界区的内存引用严格发生在锁被获取之后。
  // 在RISC-V中，这会生成一个fence指令。
  __sync_synchronize();

  // 记录有关锁获取的信息，用于holding()和调试。
  lk->cpu = mycpu();
}

// 释放锁。
void
release(struct spinlock *lk)
{
  if(!holding(lk))
    panic("release");

  lk->cpu = 0;

  // 告知C编译器和处理器在此点之后不要移动加载或存储指令，
  // 确保临界区内的所有存储对其他CPU可见，然后再释放锁，
  // 并且临界区内的所有加载发生在锁被释放之前。
  // 在RISC-V中，这会生成一个fence指令。
  __sync_synchronize();

  // 释放锁，相当于lk->locked = 0。
  // 这里不使用C的赋值操作，因为C标准暗示赋值可能由多个存储指令实现。
  // 在RISC-V中，sync_lock_release转换为原子交换指令：
  //   s1 = &lk->locked
  //   amoswap.w zero, zero, (s1)
  __sync_lock_release(&lk->locked);

  pop_off();
}

// 检查当前CPU是否持有锁。
// 中断必须关闭。
int
holding(struct spinlock *lk)
{
  int r;
  r = (lk->locked && lk->cpu == mycpu());
  return r;
}

// push_off/pop_off类似于intr_off()/intr_on()，但它们是匹配的：
// 两个push_off()需要两个pop_off()才能撤消。另外，如果中断一开始就关闭，
// 那么push_off，pop_off将使中断仍然关闭。

// 关闭中断。
void
push_off(void)
{
  // 获取当前中断状态
  int old = intr_get();

  // 禁用中断
  intr_off();

  // 如果noff为0，说明之前中断是开启的，保存旧的中断状态到intena中
  if(mycpu()->noff == 0)
    mycpu()->intena = old;

  // 将当前CPU的noff加1，表示关闭了一个中断
  mycpu()->noff += 1;
}

// 恢复中断状态。
void
pop_off(void)
{
  // 获取当前CPU
  struct cpu *c = mycpu();

  // 检查当前中断是否开启，如果开启则触发panic
  if(intr_get())
    panic("pop_off - interruptible");

  // 检查noff是否小于1，如果小于1说明中断状态有问题，触发panic
  if(c->noff < 1)
    panic("pop_off");

  // 将当前CPU的noff减1，表示恢复了一个中断
  c->noff -= 1;

  // 如果noff为0且intena为非零，说明之前中断是开启的，恢复中断
  if(c->noff == 0 && c->intena)
    intr_on();
}
