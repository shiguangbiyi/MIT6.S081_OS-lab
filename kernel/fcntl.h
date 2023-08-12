// fcntl.h - 文件控制相关的常量和宏定义

// O_RDONLY: 以只读模式打开文件
#define O_RDONLY  0x000

// O_WRONLY: 以只写模式打开文件
#define O_WRONLY  0x001

// O_RDWR: 以读写模式打开文件
#define O_RDWR    0x002

// O_CREATE: 如果文件不存在，则创建文件
#define O_CREATE  0x200

// O_TRUNC: 如果文件已存在，在打开时截断文件大小为零
#define O_TRUNC   0x400

// O_NOFOLLOW: 在打开文件时不跟随符号链接
#define O_NOFOLLOW 0x800
