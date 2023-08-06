// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

//缓冲区缓存。
//
//缓冲区缓存是保存磁盘块内容的缓存副本的buf结构的链接列表。在内存中缓存磁盘块可以减少磁盘读取次数，还可以为多个进程使用的磁盘块提供同步点。
//接口：
//*要获取特定磁盘块的缓冲区，请调用bread。
//*更改缓冲区数据后，调用bwrite将其写入磁盘。
//*缓冲区用完后，调用brelse。
//*调用brelse后不要使用缓冲区。
//*一次只能有一个进程使用缓冲区，因此不要将其保留超过必要的时间。


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

// 磁盘缓冲区高速缓存（buffer cache）。
// 它的作用是存储最近访问过的磁盘块，加速对磁盘的读写操作。
struct {
  // 自旋锁，用于保护对缓冲区高速缓存的并发访问。
  // 在多处理器系统中，可能有多个CPU同时访问磁盘缓冲区，为了防止竞争条件，需要使用锁来保护访问。
  struct spinlock lock;
  // 存储NBUF个磁盘缓冲区（buffer）。
  // 每个缓冲区对应一个磁盘块，它们用于暂时存储从磁盘读取的数据或者要写入磁盘的数据。
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // 所有缓冲区的双向链表，通过 prev/next 字段连接。
  // 根据缓冲区的使用顺序进行排序。
  // head.next 指向最近使用的缓冲区，head.prev 指向最久未使用的缓冲区。
  struct buf head;
} bcache;


// 用于初始化磁盘缓冲区高速缓存
void
binit(void)
{
  struct buf *b;
  // 初始化缓冲区高速缓存的锁
  initlock(&bcache.lock, "bcache");

  // 创建缓冲区的双向链表
  bcache.head.prev = &bcache.head; // 将 bcache.head 的 prev 指针指向自身，这是一个双向链表的结束标志。
  bcache.head.next = &bcache.head; // 将 bcache.head 的 next 指针指向自身，形成一个空链表。
  // 使用 for 循环迭代 bcache.buf 数组中的每个缓冲区（共有 NBUF 个缓冲区）。
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;    // 将当前缓冲区的 next 指针指向原链表的头部，即将当前缓冲区插入到链表头部。
    b->prev = &bcache.head;        // 将当前缓冲区的 prev 指针指向链表头部，表示当前缓冲区是链表的第一个元素。
    initsleeplock(&b->lock, "buffer");  // 初始化当前缓冲区的睡眠锁，用于保护对该缓冲区的并发访问。
    bcache.head.next->prev = b;    // 将原链表头部的 prev 指针指向当前缓冲区，使其成为新的链表头部的前一个元素。
    bcache.head.next = b;          // 将链表头部的 next 指针指向当前缓冲区，使其成为新的链表头部。
  }
}

// 从磁盘缓冲区高速缓存（bcache）中查找指定设备（dev）上的块（blockno）。
// 如果该块已经被缓存，那么它会返回对应的缓冲区（buf）并锁定它。
// 如果该块尚未被缓存，它会分配一个新的缓冲区，并返回对应的缓冲区并锁定它。
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // 获取对缓冲区高速缓存的锁，确保并发访问时的正确性
  acquire(&bcache.lock);

  // 在缓冲区高速缓存中查找指定设备和块号的缓冲区
  for (b = bcache.head.next; b != &bcache.head; b = b->next) {
    // 如果找到了对应的缓冲区
    if (b->dev == dev && b->blockno == blockno) {
      // 增加缓冲区的引用计数，表示有更多的用户正在使用该缓冲区
      b->refcnt++;
      // 释放对缓冲区高速缓存的锁，允许其他线程访问缓冲区高速缓存
      release(&bcache.lock);
      // 获取对缓冲区的睡眠锁，确保该缓冲区不会被其他线程修改
      acquiresleep(&b->lock);
      // 返回对应的缓冲区，该缓冲区已经被锁定
      return b;
    }
  }

  // 如果没有找到对应的缓冲区，则需要分配一个新的缓冲区

  // 遍历缓冲区高速缓存中的所有缓冲区，查找最久未使用的未被引用的缓冲区（LRU算法）
  for (b = bcache.head.prev; b != &bcache.head; b = b->prev) {
    // 如果找到了未被引用的缓冲区，可以将其重新用于当前的设备和块号
    if (b->refcnt == 0) {
      // 将当前缓冲区重新分配给指定设备和块号
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      // 释放对缓冲区高速缓存的锁，允许其他线程访问缓冲区高速缓存
      release(&bcache.lock);
      // 获取对缓冲区的睡眠锁，确保该缓冲区不会被其他线程修改
      acquiresleep(&b->lock);
      // 返回对应的缓冲区，该缓冲区已经被锁定
      return b;
    }
  }

  // 如果没有未被引用的缓冲区可用，则发生了错误，触发 panic
  panic("bget: no buffers");
}

// 通过提供的设备号（dev）和块号（blockno），返回一个带有相应块内容的锁定缓冲区（buf）。
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  // 通过 bget 函数获取对应设备和块号的缓冲区，并锁定它
  b = bget(dev, blockno);

  // 如果缓冲区中的数据无效（未加载），则从磁盘读取块数据
  if (!b->valid) {
    // 调用 virtio_disk_rw 函数从磁盘读取块数据到缓冲区中
    virtio_disk_rw(b, 0);
    // 将缓冲区标记为有效（数据已加载）
    b->valid = 1;
  }

  // 返回带有块内容的锁定缓冲区
  return b;
}

// 将缓冲区 b 的内容写入磁盘。必须对缓冲区进行锁定（持有睡眠锁）。
void
bwrite(struct buf *b)
{
  // 如果没有持有缓冲区的睡眠锁，发生错误，触发 panic
  if (!holdingsleep(&b->lock))
    panic("bwrite");

  // 调用 virtio_disk_rw 函数将缓冲区 b 中的数据写入磁盘
  virtio_disk_rw(b, 1);
}

// 释放一个锁定的缓冲区（buffer）。
// 将其移动到最近使用的缓冲区链表的头部。
void
brelse(struct buf *b)
{
  // 如果没有持有缓冲区的睡眠锁，发生错误，触发 panic
  if (!holdingsleep(&b->lock))
    panic("brelse");

  // 释放缓冲区的睡眠锁，允许其他线程访问缓冲区
  releasesleep(&b->lock);

  // 获取对缓冲区高速缓存的锁，确保并发访问时的正确性
  acquire(&bcache.lock);

  // 减少缓冲区的引用计数，表示有一个用户不再使用该缓冲区
  b->refcnt--;

  // 如果引用计数为零，表示没有用户在使用该缓冲区，可以将其移出缓冲区高速缓存
  if (b->refcnt == 0) {
    // 将缓冲区从链表中移出，即从链表的 prev 和 next 指针中解除链接
    b->next->prev = b->prev;
    b->prev->next = b->next;

    // 将缓冲区移到链表头部，作为最近使用的缓冲区
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  // 释放对缓冲区高速缓存的锁，允许其他线程访问缓冲区高速缓存
  release(&bcache.lock);
}

// 增加缓冲区的引用计数，使其被固定在缓冲区高速缓存中。
void
bpin(struct buf *b)
{
  // 获取对缓冲区高速缓存的锁，确保并发访问时的正确性
  acquire(&bcache.lock);
  // 增加缓冲区的引用计数，表示有更多的用户正在使用该缓冲区
  b->refcnt++;
  // 释放对缓冲区高速缓存的锁，允许其他线程访问缓冲区高速缓存
  release(&bcache.lock);
}

// 减少缓冲区的引用计数，解除对其的固定，并允许其被替换出缓冲区高速缓存。
void
bunpin(struct buf *b)
{
  // 获取对缓冲区高速缓存的锁，确保并发访问时的正确性
  acquire(&bcache.lock);
  // 减少缓冲区的引用计数，表示有一个用户不再使用该缓冲区
  b->refcnt--;
  // 释放对缓冲区高速缓存的锁，允许其他线程访问缓冲区高速缓存
  release(&bcache.lock);
}
