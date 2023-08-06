// ELF可执行文件的格式

// "\x7FELF" in little endian
#define ELF_MAGIC 0x464C457FU 

// File header
struct elfhdr {
  uint magic;       // 魔数，必须等于 ELF_MAGIC
  uchar elf[12];    // ELF 文件标识符
  ushort type;      // 文件类型，指示 ELF 文件的类型（可执行文件、共享目标文件等）
  ushort machine;   // 目标体系结构，表示生成该文件的目标处理器架构
  uint version;     // 文件版本号
  uint64 entry;     // 程序入口点的虚拟地址，即程序的执行起始地址
  uint64 phoff;     // 程序头表（Program Header Table）的偏移量，即程序头表在文件中的位置
  uint64 shoff;     // 节头表（Section Header Table）的偏移量，即节头表在文件中的位置
  uint flags;       // 与文件相关的标志
  ushort ehsize;    // ELF 头的大小（字节数）
  ushort phentsize; // 程序头表项的大小（字节数）
  ushort phnum;     // 程序头表中的项数
  ushort shentsize; // 节头表项的大小（字节数）
  ushort shnum;     // 节头表中的节的数量
  ushort shstrndx;  // 包含节名称字符串的节头表索引
};

// Program section header
struct proghdr {
  uint32 type;     // 段类型，指示该段是可加载段还是其他类型的段
  uint32 flags;    // 段标志，表示该段的属性，如可执行、可写和可读等
  uint64 off;      // 段在文件中的偏移量
  uint64 vaddr;    // 段在虚拟地址空间中的虚拟地址
  uint64 paddr;    // 段在物理地址空间中的物理地址（一般不用）
  uint64 filesz;   // 段在文件中的大小（字节数）
  uint64 memsz;    // 段在内存中的大小（字节数）
  uint64 align;    // 段在文件和内存中的对齐方式
};

// Proghdr类型的值
#define ELF_PROG_LOAD 1

// Proghdr标志的标志位
#define ELF_PROG_FLAG_EXEC 1
#define ELF_PROG_FLAG_WRITE 2
#define ELF_PROG_FLAG_READ 4
