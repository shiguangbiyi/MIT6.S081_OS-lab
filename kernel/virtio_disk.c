// 驱动程序用于 qemu 的 virtio 磁盘设备。
// 使用 qemu 的 mmio 接口来与 virtio 进行通信。
// qemu 提供了一个“legacy” virtio 接口。
//
// 启动 qemu 时，可以通过以下命令配置 virtio 磁盘设备：
// qemu ... -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
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
#include "virtio.h"

// virtio mmio 寄存器地址的宏定义
#define R(r) ((volatile uint32 *)(VIRTIO0 + (r)))

// 定义 virtio 磁盘结构
static struct disk {
  // virtio 驱动程序和设备主要通过一组 RAM 中的结构进行通信。pages[] 分配了这些内存。
  // pages[] 是全局变量（而不是通过 kalloc() 调用），因为它必须由两个连续页面组成，且页面对齐。
  char pages[2*PGSIZE];

  // pages[] 分为三个区域（描述符、可用和已使用），在 virtio 规范的第 2.6 节（用于 legacy 接口）中有解释。
  // https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.pdf

  // pages[] 的第一个区域是一组（非环形）DMA 描述符，驱动程序通过这些描述符告诉设备在哪里读取和写入各个磁盘操作。
  // 描述符的数量为 NUM。大多数命令由这些描述符的“链”（链表）组成。
  // 指向 pages[]。
  struct virtq_desc *desc;

  // 下一个是一个环形队列，驱动程序将希望设备处理的描述符号写入其中。
  // 它只包含每个链的头描述符。该队列共有 NUM 个元素。
  // 指向 pages[]。
  struct virtq_avail *avail;

  // 最后一个是设备写入已处理的描述符号的环形队列（只包含每个链的头部）。
  // 有 NUM 个 used ring 条目。
  // 指向 pages[]。
  struct virtq_used *used;

  // 我们自己的记录。
  char free[NUM];  // 描述符是否空闲？
  uint16 used_idx; // 我们在 used[2..NUM] 中查找到这里。

  // 记录关于进行中操作的信息，用于在完成中断到达时使用。
  // 由链的第一个描述符索引索引。
  struct {
    struct buf *b;
    char status;
  } info[NUM];

  // 磁盘命令头。
  // 为了方便，与描述符一一对应。
  struct virtio_blk_req ops[NUM];
  
  struct spinlock vdisk_lock;
  
} __attribute__ ((aligned (PGSIZE))) disk;

// virtio_disk_init 函数初始化 virtio 磁盘设备
void
virtio_disk_init(void)
{
  uint32 status = 0;

  initlock(&disk.vdisk_lock, "virtio_disk");

  // 检查 virtio 磁盘设备是否可用
  if(*R(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
     *R(VIRTIO_MMIO_VERSION) != 1 ||
     *R(VIRTIO_MMIO_DEVICE_ID) != 2 ||
     *R(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551){
    panic("could not find virtio disk");
  }
  
  // 向设备发送 ACKNOWLEDGE 信号，表示驱动收到了设备的存在
  status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
  *R(VIRTIO_MMIO_STATUS) = status;

  // 向设备发送 DRIVER 信号，表示驱动想要驱动该设备
  status |= VIRTIO_CONFIG_S_DRIVER;
  *R(VIRTIO_MMIO_STATUS) = status;

  // 协商设备和驱动支持的特性
  uint64 features = *R(VIRTIO_MMIO_DEVICE_FEATURES);
  // 将一些不需要的特性屏蔽掉
  features &= ~(1 << VIRTIO_BLK_F_RO);
  features &= ~(1 << VIRTIO_BLK_F_SCSI);
  features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
  features &= ~(1 << VIRTIO_BLK_F_MQ);
  features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
  features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
  features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
  *R(VIRTIO_MMIO_DRIVER_FEATURES) = features;

  // 告诉设备特性协商完成
  status |= VIRTIO_CONFIG_S_FEATURES_OK;
  *R(VIRTIO_MMIO_STATUS) = status;

  // 告诉设备驱动准备就绪
  status |= VIRTIO_CONFIG_S_DRIVER_OK;
  *R(VIRTIO_MMIO_STATUS) = status;

  *R(VIRTIO_MMIO_GUEST_PAGE_SIZE) = PGSIZE;

  // 初始化队列 0。
  *R(VIRTIO_MMIO_QUEUE_SEL) = 0;
  uint32 max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
  if(max == 0)
    panic("virtio disk has no queue 0");
  if(max < NUM)
    panic("virtio disk max queue too short");
  *R(VIRTIO_MMIO_QUEUE_NUM) = NUM;
  memset(disk.pages, 0, sizeof(disk.pages));
  *R(VIRTIO_MMIO_QUEUE_PFN) = ((uint64)disk.pages) >> PGSHIFT;

  // desc = pages -- num * virtq_desc
  // avail = pages + 0x40 -- 2 * uint16, then num * uint16
  // used = pages + 4096 -- 2 * uint16, then num * vRingUsedElem

  disk.desc = (struct virtq_desc *) disk.pages;
  disk.avail = (struct virtq_avail *)(disk.pages + NUM*sizeof(struct virtq_desc));
  disk.used = (struct virtq_used *) (disk.pages + PGSIZE);

  // 所有 NUM 个描述符开始时都是未使用的。
  for(int i = 0; i < NUM; i++)
    disk.free[i] = 1;

  // plic.c 和 trap.c 安排从 VIRTIO0_IRQ 中断获得中断。
}

// 寻找一个空闲的描述符，将其标记为非空闲，返回其索引。
static int
alloc_desc()
{
  for(int i = 0; i < NUM; i++){
    if(disk.free[i]){
      disk.free[i] = 0;
      return i;
    }
  }
  return -1;
}

// 标记一个描述符为空闲。
static void
free_desc(int i)
{
  if(i >= NUM)
    panic("free_desc 1");
  if(disk.free[i])
    panic("free_desc 2");
  disk.desc[i].addr = 0;
  disk.desc[i].len = 0;
  disk.desc[i].flags = 0;
  disk.desc[i].next = 0;
  disk.free[i] = 1;
  wakeup(&disk.free[0]);
}

// 释放一个描述符链。
static void
free_chain(int i)
{
  while(1){
    int flag = disk.desc[i].flags;
    int nxt = disk.desc[i].next;
    free_desc(i);
    if(flag & VRING_DESC_F_NEXT)
      i = nxt;
    else
      break;
  }
}

// 分配三个描述符（它们不必是连续的）。
// 磁盘传输总是使用三个描述符。
static int
alloc3_desc(int *idx)
{
  for(int i = 0; i < 3; i++){
    idx[i] = alloc_desc();
    if(idx[i] < 0){
      for(int j = 0; j < i; j++)
        free_desc(idx[j]);
      return -1;
    }
  }
  return 0;
}

// virtio_disk_rw 函数实现了对磁盘的读写操作
void
virtio_disk_rw(struct buf *b, int write)
{
  uint64 sector = b->blockno * (BSIZE / 512);

  acquire(&disk.vdisk_lock);

  // 规范的第 5.2 节指出，legacy 块操作使用三个描述符：
  // 一个用于 type/reserved/sector，一个用于数据，一个用于 1 字节的状态结果。

  // 分配三个描述符。
  int idx[3];
  while(1){
    if(alloc3_desc(idx) == 0) {
      break;
    }
    sleep(&disk.free[0], &disk.vdisk_lock);
  }

  // 格式化三个描述符。
  // qemu 的 virtio-blk.c 会读取它们。

  struct virtio_blk_req *buf0 = &disk.ops[idx[0]];

  // 根据读写操作设置请求类型
  if(write)
    buf0->type = VIRTIO_BLK_T_OUT; // 写入磁盘
  else
    buf0->type = VIRTIO_BLK_T_IN; // 从磁盘读取
  buf0->reserved = 0;
  buf0->sector = sector;

  disk.desc[idx[0]].addr = (uint64) buf0;
  disk.desc[idx[0]].len = sizeof(struct virtio_blk_req);
  disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
  disk.desc[idx[0]].next = idx[1];

  disk.desc[idx[1]].addr = (uint64) b->data;
  disk.desc[idx[1]].len = BSIZE;
  if(write)
    disk.desc[idx[1]].flags = 0; // 设备读取 b->data
  else
    disk.desc[idx[1]].flags = VRING_DESC_F_WRITE; // 设备写入 b->data
  disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
  disk.desc[idx[1]].next = idx[2];

  disk.info[idx[0]].status = 0xff; // 设备在成功时写入 0
  disk.desc[idx[2]].addr = (uint64) &disk.info[idx[0]].status;
  disk.desc[idx[2]].len = 1;
  disk.desc[idx[2]].flags = VRING_DESC_F_WRITE; // 设备写入状态
  disk.desc[idx[2]].next = 0;

  // 记录 struct buf 用于 virtio_disk_intr()。
  b->disk = 1;
  disk.info[idx[0]].b = b;

  // 告诉设备我们的描述符链中的第一个索引。
  disk.avail->ring[disk.avail->idx % NUM] = idx[0];

  __sync_synchronize();

  // 告诉设备可用环形队列有一个新的条目。
  disk.avail->idx += 1; // 不是 % NUM ...

  __sync_synchronize();

  // 发送通知告诉设备有新的请求
  *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // 值是队列号

  // 等待 virtio_disk_intr() 表示请求已完成。
  while(b->disk == 1) {
    sleep(b, &disk.vdisk_lock);
  }

  disk.info[idx[0]].b = 0;
  free_chain(idx[0]);

  release(&disk.vdisk_lock);
}

// virtio_disk_intr 函数处理磁盘操作完成的中断
void
virtio_disk_intr()
{
  acquire(&disk.vdisk_lock);

  // 设备在我们告诉它我们已经收到了该中断之前不会再次触发中断，
  // 下面这一行实现了这一点。
  // 这可能与设备正在将新条目写入“used”环形队列的情况产生竞争，
  // 在这种情况下，我们可能在此中断中处理新的完成条目，并在下一个中断中没有任何操作，这是无害的。
  *R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;

  __sync_synchronize();

  // 设备在添加条目到 used 环形队列时会增加 disk.used->idx。

  while(disk.used_idx != disk.used->idx){
    __sync_synchronize();
    int id = disk.used->ring[disk.used_idx % NUM].id;

    if(disk.info[id].status != 0)
      panic("virtio_disk_intr status");

    struct buf *b = disk.info[id].b;
    b->disk = 0;   // 磁盘操作完成
    wakeup(b);

    disk.used_idx += 1;
  }

  release(&disk.vdisk_lock);
}
