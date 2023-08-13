struct buf {
  int valid;             // 标志：磁盘块中的数据是否已经被读入缓冲区
  int disk;              // 标志：磁盘是否“拥有”这个缓冲区
  uint dev;              // 设备号：标识磁盘设备
  uint blockno;          // 块号：标识磁盘块的编号
  struct sleeplock lock; // 用于同步的睡眠锁，保护缓冲区的访问
  uint refcnt;           // 引用计数：表示当前缓冲区被引用的次数
//  struct buf *prev;     // LRU cache list - lab8-2
  struct buf *next;      // 哈希链表指针：用于哈希表中存储缓冲区
  uchar data[BSIZE];     // 实际存储数据的数组：大小为一个磁盘块的大小（BSIZE）
  uint timestamp;        // 缓冲区上次使用的时间戳 - lab8-2
};
