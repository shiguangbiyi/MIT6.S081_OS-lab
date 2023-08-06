#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

void main();
void timerinit();

// entry.S 需要每个 CPU 一个堆栈。
// 定义一个每个 CPU 的堆栈空间，长度为 4096 字节。
// 有 NCPU 个 CPU，因此分配 NCPU 个堆栈空间，存储在数组 stack0 中。
__attribute__ ((aligned (16))) char stack0[4096 * NCPU];

// 每个 CPU 需要一个用于机器模式时钟中断的临时空间。
// 使用 timer_scratch 数组存储 NCPU 个 CPU 的临时空间，每个空间包含 5 个 64 位的元素。
uint64 timer_scratch[NCPU][5];

// kernelvec.S 中机器模式时钟中断处理的汇编代码。
extern void timervec();

// entry.S 运行在机器模式下，使用 stack0 中的堆栈。
void
start()
{
  // 设置 M Previous Privilege mode 为 Supervisor 模式，用于 mret 指令。
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S;
  w_mstatus(x);

  // 设置 M Exception Program Counter 为 main 函数的地址，用于 mret 指令。
  // 需要使用 gcc -mcmodel=medany 编译。
  w_mepc((uint64)main);

  // 禁用分页机制。
  w_satp(0);

  // 委派所有中断和异常给 Supervisor 模式处理。
  w_medeleg(0xffff);
  w_mideleg(0xffff);
  w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);

  // 配置 Physical Memory Protection，允许 Supervisor 模式访问所有物理内存。
  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(0xf);

  // 配置时钟中断。
  timerinit();

  // 将每个 CPU 的 hartid 存储在它的 tp 寄存器中，用于 cpuid() 函数。
  int id = r_mhartid();
  w_tp(id);

  // 切换到 Supervisor 模式，并跳转到 main 函数。
  asm volatile("mret");
}

// 初始化时钟中断，配置每个 CPU 接收时钟中断的方式。
void
timerinit()
{
  // 每个 CPU 需要独立的时钟中断源。
  int id = r_mhartid();

  // 向 CLINT 请求一个时钟中断。
  int interval = 1000000; // 周期数，大约1/10秒在 qemu 中。
  *(uint64*)CLINT_MTIMECMP(id) = *(uint64*)CLINT_MTIME + interval;

  // 准备 timervec 所需的 scratch[] 信息。
  // scratch[0..2]：为 timervec 保存寄存器的空间。
  // scratch[3]：CLINT MTIMECMP 寄存器的地址。
  // scratch[4]：期望的时钟中断间隔（以周期计）。
  uint64 *scratch = &timer_scratch[id][0];
  scratch[3] = CLINT_MTIMECMP(id);
  scratch[4] = interval;
  w_mscratch((uint64)scratch);

  // 设置 machine 模式的陷阱处理函数。
  w_mtvec((uint64)timervec);

  // 启用 machine 模式的中断。
  w_mstatus(r_mstatus() | MSTATUS_MIE);

  // 启用 machine 模式的时钟中断。
  w_mie(r_mie() | MIE_MTIE);
}
