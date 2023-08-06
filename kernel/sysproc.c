// 连接用户空间与内核空间的桥梁，通过它可以实现用户程序和操作系统内核之间的交互。

#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

// 系统调用：终止当前进程
uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // 不会执行到这里
}

// 系统调用：获取当前进程的进程ID
uint64
sys_getpid(void)
{
  return myproc()->pid;
}

// 系统调用：创建一个新的子进程
uint64
sys_fork(void)
{
  return fork();
}

// 系统调用：等待子进程退出，并获取子进程的退出状态
uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

// 系统调用：改变当前进程的堆大小
uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;

  addr = myproc()->sz; // 获取当前进程的堆大小
  if(growproc(n) < 0)  // 调用 growproc 函数扩展堆空间
    return -1;
  return addr; // 返回旧的堆大小作为调用结果
}

// 系统调用：使当前进程进入睡眠状态一段时间
uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;

  acquire(&tickslock);
  ticks0 = ticks; // 记录当前时间
  while(ticks - ticks0 < n){
    if(myproc()->killed){ // 如果当前进程被标记为 killed，则立即返回
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock); // 释放锁并进入睡眠状态，等待时钟中断唤醒
  }
  release(&tickslock);
  return 0;
}

#ifdef LAB_PGTBL
// 系统调用：用于实验中的页表操作（待学习实验内容后，填写相关代码）
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  return 0;
}
#endif

// 系统调用：向指定进程发送信号，终止指定进程
uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid); // 调用 kill 函数终止指定进程
}

// 系统调用：返回自系统启动以来经过的时钟滴答数（时钟中断次数）
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks; // 获取当前时钟滴答数
  release(&tickslock);
  return xticks; // 返回时钟滴答数作为调用结果
}
