//
// Console input and output, to the uart.
// Reads are line at a time.
// Implements special input characters:
//   newline -- end of line
//   control-h -- backspace
//   control-u -- kill line
//   control-d -- end of file
//   control-p -- print process list
//

//
//   控制台输入和输出，到uart。
//   每次读取一行。
//   实现特殊输入字符：
//   newline—行尾
//   control-h—退格
//   control-u—压井管线
//   control-d—文件结尾
//   control-p—打印进程列表
//

#include <stdarg.h>     // 包含标准头文件 <stdarg.h>，用于实现可变参数函数
#include "types.h"      // 包含自定义头文件 "types.h"，定义了一些常用数据类型
#include "param.h"      // 包含自定义头文件 "param.h"，定义了系统的一些参数和常量
#include "spinlock.h"   // 包含自定义头文件 "spinlock.h"，用于实现自旋锁
#include "sleeplock.h"  // 包含自定义头文件 "sleeplock.h"，用于实现睡眠锁
#include "fs.h"         // 包含自定义头文件 "fs.h"，用于实现文件系统相关功能
#include "file.h"       // 包含自定义头文件 "file.h"，用于文件操作相关功能
#include "memlayout.h"  // 包含自定义头文件 "memlayout.h"，定义了内存布局相关信息
#include "riscv.h"      // 包含自定义头文件 "riscv.h"，包含 RISC-V 汇编相关功能
#include "defs.h"       // 包含自定义头文件 "defs.h"，定义了一些宏和全局变量
#include "proc.h"       // 包含自定义头文件 "proc.h"，用于进程管理相关功能

// 定义了一个常量 BACKSPACE，表示退格键的 ASCII 码
#define BACKSPACE 0x100  
// 定义了一个宏 C(x)，用于将字符 x 转换为控制字符
#define C(x)  ((x)-'@')  


// 将一个字符发送到 UART（串行通信接口）。
// 该函数被 printf 调用，用于输出字符到终端；
// 也被用于回显输入字符，但不会被 write() 调用。
void
consputc(int c)
{
  // 如果输入的字符是退格键（BACKSPACE，ASCII 码为 0x100），
  // 则将其覆盖为一个空格，然后再输出退格键，实现回退效果。
  if (c == BACKSPACE) {
    uartputc_sync('\b'); uartputc_sync(' '); uartputc_sync('\b');
  } else {
    // 否则直接将字符发送到 UART 输出
    uartputc_sync(c);
  }
}


// 定义一个结构体，表示终端控制台的状态信息
struct {
  struct spinlock lock;  // 自旋锁，用于保护终端控制台的并发访问

  // 输入缓冲区相关信息
// 输入缓冲区的大小为 128 字节
#define INPUT_BUF 128     
  char buf[INPUT_BUF];    // 输入缓冲区，用于存储用户输入的字符
  uint r;                 // 读索引，指向缓冲区中即将被读取的位置
  uint w;                 // 写索引，指向缓冲区中即将被写入的位置
  uint e;                 // 编辑索引，指向正在被编辑的位置
} cons;


// 用于处理用户写入控制台的数据。
// user_src 表示数据源的用户地址空间。
// src 是数据源的起始地址。
// n 是要写入控制台的字节数。
// 该函数会将用户数据从用户地址空间复制到内核，并逐字节输出到控制台。
// 函数返回实际成功写入控制台的字节数。
int
consolewrite(int user_src, uint64 src, int n)
{
  int i;

  // 遍历用户数据，逐字节进行拷贝和输出
  for (i = 0; i < n; i++) {
    char c;

    // 从用户地址空间复制一个字符到变量 c
    // 调用 either_copyin 函数用于实现从用户地址空间到内核的复制。
    // 如果复制失败（返回值为 -1），则退出循环。
    if (either_copyin(&c, user_src, src + i, 1) == -1)
      break;

    // 将字符 c 输出到控制台（UART）
    uartputc(c);
  }

  // 返回实际成功写入控制台的字节数
  return i;
}


// 用于处理用户从控制台读取数据。
// user_dst 表示数据目标的用户地址空间。
// dst 是数据目标的起始地址。
// n 是要读取的字节数。
// 该函数将用户从控制台读取的数据复制到指定的目标地址。
// 函数返回实际成功读取的字节数。
int
consoleread(int user_dst, uint64 dst, int n)
{
  uint target;    // 目标读取的字节数
  int c;          // 临时存储读取的字符
  char cbuf;      // 用于复制字符的临时缓冲区

  target = n;     // 记录目标读取的字节数
  acquire(&cons.lock);   // 获取终端控制台锁，用于保护并发访问

  while (n > 0) {
    // 等待直到中断处理程序将一些输入放入 cons.buffer。
    // 当 cons.r == cons.w 时，说明还没有新的输入字符。
    // 这时线程会进入睡眠状态，等待输入数据。
    while (cons.r == cons.w) {
      // 如果当前进程被终止（killed），则释放锁并返回 -1。
      // 表示读取过程被终止，没有成功读取任何数据。
      if (myproc()->killed) {
        release(&cons.lock);
        return -1;
      }
      sleep(&cons.r, &cons.lock);
    }

    c = cons.buf[cons.r++ % INPUT_BUF];   // 从输入缓冲区读取一个字符

    if (c == C('D')) {  // 如果输入字符是 Ctrl + D（end-of-file），表示输入结束
      if (n < target) {
        // 将 ^D 保存到下一次读取，确保调用者获取一个 0 字节的结果。
        cons.r--;
      }
      break;
    }

    // 将输入的字符复制到用户或内核地址空间的缓冲区中
    cbuf = c;
    if (either_copyout(user_dst, dst, &cbuf, 1) == -1)
      break;

    dst++;    // 更新目标地址，指向下一个写入位置
    --n;      // 剩余要读取的字节数减一

    if (c == '\n') {
      // 如果读取到换行符，则说明一行数据已经到达，返回给用户层面的 read() 函数。
      break;
    }
  }

  release(&cons.lock);  // 释放终端控制台锁
  return target - n;    // 返回实际成功读取的字节数
}


//控制台输入中断处理程序。
//uartitr（）为输入字符调用此函数。
//执行擦除/终止处理，附加到cons.buf，
//如果有整条线路到达，请唤醒consoleread（）。
void
consoleintr(int c)
{
  acquire(&cons.lock);  // 获取终端控制台锁，用于保护并发访问

  switch (c) {
  case C('P'):  // 如果接收到 Ctrl + P，打印进程列表（procdump）
    procdump();
    break;
  case C('U'):  // 如果接收到 Ctrl + U，删除当前行中的所有字符（Kill line）。
    while (cons.e != cons.w &&
           cons.buf[(cons.e - 1) % INPUT_BUF] != '\n') {
      cons.e--;   // 将编辑索引 cons.e 前移一个位置
      consputc(BACKSPACE);  // 输出退格键字符，擦除当前字符
    }
    break;
  case C('H'):  // 如果接收到 Ctrl + H，或者是退格键 '\x7f'，
               // 则删除当前行中的一个字符（Backspace）。
    if (cons.e != cons.w) {
      cons.e--;   // 将编辑索引 cons.e 前移一个位置
      consputc(BACKSPACE);  // 输出退格键字符，擦除当前字符
    }
    break;
  default:
    // 如果接收到的是普通字符（非控制字符），并且编辑缓冲区没有满
    if (c != 0 && cons.e - cons.r < INPUT_BUF) {
      // 将回车符 '\r' 转换为换行符 '\n'，以统一处理行结束标志
      c = (c == '\r') ? '\n' : c;

      // 回显字符给用户（输出到终端）
      consputc(c);

      // 将字符存储到编辑缓冲区（输入缓冲区），准备被 consoleread() 函数消费。
      cons.buf[cons.e++ % INPUT_BUF] = c;

      // 如果字符是换行符 '\n'，或者是 Ctrl + D（end-of-file），
      // 或者编辑缓冲区已满（即已接收了一行数据），
      // 则唤醒等待 consoleread() 的进程或线程。
      if (c == '\n' || c == C('D') || cons.e == cons.r + INPUT_BUF) {
        // 更新等待读取位置 cons.w，使其等于编辑位置 cons.e，
        // 以告知 consoleread() 有一行（或一个文件结束）已经到达。
        cons.w = cons.e;

        // 唤醒等待读取的进程或线程
        wakeup(&cons.r);
      }
    }
    break;
  }

  release(&cons.lock);  // 释放终端控制台锁
}


//
// 初始化终端控制台。
// 该函数用于初始化终端控制台的相关数据结构和设备。
//
void
consoleinit(void)
{
  // 初始化终端控制台锁（cons.lock），用于保护并发访问。
  initlock(&cons.lock, "cons");

  // 初始化 UART（串口），用于与终端设备通信。
  uartinit();

  // 将终端控制台的 read 和 write 系统调用连接到 consoleread 和 consolewrite 函数。
  // 这样，在用户程序中调用 read 和 write 时，实际上是调用 consoleread 和 consolewrite 函数。
  // devsw 是设备驱动的数据结构，它是设备驱动的接口。CONSOLE 表示终端控制台设备。
  // devsw[CONSOLE].read 表示终端控制台设备的读取函数，即 consoleread。
  // devsw[CONSOLE].write 表示终端控制台设备的写入函数，即 consolewrite。
  devsw[CONSOLE].read = consoleread;
  devsw[CONSOLE].write = consolewrite;
}
