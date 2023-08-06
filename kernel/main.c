// 入口文件，包含了操作系统启动时的初始化代码和核心的启动过程
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;

// start() jumps here in supervisor mode on all CPUs.
// start() 函数在所有 CPU 上以 supervisor 模式跳转到这里执行。

void
main()
{
  if(cpuid() == 0){
    // 为第一个 CPU 初始化控制台和 printf 子系统。
    consoleinit();
    printfinit();
    printf("\n");
    printf("xv6 kernel is booting\n");
    printf("\n");

    kinit();         // 初始化物理页面分配器。
    kvminit();       // 创建内核页表。
    kvminithart();   // 启用第一个 CPU 的分页机制。
    procinit();      // 初始化进程表。
    trapinit();      // 初始化中断向量表。
    trapinithart();  // 安装第一个 CPU 的内核中断向量。
    plicinit();      // 设置中断控制器。
    plicinithart();  // 请求第一个 CPU 的设备中断。
    binit();         // 初始化缓冲区缓存。
    iinit();         // 初始化 inode 表。
    fileinit();      // 初始化文件表。
    virtio_disk_init(); // 初始化模拟硬盘。
    userinit();      // 初始化第一个用户进程。
    __sync_synchronize(); // 同步内存。
    started = 1;     // 标记第一个 CPU 启动完成。
  } else {
    // 在继续之前等待第一个 CPU 启动。
    while(started == 0)
      ;
    __sync_synchronize(); // 同步内存。
    printf("hart %d starting\n", cpuid()); // 打印 CPU ID。
    kvminithart();    // 启用此 CPU 的分页机制。
    trapinithart();   // 安装此 CPU 的内核中断向量。
    plicinithart();   // 请求此 CPU 的设备中断。
  }

  scheduler();        // 启动调度器，运行用户进程。
}
