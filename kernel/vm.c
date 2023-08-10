// 虚拟内存管理的模块
#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "elf.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"

// 内核的页表。
pagetable_t kernel_pagetable;

// kernel.ld设置etext为内核代码的结束地址。
extern char etext[];

// trampoline.S，用于处理中断和异常的汇编代码。
extern char trampoline[];

// 为内核创建一个直接映射的页表。
pagetable_t kvmmake(void)
{
  pagetable_t kpgtbl;

  // 为内核页表分配内存并初始化为0。
  kpgtbl = (pagetable_t)kalloc();
  memset(kpgtbl, 0, PGSIZE);

  // 映射uart寄存器。
  kvmmap(kpgtbl, UART0, UART0, PGSIZE, PTE_R | PTE_W);

  // 映射virtio mmio磁盘接口。
  kvmmap(kpgtbl, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

  // 映射PLIC。
  kvmmap(kpgtbl, PLIC, PLIC, 0x400000, PTE_R | PTE_W);

  // 映射内核文本段，设置为可执行和只读。
  kvmmap(kpgtbl, KERNBASE, KERNBASE, (uint64)etext - KERNBASE, PTE_R | PTE_X);

  // 映射内核数据段和使用的物理RAM。
  kvmmap(kpgtbl, (uint64)etext, (uint64)etext, PHYSTOP - (uint64)etext, PTE_R | PTE_W);

  // 映射中断/异常的跳板代码。
  kvmmap(kpgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);

  // 映射内核栈。
  proc_mapstacks(kpgtbl);

  return kpgtbl;
}

// 初始化唯一的内核页表。
void kvminit(void)
{
  kernel_pagetable = kvmmake();
}

// 切换硬件页表寄存器为内核页表，并启用分页机制。
void kvminithart()
{
  w_satp(MAKE_SATP(kernel_pagetable));
  sfence_vma();
}

// 在页表pagetable中查找虚拟地址va对应的PTE的地址。如果alloc非0，则会创建所需的页表页。
pte_t *walk(pagetable_t pagetable, uint64 va, int alloc)
{
  // 检查虚拟地址是否越界
  if (va >= MAXVA)
    panic("walk");

  // 遍历页表的层级，从最高层级开始向下查找
  for (int level = 2; level > 0; level--)
  {
    // 计算虚拟地址在当前层级的页表项索引
    pte_t *pte = &pagetable[PX(level, va)];

    // 检查当前页表项是否有效（存在）
    if (*pte & PTE_V)
    {
      // 如果当前页表项有效，获取下一级页表的地址，并继续向下查找
      pagetable = (pagetable_t)PTE2PA(*pte);
    }
    else
    {
      // 如果当前页表项无效，说明虚拟地址所对应的物理页尚未映射

      // 如果alloc为0，表示不需要分配新的页表，直接返回0
      if (!alloc || (pagetable = (pde_t *)kalloc()) == 0)
        return 0;

      // 将新分配的页表清零初始化
      memset(pagetable, 0, PGSIZE);

      // 将当前页表项设置为指向新分配的页表的物理地址，并标记为有效
      *pte = PA2PTE(pagetable) | PTE_V;
    }
  }

  // 返回虚拟地址对应的最终页表项的地址
  return &pagetable[PX(0, va)];
}


// 查找虚拟地址va对应的物理地址，并返回。如果未映射，则返回0。
// 只能用于查找用户页。
uint64 walkaddr(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  uint64 pa;

  if (va >= MAXVA)
    return 0;

  pte = walk(pagetable, va, 0);
  if (pte == 0)
    return 0;
  if ((*pte & PTE_V) == 0)
    return 0;
  if ((*pte & PTE_U) == 0)
    return 0;
  pa = PTE2PA(*pte);
  return pa;
}

// 将一个映射添加到内核页表中，仅在启动时使用，不会刷新TLB或启用分页。
void kvmmap(pagetable_t kpgtbl, uint64 va, uint64 pa, uint64 sz, int perm)
{
  if (mappages(kpgtbl, va, sz, pa, perm) != 0)
    panic("kvmmap");
}

// 为从va开始的虚拟地址创建PTE，映射到从pa开始的物理地址。va和size可能不对齐到页面。
// 成功返回0，如果walk()无法分配所需的页表页，则返回-1。
int mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
{
  uint64 a, last;
  pte_t *pte;

  if (size == 0)
    panic("mappages: size");

  a = PGROUNDDOWN(va);
  last = PGROUNDDOWN(va + size - 1);
  for (;;)
  {
    if ((pte = walk(pagetable, a, 1)) == 0)
      return -1;
    if (*pte & PTE_V)
      panic("mappages: remap");
    *pte = PA2PTE(pa) | perm | PTE_V;
    if (a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// 从va开始移除npages个映射。va必须对齐到页面，映射必须存在。
// 可选择释放物理内存。
void uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  uint64 a;
  pte_t *pte;

  if ((va % PGSIZE) != 0)
    panic("uvmunmap: not aligned");

  for (a = va; a < va + npages * PGSIZE; a += PGSIZE)
  {
    if ((pte = walk(pagetable, a, 0)) == 0)
      panic("uvmunmap: walk");
    if ((*pte & PTE_V) == 0)
      panic("uvmunmap: not mapped");
    if (PTE_FLAGS(*pte) == PTE_V)
      panic("uvmunmap: not a leaf");
    if (do_free)
    {
      uint64 pa = PTE2PA(*pte);
      kfree((void *)pa);
    }
    *pte = 0;
  }
}

// 创建一个空的用户页表。内存不足时返回0。
pagetable_t uvmcreate()
{
  pagetable_t pagetable;
  pagetable = (pagetable_t)kalloc();
  if (pagetable == 0)
    return 0;
  memset(pagetable, 0, PGSIZE);
  return pagetable;
}

// 将用户initcode加载到页表的地址0处，用于第一个进程。
// sz必须小于一个页面。
void uvminit(pagetable_t pagetable, uchar *src, uint sz)
{
  char *mem;

  if (sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pagetable, 0, PGSIZE, (uint64)mem, PTE_W | PTE_R | PTE_X | PTE_U);
  memmove(mem, src, sz);
}

// 为从oldsz增长到newsz的进程分配PTE和物理内存，不需要对齐到页面。
// 返回新的进程大小或错误时返回0。
uint64 uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
  char *mem;
  uint64 a;

  if (newsz < oldsz)
    return oldsz;

  oldsz = PGROUNDUP(oldsz);
  for (a = oldsz; a < newsz; a += PGSIZE)
  {
    mem = kalloc();
    if (mem == 0)
    {
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if (mappages(pagetable, a, PGSIZE, (uint64)mem, PTE_W | PTE_X | PTE_R | PTE_U) != 0)
    {
      kfree(mem);
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
  }
  return newsz;
}

// 回收用户内存页，将进程大小从oldsz缩小到newsz。oldsz和newsz不需要对齐到页面，也不需要newsz小于oldsz。
uint64 uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
  if (newsz >= oldsz)
    return oldsz;

  if (PGROUNDUP(newsz) < PGROUNDUP(oldsz))
  {
    int npages = (PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
    uvmunmap(pagetable, PGROUNDUP(newsz), npages, 1);
  }

  return newsz;
}

// 递归地释放页表页。
// 所有叶子映射必须已经被删除。
void freewalk(pagetable_t pagetable)
{
  // 每个页表有2^9 = 512个PTE。
  for (int i = 0; i < 512; i++)
  {
    pte_t pte = pagetable[i];
    if ((pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X)) == 0)
    {
      // 此PTE指向一个更低级别的页表。
      uint64 child = PTE2PA(pte);
      freewalk((pagetable_t)child);
      pagetable[i] = 0;
    }
    else if (pte & PTE_V)
    {
      panic("freewalk: leaf");
    }
  }
  kfree((void *)pagetable);
}

// 释放用户内存页，然后释放页表页。
void uvmfree(pagetable_t pagetable, uint64 sz)
{
  if (sz > 0)
    uvmunmap(pagetable, 0, PGROUNDUP(sz) / PGSIZE, 1);
  freewalk(pagetable);
}

// 给定一个父进程的页表，将其内存复制到子进程的页表中。
// 复制包括页表和物理内存。
// 成功返回0，失败返回-1。
// 在失败时释放任何已分配的内存。
int uvmcopy(pagetable_t old, pagetable_t new, uint64 sz) {
  pte_t *pte;       // 指向页表项的指针
  uint64 pa, i;     // 物理地址和循环计数器
  uint flags;       // 页表项的标志位
  char *mem;        // 用于分配临时内存

  // 通过循环以页的大小为单位，复制给定大小的内存
  for(i = 0; i < sz; i += PGSIZE) {
    // 在旧页表中找到指定地址的页表项
    if((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist");  // 如果页表项不存在，产生内核崩溃
    if((*pte & PTE_V) == 0)
      panic("uvmcopy: page not present");  // 如果页不在内存中，产生内核崩溃
    pa = PTE2PA(*pte); // 获取物理地址
    flags = PTE_FLAGS(*pte); // 获取页表项的标志位

    // 分配内核内存并将源内存内容复制到临时内存中
    if((mem = kalloc()) == 0) // 分配内存
      goto err;  // 如果分配失败，跳转到错误处理部分
    memmove(mem, (char*)pa, PGSIZE); // 复制源内存到临时内存

    // 映射新页表中的地址到新分配的内存
    if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0) {
      kfree(mem);  // 如果映射失败，释放临时内存
      goto err;    // 跳转到错误处理部分
    }
  }
  return 0; // 成功复制内存，返回0

err:
  // 复制失败时，解除新页表中已映射的内存，然后返回-1表示失败
  uvmunmap(new, 0, i / PGSIZE, 1);
  return -1;
}


// 标记PTE为不可用户访问。
// 用于exec函数为用户堆栈的守护页。
void uvmclear(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;

  pte = walk(pagetable, va, 0);
  if (pte == 0)
    panic("uvmclear");
  *pte &= ~PTE_U;
}

// 从内核复制到用户空间。
// 从虚拟地址srcva开始的页面表中复制len字节到dst。
// 成功返回0，失败返回-1。
int copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
  uint64 n, va0, pa0;

  while (len > 0)
  {
    va0 = PGROUNDDOWN(dstva);
    pa0 = walkaddr(pagetable, va0);
    if (pa0 == 0)
      return -1;
    n = PGSIZE - (dstva - va0);
    if (n > len)
      n = len;
    memmove((void *)(pa0 + (dstva - va0)), src, n);

    len -= n;
    src += n;
    dstva = va0 + PGSIZE;
  }
  return 0;
}

// 从用户空间复制到内核空间。
// 从虚拟地址srcva开始的页面表中复制len字节到dst。
// 成功返回0，失败返回-1。
int copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  uint64 n, va0, pa0;

  while (len > 0)
  {
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if (pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if (n > len)
      n = len;
    memmove(dst, (void *)(pa0 + (srcva - va0)), n);

    len -= n;
    dst += n;
    srcva = va0 + PGSIZE;
  }
  return 0;
}

// 从用户空间复制一个以null结尾的字符串到内核空间。
// 从虚拟地址srcva开始的页面表中复制字节到dst，直到遇到'\0'或达到max。
// 成功返回0，失败返回-1。
int copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
  uint64 n, va0, pa0;
  int got_null = 0;

  while (got_null == 0 && max > 0)
  {
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if (pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if (n > max)
      n = max;

    char *p = (char *)(pa0 + (srcva - va0));
    while (n > 0)
    {
      if (*p == '\0')
      {
        *dst = '\0';
        got_null = 1;
        break;
      }
      else
      {
        *dst = *p;
      }
      --n;
      --max;
      p++;
      dst++;
    }

    srcva = va0 + PGSIZE;
  }
  if (got_null)
  {
    return 0;
  }
  else
  {
    return -1;
  }
}
