// file.h 定义了文件结构和与文件相关的一些宏和设备函数映射结构

// 文件结构，包含了文件类型、引用计数以及其他信息
struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type; // 文件类型
  int ref; // 引用计数
  char readable; // 是否可读
  char writable; // 是否可写
  struct pipe *pipe; // FD_PIPE 类型的文件
  struct inode *ip;  // FD_INODE 和 FD_DEVICE 类型的文件
  uint off;          // FD_INODE 类型的文件的偏移量
  short major;       // FD_DEVICE 类型的文件的主设备号
};

// 一些与设备号相关的宏
#define major(dev)  ((dev) >> 16 & 0xFFFF)
#define minor(dev)  ((dev) & 0xFFFF)
#define mkdev(m,n)  ((uint)((m)<<16| (n)))

// 内存中的 inode 副本
struct inode {
  uint dev;           // 设备号
  uint inum;          // inode 编号
  int ref;            // 引用计数
  struct sleeplock lock; // 保护以下所有内容的自旋锁
  int valid;          // inode 是否已从磁盘读取？

  short type;         // 磁盘上的 inode 类型
  short major;        // 设备主设备号
  short minor;        // 设备次设备号
  short nlink;        // 硬链接数
  uint size;          // 文件大小
  uint addrs[NDIRECT+1]; // 直接块和间接块的地址
};

// 设备函数映射结构，将设备主设备号映射到设备读写函数
struct devsw {
  int (*read)(int, uint64, int); // 读设备函数指针
  int (*write)(int, uint64, int); // 写设备函数指针
};

// 外部全局变量，用于存储设备函数映射数组
extern struct devsw devsw[];

// 控制台设备号
#define CONSOLE 1
