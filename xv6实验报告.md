## Lab1 Utilities

### 1.1 Sleep

#### 1.1.1 实验目的

#### 1.1.2 实验提示

#### 1.1.3 实验步骤

#### 1.1.4 实验结果

#### 1.1.5 实验分析

#### 1.1.6 实验总结



### 1.2 Pingpong

#### 1.2.1 实验目的

#### 1.2.2 实验提示

#### 1.2.3 实验步骤

#### 1.2.4 实验结果

#### 1.2.5 实验分析

#### 1.2.6 实验总结

### 1.3 Primes

#### 1.3.1 实验目的

#### 1.3.2 实验提示

#### 1.3.3 实验步骤

#### 1.3.4 实验结果

#### 1.3.5 实验分析

#### 1.3.6 实验总结

### 1.4 Find

#### 1.4.1 实验目的

#### 1.4.2 实验提示

#### 1.4.3 实验步骤

#### 1.4.4 实验结果

#### 1.4.5 实验分析

#### 1.4.6 实验总结

### 1.5 Xargs

#### 1.5.1 实验目的

#### 1.5.2 实验提示

#### 1.5.3 实验步骤

#### 1.5.4 实验结果

#### 1.5.5 实验分析

#### 1.5.6 实验总结





## Lab2 system calls

### 2.1 System call tracing

#### 2.1.1 实验目的

#### 2.1.2 实验提示

#### 2.1.3 实验步骤

#### 2.1.4 实验结果

#### 2.1.5 实验分析

#### 2.1.6 实验总结

### 2.2 Sysiinfo

#### 2.2.1 实验目的

#### 2.2.2 实验提示

#### 2.2.3 实验步骤

#### 2.2.4 实验结果

#### 2.2.5 实验分析

#### 2.2.6 实验总结





## Lab3 page tables

### 3.1 Speed up system calls

#### 3.1.1 实验目的

#### 3.1.2 实验提示

#### 3.1.3 实验步骤

#### 3.1.4 实验结果

#### 3.1.5 实验分析

#### 3.1.6 实验总结

### 3.2 Print a page table（打印页表）

#### 3.2.1 实验目的

​		编写一个函数来打印页表的内容。

#### 3.2.2 实验提示

1. 可以把vmprint()放在kernel/vm.c中。
2. 使用kernel/riscv.h文件末尾的宏。
3. 函数freewalk可能会给一些启示。
4. 在kernel/defs.h中定义vmprint的原型，这样就可以从exec.c中调用它。
5. 在printf调用中使用%p，以按照示例中显示的方式打印完整的64位十六进制PTE和地址。

#### 3.2.3 实验步骤

#### 3.2.4 实验结果

#### 3.2.5 实验分析

#### 3.2.6 实验总结

### 3.3 Detecting which pages have been accessed

#### 3.3.1 实验目的

​		检测已访问的页面。为xv6添加一个新功能，通过检查RISC-V页表中的访问位，检测并将此信息报告给用户空间。当RISC-V硬件页行走器解析TLB（转换查找缓冲）未命中时，它会在PTE（页表项）中标记这些位。

​		实现 pgaccess() 系统调用，该调用报告哪些页面已被访问。系统调用有三个参数。首先，接受要检查的第一个用户页的起始虚拟地址。其次，接受要检查的页面数量。最后，接受一个用户地址，用于将结果存储到位掩码中。

#### 3.3.2 实验提示

- 在kernel/proc.c的proc_pagetable()中执行映射。首先，在kernel/sysproc.c中实现sys_pgaccess()。

- 使用argaddr()和argint()来解析参数。

- 对于输出的位掩码，最好在内核中存储一个临时缓冲区，并在填充正确的位后将其复制到用户空间（通过copyout()）。

- 对可以扫描的页面数量设置上限。

- kernel/vm.c中的walk()对于查找正确的PTE非常有用。

- 需要在kernel/riscv.h中定义PTE_A，即访问位。请查阅RISC-V手册以确定其值。

- 确保在检查是否已设置PTE_A后将其清除。否则，将无法确定自上次调用pgaccess()以来是否已访问页面（即位将永远设置）。

- 用于调试页面表的vmprint()可能会很有用。

#### 3.3.3 实验步骤

1. ##### 在 kernel/riscv.h 中定义PTE_A（访问位，根据RISC-V手册查阅）

   ![image-20230809000646627](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230809000646627.png)

   ```c
   #define PTE_A (1L << 6)  // 将访问位的位置左移6位
   ```

2. ##### 在kernel/sysproc.c指定的位置中实现pgaccess()


```c
#ifdef LAB_PGTBL // 仅在定义了LAB_PGTBL时编译以下代码
// 系统调用：报告已访问的页面
int
sys_pgaccess(void)
{
  uint64 srcva, st; // 输入参数的虚拟地址和存储位掩码的用户地址
  int len; // 要检查的页面数量
  uint64 buf = 0; // 用于存储位掩码的缓冲区
  struct proc *p = myproc(); // 获取当前进程

  acquire(&p->lock); // 获取进程锁，确保不会发生竞争

  // 解析系统调用参数（实验提示的第二步）
  argaddr(0, &srcva); // 获取第一个参数：要检查的用户虚拟地址的起始位置
  argint(1, &len);    // 获取第二个参数：要检查的页面数量
  argaddr(2, &st);    // 获取第三个参数：存储位掩码的用户地址

  // 检查页面数量是否合法,设置上限（实验提示的第四步）
  if ((len > 32) || (len < 1))
    return -1;

  pte_t *pte;
  for (int i = 0; i < len; i++) {
    // 获取当前页面的页表项（用walk函数寻找PTE的地址，实验提示的第五步）
    pte = walk(p->pagetable, srcva + i * PGSIZE, 0);
    if (pte == 0) {
      release(&p->lock); // 释放进程锁
      return -1; // 无效页面，返回错误
    }
    
    // 检查页面是否有效且属于用户空间
    if ((*pte & PTE_V) == 0 || (*pte & PTE_U) == 0) {
      release(&p->lock); // 释放进程锁
      return -1; // 无效页面或不属于用户空间，返回错误
    }
    
    // 检查页面的访问位是否已设置
    if (*pte & PTE_A) {  // 按位与运算
      *pte = *pte & ~PTE_A; // 清除访问位
      buf |= (1 << i); // 设置位掩码中的相应位
    }
  }
  
  release(&p->lock); // 释放进程锁

  // 将位掩码复制到用户空间（实验提示的第三步）
  copyout(p->pagetable, st, (char *)&buf, ((len - 1) / 8) + 1);

  return 0; // 返回成功
}
#endif // 结束LAB_PGTBL编译条件
```

#### 3.3.4 实验结果

![image-20230808234754076](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230808234754076.png)

![image-20230808235023512](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230808235023512.png)

#### 3.3.5 实验分析

- ##### 在riscv.h中定义的PTE_A是什么？


​		访问位（Access Bit）表示页面是否被访问过（读或写）。当程序访问页面时，硬件会将访问位设置为1，从而记录页面的访问情况。通常用于页面置换算法中。

- ##### 本实验引用的kernel/vm.c中的walk函数的作用是什么？


​		由代码可知，传入walk函数的参数为：页表、虚拟地址、一个信号量，该函数的作用是在页表pagetable中查找虚拟地址va对应的PTE的地址。如果alloc非0，则会创建所需的页表页。

​		本实验中，walk函数找到对应的页表项后，即可判断是否被访问，从而进行检测。

- ##### 清除访问位的作用是什么？


​		清除访问位的目的是为了在下一次检测页面访问时能够正确地判断哪些页面被访问过。防止因为遍历导致的访问影响下一次判断。

#### 3.3.6 实验总结

​		本次实验的核心在于页表标志位的使用，通过PTE标志位判断访问情况，同时继续熟悉用户态进行系统调用后进入内核态的过程。对于页表的运作有更加清晰的认知。



## Lab4 traps

### 4.1 RISC-V assembly（RISC-V 汇编）

#### 4.1.1 实验目的

​		理解RISC-V汇编指令集。

​		在你的 xv6 仓库中有一个名为 `user/call.c` 的文件。执行 `make fs.img` 命令去编译，并且还会生成一个可读的程序汇编版本 `user/call.asm`。 阅读 `call.asm` 文件中关于函数 `g`、`f` 和 `main` 的代码。

#### 4.1.2 实验问题

1. 哪些寄存器包含函数的参数？例如，在 `main` 调用 `printf` 时，哪个寄存器包含数字 13？

2. 在 `main` 的汇编代码中，函数 `f` 的调用在哪里？函数 `g` 的调用在哪里？（提示：编译器可能会内联函数。）

3. 函数 `printf` 的地址是多少？

4. 在 `main` 中的 `jalr` 调用 `printf` 后，寄存器 `ra` 中的值是什么？

5. 运行以下代码。

   ```c
   unsigned int i = 0x00646c72;
   printf("H%x Wo%s", 57616, &i);
   ```

   输出是什么？下面是一个将字节映射到字符的 ASCII 表格。输出取决于 RISC-V 采用的小端序。如果 RISC-V 采用大端序，为了得到相同的输出，你会将 `i` 设置为多少？你需要将 57616 更改为不同的值吗？

6. 在下面的代码中，在打印出 'y=' 之后会打印什么？为什么会发生这种情况？

   ```c
   printf("x=%d y=%d", 3);
   ```

#### 4.1.3 实验步骤

1. ##### 生成可读汇编版本。

   ​		![image-20230809140143806](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230809140143806.png)

2. ##### 阅读 call.asm 中函数 `g`、`f` 和 `main` 的代码。


```assembly
    000000000000001c <main>:
    void main(void) {
      1c:	1141                	addi	sp,sp,-16
      1e:	e406                	sd	ra,8(sp)
      20:	e022                	sd	s0,0(sp)
      22:	0800                	addi	s0,sp,16
      printf("%d %d\n", f(8)+1, 13);
      24:	4635                	li	a2,13
      26:	45b1                	li	a1,12
      28:	00000517          	auipc	a0,0x0
      2c:	7a050513          	addi	a0,a0,1952 # 7c8 <malloc+0xe8>
      30:	00000097          	auipc	ra,0x0
      34:	5f8080e7          	jalr	1528(ra) # 628 <printf>
      exit(0);
      38:	4501                	li	a0,0
      3a:	00000097          	auipc	ra,0x0
      3e:	274080e7          	jalr	628(ra) # 2ae <exit>
```

#### 4.1.4 回答问题

1. 哪些寄存器包含函数的参数？例如，在 `main` 调用 `printf` 时，哪个寄存器包含数字 13？

   ​	查RISC-V调用约定寄存器表

   ​	![image-20230809143623120](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230809143623120.png)

   ​	由图，可知，a0~a7是包含函数参数的寄存器，在main调用printf时，包含数字13的寄存器，由上述查看的main代码，此行代码：

   ```assembly
     24:	4635                	li	a2,13
   ```

   可知为a2。

2. 在 `main` 的汇编代码中，函数 `f` 的调用在哪里？函数 `g` 的调用在哪里？（提示：编译器可能会内联函数。）

   ​	没有。 g(x) 被内联到 f(x) 中，然后 f(x) 又被进一步内联到 main() 中

3. 函数 `printf` 的地址是多少？

   ​	根据以下代码可知：

   ​	printf函数的入口处地址为0x0000000000000628,此即为该函数地址。

   ```assembly
   void
   printf(const char *fmt, ...)
   {
    628:	711d                	addi	sp,sp,-96
    62a:	ec06                	sd	ra,24(sp)
    62c:	e822                	sd	s0,16(sp)
    62e:	1000                	addi	s0,sp,32
    630:	e40c                	sd	a1,8(s0)
    632:	e810                	sd	a2,16(s0)
    634:	ec14                	sd	a3,24(s0)
    636:	f018                	sd	a4,32(s0)
    638:	f41c                	sd	a5,40(s0)
    63a:	03043823          	sd	a6,48(s0)
    63e:	03143c23          	sd	a7,56(s0)
     va_list ap;
   
     va_start(ap, fmt);
    642:	00840613          	addi	a2,s0,8
    646:	fec43423          	sd	a2,-24(s0)
     vprintf(1, fmt, ap);
    64a:	85aa                	mv	a1,a0
    64c:	4505                	li	a0,1
    64e:	00000097          	auipc	ra,0x0
    652:	dce080e7          	jalr	-562(ra) # 41c <vprintf>
   }
    656:	60e2                	ld	ra,24(sp)
    658:	6442                	ld	s0,16(sp)
    65a:	6125                	addi	sp,sp,96
    65c:	8082                	ret
   ```

4. 在 `main` 中的 `jalr` 调用 `printf` 后，寄存器 `ra` 中的值是什么？

   ​	jalr指令调用printf后，寄存器ra中的值为下一条指令的地址0x0000000000000038

   ```assembly
   34:	5f8080e7          	jalr	1528(ra) # 628 <printf>
   exit(0);
   38:	4501                	li	a0,0
   ```

5. 运行以下代码。

   ```c
   unsigned int i = 0x00646c72;
   printf("H%x Wo%s", 57616, &i);
   ```

   输出是什么？下面是一个将字节映射到字符的 ASCII 表格。输出取决于 RISC-V 采用的小端序。如果 RISC-V 采用大端序，为了得到相同的输出，你会将 `i` 设置为多少？你需要将 57616 更改为不同的值吗？

   在user/call.c的main函数中插入该代码：

   ```c
   void main(void) {
    unsigned int i = 0x00646c72;
    printf("H%x Wo%s", 57616, &i);  
    printf("%d %d\n", f(8)+1, 13);
    exit(0);
   }
   ```

   运行后输出：

   ![image-20230810112211173](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230810112211173.png)

   由小端序可知在i对应的字符中，‘0x72’对应‘r’，‘0x6c’对应‘l’，‘0x64’对应‘d’，‘00’代表字符串结束。

   如果采用大端序，则i应该设置为0x726c6400。57616对应的十六进制数为E110，不需要修改。

6. 在下面的代码中，在打印出 'y=' 之后会打印什么？为什么会发生这种情况？

   ```c
   printf("x=%d y=%d", 3);
   ```

​		调试方法同上，输出结果：

![image-20230810113019555](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230810113019555.png)

​		这里的y是一个不固定的值，由调度前的寄存器情况决定，因为该调用的请求参数的数量比已给参数数量多，所以第二个请求的参数就会给出原本寄存器中的值。

#### 4.1.5 实验分析

- ##### 1.call.asm是什么文件？call.c的作用是什么？

​		call.asm是call.c编译后保存的可读的汇编语言代码文件，方便进行汇编代码的研究。

​		call.c是编译的示例性代码，用于本实验的代码插入等实验过程。





### 4.2 Backtrace（回溯）

#### 4.2.1 实验目的

​		在xv6内核中实现一个回溯（backtrace）功能，并将其用于调试和错误处理。通过此实验，学习如何在内核代码中实现一个能够追踪函数调用链的功能，从而在出现错误或异常情况时更容易定位问题。

​		在 `kernel/printf.c` 中实现一个名为 `backtrace()` 的函数。在 `sys_sleep` 函数中插入对该函数的调用，然后运行 `bttest`，该测试调用了 `sys_sleep`。你的输出应该如下所示：

```bash
makefileCopy codebacktrace:
0x0000000080002cda
0x0000000080002bb6
0x0000000080002898
```

在 `bttest` 完成后退出 QEMU。在终端中运行以下命令（地址可能稍有不同），使用 `addr2line` 命令将上述地址映射到源代码行号：

```bash
shellCopy code$ addr2line -e kernel/kernel
0x0000000080002de2
0x0000000080002f4a
0x0000000080002bfc
Ctrl-D
```

应该会看到类似以下的输出：

```bash
bashCopy codekernel/sysproc.c:74
kernel/syscall.c:224
kernel/trap.c:85
```

​		编译器在每个堆栈帧中都放置一个帧指针，它保存了调用者的帧指针地址。你的回溯应该使用这些帧指针来遍历堆栈，并在每个堆栈帧中打印保存的返回地址。

#### 4.2.2 实验提示

1. 在 `kernel/defs.h` 中添加 `backtrace` 的原型，以便在 `sys_sleep` 中调用它。

2. GCC 编译器将当前执行函数的帧指针存储在寄存器 s0 中。在 kernel/riscv.h中添加以下函数：

   ```c
   static inline uint64
   r_fp()
   {
     uint64 x;
     asm volatile("mv %0, s0" : "=r" (x) );
     return x;
   }
   ```

   并在backtrace中调用此函数以读取当前帧指针。此函数使用内联汇编来读取寄存器 s0。

3. 这些讲义笔记中有一个关于堆栈帧布局的图片。注意返回地址位于堆栈帧的帧指针固定偏移量（-8），而保存的帧指针位于帧指针的固定偏移量（-16）。

4. 在 xv6 内核中，每个堆栈分配一个页，位于 PAGE 对齐的地址。可以使用 `PGROUNDDOWN(fp)` 和 `PGROUNDUP(fp)` 计算堆栈页的顶部和底部地址（参见 `kernel/riscv.h`）。这些数字有助于 `backtrace` 终止循环。

5. 一旦 `backtrace` 生效，从 `kernel/printf.c` 中的 `panic` 函数调用它，这样在内核发生 panic 时，你会看到内核的回溯信息。

#### 4.2.3 实验步骤

1. 在 defs.h 中添加声明

   ```c
   // printf.c
   void       printf(char*, ...);
   void       panic(char*) __attribute__((noreturn));
   void       printfinit(void);
   void       backtrace(void);
   ```

2. 在 riscv.h 中添加获取当前 fp（frame pointer）寄存器的方法

3. 实现 backtrace 函数

   ```C
   void backtrace() 
   {
     uint64 fp = r_fp(); // 获取当前帧指针的值
     while(fp != PGROUNDUP(fp)) { // 当还未到达栈底时
       uint64 ra = *(uint64*)(fp - 8); // 从当前帧指针中读取返回地址
       printf("%p\n", ra); // 打印返回地址
       fp = *(uint64*)(fp - 16); // 从当前帧指针中读取上一个帧指针的值
     }
   }
   ```

4. 在 sys_sleep 调用一次 backtrace()

   ```c
   uint64
   sys_sleep(void)
   {
     int n;
     uint ticks0;
     if(argint(0, &n) < 0)
       return -1;
     acquire(&tickslock);
     ticks0 = ticks;
     while(ticks - ticks0 < n){
       if(myproc()->killed){
         release(&tickslock);
         return -1;
       }
       sleep(&ticks, &tickslock);
     }
     release(&tickslock);
     backtrace();
     return 0;
   }
   ```

#### 4.2.4 实验结果

​		![image-20230810173349321](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230810173349321.png)

​		![image-20230810174744745](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230810174744745.png)

​		分析实验结果可知，该指令将地址转换成代码中位置，对应的即为函数调用返回的位置。

#### 4.2.5 实验分析

1. 什么是堆栈？什么是堆栈帧？

   ​		堆栈，是一种用于存储函数调用和局部变量的数据结构。堆栈是一块内存区域，用于管理程序的函数调用和控制流。在调用一个函数时，会将一些信息压入堆栈，当函数返回时，这些信息会被弹出，从而确保程序的控制流能够正确地传递。

   ​		堆栈帧（Stack Frame），也称为栈帧或活动记录（Activation Record），是堆栈中的一个小区域，用于存储特定函数调用的相关信息。每个函数调用都会对应一个堆栈帧。堆栈帧包含了以下信息：

   - **返回地址**：在函数调用结束后，程序需要知道从哪里继续执行。返回地址就是告诉程序应该回到哪个位置继续执行。
   - **帧指针**：帧指针是指向当前堆栈帧的指针，它指示了当前函数的栈帧在内存中的位置。帧指针有助于函数调用结束后恢复调用者的堆栈状态。
   - **局部变量**：函数内部声明的局部变量会被分配在堆栈帧中。它们在函数调用过程中存储数据，函数调用结束后被释放。
   - **函数参数**：函数调用时传递的参数也会存储在堆栈帧中，以便被调用函数使用。
   - **其他信息**：根据编程语言和体系结构的不同，堆栈帧还可能包含其他信息，如寄存器的值等。

2. 函数调用的过程中堆栈和堆栈帧有什么变化？

   ​	当一个函数调用另一个函数时，涉及到堆栈和堆栈帧的变化过程如下：

   1. **调用者函数的堆栈帧：** 在调用者函数中，当前函数的堆栈帧已经存在，其中包含了局部变量、函数参数、返回地址和帧指针等信息。
   2. **保存信息：** 在调用者函数调用另一个函数时，它会将一些信息保存到自己的堆栈帧中，以便在被调用函数执行完毕后能够正确返回。这些信息包括当前函数的返回地址和帧指针。
   3. **切换到被调用函数的堆栈帧：** 当调用者函数调用另一个函数时，程序会切换到被调用函数的堆栈帧，以便在其中执行被调用函数的代码。
   4. **创建被调用函数的堆栈帧：** 被调用函数会为自己的局部变量和参数分配空间，同时设置自己的帧指针，指向当前堆栈帧的位置。
   5. **执行被调用函数：** 在被调用函数的堆栈帧中，被调用函数开始执行其自身的代码，可以使用局部变量和参数。
   6. **恢复信息：** 当被调用函数执行完毕后，程序会从被调用函数的堆栈帧中恢复之前保存的信息，包括返回地址和帧指针。
   7. **返回到调用者函数：** 有了恢复的信息，程序可以正确地返回到调用者函数，并从保存的返回地址处继续执行。
   8. **销毁被调用函数的堆栈帧：** 被调用函数的堆栈帧会被销毁，释放局部变量和参数占用的内存。
   9. **恢复调用者函数的堆栈帧：** 最后，调用者函数的堆栈帧会被恢复，以确保调用者函数继续执行时，其局部变量、参数和控制流都保持正确的状态。

#### 4.2.6 实验总结

​		这个实验帮助理解了函数调用过程、堆栈帧的结构以及如何通过堆栈帧中的信息来回溯函数调用链。这对于调试和理解程序的运行时行为非常有帮助，尤其是在需要定位错误或分析代码执行路径时。通过这个实验，更深入地理解了操作系统底层机制。





## 4.3 Alarm（警报）

### 4.3.1 实验目的

​		在这个实验中，向xv6添加一个功能，以便在进程使用CPU时间时定期发出警报。这对于想要限制自己使用的CPU时间的计算密集型进程可能很有用，或者对于想要进行计算但也想要采取一些周期性操作的进程可能也很有用。更一般地说，将实现一种基本形式的用户级中断/故障处理程序；例如，可以使用类似的方法来处理应用程序中的页面故障。只要解决方案通过alarmtest和usertests，就可以认为是正确的。

### 4.3.2 实验提示

​		应该添加一个新的sigalarm（间隔，处理程序）系统调用。如果应用程序调用sigalarm（n，fn），那么在程序消耗的每n个"ticks" CPU时间之后，内核都应该调用应用程序函数fn。当fn返回时，应用程序应该从它离开的地方继续执行。在xv6中，一个tick是一个相当任意的时间单位，由硬件定时器生成中断的频率决定。如果应用程序调用sigalarm（0，0），内核应停止生成周期性的警报调用。

​		在xv6存储库中找到一个名为user/alarmtest.c的文件。将其添加到Makefile中。在添加sigalarm和sigreturn系统调用之前，它将无法正确编译。

​		alarmtest在test0中调用sigalarm（2，periodic），要求内核每2个tick强制调用periodic（），然后会自旋一段时间。可以在user/alarmtest.asm中看到alarmtest的汇编代码，这可能对调试很有帮助。当alarmtest产生类似以下输出且usertests也正确运行时，您的解决方案就是正确的：

```bash
$ alarmtest
test0 start
........alarm!
test0 passed
test1 start
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
...alarm!
..alarm!
test1 passed
test2 start
................alarm!
test2 passed
$ usertests
...
ALL TESTS PASSED
$
```

### 4.3.3 实验步骤

#### test0：invoke handler（**调用处理程序**）

​		首先通过修改内核，使其跳转到用户空间的警报处理程序，这将导致test0打印"警报！"。暂时不必担心"警报！"输出后会发生什么；如果在打印"警报！"后程序崩溃，那么现在只要这样做是可以的。

**提示**

- 需要修改Makefile，以便将alarmtest.c编译为xv6用户程序。
- 放在user/user.h中的正确声明是：

```C
  int sigalarm(int ticks, void (*handler)());
  int sigreturn(void);
```

- 更新user/usys.pl（生成user/usys.S）、kernel/syscall.h和kernel/syscall.c，以允许alarmtest调用sigalarm和sigreturn系统调用。
- 目前，您的sys_sigreturn只需返回零。
- sys_sigalarm（）应在proc结构（位于kernel/proc.h中）中的新字段中存储警报间隔和处理程序函数的指针。
- 需要跟踪自上次调用处理程序以来已经过去或剩余的tick数；对此，还需要proc结构的新字段。可以在proc.c的allocproc（）中初始化proc字段。
- 每次tick，硬件时钟都会强制产生中断，该中断在kernel/trap.c的usertrap（）中处理。
- 仅当存在定时器中断时才要操纵进程的警报tick；需要类似于以下内容：

```c
  if (which_dev == 2) ...
```

- 仅在进程有未处理的定时器时才调用警报函数。注意，用户的警报函数的地址可能为0（例如，在user/alarmtest.asm中，periodic位于地址0）。
- 需要修改usertrap（），以便在进程的警报间隔到期时，用户进程执行处理程序函数。在RISC-V上发生陷阱并返回到用户空间时，是什么决定了用户空间代码恢复执行的指令地址？
- 如果使用gdb查看陷阱，将qemu设置为仅使用一个CPU会更容易，您可以通过运行以下命令实现：

```bash
  make CPUS=1 qemu-gdb
```

- 如果alarmtest打印"警报！"，则表示已成功。

#### test1/test2()：resume interrupted code（恢复中断的代码）

​		很有可能alarmtest在打印"警报！"后在test0或test1中崩溃，或者alarmtest（最终）打印"test1失败"，或者alarmtest在没有打印"test1通过"的情况下退出。要修复这个问题，必须确保在警报处理程序完成后，控制权返回到用户程序最初被定时器中断中断的指令。必须确保恢复中断时寄存器内容被恢复为定时器中断发生时的值，以便用户程序可以在处理警报后继续运行。最后，应该在每次警报触发后“重新启用”警报计数器，以便定期调用处理程序。

​		作为起点，我们已为您做出了一个设计决策：用户警报处理程序在完成后需要调用sigreturn系统调用。请查看alarmtest.c中的periodic作为示例。这意味着您可以向usertrap和sys_sigreturn添加代码，以协作使用户进程在处理警报后正确恢复。

**提示**

- 解决方案将需要保存和恢复寄存器——需要保存和恢复哪些寄存器以正确恢复中断的代码？（提示：将有很多寄存器）。
- 当定时器触发时，usertrap在struct proc中保存足够的状态，以便sigreturn可以在处理警报后正确返回中断的用户代码。
- 防止对处理程序的可重入调用——如果处理程序尚未返回，内核不应再次调用它。test2会测试这一点。

一旦通过了test0、test1和test2，请运行usertests以确保没有破坏内核的任何其他部分。

### 4.3.4 实验结果







## Lab5 Copy-on-Write Fork for xv6

### 5.1 Implement copy-on write（实现写时复制）

#### 5.1.1 实验目的

​		在 xv6 内核中实现写时复制的 fork。

#### 5.1.2 实验提示

1. 懒惰的页面分配实验可能已经让你熟悉了与写时复制相关的 xv6 内核代码。然而，你不应该基于你的延迟分配解决方案来完成这个实验；请按照上述指示从头开始使用新的 xv6 副本。
2. 对于每个 PTE，记录它是否是 COW 映射可能会有所帮助。你可以使用 RISC-V PTE 中的 RSW（保留给软件）位来实现这一点。
3. usertests 探索了 cowtest 未测试的情景，所以不要忘记检查所有测试是否都通过了。
4. 在 kernel/riscv.h 的末尾有一些有用的页面表标志的宏和定义。 如果发生 COW 页面错误并且没有空闲内存，则应该终止该进程。

#### 5.1.3 实验步骤

1. 修改 uvmcopy()，将父进程的物理页面映射到子进程，而不是分配新页面。在子进程和父进程的 PTE 中清除 PTE_W。

   修改如下（注释部分为原代码删除部分）：

   ```c
   int
   uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
   {
     pte_t *pte;
     uint64 pa, i;
     uint flags;
     // char *mem;
   
     for(i = 0; i < sz; i += PGSIZE){
       if((pte = walk(old, i, 0)) == 0)
         panic("uvmcopy: pte should exist");
       if((*pte & PTE_V) == 0)
         panic("uvmcopy: page not present");
       pa = PTE2PA(*pte);
       *pte = *pte & ~(PTE_W);
       flags = PTE_FLAGS(*pte);
   	// 删除分配内存的操作代码
       // if((mem = kalloc()) == 0)
       //   goto err;
       // memmove(mem, (char*)pa, PGSIZE);
       if(mappages(new, i, PGSIZE, (uint64)pa, flags) != 0){  // 将mem修改为pa，将地址映射到父进程地址
         // kfree(mem);
         goto err;
       }
     }
     return 0;
   
    err:
     uvmunmap(new, 0, i / PGSIZE, 1);
     return -1;
   }
   ```

2. 修改 usertrap()，以识别页面错误。当 COW 页面发生页面错误时，使用 kalloc() 分配一个新页面，将旧页面复制到新页面，并在 PTE 中安装新页面，同时设置 PTE_W。

   查图可知，RSW字段在第8/9位。

   ![image-20230809000646627](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230809000646627.png)

   设置PTE_W

   ```c
   #define PTE_V (1L << 0) // valid
   #define PTE_R (1L << 1)
   #define PTE_W (1L << 2)
   #define PTE_X (1L << 3)
   #define PTE_U (1L << 4) // 1 -> user can access
   #define PTE_COW (1L << 8)  // cow page
   ```

3. 确保在最后一个 PTE 引用消失时释放每个物理页面，但不要在之前释放。一个好的方法是，为每个物理页面保留一个“引用计数”，表示引用该页面的用户页表数量。当 kalloc() 分配页面时，将页面的引用计数设置为一。当 fork 导致子进程共享页面时，增加页面的引用计数；每当任何进程从其页表中删除页面时，减少页面的引用计数。只有当页面的引用计数为零时，kfree() 才应该将页面放回空闲列表中。在固定大小的整数数组中保持这些计数是可以的。需要想出一个方案，如何索引数组以及如何选择其大小。例如，你可以使用页面的物理地址除以 4096 来索引数组，并为数组分配与 kalloc.c 中的 kinit() 放置在空闲列表中的任何页面的最高物理地址相等数量的元素。

   实现引用计数机制：

   ```c
   // kalloc.c
   // 定义一个结构体，用于管理内核内存分配
   struct {
     struct spinlock lock; // 自旋锁，用于保护分配内存的并发访问
     struct run *freelist; // 空闲内存块链表
     char *ref_page; // 每个物理页面的引用计数数组
     int page_cnt; // 引用计数数组中的页面数量
     char *end_; // 引用计数数组的末尾地址
   } kmem;
   
   // 计算给定范围内的页面数量
   int pagecnt(void *pa_start, void *pa_end) {
     char *p;
     int cnt = 0;
     p = (char *)PGROUNDUP((uint64)pa_start);
     for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
       cnt++;
     return cnt;
   }
   
   // 初始化内核内存分配
   void kinit() {
     initlock(&kmem.lock, "kmem"); // 初始化自旋锁
     kmem.page_cnt = pagecnt(end, (void *)PHYSTOP); // 计算引用计数数组中的页面数量
   
     kmem.ref_page = end; // 引用计数数组的起始地址为 end
     for (int i = 0; i < kmem.page_cnt; ++i) {
       kmem.ref_page[i] = 0; // 初始化引用计数数组，全部置零
     }
     kmem.end_ = kmem.ref_page + kmem.page_cnt; // 引用计数数组的末尾地址
   
     freerange(kmem.end_, (void *)PHYSTOP); // 初始化空闲内存块链表
   }
   
   // 根据物理地址计算页面在引用计数数组中的索引
   int page_index(uint64 pa) {
     pa = PGROUNDDOWN(pa);
     int res = (pa - (uint64)kmem.end_) / PGSIZE;  // 根据物理地址和末地址对页大小的除法运算可以计算索引
     if (res < 0 || res >= kmem.page_cnt) {
       panic("page_index illegal"); // 如果索引越界，产生内核崩溃
     }
     return res;
   }
   
   // 增加物理页面的引用计数
   void incr(void *pa) {
     int index = page_index((uint64)pa);
     acquire(&kmem.lock); // 获取自旋锁，保护对引用计数数组的访问
     kmem.ref_page[index]++; // 增加引用计数
     release(&kmem.lock); // 释放自旋锁
   }
   
   // 减少物理页面的引用计数
   void decr(void *pa) {
     int index = page_index((uint64)pa);
     acquire(&kmem.lock); // 获取自旋锁，保护对引用计数数组的访问
     kmem.ref_page[index]--; // 减少引用计数
     release(&kmem.lock); // 释放自旋锁
   }
   
   
   // 释放一个物理页面
   void kfree(void *pa) {
     // 只有当页面的引用计数为零时，kfree() 才应该将页面放回空闲列表中。
     int index = page_index((uint64)pa); // 获取物理页面的索引
       
     if (kmem.ref_page[index] > 1) { // 如果有多个引用
       decr(pa); // 减少引用计数
       return; // 退出函数，不立即释放页面
     }
     if (kmem.ref_page[index] == 1) { // 如果只有一个引用
       decr(pa); // 减少引用计数
     }
     struct run *r;
   
     if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
       panic("kfree"); // 物理页面的地址不合法，产生内核崩溃
   
     // 填充页面数据，以便捕获悬空引用
     memset(pa, 1, PGSIZE);
   
     r = (struct run *)pa; // 将物理页面转换为 run 结构体指针
   
     acquire(&kmem.lock); // 获取自旋锁，保护空闲内存块链表的访问
     r->next = kmem.freelist; // 将当前页面插入到空闲内存块链表的头部
     kmem.freelist = r; // 更新空闲内存块链表的头指针
     release(&kmem.lock); // 释放自旋锁，允许其他线程访问空闲内存块链表
   }
   ```

   kalloc()分配页面引用计数设置为一：

   ```c
   // kalloc.c
   void *
   kalloc(void)
   {
    struct run *r;
    acquire(&kmem.lock);
    r = kmem.freelist;
    if(r)
     kmem.freelist = r->next;
    release(&kmem.lock);
   
    // 当 kalloc() 分配页面时，将页面的引用计数设置为一。
    if(r){
     memset((char*)r, 5, PGSIZE); // fill with junk
     incr(r);  // 引用计数+1
    }
   
    return (void*)r;
   }
   ```

   写时复制实现代码：

   ```c
   // vm.c
   // 复制用户空间的内存页面到新的页表，启用写时复制机制
   int uvmcopy(pagetable_t old, pagetable_t new, uint64 sz) {
    pte_t *pte;
    uint64 pa, i;
    uint flags;
    for (i = 0; i < sz; i += PGSIZE) {
     if ((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist"); // 确保页表项存在
     if ((*pte & PTE_V) == 0)
      panic("uvmcopy: page not present"); // 确保页面在内存中
     pa = PTE2PA(*pte); // 获取物理地址
     *pte = *pte & ~(PTE_W); // 去除写标志
     *pte = *pte | PTE_COW; // 设置写时复制标志
     flags = PTE_FLAGS(*pte); // 获取页表项的标志位
   
     // 原方案
     // if((mem = kalloc()) == 0)
     //  goto err;
     // memmove(mem, (char*)pa, PGSIZE);
   
     // 将原始的内存映射到新页表中，mem换为pa，即将映射由新内存转到父进程地址，以实现共享内存空间。
     if (mappages(new, i, PGSIZE, (uint64)pa, flags) != 0) {
      goto err; // 映射失败，跳转到错误处理
     }
     incr((void *)pa); // 增加物理页面的引用计数
    }
    return 0; // 复制成功，返回0
   err:
    uvmunmap(new, 0, i / PGSIZE, 1); // 解除映射关系
    return -1; // 复制失败，返回-1
   }
   ```

   ```C
   // vm.c
   // 检查是否发生写时复制（Copy-on-Write）页面错误，如果是写时复制页面，则可以进行写时复制。
   int is_cow_fault(pagetable_t pagetable, uint64 va) {
     if (va >= MAXVA)
       return 0; // 如果虚拟地址超出范围，返回0
     va = PGROUNDDOWN(va); // 将虚拟地址按页对齐
     pte_t *pte = walk(pagetable, va, 0); // 获取虚拟地址对应的页表项
     if (pte == 0)
       return 0; // 页表项不存在，返回0
     if ((*pte & PTE_V) == 0)
       return 0; // 页不在内存中，返回0
     if ((*pte & PTE_U) == 0)
       return 0; // 非用户模式的页，返回0
   
     if (*pte & PTE_COW)
       return 1; // 如果是写时复制页面，返回1
     return 0; // 否则返回0
   }
   ```

   ```c
   // vm.c
   // 分配一个新的页面并处理写时复制。分配新页面以供写请求，需要创建新映射并去除写时复制标志。
   int cow_alloc(pagetable_t pagetable, uint64 va) {
     va = PGROUNDDOWN(va); // 将虚拟地址按页对齐
     pte_t *pte = walk(pagetable, va, 0); // 获取虚拟地址对应的页表项
     uint64 pa = PTE2PA(*pte); // 获取物理地址
     int flag = PTE_FLAGS(*pte); // 获取页表项的标志位
   
     char *mem = kalloc(); // 分配内核内存用于复制页面数据
     if (mem == 0)
       return -1; // 分配失败，返回-1
     memmove(mem, (char*)pa, PGSIZE); // 复制页面数据到新的内存
   
     uvmunmap(pagetable, va, 1, 1); // 解除原映射关系，同时通知 TLB
   
     // 创建新映射，去除写时复制标志，并添加写标志
     flag &= ~(PTE_COW);
     flag |= PTE_W;
     if (mappages(pagetable, va, PGSIZE, (uint64)mem, flag) < 0) {
       kfree(mem); // 映射失败，释放内存
       return -1;
     }
     return 0;
   }
   ```

4. 修改 copyout()，在遇到 COW 页面时使用与页面错误相同的方案。

   ```c
   int
   copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
   {
    uint64 n, va0, pa0;
    while(len > 0){
     va0 = PGROUNDDOWN(dstva);
        
     // 添加的代码
     if(is_cow_fault(pagetable,va0)){
      if(cow_alloc(pagetable,va0)<0){
         return -1;
      }
     }
     // 添加的代码结束
        
     pa0 = walkaddr(pagetable, va0);
     if(pa0 == 0)
      return -1;
     n = PGSIZE - (dstva - va0);
     if(n > len)
      n = len;
     memmove((void *)(pa0 + (dstva - va0)), src, n);
     len -= n;
     src += n;
     dstva = va0 + PGSIZE;
    }
    return 0;
   }
   ```


#### 5.1.4 实验结果

​	![image-20230811020429263](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230811020429263.png)

​	![image-20230811020524982](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230811020524982.png)

​	![image-20230811020503947](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230811020503947.png)

​	![image-20230811161315702](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230811161315702.png)

#### 5.1.5 实验分析

1. 写时复制机制是什么？和传统机制有什么区别？

   ​		写时复制是一种内存管理技术，它允许在创建子进程时，父进程和子进程共享相同的物理内存页，直到其中一个进程试图对共享的内存进行写操作。当发生写操作时，才会真正分配新的物理内存页，并将数据复制到新页中，从而确保父进程和子进程之间的数据独立性。

   ​		在传统的进程复制机制中，当一个进程调用 fork() 创建一个子进程时，操作系统会将父进程的所有内存内容（包括数据、代码、堆栈等）完全复制到子进程中。这意味着子进程会获得与父进程相同的内存内容。这种复制方式会消耗大量的时间和内存资源，尤其是在父进程的内存较大时。

   ​		写时复制机制允许操作系统更有效地管理内存，减少不必要的内存复制开销，提高了进程创建和执行的效率。

1. 写时复制位的作用是什么？

   在父进程创建子进程时，子进程的写时复制位保留，以确保当发生写请求时可以执行写时复制。当复制过后，写时复制位清除。

1. 引用计数的作用是什么？

   准确确定释放物理页面的时机，以免出现内存泄漏。

1. 为什么在遇到 COW 页面时使用与页面错误相同的方案？

   为了防止进程写入一个COW页面（也就是未分配新内存，无法写入的页面），当分配了新的内存，清除了COW标志位，添加了可写标志位，才不会出现页面错误。

#### 5.1.6 实验总结

​		在本次实验中，我深入了解了操作系统内存管理的重要概念之一，即写时复制（Copy-on-Write，COW）机制。通过实现COW机制，学习了许多有关虚拟内存管理、页面错误处理、引用计数和物理内存分配的知识和方法。本实验的收获如下：

​		写时复制的概念： 我了解了写时复制机制的工作原理，即在父进程和子进程之间共享页面，并只有在需要修改页面内容时才会进行实际复制，从而节省内存和提高性能。

​		引用计数： 实验中我掌握了引用计数的概念，即通过跟踪物理页面的引用次数，确保只有在页面不再被任何进程使用时才释放它。

​		通过实验，我不仅学到了相关的理论知识，还深入了解了操作系统内核的实际开发和调试过程。这让我更好地理解了操作系统如何管理内存资源，实现虚拟内存机制以及提高系统性能。

​		这一实验也说明，实现节省内存开销、提高性能是操作系统的重要目标，这一实验完整体验了这一过程。





## Lab6 Multithreading

### 6.1 Uthread: switching between threads（线程切换）

#### 6.1.1 实验目的

​		在这个练习中，将设计一个用户级线程系统的上下文切换机制，并实现它。为了帮助入门， xv6 有两个文件 user/uthread.c 和 user/uthread_switch.S，以及一个 Makefile 规则来构建一个 uthread 程序。uthread.c 包含了大部分用户级线程库的代码，以及三个简单测试线程的代码。线程库缺少一些用于创建线程和在线程之间切换的代码。

​		本实验的任务是制定一个计划来创建线程，保存/恢复寄存器以实现线程之间的切换，并实现该计划。完成后，运行 make grade 应该显示你的解决方案通过了 uthread 测试。

​		完成后，当你在 xv6 上运行 uthread 时，你应该会看到以下输出（这三个线程可能会以不同的顺序开始）：

```bash
$ make qemu
...
$ uthread
thread_a started
thread_b started
thread_c started
thread_c 0
thread_a 0
thread_b 0
thread_c 1
thread_a 1
thread_b 1
...
thread_c 99
thread_a 99
thread_b 99
thread_c: exit after 100
thread_a: exit after 100
thread_b: exit after 100
thread_schedule: no runnable threads
$
```

​		这个输出来自三个测试线程，每个线程都有一个循环，打印一行文字，然后将 CPU 让给其他线程。

​		然而，在没有上下文切换代码的情况下，你将看不到任何输出。

​		你需要在 user/uthread.c 的 thread_create() 和 thread_schedule() 中，以及在 user/uthread_switch.S 的 thread_switch 中添加代码。一个目标是确保当 thread_schedule() 第一次运行给定的线程时，该线程在其自己的栈上执行传递给 thread_create() 的函数。另一个目标是确保 thread_switch 保存被切换出的线程的寄存器，恢复被切换到的线程的寄存器，并返回到后者线程的指令中上次离开的地方。你需要决定在哪里保存/恢复寄存器；修改 struct thread 以保存寄存器是一个不错的计划。你需要在 thread_schedule 中添加一个调用 thread_switch 的语句；你可以传递 thread_switch 需要的任何参数，但意图是从线程 t 切换到 next_thread。

#### 6.1.2 实验提示

- thread_switch 只需要保存/恢复被调用者保存的寄存器。为什么？ 

  ​		`thread_switch` 只需要保存和恢复被调用者保存的寄存器，而不需要保存和恢复调用者保存的寄存器，是因为在线程切换的过程中，上下文切换代码是运行在被调用者（即当前线程）的上下文中的。

  在汇编语言中，一般将寄存器分为两类：被调用者保存的寄存器和调用者保存的寄存器。

  - **被调用者保存的寄存器（Callee-Saved Registers）**：这些寄存器的值在函数调用过程中会被被调用者保存，并在函数返回时恢复。被调用者保存的寄存器通常用于存储临时的局部变量和其他重要的上下文信息，以便在函数调用结束后可以恢复到调用前的状态。在线程切换过程中，切换到下一个线程时，会将被调用者保存的寄存器的值从线程上下文中恢复，确保下一个线程可以继续运行。
  - **调用者保存的寄存器（Caller-Saved Registers）**：这些寄存器的值在函数调用过程中可能会被改变，但通常由调用者来保存和恢复。调用者保存的寄存器用于存储临时数据，在函数调用时需要由调用者自己来保存寄存器的值，以便在函数调用结束后恢复。在线程切换的过程中，切换到下一个线程时，这些寄存器的值不需要保存和恢复，因为它们的状态由调用者负责维护。

  在 `thread_switch` 函数中，只有被调用者保存的寄存器的值需要被保存和恢复，因为切换线程时，切换代码是在当前线程的上下文中运行的。调用者保存的寄存器的值不会在切换代码执行期间被改变，因此不需要保存和恢复。

  ​		这种设计可以提高上下文切换的效率，因为只需要保存和恢复少量的寄存器。这样，在线程切换时的开销较小，从而提高了多线程编程的性能。

- 你可以在 user/uthread.asm 中看到 uthread 的汇编代码，这可能有助于调试。 

- 为了测试你的代码，通过 riscv64-linux-gnu-gdb 逐步执行 thread_switch 可能会有所帮助。你可以这样开始：

  ```bash
  (gdb) file user/_uthread
  Reading symbols from user/_uthread...
  (gdb) b uthread.c:60
  ```

  这在 uthread.c 的第 60 行设置了一个断点。这个断点可能在你运行 uthread 之前触发，这是怎么发生的呢？

  一旦你的 xv6 shell 运行起来，输入 "uthread"，gdb 就会在第 60 行断下。现在你可以输入像下面这样的命令来检查 uthread 的状态：

  ```bash
  (gdb) p/x *next_thread
  ```

  使用 "x"，你可以检查一个内存位置的内容：

  ```bash
  (gdb) x/x next_thread->stack
  ```

  你可以跳到 thread_switch 的开始部分：

  ```bash
  (gdb) b thread_switch
  (gdb) c
  ```

  你可以使用如下命令逐步执行汇编指令：

  ```bash
  (gdb) si
  ```

#### 6.1.3 实验步骤

1. 在uthread_switch.S 中实现上下文切换的代码。

   ```assembly
   // uthread_switch.S
   	.text
   
   /*
   
   save the old thread's registers,
   
   restore the new thread's registers.
   */
   
   // void thread_switch(struct context *old, struct context *new);
   	.globl thread_switch
   thread_switch:
   	sd ra, 0(a0)
   	sd sp, 8(a0)
   	sd s0, 16(a0)
   	sd s1, 24(a0)
   	sd s2, 32(a0)
   	sd s3, 40(a0)
   	sd s4, 48(a0)
   	sd s5, 56(a0)
   	sd s6, 64(a0)
   	sd s7, 72(a0)
   	sd s8, 80(a0)
   	sd s9, 88(a0)
   	sd s10, 96(a0)
   	sd s11, 104(a0)
   
   	ld ra, 0(a1)
   	ld sp, 8(a1)
   	ld s0, 16(a1)
   	ld s1, 24(a1)
   	ld s2, 32(a1)
   	ld s3, 40(a1)
   	ld s4, 48(a1)
   	ld s5, 56(a1)
   	ld s6, 64(a1)
   	ld s7, 72(a1)
   	ld s8, 80(a1)
   	ld s9, 88(a1)
   	ld s10, 96(a1)
   	ld s11, 104(a1)
   
   ret    /* return to ra */
   ```

2. 从 proc.h 中借鉴一下 context 结构体，用于保存 ra、sp 以及 callee-saved registers：

   ```c
   struct context {
    uint64 ra;
    uint64 sp;
    // callee-saved
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
   };
   
   struct thread {
    char    stack[STACK_SIZE]; /* the thread's stack */
    int     state;       /* FREE, RUNNING, RUNNABLE */
    struct context ctx; // 在 thread 中添加 context 结构体
   };
   struct thread all_thread[MAX_THREAD];
   struct thread *current_thread;
   
   extern void thread_switch(struct context* old, struct context* new); // 修改 thread_switch 函数声明
   ```

3. 在 thread_schedule 中调用 thread_switch 进行上下文切换：

   ```c
    if (current_thread != next_thread) {     /* switch threads?  */
     next_thread->state = RUNNING;
     t = current_thread;
     current_thread = next_thread;
     /* YOUR CODE HERE
      * Invoke thread_switch to switch from t to next_thread:
      * thread_switch(??, ??);
      */
     thread_switch(&t->ctx, &next_thread->ctx); // 切换线程
    } 
    else
     next_thread = 0;
   ```

4. 再补齐 thread_create：

   ```c
   void 
   thread_create(void (*func)())
   {
    struct thread *t;
    for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
     if (t->state == FREE) break;
    }
    t->state = RUNNABLE;
    // YOUR CODE HERE
     t->ctx.ra = (uint64)func;    // 返回地址
    // thread_switch 的结尾会返回到 ra，从而运行线程代码
    t->ctx.sp = (uint64)&t->stack + (STACK_SIZE - 1);  // 栈指针
    // 将线程的栈指针指向其独立的栈，注意到栈的生长是从高地址到低地址，所以
    // 要将 sp 设置为指向 stack 的最高地址
   }
   ```

#### 6.1.4 实验结果

​	![image-20230812225149249](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230812225149249.png)

​	![image-20230812225245607](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230812225245607.png)

#### 6.1.5 实验分析

1. 本实验模拟的过程是什么？

   ​		用户级线程上下文切换是指在用户空间中切换正在执行的线程。在用户级线程中，操作系统并不直接参与线程的管理和切换，而是由用户程序或用户级线程库来负责。

   ​		上下文切换是在一个线程需要暂停执行，并且切换到另一个线程执行时发生的过程。在上下文切换期间，必须保存当前线程的状态（例如寄存器值、程序计数器等），然后加载下一个线程的状态，使其能够继续执行。这个过程通常涉及保存和恢复寄存器值、堆栈指针以及其他与线程执行相关的信息。

2. thread_switch.S的作用是什么？

   ​		`uthread_switch.S` 文件的作用是实现用户级线程之间的上下文切换。在用户级线程中，操作系统不直接管理线程的切换，而是由用户级线程库来负责。`uthread_switch.S` 中的汇编代码定义了一个用于线程切换的汇编函数 `thread_switch`，它在切换线程时保存当前线程的状态并恢复下一个线程的状态。

3. 本实验的代码思路是什么？

   这个实验其实相当于在用户态重新实现一遍 xv6 kernel 中的 scheduler() 和 swtch() 的功能，所以大多数代码都是可以借鉴的。

#### 6.1.6 实验总结

通过这个实验，可以获得以下收获和心得：

- 理解了用户级线程的概念，以及如何通过用户态代码来实现线程的创建、调度和上下文切换。
- 加深了对多线程编程中上下文切换机制的理解，明白了寄存器状态的保存和恢复在线程切换中的重要性。



### 6.2 Using threads（使用线程）

#### 6.2.1 实验目的

​		在这个任务中，你将使用线程和锁探索并行编程，使用哈希表。应该在一台拥有多个核心的真实Linux或MacOS计算机上完成此任务（不是xv6，不是qemu）。大多数最新的笔记本电脑都配备了多核处理器。

​		文件notxv6/ph.c包含了一个简单的哈希表，如果从单个线程中使用是正确的，但在多个线程中使用时是不正确的。在你的主要xv6目录（可能是~/xv6-labs-2021），输入以下命令：

```assembly
$ make ph
$ ./ph 1
```

​		需要注意的是，为了构建ph，Makefile使用的是你的操作系统的gcc，而不是6.S081工具。ph的参数指定执行对哈希表进行put和get操作的线程数。运行一段时间后，ph 1将会产生类似于以下的输出： 100000 个 puts，用时 3.991 秒，每秒 25056 个 puts 0: 0 个键丢失 100000 个 gets，用时 3.981 秒，每秒 25118 个 gets 你看到的数字可能与这个示例输出有两倍或更多的差异，这取决于你的计算机速度有多快，是否有多个核心以及是否正在执行其他任务。

​		ph运行了两个基准测试。首先，通过调用put()向哈希表添加大量键，并打印每秒实现的插入速率。然后，它使用get()从哈希表中获取键。它打印应该作为put操作结果而应该在哈希表中的键的数量（在这种情况下为零），并且打印实现的每秒获取次数。

​		你可以通过给ph传递大于1的参数来告诉它在多个线程中同时使用其哈希表。尝试ph 2：

```assembly
$ ./ph 2
100000 puts, 1.885 seconds, 53044 puts/second
1: 16579 keys missing
0: 16579 keys missing
200000 gets, 4.322 seconds, 46274 gets/second
```

​		这个ph 2输出的第一行表示，当两个线程同时向哈希表中添加条目时，它们总共实现了每秒53044个插入的速率。这大约是单个线程从运行ph 1中获得的速率的两倍。这是一个优秀的“并行加速”，大约是2倍，正如人们可能希望的那样（即每个单位时间内有两倍的核心产生两倍的工作量）。 然而，显示16579个丢失键的两行表明，许多应该在哈希表中的键确实不在那里。也就是说，这些键应该由put操作添加到哈希表中，但出现了问题。查看notxv6/ph.c，特别是put()和insert()部分。

​		为什么在2个线程的情况下会丢失键，但在1个线程的情况下不会？在2个线程中识别出一个可能导致键丢失的事件序列。将你的序列连同简要说明提交在answers-thread.txt文件中。 

​		为了避免这种事件序列，向notxv6/ph.c的put和get中插入锁定和解锁语句，以便在两个线程的情况下丢失的键总是为0。相关的pthread调用如下：

```assembly
pthread_mutex_t lock;            // declare a lock
pthread_mutex_init(&lock, NULL); // initialize the lock
pthread_mutex_lock(&lock);       // acquire lock
pthread_mutex_unlock(&lock);     // release lock
```

​		当make grade命令显示你的代码通过了ph_safe测试时，你就完成了任务，这要求两个线程时没有丢失的键。此时不通过ph_fast测试也是可以的。

​		不要忘记调用pthread_mutex_init()。首先使用1个线程测试你的代码，然后使用2个线程测试。它是否正确（即是否消除了丢失的键）？双线程版本是否实现了并行加速（即每个单位时间内的总工作量是否增加）？

​		有些情况下，并发的put()在它们读写哈希表中没有重叠的内存，因此不需要锁来保护彼此。你能否更改ph.c以利用这些情况来为某些put()获得并行加速？提示：每个哈希桶使用一个锁如何？

​		修改你的代码，使得一些put操作在保持正确性的同时可以并行运行。当make grade命令显示你的代码通过了ph_safe和ph_fast测试时，你就完成了任务。ph_fast测试要求两个线程的插入速率至少是一个线程的1.25倍。

#### 6.2.2 实验提示

​		本实验无提示。

#### 6.2.3 实验步骤

1. 构建 `ph` 程序

   ​	![image-20230813102400772](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230813102400772.png)

2. 执行单线程

3. 执行多线程后观察发现键值对缺失

4. 加锁（notxv6/ph.c）

   ```c
   // 加锁
   pthread_mutex_t locks[NBUCKET];
   ```

   ```c
   static 
   void put(int key, int value)
   {
    int i = key % NBUCKET;
   
    // 上锁
    pthread_mutex_lock(&locks[i]);
   
    // is the key already present?
    struct entry *e = 0;
    for (e = table[i]; e != 0; e = e->next) {
     if (e->key == key)
      break;
    }
    if(e){
     // update the existing key.
     e->value = value;
    } else {
     // the new is new.
     insert(key, value, &table[i], table[i]);
    }
   
    // 解锁
    pthread_mutex_unlock(&locks[i]);
   
   
   ```

5. 多线程执行查看结果

   

   

​	

#### 6.2.4 实验结果

#### 6.2.5 实验分析

1. 单线程执行时的输出是什么意思？
2. 当执行单线程时，不会出现键值对丢失，为什么两个线程执行时，会出现键值对丢失？
3. 本实验的最重要步骤是什么？
4. 为什么本实验的 get 操作不用加锁？

- **100000 puts, 6.153 seconds, 16252 puts/second**： 这一部分表示在单线程环境下，程序执行了 100,000 次插入（put）操作。它花费了大约 6.153 秒的时间，平均每秒执行了 16,252 次插入操作。
- **0: 0 keys missing**： 在单线程情况下，没有发生任何键丢失（missing keys）。这意味着所有的插入操作都成功地将键-值对添加到哈希表中。
- **100000 gets, 6.500 seconds, 15386 gets/second**： 在单线程环境下，程序执行了 100,000 次获取（get）操作。它花费了大约 6.500 秒的时间，平均每秒执行了 15,386 次获取操作。



​		出现键值对丢失的问题是由于多线程并发访问共享数据结构（在这个实验中是哈希表）时引发的并发问题。让我解释一下为什么会出现这个问题：

​		在单线程情况下，代码按照预期的顺序执行。插入操作（put）一个接一个地被执行，没有其他线程同时在操作哈希表。这种情况下，没有竞争条件，每个插入操作都能正确地将键-值对添加到哈希表中。

​		然而，在多线程情况下，两个线程（或更多）可以同时执行插入操作。这时，就可能发生以下情况：

1. **竞争条件**：两个线程同时试图往哈希表中插入键-值对，由于没有同步机制，它们可能会同时访问并修改哈希表中的同一个位置，导致数据覆盖或者其他不一致性问题。
2. **数据竞争**：多个线程同时访问共享数据（即哈希表），并且至少有一个线程在写入数据。在没有适当同步的情况下，这可能导致未定义的行为，包括数据的不一致性和丢失。
3. **临界区问题**：在代码中没有使用互斥锁等同步机制的情况下，多个线程可能会同时进入关键的临界区，导致数据不一致。

​		这些问题可能导致在多线程环境下，一些插入操作无法正确地将键-值对添加到哈希表中，从而造成键值对丢失。



​		实现互斥锁，其中，由于对不同的桶进行插入操作互不影响，所以在对同一个桶进行插入是需要进行互斥，所以可以设置一个锁组，一个锁对应一个桶。



​		`get()` 函数主要是遍历 bucket 链表找寻对应的 `entry`, 并不会对 bucket 链表进行修改, 实际上只是读操作, 因此无需加锁。



#### 6.2.6 实验总结

### 6.3 Barrier



## Lab7 networking





## Lab8 locks

### 8.1 Memory allocator

### 8.2 Buffer cache





## Lab9 file system

### 9.1 Large files（大文件）

#### 9.1.1 实验目的

​		在这个作业中，将增加 xv6 文件的最大大小。目前 xv6 文件被限制为 268 个块，即268*BSIZE 字节（在 xv6 中，BSIZE 为 1024）。这个限制来自于 xv6 inode 包含 12 个“直接”块号和一个“单间接”块号，它引用一个包含最多 256 个更多块号的块，总共为 12+256=268 个块。

​		bigfile 命令创建最长的文件，并报告该大小：

```bash
$ bigfile 
..
wrote 268 blocks 
bigfile: file is too small 
$ 
```

​		测试失败是因为 bigfile 期望能够创建一个由 65803 个块组成的文件，但未修改的 xv6 限制文件为 268 个块。 您将更改 xv6 文件系统代码，以支持每个 inode 中的“双间接”块，其中包含 256 个单间接块的地址，每个单间接块可以包含最多 256 个数据块的地址。结果将是一个文件能够由最多 65803 个块组成，或者是 256*256+256+11 个块（11 代替 12，因为我们将为双间接块牺牲一个直接块号）。

- 准备工作 

​		mkfs 程序创建 xv6 文件系统磁盘映像，并确定文件系统拥有多少总块；这个大小由 kernel/param.h 中的 FSSIZE 控制。您将会看到此实验室存储库中的 FSSIZE 被设置为 200,000 个块。您应该从 make 输出中的 mkfs/mkfs 看到以下输出：

```bash
nmeta 70 (boot, super, log blocks 30 inode blocks 13, bitmap blocks 25) blocks 199930 total 200000
```

​		此行描述了 mkfs/mkfs 构建的文件系统：它有 70 个元数据块（用于描述文件系统的块）和 199,930 个数据块，总共 200,000 个块。 如果在实验过程中您发现自己不得不从头开始重新构建文件系统，可以运行 make clean，它会强制 make 重新构建 fs.img。

- 需要查看的内容

​		磁盘上 inode 的格式由 fs.h 中的 struct dinode 定义。您尤其关心 NDIRECT、NINDIRECT、MAXFILE 以及 struct dinode 的 addrs[] 元素。在 xv6 文本的图 8.3 中查看标准 xv6 inode 的图示。 

![image-20230811231100054](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230811231100054.png)

​		在 fs.c 中，查找文件在磁盘上的数据的代码位于 bmap()。查看一下它，确保您理解它在做什么。在读取和写入文件时，都会调用 bmap()。在写入时，bmap() 根据需要分配新的块来容纳文件内容，还会分配一个间接块（如果需要）来容纳块地址。

​		bmap() 处理两种类型的块号。bn 参数是“逻辑块号”——相对于文件开头，在文件内的块号。ip->addrs[] 中的块号以及传递给 bread() 的参数是磁盘块号。您可以将 bmap() 视为将文件的逻辑块号映射到磁盘块号。

- 实验任务

​		修改 bmap()，以实现双间接块，除了直接块和单间接块。为了给新的双间接块腾出空间，您只能有 11 个直接块，而不是 12 个；您不允许改变磁盘上 inode 的大小。ip->addrs[] 的前 11 个元素应该是直接块；第 12 个应该是单间接块（就像当前的一样）；第 13 个应该是您的新的双间接块。当 bigfile 写入 65803 个块并且 usertests 成功运行时，您完成了这个练习：

```bash
$ bigfile
..................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................
wrote 65803 blocks
done; ok
$ usertests
...
ALL TESTS PASSED
$ 
```

#### 9.1.2 实验提示

- 确保您理解 bmap()。绘制出 ip->addrs[]、间接块、双间接块以及它所指向的单间接块和数据块之间的关系的图表。确保您理解为什么添加双间接块会使最大文件大小增加 256*256 个块（实际上是 -1，因为您必须减少一个直接块的数量）。 
- 考虑一下如何使用逻辑块号索引双间接块，以及它所指向的单间接块。 
- 如果更改 NDIRECT 的定义，您可能需要更改 file.h 中 struct inode 的 addrs[] 声明。确保 struct inode 和 struct dinode 在它们的 addrs[] 数组中具有相同数量的元素。 
- 如果更改 NDIRECT 的定义，请确保创建一个新的 fs.img，因为 mkfs 使用 NDIRECT 来构建文件系统。 
- 如果您的文件系统陷入糟糕的状态，可能是崩溃引起的，删除 fs.img（从 Unix 而不是 xv6 中执行此操作）。make 将为您构建一个新的干净的文件系统映像。 
- 不要忘记对您读取的每个块执行 brelse()。
- 您应该像原始的 bmap() 一样，只在需要时分配间接块和双间接块。 
- 确保 itrunc 释放文件的所有块，包括双间接块。

#### 9.1.3 实验步骤

修改 kernel/fs.h 中 dinode 结构体的结构，使之支持二级索引的盘块结构，同时kernel/file.h的inode结构体同样需要修改：

```c
#define NDIRECT 11  // 直接索引盘块数减一，为二级索引盘块让出一个块
#define NINDIRECT (BSIZE / sizeof(uint))   // 单间接索引块数量
#define MAXFILE (NDIRECT + NINDIRECT + NINDIRECT * NINDIRECT);
// On-disk inode structure
struct dinode {
 short type;             // File type
 short major;            // Major device number (T_DEVICE only)
 short minor;            // Minor device number (T_DEVICE only)
 short nlink;            // Number of links to inode in file system
 uint size;              // Size of file (bytes)
 uint addrs[NDIRECT+2];  // Data block addresses
};
```

```c
struct inode {
  uint dev;                // 设备号，表示文件所在的设备
  uint inum;               // inode 编号，唯一标识一个 inode
  int ref;                 // 引用计数，表示当前有多少个指针指向这个 inode
  struct sleeplock lock;   // 睡眠锁，用于保护以下内容
  int valid;               // 表示这个 inode 是否已经从磁盘读取过

  short type;              // 文件类型，如普通文件、目录等
  short major;             // 对于设备文件，表示主设备号
  short minor;             // 对于设备文件，表示次设备号
  short nlink;             // 链接数，表示有多少个目录项链接到这个 inode
  uint size;               // 文件大小，以字节为单位
  uint addrs[NDIRECT+2];   // 存储数据块地址的数组，可以存储直接块、单间接块、双间接块的地址
};
```

修改bmap函数（kernel/fs.c），使之接受二级索引转换：

```c
static uint
bmap(struct inode *ip, uint bn)
{
  uint addr, *a;
  struct buf *bp;

  if(bn < NDIRECT) {
    // 逻辑块号在直接块范围内
    if((addr = ip->addrs[bn]) == 0)
      ip->addrs[bn] = addr = balloc(ip->dev); // 如果对应的数据块不存在，则分配一个
    return addr;
  }
  bn -= NDIRECT; // 减去直接块的数量，得到剩余逻辑块号

  if(bn < NINDIRECT) {
    // 逻辑块号在单间接块范围内
    // 加载单间接块，必要时分配
    if((addr = ip->addrs[NDIRECT]) == 0)
      ip->addrs[NDIRECT] = addr = balloc(ip->dev); // 如果单间接块不存在，则分配一个
    bp = bread(ip->dev, addr); // 读取单间接块的内容
    a = (uint*)bp->data; // 将单间接块的内容转换为整数数组
    if((addr = a[bn]) == 0){
      a[bn] = addr = balloc(ip->dev); // 如果对应的数据块不存在，则分配一个
      log_write(bp); // 将修改后的单间接块写回磁盘
    }
    brelse(bp); // 释放单间接块的缓存
    return addr;
  }
  bn -= NINDIRECT; // 减去单间接块的数量，得到剩余逻辑块号

  if(bn < NINDIRECT * NINDIRECT) {
    // 逻辑块号在双间接块范围内（新增的部分）
    // 加载双间接块，必要时分配
    if((addr = ip->addrs[NDIRECT+1]) == 0)
      ip->addrs[NDIRECT+1] = addr = balloc(ip->dev); // 如果双间接块不存在，则分配一个
    bp = bread(ip->dev, addr); // 读取双间接块的内容
    a = (uint*)bp->data; // 将双间接块的内容转换为整数数组
    if((addr = a[bn/NINDIRECT]) == 0){
      a[bn/NINDIRECT] = addr = balloc(ip->dev); // 如果对应的单间接块不存在，则分配一个
      log_write(bp); // 将修改后的双间接块写回磁盘
    }
    brelse(bp); // 释放双间接块的缓存
    bn %= NINDIRECT; // 取余得到在单间接块中的索引
    bp = bread(ip->dev, addr); // 加载对应的单间接块
    a = (uint*)bp->data; // 将单间接块的内容转换为整数数组
    if((addr = a[bn]) == 0){
      a[bn] = addr = balloc(ip->dev); // 如果对应的数据块不存在，则分配一个
      log_write(bp); // 将修改后的单间接块写回磁盘
    }
    brelse(bp); // 释放单间接块的缓存
    return addr;
  }

  panic("bmap: out of range"); // 超出支持的逻辑块号范围，触发内核恐慌
}
```

修改 itrunc 函数（kernel/fs.c），使之接受双间接块的文件清空：

```c
void
itrunc(struct inode *ip)
{
  int i, j;
  struct buf *bp;
  uint *a;

  // 释放直接块
  for(i = 0; i < NDIRECT; i++) {
    if(ip->addrs[i]){
      bfree(ip->dev, ip->addrs[i]); // 释放直接块
      ip->addrs[i] = 0; // 将 inode 的直接块地址清零
    }
  }

  // 释放单间接块
  if(ip->addrs[NDIRECT]){
    bp = bread(ip->dev, ip->addrs[NDIRECT]); // 读取单间接块
    a = (uint*)bp->data; // 转换为整数数组
    for(j = 0; j < NINDIRECT; j++) {
      if(a[j])
        bfree(ip->dev, a[j]); // 释放单间接块中的数据块
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[NDIRECT]); // 释放单间接块
    ip->addrs[NDIRECT] = 0; // 将 inode 的单间接块地址清零
  }

  // 释放双间接块
  if(ip->addrs[NDIRECT+1]){
    bp = bread(ip->dev, ip->addrs[NDIRECT+1]); // 读取双间接块
    a = (uint*)bp->data; // 转换为整数数组
    for(j = 0; j < NINDIRECT; j++) {
      if(a[j]) {
        struct buf *bp2 = bread(ip->dev, a[j]); // 读取单间接块
        uint *a2 = (uint*)bp2->data; // 转换为整数数组
        for(int k = 0; k < NINDIRECT; k++) {
          if(a2[k])
            bfree(ip->dev, a2[k]); // 释放单间接块中的数据块
        }
        brelse(bp2);
        bfree(ip->dev, a[j]); // 释放单间接块
      }
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[NDIRECT+1]); // 释放双间接块
    ip->addrs[NDIRECT+1] = 0; // 将 inode 的双间接块地址清零
  }

  // 更新文件大小为 0，然后更新元数据
  ip->size = 0;
  iupdate(ip);
}
```

#### 9.1.4 实验结果

​	![image-20230812001150416](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230812001150416.png)

#### 9.1.5 实验分析

- 本实验中的“双间接块”是什么？

​		即为二级索引块，原来的xv6文件系统最多包含一级索引块，而修改后的系统包含二级索引块，所以文件包含的最大块数会增加。

#### 9.1.6 实验总结

​		本实验总体偏简单，主要任务即修改文件系统的物理块组成结构，引入二级索引，扩充了文件的最大块数。

​		同时要注意实验的严谨性，对于文件的创建以及回收块等都要保证满足新的文件结构，以确保系统的完整性。



### 9.2 Symbolic links（符号链接）

#### 9.2.1 实验目的

​		在这个练习中，将会给 xv6 添加符号链接（symbolic links）。符号链接（或软链接）通过路径名来引用一个链接的文件；当打开一个符号链接时，内核会根据链接跟踪到所引用的文件。符号链接类似于硬链接，但是硬链接仅限于指向同一磁盘上的文件，而符号链接可以跨越磁盘设备。尽管 xv6 并不支持多个设备，实现这个系统调用是一个理解路径名查找工作原理的好练习。

​		本实验将实现 `symlink(char *target, char *path)` 系统调用，该调用会在给定路径 `path` 创建一个新的符号链接，该链接引用了由 `target` 命名的文件。详细信息请参见 `man` 页的 `symlink` 部分。为了测试，将 `symlinktest` 添加到 `Makefile` 并运行它。当测试产生以下输出时（包括 `usertests` 成功通过）表示解决方案是完整的：

```bash
$ symlinktest
Start: test symlinks
test symlinks: ok
Start: test concurrent symlinks
test concurrent symlinks: ok
$ usertests
...
ALL TESTS PASSED
$ 
```

#### 9.2.2 实验提示

1. 首先，为 `symlink` 创建一个新的系统调用编号，将其添加到 `user/usys.pl`、`user/user.h` 中，并在 `kernel/sysfile.c` 中实现一个空的 `sys_symlink` 函数。
2. 在 `kernel/stat.h` 中添加一个新的文件类型（`T_SYMLINK`）来表示符号链接。
3. 在 `kernel/fcntl.h` 中添加一个新的标志（`O_NOFOLLOW`），该标志可以与 `open` 系统调用一起使用。请注意，传递给 `open` 的标志使用按位或运算符进行组合，因此你的新标志不应与任何现有标志重叠。这将允许你在将其添加到 `Makefile` 后，编译 `user/symlinktest.c`。
4. 实现 `symlink(target, path)` 系统调用，以在给定路径 `path` 创建一个新的符号链接，该链接引用了 `target`。请注意，为了使系统调用成功，`target` 不需要存在。你需要选择一个地方来存储符号链接的目标路径，例如在 inode 的数据块中。`symlink` 应该返回表示成功（0）或失败（-1）的整数，类似于 `link` 和 `unlink`。
5. 修改 `open` 系统调用，以处理路径引用到符号链接的情况。如果文件不存在，`open` 必须失败。当进程在 `open` 的标志中指定了 `O_NOFOLLOW` 时，`open` 应该打开符号链接（而不是跟踪符号链接）。
6. 如果链接的文件本身也是一个符号链接，你必须递归地跟踪它，直到达到非链接文件。如果链接形成一个循环，你必须返回一个错误代码。你可以通过在链接深度达到某个阈值时（例如，10）返回错误代码来近似处理这个情况。
7. 其他系统调用（例如 `link` 和 `unlink`）不得跟随符号链接；这些系统调用操作的是符号链接本身。
8. 对于这个实验，不必处理指向目录的符号链接。

#### 9.2.3 实验步骤

1. 首先实现 symlink 系统调用，用于创建符号链接。 符号链接与普通的文件一样，需要占用 inode 块。这里使用 inode 中的第一个 direct-mapped 块（1024字节）来存储符号链接指向的文件。

   ```c
   uint64
   sys_symlink(void)
   {
    struct inode *ip;                     // 用于存储新符号链接的 inode 结构指针
    char target[MAXPATH], path[MAXPATH];  // 用于存储从用户空间传递过来的目标和路径字符串
    
    // 从用户空间获取目标路径和链接路径的字符串
    if(argstr(0, target, MAXPATH) < 0 || argstr(1, path, MAXPATH) < 0)  // 不合法返回错误信息
     return -1;
    // 开始文件系统操作
    begin_op();
   
    // 调用 create 函数创建一个新的符号链接文件，类型为 T_SYMLINK
    ip = create(path, T_SYMLINK, 0, 0);
    if(ip == 0){
     end_op();
     return -1;
    }
   
    // 将目标路径写入符号链接的数据块中
    if(writei(ip, 0, (uint64)target, 0, strlen(target)) < 0) {
     end_op();
     return -1;
    }
   
    // 解锁并释放 inode
    iunlockput(ip);
   
    // 结束文件系统操作
    end_op();
    return 0;
   }
   ```

2. 在fcntl.h下补全定义

   ```c
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
   ```

3. 修改sys_open，在遇到符号链接的时候，可以递归跟随符号链接，直到跟随到非符号链接的 inode 为止。

   ```C
   // sys_open 系统调用，用于打开文件并返回文件描述符。
   uint64
   sys_open(void)
   {
     char path[MAXPATH];   // 用于存储传递的文件路径
     int fd, omode;        // fd 用于存储文件描述符，omode 用于存储打开模式
     struct file *f;       // 文件结构指针，表示将被打开的文件
     struct inode *ip;     // inode 结构指针，表示被打开文件的 inode
     int n;
   
     if((n = argstr(0, path, MAXPATH)) < 0 || argint(1, &omode) < 0)
       return -1;
   
     begin_op();  // 开始文件系统操作
   
     if(omode & O_CREATE){  // 如果指定了创建标志
       ip = create(path, T_FILE, 0, 0);  // 创建文件的 inode
       if(ip == 0){
         end_op();
         return -1;
       }
     } else {
       int symlink_depth = 0;  // 符号链接深度，用于检测循环符号链接
       while(1) { // 递归跟随符号链接
         if((ip = namei(path)) == 0){  // 根据路径查找 inode
           end_op();
           return -1;
         }
         ilock(ip);  // 锁定 inode
         if(ip->type == T_SYMLINK && (omode & O_NOFOLLOW) == 0) {
           if(++symlink_depth > 10) {
             // 太多层的符号链接，可能是一个循环
             iunlockput(ip);
             end_op();
             return -1;
           }
           if(readi(ip, 0, (uint64)path, 0, MAXPATH) < 0) {
             iunlockput(ip);
             end_op();
             return -1;
           }
           iunlockput(ip);  // 解锁 inode
         } else {
           break;
         }
       }
       if(ip->type == T_DIR && omode != O_RDONLY){
         iunlockput(ip);
         end_op();
         return -1;
       }
     }
   
     // ...... 其他操作
   
     iunlock(ip);  // 解锁 inode
     end_op();     // 结束文件系统操作
   
     return fd;    // 返回文件描述符
   }
   ```

#### 9.2.4 实验结果

#### 9.2.5 实验分析

1. 符号链接是什么？作用是什么？

   ​		符号链接（Symbolic Link），也称为软链接（Soft Link），是操作系统中的一种特殊文件类型。它是一个包含了对另一个文件或目录的路径引用的文件。类似于Windows中的快捷方式。与硬链接不同，符号链接并不直接指向文件的数据内容，而是包含了目标文件的路径信息。

   ​		符号链接可以用于为文件或目录创建别名，使得它们可以通过不同的路径访问。这对于文件系统的组织和管理非常有用。例如，你可以在不移动文件的情况下，为一个文件创建多个不同的访问路径，以提供更方便的访问方式。

2. 软链接和硬链接的区别是什么？

   ​		硬链接与符号链接之间的主要区别在于它们的工作原理。硬链接是指多个文件名链接到同一物理数据块，因此更像是多个文件名指向同一个文件。而符号链接则是一个指向文件的路径，可以链接到不同设备上的文件。此外，硬链接只能链接到文件，而符号链接可以链接到文件或目录。

   见下图：

   ![image-20230812153859543](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20230812153859543.png)

3. 为什么需要处理循环符号链接？

   ​		循环符号链接指的是一系列符号链接形成的环路，导致无限递归地跟随链接会造成系统崩溃或无限循环。在文件系统实现中，需要限制符号链接的递归深度，以避免出现循环符号链接。

#### 9.1.6 实验总结

​		这个实验涉及实现符号链接（软链接）功能，以及在文件系统中处理路径查找、文件操作和链接的问题。通过完成这个实验，学到了很多关于文件系统和操作系统的知识和方法，以下是一些主要的收获：

1. **文件系统的组成和设计：** 通过实现符号链接，深入了解了文件系统的基本组成部分，包括 inode、文件描述符、路径查找和链接等。我了解了如何在文件系统中存储和管理文件的元数据和数据块。
2. **符号链接的实现：** 学会了如何在文件系统中实现符号链接功能。这涉及到创建符号链接、递归地跟随链接、处理循环链接、存储链接信息等。同时掌握了如何在文件系统中模拟路径引用，并处理链接类型文件的特殊情况。
3. **路径查找和文件操作：** 在实现符号链接的过程中，深入了解了如何根据给定的路径查找文件的 inode。我还学习了如何打开、读取和写入文件，以及如何处理文件的元数据和数据。



## Lab10 mmap
