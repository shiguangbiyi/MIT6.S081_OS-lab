// plic.c 文件实现了 RISC-V 平台级中断控制器（PLIC）相关的函数。

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

//
// RISC-V 平台级中断控制器（PLIC）初始化函数
//
void
plicinit(void)
{
  // 设置所需的中断请求（IRQ）优先级为非零值（否则会被禁用）。
  *(uint32*)(PLIC + UART0_IRQ*4) = 1;
  *(uint32*)(PLIC + VIRTIO0_IRQ*4) = 1;
}

//
// RISC-V 平台级中断控制器（PLIC）初始化函数，针对当前 Hart 初始化
//
void
plicinithart(void)
{
  int hart = cpuid();
  
  // 为当前 Hart 的 S-mode 设置 UART0 和 VIRTIO0 的使能位（enable bit）。
  *(uint32*)PLIC_SENABLE(hart)= (1 << UART0_IRQ) | (1 << VIRTIO0_IRQ);

  // 将当前 Hart 的 S-mode 优先级阈值设置为 0。
  *(uint32*)PLIC_SPRIORITY(hart) = 0;
}

//
// 查询 PLIC 以确定要服务的中断（IRQ）。
//
int
plic_claim(void)
{
  int hart = cpuid();
  int irq = *(uint32*)PLIC_SCLAIM(hart);
  return irq;
}

//
// 告知 PLIC 我们已经服务完这个中断（IRQ）。
//
void
plic_complete(int irq)
{
  int hart = cpuid();
  *(uint32*)PLIC_SCLAIM(hart) = irq;
}
