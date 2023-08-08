#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"


uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL // 仅在定义了LAB_PGTBL时编译以下代码
// 系统调用：报告已访问的页面
int
sys_pgaccess(void)
{
  uint64 srcva, st; // 输入参数的虚拟地址和存储位掩码的用户地址
  int len; // 要检查的页面数量
  uint64 buf = 0; // 用于存储位掩码的缓冲区
  struct proc *p = myproc(); // 获取当前进程

  acquire(&p->lock); // 获取进程锁，确保不会发生竞争

  // 解析系统调用参数
  argaddr(0, &srcva); // 获取第一个参数：要检查的用户虚拟地址的起始位置
  argint(1, &len);    // 获取第二个参数：要检查的页面数量
  argaddr(2, &st);    // 获取第三个参数：存储位掩码的用户地址

  // 检查页面数量是否合法,设置限制
  if ((len > 32) || (len < 1))
    return -1;

  pte_t *pte;
  for (int i = 0; i < len; i++) {
    // 获取当前页面的页表项
    pte = walk(p->pagetable, srcva + i * PGSIZE, 0);
    if (pte == 0) {
      release(&p->lock); // 释放进程锁
      return -1; // 无效页面，返回错误
    }
    
    // 检查页面是否有效且属于用户空间
    if ((*pte & PTE_V) == 0 || (*pte & PTE_U) == 0) {
      release(&p->lock); // 释放进程锁
      return -1; // 无效页面或不属于用户空间，返回错误
    }
    
    // 检查页面的访问位是否已设置
    if (*pte & PTE_A) {
      *pte = *pte & ~PTE_A; // 清除访问位
      buf |= (1 << i); // 设置位掩码中的相应位
    }
  }
  
  release(&p->lock); // 释放进程锁

  // 将位掩码复制到用户空间
  copyout(p->pagetable, st, (char *)&buf, ((len - 1) / 8) + 1);

  return 0; // 返回成功
}
#endif // 结束LAB_PGTBL编译条件


uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
