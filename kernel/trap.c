// 处理中断和异常的相关功能。

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// 在 kernelvec.S 中调用 kerneltrap()。
void kernelvec();

extern int devintr();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// 设置在内核中接收异常和中断。
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// 从用户空间处理中断、异常或系统调用。
// 从 trampoline.S 调用。
//
void
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // 由于我们现在在内核中，将中断和异常发送到 kerneltrap()。
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  
  // 保存用户程序计数器。
  p->trapframe->epc = r_sepc();
  
  if(r_scause() == 8){
    // 系统调用

    if(p->killed)
      exit(-1);

    // sepc 指向 ecall 指令，
    // 但我们希望返回到下一条指令。
    p->trapframe->epc += 4;

    // 中断会更改 sstatus 和其他寄存器，
    // 所以在完成这些寄存器操作之前不要启用中断。
    intr_on();

    syscall();
  } else if((which_dev = devintr()) != 0){
    // ok
  } else {
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    p->killed = 1;
  }

  if(p->killed)
    exit(-1);

  // 如果这是计时器中断，则放弃 CPU。
  if(which_dev == 2)
    yield();

  usertrapret();
}

//
// 返回到用户空间
//
void
usertrapret(void)
{
  struct proc *p = myproc();

  // 我们即将将陷阱的目标从 kerneltrap() 切换到 usertrap()，
  // 因此在返回到用户空间之前，关闭中断。
  intr_off();

  // 将系统调用、中断和异常发送到 trampoline.S
  w_stvec(TRAMPOLINE + (uservec - trampoline));

  // 设置 trampoline.S 需要的 trapframe 值，
  // 当进程下次重新进入内核时会用到。
  p->trapframe->kernel_satp = r_satp();         // 内核页表
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // 进程的内核栈
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // cpuid() 的 hartid

  // 设置 trampoline.S 的 sret 指令要使用的寄存器，
  // 以便切换到用户空间。
  
  // 设置 S Previous Privilege mode 为 User。
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // 将 SPP 置为 0，表示用户模式
  x |= SSTATUS_SPIE; // 在用户模式下启用中断
  w_sstatus(x);

  // 设置 S Exception Program Counter 为保存的用户 pc。
  w_sepc(p->trapframe->epc);

  // 告诉 trampoline.S 切换到的用户页表。
  uint64 satp = MAKE_SATP(p->pagetable);

  // 跳转到内存顶部的 trampoline.S，
  // 它将切换到用户页表，恢复用户寄存器，
  // 并使用 sret 切换到用户模式。
  uint64 fn = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64,uint64))fn)(TRAPFRAME, satp);
}

// 内核代码的中断和异常都通过 kernelvec 进入这里，
// 使用当前的内核栈。
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    printf("scause %p\n", scause);
    printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // 如果这是计时器中断，则放弃 CPU。
  if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
    yield();

  // yield() 可能会导致一些陷阱发生，
  // 因此将陷阱寄存器恢复为 kernelvec.S 的 sepc 指令使用的值。
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  acquire(&tickslock);
  ticks++;
  wakeup(&ticks);
  release(&tickslock);
}

// 检查是否是外部中断或软中断，
// 并处理它。
// 返回 2 表示计时器中断，
// 返回 1 表示其他设备中断，
// 返回 0 表示不被识别的中断。
int
devintr()
{
  uint64 scause = r_scause();

  if((scause & 0x8000000000000000L) &&
     (scause & 0xff) == 9){
    // 这是超级外部中断，通过 PLIC。

    // irq 表示哪个设备中断。
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // PLIC 允许每个设备最多引发一个中断；
    // 告诉 PLIC 设备现在可以再次中断。
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000001L){
    // 来自机器模式计时器中断的软中断，
    // 由 kernelvec.S 中的 timervec 转发。

    if(cpuid() == 0){
      clockintr();
    }
    
    // 通过清除 sip 中的 SSIP 位确认软中断。
    w_sip(r_sip() & ~2);

    return 2;
  } else {
    return 0;
  }
}
