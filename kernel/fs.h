// fs.h 文件定义了磁盘上文件系统的格式，内核和用户程序都使用这个头文件。

#define ROOTINO  1   // 根目录的 i 节点编号
#define BSIZE 1024  // 块大小

// 磁盘布局:
// [ 引导块 | 超级块 | 日志 | i 节点块 | 空闲位图 | 数据块]
//
// mkfs 计算超级块并构建初始文件系统。超级块描述了磁盘布局：
struct superblock {
  uint magic;        // 必须为 FSMAGIC
  uint size;         // 文件系统镜像的大小（块数）
  uint nblocks;      // 数据块数
  uint ninodes;      // i 节点数
  uint nlog;         // 日志块数
  uint logstart;     // 第一个日志块的块号
  uint inodestart;   // 第一个 i 节点块的块号
  uint bmapstart;    // 第一个空闲位图块的块号
};

#define FSMAGIC 0x10203040

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// 磁盘上的 i 节点结构
struct dinode {
  short type;           // 文件类型
  short major;          // 主设备号（仅对 T_DEVICE 类型有效）
  short minor;          // 次设备号（仅对 T_DEVICE 类型有效）
  short nlink;          // 文件系统中连接到此 i 节点的数量
  uint size;            // 文件大小（字节数）
  uint addrs[NDIRECT+1];   // 数据块地址
};

// 每块中的 i 节点数。
#define IPB           (BSIZE / sizeof(struct dinode))

// 包含 i 节点 i 的块
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// 每块中的位图位数
#define BPB           (BSIZE*8)

// 包含块 b 的空闲位图块
#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart)

// 目录是一个包含一系列 dirent 结构的文件。
#define DIRSIZ 14

struct dirent {
  ushort inum; // i 节点号
  char name[DIRSIZ]; // 目录项名称
};
