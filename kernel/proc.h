// 用于内核上下文切换的保存的寄存器结构体。
struct context {
uint64 ra; // 返回地址寄存器（return address）
uint64 sp; // 栈指针寄存器（stack pointer）

// 被调用者保存寄存器
uint64 s0; // 保留寄存器 s0
uint64 s1; // 保留寄存器 s1
uint64 s2; // 保留寄存器 s2
uint64 s3; // 保留寄存器 s3
uint64 s4; // 保留寄存器 s4
uint64 s5; // 保留寄存器 s5
uint64 s6; // 保留寄存器 s6
uint64 s7; // 保留寄存器 s7
uint64 s8; // 保留寄存器 s8
uint64 s9; // 保留寄存器 s9
uint64 s10; // 保留寄存器 s10
uint64 s11; // 保留寄存器 s11
};

// 每个 CPU 的状态。
struct cpu {
struct proc *proc; // 指向当前运行在该 CPU 上的进程，如果没有进程在运行则为 null。
struct context context; // 用于在内核态下进行上下文切换的寄存器保存区域。当需要从进程切换到调度器时使用 swtch() 函数。
int noff; // push_off() 函数的嵌套深度，用于记录 push_off() 函数的调用次数，用于实现中断嵌套。
int intena; // 在 push_off() 函数调用之前，中断是否被允许的标志。

};

extern struct cpu cpus[NCPU];

//蹦床中陷阱处理代码的每个过程数据。
//单独坐在蹦床页面下方
//用户页面表。在内核页面表中没有特别映射。
//sscratch寄存器指向这里。
//在蹦床中的uservec.S将用户注册保存在trapframe中，
//然后从trapframe的
//kernel_sp、kernel_hartid、kernel_satp，然后跳到kernel_trap。
//usertrapret（）和userret在蹦床上。S设置
//trapframe的kernel_*，从
//trapframe，切换到用户页面表，然后输入用户空间。
//trapframe包括被调用者保存的用户寄存器，如s0-s11，因为
//通过usertrapret（）返回到用户路径不会通过
//整个内核调用堆栈。

// trapframe 结构体用于保存进程或线程的上下文信息，在进行内核态与用户态切换或进行中断处理时，保存寄存器状态，以便在需要时进行恢复。
struct trapframe {
  /*   0 */ uint64 kernel_satp;   // 内核页表的 satp 寄存器的值，用于内核模式下的地址映射。
  /*   8 */ uint64 kernel_sp;     // 进程内核栈的顶部地址，用于恢复进程的内核栈指针。
  /*  16 */ uint64 kernel_trap;   // usertrap() 函数的地址，用于恢复进程的返回地址。
  /*  24 */ uint64 epc;           // 用户程序计数器的值，保存用户程序在触发中断或异常之前的地址。
  /*  32 */ uint64 kernel_hartid; // 内核模式下的 hartid 寄存器的值，保存内核模式下的 hartid。
  /*  40 */ uint64 ra;            // 通用寄存器 ra 的值，用于保存返回地址。
  /*  48 */ uint64 sp;            // 通用寄存器 sp 的值，用于保存栈指针。
  /*  56 */ uint64 gp;            // 通用寄存器 gp 的值，保存全局指针。
  /*  64 */ uint64 tp;            // 通用寄存器 tp 的值，保存线程指针。
  /*  72 */ uint64 t0;            // 通用寄存器 t0 的值。
  /*  80 */ uint64 t1;            // 通用寄存器 t1 的值。
  /*  88 */ uint64 t2;            // 通用寄存器 t2 的值。
  /*  96 */ uint64 s0;            // 通用寄存器 s0 (fp) 的值，保存帧指针。
  /* 104 */ uint64 s1;            // 通用寄存器 s1 的值。
  /* 112 */ uint64 a0;            // 通用寄存器 a0 的值，用于保存函数参数。
  /* 120 */ uint64 a1;            // 通用寄存器 a1 的值，用于保存函数参数。
  /* 128 */ uint64 a2;            // 通用寄存器 a2 的值，用于保存函数参数。
  /* 136 */ uint64 a3;            // 通用寄存器 a3 的值，用于保存函数参数。
  /* 144 */ uint64 a4;            // 通用寄存器 a4 的值，用于保存函数参数。
  /* 152 */ uint64 a5;            // 通用寄存器 a5 的值，用于保存函数参数。
  /* 160 */ uint64 a6;            // 通用寄存器 a6 的值，用于保存函数参数。
  /* 168 */ uint64 a7;            // 通用寄存器 a7 的值，用于保存函数参数。
  /* 176 */ uint64 s2;            // 通用寄存器 s2 的值。
  /* 184 */ uint64 s3;            // 通用寄存器 s3 的值。
  /* 192 */ uint64 s4;            // 通用寄存器 s4 的值。
  /* 200 */ uint64 s5;            // 通用寄存器 s5 的值。
  /* 208 */ uint64 s6;            // 通用寄存器 s6 的值。
  /* 216 */ uint64 s7;            // 通用寄存器 s7 的值。
  /* 224 */ uint64 s8;            // 通用寄存器 s8 的值。
  /* 232 */ uint64 s9;            // 通用寄存器 s9 的值。
  /* 240 */ uint64 s10;           // 通用寄存器 s10 的值。
  /* 248 */ uint64 s11;           // 通用寄存器 s11 的值。
  /* 256 */ uint64 t3;            // 通用寄存器 t3 的值。
  /* 264 */ uint64 t4;            // 通用寄存器 t4 的值。
  /* 272 */ uint64 t5;            // 通用寄存器 t5 的值。
  /* 280 */ uint64 t6;            // 通用寄存器 t6 的值。
};

// 枚举类型 procstate 表示进程的状态，包括 UNUSED（未使用）、USED（已使用）、SLEEPING（睡眠中）、RUNNABLE（可运行）、RUNNING（运行中）和 ZOMBIE（僵尸）等状态。
enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// struct proc 结构体表示一个进程的状态信息。
// 每个进程都有一个对应的 struct proc 结构体，用于存储进程的各种状态和属性。
struct proc {
  struct spinlock lock;         // 进程锁，用于保护进程的状态。

  // 在使用下面的字段时必须持有 p->lock：
  enum procstate state;         // 进程状态，表示进程当前所处的状态。
  void *chan;                   // 如果非零，表示进程正在等待 chan 上的事件。
  int killed;                   // 如果非零，表示进程已被终止。
  int xstate;                   // 用于返回给父进程的退出状态。
  int pid;                      // 进程ID，用于标识进程的唯一标识符。

  // 在使用下面的字段时必须持有 wait_lock：
  struct proc *parent;          // 父进程指针，用于标识当前进程的父进程。

  // 下面的字段是私有的，因此不需要持有 p->lock：
  uint64 kstack;                // 进程内核栈的虚拟地址。
  uint64 sz;                    // 进程内存大小（字节）。
  pagetable_t pagetable;        // 进程用户态页表。
  struct trapframe *trapframe;  // trampoline.S 的数据页，用于保存陷阱帧信息。
  struct context context;       // swtch() 调用的上下文，用于运行进程。
  struct file *ofile[NOFILE];   // 进程打开的文件数组。
  struct inode *cwd;            // 当前工作目录。
  char name[16];                // 进程名（用于调试）。
};
