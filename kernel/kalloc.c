//物理内存分配器，用于用户进程、内核堆栈、页表页和管道缓冲区。分配整个4096字节的页面。

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

//释放v指向的物理内存页，该页通常应该由对kalloc（）的调用返回。（初始化分配器时出现异常；请参阅上面的kinit。）
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

//分配一个4096字节的物理内存页。返回内核可以使用的指针。如果无法分配内存，则返回0。
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


// 获取空闲内存的页数
uint64
getfreemem()
{
  // 声明一个指向内存空闲链表的指针
  struct run *r;
  // 用于保存空闲内存页数的计数器
  uint64 count=0;
  // 获取内核内存管理锁，防止其他进程同时访问内存管理数据结构
  acquire(&kmem.lock);
  // 将指针r指向内存空闲链表的头部
  r = kmem.freelist;
  // 遍历整个内存空闲链表，统计空闲内存页数
  while(r){
    // 移动指针r到下一个空闲内存块
    r = r->next;
    // 计数器加1，表示遇到了一个空闲内存页
    count++;
  }
  // 释放内核内存管理锁，允许其他进程访问内存管理数据结构
  release(&kmem.lock);
  // 返回空闲内存页数乘以页大小，得到空闲内存的字节数
  return count*PGSIZE;
}

