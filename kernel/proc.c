#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

// 定义全局变量，存储每个CPU的信息
struct cpu cpus[NCPU];

// 定义全局变量，存储进程信息的数组，数组长度为NPROC
struct proc proc[NPROC];

// 定义指向初始进程的指针
struct proc *initproc;

// 下一个可用的进程ID，初始值为1
int nextpid = 1;

// 用于保护nextpid的互斥锁
struct spinlock pid_lock;

// 声明trampoline变量，实际定义在trampoline.S文件中
extern char trampoline[]; // trampoline.S

// 函数声明
extern void forkret(void);
static void freeproc(struct proc *p);

// 定义用于确保在唤醒wait()等待中的父进程时不会丢失的自旋锁wait_lock。
// 在使用p->parent时遵循内存模型时有帮助。必须在任何p->lock之前获取。
struct spinlock wait_lock;

// 为每个进程的内核栈分配一个页面。
// 将其映射到高内存，后跟一个无效的警戒页面。
void
proc_mapstacks(pagetable_t kpgtbl) {
  struct proc *p;
  
  // 遍历所有进程
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc(); // 分配一个内核栈页面
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc)); // 计算内核栈的虚拟地址
    // 将内核栈页面映射到对应虚拟地址，并设置PTE_R | PTE_W标志表示可读写
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// 在启动时初始化进程表。
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid"); // 初始化用于保护nextpid的互斥锁pid_lock
  initlock(&wait_lock, "wait_lock"); // 初始化wait_lock自旋锁
  // 遍历所有进程
  for(p = proc; p < &proc[NPROC]; p++) {
    initlock(&p->lock, "proc"); // 为每个进程初始化进程自旋锁
    p->kstack = KSTACK((int) (p - proc)); // 计算进程的内核栈地址
  }
}

// 必须在禁用中断的情况下调用，以防止与将进程移动到不同CPU的竞争。
// 获取当前CPU的ID。
int
cpuid()
{
  int id = r_tp(); // 读取riscv的线程指针寄存器tp，其中保存了当前CPU的ID
  return id;
}

// 返回当前CPU的cpu struct。
// 必须在禁用中断的情况下调用。
struct cpu*
mycpu(void) {
  int id = cpuid(); // 获取当前CPU的ID
  struct cpu *c = &cpus[id]; // 根据CPU ID获取相应的CPU结构体指针
  return c;
}

// 返回当前的struct proc *，如果没有则返回空指针。
struct proc*
myproc(void) {
  push_off(); // 禁用中断
  struct cpu *c = mycpu(); // 获取当前CPU的cpu struct指针
  struct proc *p = c->proc; // 获取当前CPU正在运行的进程指针
  pop_off(); // 恢复中断状态
  return p;
}

// 分配一个新的进程ID。
int
allocpid() {
  int pid;
  
  acquire(&pid_lock); // 获取pid_lock互斥锁
  pid = nextpid; // 获取下一个可用的进程ID
  nextpid = nextpid + 1; // 更新下一个可用的进程ID
  release(&pid_lock); // 释放pid_lock互斥锁

  return pid; // 返回分配的进程ID
}

// 在进程表中查找一个UNUSED（未使用）的进程。
// 如果找到，初始化在内核中运行所需的状态，
// 并在返回之前持有p->lock互斥锁。
// 如果没有空闲进程或者内存分配失败，则返回0。
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock); // 获取进程的互斥锁，以防止竞争
    if(p->state == UNUSED) { // 找到一个未使用的进程
      goto found; // 跳转到found标签
    } else {
      release(&p->lock); // 释放进程的互斥锁，继续寻找下一个进程
    }
  }
  return 0; // 没有找到空闲进程，返回0

found:
  p->pid = allocpid(); // 分配一个新的进程ID
  p->state = USED; // 将进程状态设置为USED，表示进程已经被使用

  // 分配一个陷阱帧页，用于保存进程上下文信息
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){ // 分配陷阱帧页内存
    freeproc(p); // 分配失败，释放进程
    release(&p->lock); // 释放进程的互斥锁
    return 0; // 返回0，表示分配失败
  }

  // 为用户空间创建一个空的页表。
  p->pagetable = proc_pagetable(p); // 创建用户空间的页表
  if(p->pagetable == 0){ // 创建失败
    freeproc(p); // 释放进程
    release(&p->lock); // 释放进程的互斥锁
    return 0; // 返回0，表示创建失败
  }

  // 设置新的上下文以从forkret开始执行，
  // forkret返回到用户空间。
  memset(&p->context, 0, sizeof(p->context)); // 清零上下文结构体
  p->context.ra = (uint64)forkret; // 设置返回地址为forkret函数的地址
  p->context.sp = p->kstack + PGSIZE; // 设置栈指针为内核栈顶地址

  return p; // 返回分配的进程指针
}


// 释放一个进程结构以及与其关联的数据，
// 包括用户页。
// p->lock必须被持有。
static void
freeproc(struct proc *p)
{
  if(p->trapframe) // 如果有陷阱帧页，释放陷阱帧页内存
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->pagetable) // 如果有页表，释放页表以及与之关联的内存
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0; // 进程内存大小置0
  p->pid = 0; // 进程ID置0
  p->parent = 0; // 父进程指针置0
  p->name[0] = 0; // 进程名置空
  p->chan = 0; // 进程等待通道置0
  p->killed = 0; // 进程是否被杀死标志置0
  p->xstate = 0; // 进程附加状态置0
  p->state = UNUSED; // 进程状态置为UNUSED，表示进程未使用
}

// 为给定进程创建一个用户页表，
// 没有用户内存，但包含trampoline页。
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // 创建一个空的页表
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // 将trampoline代码（用于系统调用返回）
  // 映射到最高的用户虚拟地址。
  // 只有监督程序在从用户空间
  // 过渡时使用它，所以不需要PTE_U权限。
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // 将trapframe映射到TRAMPOLINE的下面，用于trampoline.S。
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}


// 释放一个进程的页表，并释放它引用的物理内存。
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0); // 解除trampoline页的映射
  uvmunmap(pagetable, TRAPFRAME, 1, 0); // 解除trapframe页的映射
  uvmfree(pagetable, sz); // 释放页表以及与之关联的内存
}

// 用户程序调用exec("/init")的用户代码
// od -t xC initcode
uchar initcode[] = {
  // ...
};

// 设置第一个用户进程
void
userinit(void)
{
  struct proc *p;

  p = allocproc(); // 分配一个进程结构，并获取锁
  initproc = p; // 将initproc指针指向第一个进程

  // 分配一个用户页并将initcode的指令和数据复制到其中。
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE; // 设置进程内存大小为一页

  // 准备从内核返回到用户空间的第一个"return"。
  p->trapframe->epc = 0;      // 用户程序计数器
  p->trapframe->sp = PGSIZE;  // 用户栈指针

  safestrcpy(p->name, "initcode", sizeof(p->name)); // 将进程名设置为"initcode"
  p->cwd = namei("/"); // 将当前工作目录设置为根目录

  p->state = RUNNABLE; // 设置进程状态为RUNNABLE，表示进程可以被调度执行

  release(&p->lock); // 释放进程锁，使进程可以被调度执行
}

// 增加或减少进程的用户内存大小n字节。
// 成功返回0，失败返回-1。
int
growproc(int n)
{
  uint sz;
  struct proc *p = myproc(); // 获取当前进程结构指针

  sz = p->sz; // 获取当前进程的内存大小
  if(n > 0){ // 如果n大于0，表示增加内存
    if((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0) { // 为进程分配新内存
      return -1; // 分配失败，返回-1
    }
  } else if(n < 0){ // 如果n小于0，表示缩减内存
    sz = uvmdealloc(p->pagetable, sz, sz + n); // 释放进程的内存
  }
  p->sz = sz; // 更新进程内存大小
  return 0; // 成功，返回0
}


// 创建一个新进程，复制父进程。
// 设置子内核栈以便返回fork()系统调用的方式。
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc(); // 获取当前进程结构指针

  // 分配进程结构。
  if((np = allocproc()) == 0){
    return -1; // 分配失败，返回-1
  }

  // 从父进程复制用户内存到子进程。
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){ // 复制页表内容
    freeproc(np);
    release(&np->lock);
    return -1; // 复制失败，返回-1
  }
  np->sz = p->sz; // 设置子进程的内存大小与父进程相同

  // 复制保存的用户寄存器。
  *(np->trapframe) = *(p->trapframe);

  // 使fork在子进程中返回0。
  np->trapframe->a0 = 0;

  // 增加打开文件描述符的引用计数。
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]); // 复制文件结构指针
  np->cwd = idup(p->cwd); // 复制当前工作目录的vnode

  safestrcpy(np->name, p->name, sizeof(p->name)); // 复制进程名

  pid = np->pid; // 保存子进程的pid

  release(&np->lock); // 释放子进程锁，使其可被调度执行

  acquire(&wait_lock); // 获取wait_lock，防止子进程退出时的竞态条件
  np->parent = p; // 设置子进程的父进程为当前进程
  release(&wait_lock); // 释放wait_lock

  acquire(&np->lock); // 获取子进程锁
  np->state = RUNNABLE; // 设置子进程状态为RUNNABLE，表示子进程可以被调度执行
  release(&np->lock); // 释放子进程锁

  return pid; // 成功，返回子进程的pid
}

// 将p的被抛弃的子进程交给init进程。
// 调用者必须持有wait_lock。
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){ // 如果pp的父进程是p
      pp->parent = initproc; // 将pp的父进程设置为init进程
      wakeup(initproc); // 唤醒init进程
    }
  }
}

// 退出当前进程。不会返回。
// 已退出的进程仍处于僵尸状态，直到其父进程调用wait()。
void
exit(int status)
{
  struct proc *p = myproc(); // 获取当前进程结构指针

  if(p == initproc)
    panic("init exiting");

  // 关闭所有打开的文件。
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op(); // 开始文件系统操作
  iput(p->cwd); // 释放当前工作目录
  end_op(); // 结束文件系统操作
  p->cwd = 0;

  acquire(&wait_lock); // 获取wait_lock，防止退出的竞态条件

  // 将任何子进程交给init进程。
  reparent(p);

  // 父进程可能正在wait()中睡眠。
  wakeup(p->parent); // 唤醒父进程

  acquire(&p->lock); // 获取当前进程锁

  p->xstate = status; // 设置退出状态
  p->state = ZOMBIE; // 设置进程状态为ZOMBIE，表示进程已经退出

  release(&wait_lock); // 释放wait_lock

  // 跳转到调度器，永远不会返回。
  sched(); // 调度下一个进程执行
  panic("zombie exit"); // 调度器不应该返回，如果返回，则说明出现错误
}

// 等待一个子进程退出并返回其pid。
// 如果此进程没有子进程，则返回-1。
int
wait(uint64 addr)
{
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc(); // 获取当前进程结构指针

  acquire(&wait_lock); // 获取wait_lock，防止进程退出的竞态条件

  for(;;){
    // 扫描进程表寻找已经退出的子进程。
    havekids = 0;
    for(np = proc; np < &proc[NPROC]; np++){
      if(np->parent == p){ // 如果np的父进程是当前进程
        // 确保子进程不在exit()或swtch()中。
        acquire(&np->lock);

        havekids = 1;
        if(np->state == ZOMBIE){ // 如果子进程是ZOMBIE状态，表示已经退出
          // 找到一个子进程已经退出。
          pid = np->pid; // 获取子进程的pid
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                  sizeof(np->xstate)) < 0) { // 将子进程的退出状态复制到用户空间
            release(&np->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(np); // 释放子进程的资源
          release(&np->lock);
          release(&wait_lock);
          return pid; // 返回子进程的pid
        }
        release(&np->lock);
      }
    }

    // 如果没有子进程，或者当前进程已被杀死，则没有必要等待
    if(!havekids || p->killed){
      release(&wait_lock);
      return -1;
    }
    
    // 等待一个子进程退出。
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

// 每个CPU的进程调度程序。
// 每个CPU在设置自己后调用scheduler()。
// 调度程序永不返回。 它循环执行：
// - 选择要运行的进程。
// - 使用swtch切换到开始运行该进程。
// - 最终该进程通过swtch转移控制
// 通过swtch返回到调度程序。
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu(); // 获取当前CPU结构指针
  
  c->proc = 0; // 当前CPU没有正在运行的进程
  for(;;){
    // 确保设备中断能打断。
    intr_on();

    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // 切换到选定的进程。 进程的工作是在跳回到调度程序之前
        // 释放它的锁，然后重新获取锁。
        p->state = RUNNING; // 设置进程状态为RUNNING
        c->proc = p; // 设置当前CPU的运行进程为p
        swtch(&c->context, &p->context); // 切换上下文，开始运行进程

        // 进程暂时运行完成。
        // 进程在返回之前应该已经改变了它的p->state。
        c->proc = 0; // 当前CPU没有正在运行的进程
      }
      release(&p->lock);
    }
  }
}

// 切换到调度程序。 必须仅持有p->lock，并且已更改了proc->state。 保存和恢复
// intena，因为intena是该内核线程的属性，而不是该CPU的属性。 它应该
// 是proc->intena和proc->noff，但在很少的情况下，锁被持有，但
// 进程不存在。
void
sched(void)
{
  int intena;
  struct proc *p = myproc(); // 获取当前进程结构指针

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context); // 切换上下文，进入调度程序的执行
  mycpu()->intena = intena;
}

// 放弃CPU，让出一个调度轮次。
void
yield(void)
{
  struct proc *p = myproc(); // 获取当前进程结构指针
  acquire(&p->lock); // 获取当前进程的锁
  p->state = RUNNABLE; // 设置当前进程状态为RUNNABLE，可运行状态
  sched(); // 调度进程
  release(&p->lock); // 释放当前进程的锁
}

// 一个fork子进程的第一次调度，由scheduler()执行swtch到forkret。
void
forkret(void)
{
  static int first = 1;

  // 仍然持有p->lock，来自scheduler。
  release(&myproc()->lock); // 释放进程的锁

  if (first) {
    // 文件系统初始化必须在常规进程的上下文中运行（例如，因为它调用sleep），
    // 因此不能从main()中运行。
    first = 0;
    fsinit(ROOTDEV); // 初始化文件系统
  }

  usertrapret(); // 转到用户模式
}

// 原子地释放锁并在chan上睡眠。
// 唤醒时重新获得锁。
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc(); // 获取当前进程结构指针
  
  // 必须获取p->lock才能
  // 改变p->state然后调用sched。
  // 一旦我们持有p->lock，我们就可以确保我们不会错过任何唤醒
  // （wakeup也会锁定p->lock），
  // 因此释放lk是可以的。

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk); // 释放传入的锁

  // 进入睡眠状态。
  p->chan = chan; // 设置进程的chan为传入的chan指针
  p->state = SLEEPING; // 设置进程状态为SLEEPING，睡眠状态

  sched(); // 进程调度

  // 整理。
  p->chan = 0; // 清空chan指针

  // 重新获取原始锁。
  release(&p->lock);
  acquire(lk);
}


// 唤醒所有在chan上睡眠的进程。
// 必须在没有任何p->lock的情况下调用。
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE; // 将睡眠的进程唤醒，设置为RUNNABLE状态
      }
      release(&p->lock);
    }
  }
}

// 杀死具有给定pid的进程。
// 只有当受害者尝试返回到用户空间时，它才会退出
// （请参阅trap.c中的usertrap()）。
int
kill(int pid)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1; // 将进程标记为已杀死
      if(p->state == SLEEPING){
        // 唤醒睡眠进程。
        p->state = RUNNABLE; // 将睡眠的进程设置为RUNNABLE状态
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

// 根据usr_dst选择将数据复制到用户地址或内核地址。
// 成功返回0，错误返回-1。
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len); // 将数据从内核空间复制到用户空间
  } else {
    memmove((char *)dst, src, len); // 在内核空间中复制数据
    return 0;
  }
}

// 根据usr_src选择将数据从用户地址或内核地址复制出来。
// 成功返回0，错误返回-1。
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len); // 将数据从用户空间复制到内核空间
  } else {
    memmove(dst, (char*)src, len); // 在内核空间中复制数据
    return 0;
  }
}

// 在控制台上打印进程列表，用于调试。
// 在用户在控制台上键入^P时运行。
// 为了避免进一步使卡住的机器嵌入其中，没有锁。
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}
