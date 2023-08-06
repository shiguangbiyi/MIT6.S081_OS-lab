//用于用户进程的物理存储器分配器，
//内核堆栈、页表页，
//以及管道缓冲器。分配整个4096字节的页面。

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

// 外部声明，end 是内核代码结束后的第一个地址，由 kernel.ld 定义。
extern char end[];

// 空闲内存块链表结构
struct run {
  struct run *next;
};

// 内核内存管理相关数据结构
struct {
  struct spinlock lock; // 自旋锁，用于保护 kmem 结构的访问
  struct run *freelist; // 空闲内存块链表头指针
} kmem;

// 初始化内核内存管理
void
kinit()
{
  initlock(&kmem.lock, "kmem"); // 初始化自旋锁
  freerange(end, (void*)PHYSTOP); // 将从 end 到 PHYSTOP 的内存空间加入空闲内存块链表
}

// 将从 pa_start 到 pa_end 的内存空间加入空闲内存块链表
void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start); // 将 pa_start 向上取整到页边界
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p); // 依次释放每一页的内存空间并加入空闲内存块链表
}

// 释放物理内存页，通常用于释放由 kalloc() 分配的页。
// 在初始化内存分配器时，也可能会用到此函数（参见 kinit）。
void
kfree(void *pa)
{
  struct run *r;

  // 检查释放的物理地址是否有效，必须是页对齐的合法地址
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // 用一些特殊的数据填充释放的内存，以检查是否存在悬空引用（dangling refs）
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock); // 获取内存管理锁，保护空闲内存块链表的访问
  r->next = kmem.freelist; // 将释放的内存加入空闲内存块链表的头部
  kmem.freelist = r; // 更新空闲内存块链表头指针
  release(&kmem.lock); // 释放内存管理锁
}

// 分配一个 4096 字节（PGSIZE）的物理内存页。
// 返回内核可以使用的指针。
// 如果内存无法分配，则返回 0。
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock); // 获取内存管理锁，保护空闲内存块链表的访问
  r = kmem.freelist; // 从空闲内存块链表头部获取一个可用内存块
  if(r)
    kmem.freelist = r->next; // 更新空闲内存块链表头指针
  release(&kmem.lock); // 释放内存管理锁

  if(r)
    memset((char*)r, 5, PGSIZE); // 将分配的内存块用特殊数据填充，用于检测是否正确使用了分配的内存
  return (void*)r; // 返回分配的内存块的指针
}
