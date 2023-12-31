#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

// 从当前进程中获取地址 addr 处的 uint64 值。
int
fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  // 检查地址合法性
  if(addr >= p->sz || addr+sizeof(uint64) > p->sz)
    return -1;
  // 从地址 addr 处复制 sizeof(uint64) 字节数据到 ip 指向的地址
  if(copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

// 从当前进程中获取地址 addr 处的以 '\0' 结尾的字符串。
// 返回字符串的长度（不包括 '\0' 结尾字符），如果出错返回 -1。
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  // 调用 copyinstr 函数复制以 '\0' 结尾的字符串到 buf 中，最多复制 max 个字符
  int err = copyinstr(p->pagetable, buf, addr, max);
  if(err < 0)
    return err;
  // 返回字符串的长度（包括 '\0' 结尾字符）
  return strlen(buf);
}

// 获取第 n 个系统调用参数的原始值。
static uint64
argraw(int n)
{
  struct proc *p = myproc();
  // 根据 n 的值返回对应的系统调用参数
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw"); // 如果 n 不在有效范围内，触发内核崩溃
  return -1;
}

// 获取第 n 个系统调用参数，存入指针 ip 指向的地址。
int
argint(int n, int *ip)
{
  *ip = argraw(n);
  return 0;
}

// 获取第 n 个系统调用参数，存入指针 ip 指向的地址，不检查合法性，
// copyin/copyout 将会处理合法性检查。
int
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
  return 0;
}

// 获取第 n 个系统调用参数，并将其解释为以 '\0' 结尾的字符串，复制到 buf 中，最多复制 max 个字符。
// 如果成功，返回字符串长度（包括 '\0' 结尾字符），如果出错返回 -1。
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  if(argaddr(n, &addr) < 0)
    return -1;
  // 调用 fetchstr 函数获取字符串并复制到 buf 中
  return fetchstr(addr, buf, max);
}

// 声明系统调用的实现函数
extern uint64 sys_chdir(void);
extern uint64 sys_close(void);
extern uint64 sys_dup(void);
extern uint64 sys_exec(void);
extern uint64 sys_exit(void);
extern uint64 sys_fork(void);
extern uint64 sys_fstat(void);
extern uint64 sys_getpid(void);
extern uint64 sys_kill(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_mknod(void);
extern uint64 sys_open(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_sleep(void);
extern uint64 sys_unlink(void);
extern uint64 sys_wait(void);
extern uint64 sys_write(void);
extern uint64 sys_uptime(void);
#ifdef LAB_NET
extern uint64 sys_connect(void);
#endif
#ifdef LAB_PGTBL
extern uint64 sys_pgaccess(void);
#endif

// 定义系统调用数组，保存每个系统调用的实现函数。
static uint64 (*syscalls[])(void) = {
  [SYS_fork]    sys_fork,
  [SYS_exit]    sys_exit,
  [SYS_wait]    sys_wait,
  [SYS_pipe]    sys_pipe,
  [SYS_read]    sys_read,
  [SYS_kill]    sys_kill,
  [SYS_exec]    sys_exec,
  [SYS_fstat]   sys_fstat,
  [SYS_chdir]   sys_chdir,
  [SYS_dup]     sys_dup,
  [SYS_getpid]  sys_getpid,
  [SYS_sbrk]    sys_sbrk,
  [SYS_sleep]   sys_sleep,
  [SYS_uptime]  sys_uptime,
  [SYS_open]    sys_open,
  [SYS_write]   sys_write,
  [SYS_mknod]   sys_mknod,
  [SYS_unlink]  sys_unlink,
  [SYS_link]    sys_link,
  [SYS_mkdir]   sys_mkdir,
  [SYS_close]   sys_close,
#ifdef LAB_NET
  [SYS_connect] sys_connect,
#endif
#ifdef LAB_PGTBL
  [SYS_pgaccess] sys_pgaccess,
#endif
};

// 系统调用处理函数
void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7; // 获取系统调用号
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    p->trapframe->a0 = syscalls[num](); // 调用对应系统调用的实现函数，并将返回值存入 a0 中
  } else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num); // 如果系统调用号不在有效范围内或对应的系统调用函数未实现，则打印错误信息。
    p->trapframe->a0 = -1; // 返回错误标志 -1。
  }
}
