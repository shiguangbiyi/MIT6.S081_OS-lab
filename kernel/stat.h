// stat.h 包含了文件系统中文件的属性信息的定义和相关常量。

// 定义了文件类型的常量，用于 struct stat 中的 type 字段。
#define T_DIR     1   // 目录
#define T_FILE    2   // 普通文件
#define T_DEVICE  3   // 设备文件

// struct stat 结构体用于存储文件的属性信息，包括文件系统的设备号、inode 号、文件类型、文件链接数以及文件大小等。

struct stat {
  int dev;      // 文件系统的磁盘设备号
  uint ino;     // inode 号
  short type;   // 文件类型，表示文件是目录、普通文件还是设备文件
  short nlink;  // 链接数，表示有多少个目录项链接到该文件
  uint64 size;  // 文件大小，以字节为单位
};
