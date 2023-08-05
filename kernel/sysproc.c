// 包含系统调用实现的源文件。主要作用是实现用户程序与内核之间的接口

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64 getfreemem();
uint64 getnproc();

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

uint64
sys_trace(void)
{
  // 存储用户传递给系统调用的控制掩码。
  int mask;    
  // 从用户空间读取第一个参数，并将其存储在mask中。如果读取参数失败，将会返回-1表示系统调用执行失败。
  if(argint(0, &mask) < 0) 
	  return -1;
  myproc()->mask = mask;
  return 0;
}


// 系统调用函数，用于收集系统信息并将其复制到用户空间
uint64 sys_sysinfo(void)
{
  // 声明一个 struct sysinfo 结构变量 info，用于存储收集到的系统信息
  struct sysinfo info;
  
  // 声明一个 uint64 类型的变量 addr，用于存储用户传递的指向 struct sysinfo 结构的指针
  uint64 addr;

  // 获取当前进程的指针
  struct proc *p = myproc();

  // 获取活动进程的数量
  info.nproc = getnproc();

  // 获取可用内存的数量
  info.freemem = getfreemem();

  // 通过系统调用的参数索引 0，获取用户传递的指向 struct sysinfo 结构的指针，存储在 addr 中
  if(argaddr(0, &addr) < 0)
    return -1;

  // 将 info 结构中的数据复制到用户空间，使用 copyout 函数
  // p->pagetable 为当前进程的页表，将 info 结构中的数据从内核空间复制到用户空间地址 addr
  // 复制的字节数为 sizeof(info)
  if(copyout(p->pagetable, addr, (char*)&info, sizeof(info)) < 0)
    return -1;

  // 返回 0 表示系统调用成功执行
  return 0;
}

