// 定义了一些文件访问标志（File Access Flags），用于在操作系统中表示文件打开时的不同模式。

#define O_RDONLY  0x000    // 以只读模式打开文件
#define O_WRONLY  0x001    // 以只写模式打开文件
#define O_RDWR    0x002    // 以读写模式打开文件
#define O_CREATE  0x200    // 如果文件不存在，则创建文件
#define O_TRUNC   0x400    // 如果文件存在且以可写方式打开，则截断文件大小为0
