// 睡眠锁

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"

// 初始化睡眠锁，分别初始化lk成员，包括内部的锁lk、名称name、锁状态locked和持有锁的进程pid。
void
initsleeplock(struct sleeplock *lk, char *name)
{
  initlock(&lk->lk, "sleep lock");
  lk->name = name;
  lk->locked = 0;
  lk->pid = 0;
}

// 获取睡眠锁。
void
acquiresleep(struct sleeplock *lk)
{
  acquire(&lk->lk); // 首先获取内部的锁lk，保证操作的原子性。
  while (lk->locked) { // 如果睡眠锁已被其他进程持有，则循环等待。
    sleep(lk, &lk->lk); // 等待lk解锁，并在此处释放lk锁，让其他进程有机会获取它。
  }
  lk->locked = 1; // 当lk没有被其他进程持有时，将它标记为已锁定状态。
  lk->pid = myproc()->pid; // 记录当前进程的PID作为持有锁的进程。
  release(&lk->lk); // 释放内部的锁lk。
}

// 释放睡眠锁。
void
releasesleep(struct sleeplock *lk)
{
  acquire(&lk->lk); // 首先获取内部的锁lk，保证操作的原子性。
  lk->locked = 0; // 将睡眠锁标记为未锁定状态。
  lk->pid = 0; // 清除持有锁的进程ID。
  wakeup(lk); // 唤醒在lk上睡眠的进程，让它们有机会获取该锁。
  release(&lk->lk); // 释放内部的锁lk。
}

// 检查当前进程是否持有睡眠锁。
int
holdingsleep(struct sleeplock *lk)
{
  int r;
  
  acquire(&lk->lk); // 首先获取内部的锁lk，保证操作的原子性。
  r = lk->locked && (lk->pid == myproc()->pid); // 当lk被当前进程持有时，返回1；否则返回0。
  release(&lk->lk); // 释放内部的锁lk。
  return r;
}
