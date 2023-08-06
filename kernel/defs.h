// 声明各种数据结构的头文件

struct buf;          // 缓冲区结构体，在文件系统中用于管理数据块的缓存。
struct context;      // 上下文结构体，用于保存线程的寄存器状态。
struct file;         // 文件结构体，表示进程打开的文件描述符。
struct inode;        // inode 结构体，用于表示文件系统中的一个索引节点。
struct pipe;         // 管道结构体，用于实现进程间的通信。
struct proc;         // 进程结构体，表示一个正在运行的进程。
struct spinlock;     // 自旋锁结构体，用于实现自旋锁机制。
struct sleeplock;    // 睡眠锁结构体，用于实现睡眠锁机制。
struct stat;         // 文件状态结构体，用于获取文件信息。
struct superblock;   // 超级块结构体，表示文件系统的超级块信息。

// bio.c
void            binit(void);                                   // 初始化缓冲区缓存 (buffer cache)
struct buf*     bread(uint, uint);                             // 读取设备 (设备号为dev) 上的块号为blockno的数据块
void            brelse(struct buf*);                           // 释放一个缓冲区 (buffer)，将其标记为可复用
void            bwrite(struct buf*);                           // 将一个缓冲区的内容写回磁盘
void            bpin(struct buf*);                             // 增加缓冲区的引用计数 (refcnt)
void            bunpin(struct buf*);                           // 减少缓冲区的引用计数 (refcnt)

// console.c
void            consoleinit(void);                             // 初始化终端控制台
void            consoleintr(int);                              // 处理终端控制台的中断事件
void            consputc(int);                                 // 发送一个字符到终端控制台

// exec.c
int             exec(char*, char**);                           // 执行一个新的程序，用于加载并运行可执行文件

// file.c
struct file*    filealloc(void);                               // 分配一个文件结构体 (file)
void            fileclose(struct file*);                       // 关闭文件，释放文件结构体
struct file*    filedup(struct file*);                         // 复制文件结构体
void            fileinit(void);                                // 初始化文件系统的文件表
int             fileread(struct file*, uint64, int n);         // 从文件中读取数据到指定地址
int             filestat(struct file*, uint64 addr);           // 获取文件信息并写入到指定地址
int             filewrite(struct file*, uint64, int n);        // 将数据从指定地址写入到文件中

// fs.c
void            fsinit(int);                                   // 初始化文件系统
int             dirlink(struct inode*, char*, uint);           // 创建目录项（硬链接）
struct inode*   dirlookup(struct inode*, char*, uint*);        // 在目录中查找指定项
struct inode*   ialloc(uint, short);                           // 分配 inode，并返回对应的 inode 指针
struct inode*   idup(struct inode*);                           // 复制 inode
void            iinit();                                       // 初始化 inode 管理
void            ilock(struct inode*);                          // 锁定 inode
void            iput(struct inode*);                           // 释放 inode
void            iunlock(struct inode*);                        // 解锁 inode
void            iunlockput(struct inode*);                     // 解锁 inode 并释放
void            iupdate(struct inode*);                        // 更新 inode
int             namecmp(const char*, const char*);             // 比较文件名
struct inode*   namei(char*);                                  // 根据路径名查找 inode
struct inode*   nameiparent(char*, char*);                     // 根据路径名查找父目录的 inode
int             readi(struct inode*, int, uint64, uint, uint); // 从 inode 读取数据
void            stati(struct inode*, struct stat*);            // 获取 inode 的状态
int             writei(struct inode*, int, uint64, uint, uint); // 写入数据到 inode
void            itrunc(struct inode*);                         // 截断 inode

// ramdisk.c
void            ramdiskinit(void);                             // 初始化内存磁盘（RAM 磁盘）
void            ramdiskintr(void);                             // 处理 RAM 磁盘中断事件
void            ramdiskrw(struct buf*);                        // 从 RAM 磁盘读取/写入数据

// kalloc.c
void*           kalloc(void);                                  // 分配内核内存
void            kfree(void*);                                  // 释放内核内存
void            kinit(void);                                   // 初始化内核内存管理

// log.c
void            initlog(int, struct superblock*);              // 初始化日志系统
void            log_write(struct buf*);                        // 写入数据到日志
void            begin_op(void);                                // 开始操作（记录在日志中）
void            end_op(void);                                  // 结束操作（提交日志）

// pipe.c
int             pipealloc(struct file**, struct file**);       // 分配管道文件结构体
void            pipeclose(struct pipe*, int);                  // 关闭管道
int             piperead(struct pipe*, uint64, int);           // 从管道读取数据
int             pipewrite(struct pipe*, uint64, int);          // 写入数据到管道

// printf.c
void            printf(char*, ...);                            // 格式化输出函数，类似于 C 标准库中的 printf
void            panic(char*) __attribute__((noreturn));        // 内核恐慌，输出错误信息并终止
void            printfinit(void);                              // 初始化 printf 相关的数据结构

// proc.c
int             cpuid(void);                                   // 返回当前 CPU 的 ID
void            exit(int);                                     // 进程退出
int             fork(void);                                    // 创建一个新的进程（复制当前进程）
int             growproc(int);                                 // 扩展进程的地址空间
void            proc_mapstacks(pagetable_t);                   // 映射栈区域的页表条目
pagetable_t     proc_pagetable(struct proc *);                 // 获取进程的页表指针
void            proc_freepagetable(pagetable_t, uint64);       // 释放进程的页表
int             kill(int);                                     // 终止指定进程
struct cpu*     mycpu(void);                                   // 返回当前 CPU 的 cpu 结构体指针
struct cpu*     getmycpu(void);                                // 返回当前 CPU 的 cpu 结构体指针（旧版本）
struct proc*    myproc(void);                                  // 返回当前进程的 proc 结构体指针
void            procinit(void);                                // 进程管理子系统初始化
void            scheduler(void) __attribute__((noreturn));     // 调度器主循环（无返回值）
void            sched(void);                                   // 进程调度
void            sleep(void*, struct spinlock*);                // 进程睡眠
void            userinit(void);                                // 初始化用户进程
int             wait(uint64);                                  // 等待子进程终止
void            wakeup(void*);                                 // 唤醒睡眠进程
void            yield(void);                                   // 放弃 CPU 使用权
int             either_copyout(int user_dst, uint64 dst, void *src, uint64 len); // 从内核/用户空间复制数据到用户空间
int             either_copyin(void *dst, int user_src, uint64 src, uint64 len);  // 从用户空间复制数据到内核空间
void            procdump(void);                                // 打印进程信息

// swtch.S
void            swtch(struct context*, struct context*);       // 切换上下文

// spinlock.c
void            acquire(struct spinlock*);                     // 获取自旋锁
int             holding(struct spinlock*);                     // 判断自旋锁是否被当前 CPU 持有
void            initlock(struct spinlock*, char*);             // 初始化自旋锁
void            release(struct spinlock*);                     // 释放自旋锁
void            push_off(void);                                // 关中断（关闭中断标志位）
void            pop_off(void);                                 // 开中断（恢复中断标志位）

// sleeplock.c
void            acquiresleep(struct sleeplock*);               // 获取睡眠锁
void            releasesleep(struct sleeplock*);               // 释放睡眠锁
int             holdingsleep(struct sleeplock*);               // 判断睡眠锁是否被当前进程持有
void            initsleeplock(struct sleeplock*, char*);       // 初始化睡眠锁

// string.c
int             memcmp(const void*, const void*, uint);       // 比较内存区域内容是否相等
void*           memmove(void*, const void*, uint);            // 内存区域拷贝（处理重叠情况）
void*           memset(void*, int, uint);                     // 将内存区域设置为指定值
char*           safestrcpy(char*, const char*, int);          // 安全拷贝字符串
int             strlen(const char*);                          // 获取字符串长度
int             strncmp(const char*, const char*, uint);      // 比较指定长度的字符串是否相等
char*           strncpy(char*, const char*, int);             // 拷贝指定长度的字符串

// syscall.c
int             argint(int, int*);                            // 获取用户态参数中的整数
int             argstr(int, char*, int);                      // 获取用户态参数中的字符串
int             argaddr(int, uint64 *);                       // 获取用户态参数中的地址
int             fetchstr(uint64, char*, int);                 // 从用户态复制字符串到内核
int             fetchaddr(uint64, uint64*);                   // 从用户态复制地址到内核
void            syscall();                                    // 系统调用入口

// trap.c
extern uint     ticks;                                        // 系统启动以来的时钟滴答数
void            trapinit(void);                               // 中断初始化
void            trapinithart(void);                           // 中断初始化（针对每个 CPU）
extern struct spinlock tickslock;                             // 时钟滴答数锁
void            usertrapret(void);                            // 用户态中断返回

// uart.c
void            uartinit(void);                               // UART 初始化
void            uartintr(void);                               // UART 中断处理
void            uartputc(int);                                // 输出字符到 UART
void            uartputc_sync(int);                           // 同步输出字符到 UART
int             uartgetc(void);                               // 从 UART 获取字符

// vm.c
void            kvminit(void);                                // 内核虚拟内存初始化
void            kvminithart(void);                            // 内核虚拟内存初始化（针对每个 CPU）
void            kvmmap(pagetable_t, uint64, uint64, uint64, int); // 内核页表映射
int             mappages(pagetable_t, uint64, uint64, uint64, int); // 页表映射
pagetable_t     uvmcreate(void);                              // 创建用户进程页表
void            uvminit(pagetable_t, uchar *, uint);          // 用户进程虚拟内存初始化
uint64          uvmalloc(pagetable_t, uint64, uint64);        // 用户进程虚拟内存分配
uint64          uvmdealloc(pagetable_t, uint64, uint64);      // 用户进程虚拟内存释放
int             uvmcopy(pagetable_t, pagetable_t, uint64);    // 用户进程虚拟内存拷贝
void            uvmfree(pagetable_t, uint64);                 // 释放用户进程虚拟内存
void            uvmunmap(pagetable_t, uint64, uint64, int);   // 解除用户进程虚拟内存映射
void            uvmclear(pagetable_t, uint64);                // 清除用户进程虚拟内存映射
uint64          walkaddr(pagetable_t, uint64);                // 从页表中获取物理地址
int             copyout(pagetable_t, uint64, char *, uint64); // 从内核复制数据到用户态
int             copyin(pagetable_t, char *, uint64, uint64);  // 从用户态复制数据到内核
int             copyinstr(pagetable_t, char *, uint64, uint64); // 从用户态复制字符串到内核

// plic.c
void            plicinit(void);                               // PLIC（外部中断控制器）初始化
void            plicinithart(void);                           // PLIC 初始化（针对每个 CPU）
int             plic_claim(void);                             // 获取 PLIC 中断号
void            plic_complete(int);                           // 完成 PLIC 中断处理

// virtio_disk.c
void            virtio_disk_init(void);                       // VIRTIO 硬盘初始化
void            virtio_disk_rw(struct buf *, int);            // 读取/写入磁盘
void            virtio_disk_intr(void);                       // VIRTIO 硬盘中断处理

// number of elements in fixed-size array
// 计算固定大小数组中元素的数量
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
