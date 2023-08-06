// `exec`系统调用用于执行新的用户程序。
// 该函数从文件系统中加载ELF格式的可执行文件，并将其映射到新的用户进程的虚拟内存空间中。
// 然后设置新的用户进程的栈和程序入口地址，准备将控制转移到新的用户程序。
// 同时在用户栈中准备好用户程序的命令行参数。

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "elf.h"

static int loadseg(pde_t *pgdir, uint64 addr, struct inode *ip, uint offset, uint sz);

int
exec(char *path, char **argv)
{
  char *s, *last;           // 用于处理命令行参数的临时指针变量
  int i, off;               // 用于循环计数和ELF程序段偏移量
  uint64 argc, sz = 0, sp;  // argc保存命令行参数的个数，sz用于计算程序内存大小，sp保存用户栈顶地址
  uint64 ustack[MAXARG];    // 数组，用于保存用户程序命令行参数指针
  uint64 stackbase;         // 保存用户栈底地址
  
  struct elfhdr elf;        // 保存ELF可执行文件头部信息
  struct inode *ip;         // 文件节点指针，用于打开和操作可执行文件
  struct proghdr ph;        // 程序段头部信息结构体
  pagetable_t pagetable = 0;// 保存新的用户进程页表的指针
  pagetable_t oldpagetable; // 保存旧的用户进程页表的指针，用于释放内存
  
  struct proc *p = myproc(); // 获取当前进程的proc结构指针

  // 开始文件系统操作
  begin_op();

  // 通过路径名`path`在文件系统中查找可执行文件，并返回对应的`inode`结构指针
  if ((ip = namei(path)) == 0) {
    end_op();
    return -1;
  }
  ilock(ip);

  // 检查ELF头部，确认这是一个有效的ELF文件
  if (readi(ip, 0, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if (elf.magic != ELF_MAGIC)
    goto bad;

  // 获取当前进程的页表指针`pagetable`
  if ((pagetable = proc_pagetable(p)) == 0)
    goto bad;

  // 将可加载的程序段加载到用户空间的虚拟内存中
  for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)) {
    if (readi(ip, 0, (uint64)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if (ph.type != ELF_PROG_LOAD)
      continue;
    if (ph.memsz < ph.filesz)
      goto bad;
    if (ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    uint64 sz1;
    if ((sz1 = uvmalloc(pagetable, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    sz = sz1;
    if ((ph.vaddr % PGSIZE) != 0)
      goto bad;
    if (loadseg(pagetable, ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  // 解锁文件节点并释放该节点，因为不再需要操作可执行文件
  iunlockput(ip);
  
  // 解锁文件节点并释放该节点，因为不再需要操作可执行文件
  end_op();

  // 将文件节点指针置为NULL，防止后续误用
  ip = 0;
  
  // 获取当前进程的proc结构指针
  p = myproc();
  
  // 保存当前进程的sz字段值，即当前进程的内存大小
  uint64 oldsz = p->sz;
  
  // 将sz调整为下一个页边界的大小
  sz = PGROUNDUP(sz);
  
  // 在用户进程的页表中为用户程序分配内存空间，sz1保存分配后的实际大小
  // 如果分配失败，则跳转到bad标签，表示执行exec失败
  uint64 sz1;
  if((sz1 = uvmalloc(pagetable, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  
  // 更新sz为实际分配的大小
  sz = sz1;
  
  // 在用户进程的页表中清空用户栈的页表项，为后续的用户栈使用做准备
  uvmclear(pagetable, sz-2*PGSIZE);
  
  // 将用户栈指针sp设为用户程序的结束地址
  sp = sz;
  
  // 计算用户栈的栈底地址
  stackbase = sp - PGSIZE;
  
  // Push argument strings, prepare rest of stack in ustack.
  // 将命令行参数压入用户栈，并准备好ustack数组
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    // 检查参数个数是否超过最大限制
  
    // 根据每个参数的长度调整栈指针sp的位置，为参数留出空间
    sp -= strlen(argv[argc]) + 1;
  
    // 根据RISC-V架构要求，栈指针必须是16字节对齐的
    sp -= sp % 16; // riscv sp must be 16-byte aligned
  
    // 检查栈是否溢出
    if(sp < stackbase)
      goto bad;
  
    // 将参数字符串拷贝到用户栈中
    if(copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
  
    // 将参数在用户栈中的地址保存到ustack数组中
    ustack[argc] = sp;
  }
  // 最后将ustack数组的最后一个元素设置为0，作为参数结束标志
  ustack[argc] = 0;

  // push the array of argv[] pointers.
  // 将argv[]指针数组压入用户栈
  sp -= (argc+1) * sizeof(uint64);
  // 根据参数个数计算压入指针数组所需的栈空间大小
  sp -= sp % 16;
  // 根据RISC-V架构要求，栈指针必须是16字节对齐的
  if(sp < stackbase)
    goto bad;
  // 检查栈是否溢出
  if(copyout(pagetable, sp, (char *)ustack, (argc+1)*sizeof(uint64)) < 0)
    goto bad;
  // 将ustack数组的内容拷贝到用户栈中
  
  // arguments to user main(argc, argv)
  // 将参数传递给用户态的main函数(argc, argv)
  // argc is returned via the system call return value, which goes in a0.
  // argc通过系统调用的返回值传递给用户态，保存在a0寄存器中
  p->trapframe->a1 = sp;
  // 将用户栈中存放参数指针数组的地址保存在trapframe的a1寄存器中
  
  // Save program name for debugging.
  // 保存程序名以供调试使用
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  // 找到路径中的最后一个'/'字符，将last指向程序名的起始位置
  safestrcpy(p->name, last, sizeof(p->name));
  // 将程序名拷贝到进程的name字段中，最多拷贝sizeof(p->name)字节
  
  // Commit to the user image.
  // 提交用户镜像，准备切换到用户态执行
  oldpagetable = p->pagetable;
  // 保存旧的页表
  p->pagetable = pagetable;
  // 使用新的页表替换进程的页表
  p->sz = sz;
  // 更新进程的内存大小
  p->trapframe->epc = elf.entry;  // initial program counter = main
  // 设置进程陷入内核态时的epc寄存器值为用户程序的entry字段，即main函数的入口地址
  p->trapframe->sp = sp; // initial stack pointer
  // 设置进程陷入内核态时的sp寄存器值为用户栈指针sp的值
  proc_freepagetable(oldpagetable, oldsz);
  // 释放旧的页表所占用的内存
  
  return argc; // this ends up in a0, the first argument to main(argc, argv)
  // 返回argc，该值将保存在a0寄存器中，作为main函数的第一个参数
  // 即用户程序的入口main函数的参数argc
  // 如果执行成功，返回main函数的参数个数，表示exec执行成功
  // 如果执行失败，返回-1，表示exec执行失败
  
  bad:
  if(pagetable)
    proc_freepagetable(pagetable, sz);
  // 如果pagetable不为空，说明在分配内存或拷贝参数时出现了错误
  // 释放分配的内存空间
  
  if(ip){
    iunlockput(ip);
    end_op();
  }
  // 解锁文件节点，并释放该节点
  // 结束文件系统操作，解锁超级块
  
  return -1;
  // 返回-1，表示exec执行失败
}

// 加载程序段到页表pagetable的虚拟地址va处。
// va必须是页对齐的地址，
// 并且从va到va+sz的页面必须已经映射到物理内存。
// 加载成功返回0，加载失败返回-1。
static int
loadseg(pagetable_t pagetable, uint64 va, struct inode *ip, uint offset, uint sz)
{
  uint i, n;
  uint64 pa;

  for(i = 0; i < sz; i += PGSIZE){
    // 通过虚拟地址va + i找到对应的物理地址pa
    pa = walkaddr(pagetable, va + i);
    // 如果找不到对应的物理地址，说明页表映射有问题，产生panic
    if(pa == 0)
      panic("loadseg: address should exist");
    // 根据剩余数据大小来决定每次读取的字节数n
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    // 从文件系统中读取n字节的数据到物理地址pa处
    if(readi(ip, 0, (uint64)pa, offset+i, n) != n)
      return -1;
  }
  
  return 0;
}
