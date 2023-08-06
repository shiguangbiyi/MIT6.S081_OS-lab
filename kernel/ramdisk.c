//
// 使用由qemu -initrd fs.img加载的磁盘镜像的ramdisk。
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// 初始化ramdisk，这里没有做任何操作，因为在初始化时并不需要做什么。
void
ramdiskinit(void)
{
}

// 读写函数。如果B_DIRTY标志被设置，表示需要将buf的数据写入磁盘，
// 然后清除B_DIRTY标志，设置B_VALID标志。
// 如果B_VALID标志没有被设置，表示需要从磁盘读取数据，然后设置B_VALID标志。
void
ramdiskrw(struct buf *b)
{
  // 检查当前线程是否持有b->lock的睡眠锁。
  if(!holdingsleep(&b->lock))
    panic("ramdiskrw: buf not locked");
  
  // 检查b的标志是否设置了B_VALID标志，但未设置B_DIRTY标志。
  if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
    panic("ramdiskrw: nothing to do");

  // 检查b的块号是否超过了文件系统的大小。
  if(b->blockno >= FSSIZE)
    panic("ramdiskrw: blockno too big");

  // 计算在RAMDISK中的磁盘地址。
  uint64 diskaddr = b->blockno * BSIZE;
  
  // 将RAMDISK的指针与磁盘地址相加，得到在RAMDISK中的地址。
  char *addr = (char *)RAMDISK + diskaddr;

  // 检查b的标志是否设置了B_DIRTY标志，表示buf的数据已被修改。
  if(b->flags & B_DIRTY){
    // 写入数据到磁盘。
    memmove(addr, b->data, BSIZE);
    // 清除B_DIRTY标志，表示数据已被写入磁盘。
    b->flags &= ~B_DIRTY;
  } else {
    // 从磁盘读取数据到buf。
    memmove(b->data, addr, BSIZE);
    // 设置B_VALID标志，表示buf中的数据已被更新。
    b->flags |= B_VALID;
  }
}
