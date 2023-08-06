// 描述缓冲区（buffer）的属性和状态
struct buf {
  int valid;       // 表示缓冲区中的数据是否有效（已经从磁盘读取过）
  int disk;        // 表示缓冲区是否已经分配给磁盘
  uint dev;        // 表示缓冲区对应的设备号（设备 ID）
  uint blockno;    // 表示缓冲区对应的磁盘块号（block number）
  struct sleeplock lock;   // 睡眠锁（用于保护对缓冲区的并发访问）
  uint refcnt;     // 表示缓冲区的引用计数（表示有多少用户正在使用该缓冲区）
  struct buf *prev;        // LRU 高速缓存链表的前一个缓冲区（最近使用链表）
  struct buf *next;        // LRU 高速缓存链表的后一个缓冲区（最近使用链表）
  uchar data[BSIZE];       // 缓冲区的数据存储区域，大小为 BSIZE
};
