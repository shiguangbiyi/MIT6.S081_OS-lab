// UART 控制寄存器在内存中的映射地址是 UART0。该宏返回寄存器的地址。
#define Reg(reg) ((volatile unsigned char *)(UART0 + reg))

// UART 控制寄存器。
// 一些寄存器在读取和写入时具有不同的含义。
#define RHR 0                 // 接收保持寄存器（用于输入字节）
#define THR 0                 // 发送保持寄存器（用于输出字节）
#define IER 1                 // 中断使能寄存器
#define IER_RX_ENABLE (1<<0)  // 接收中断使能位
#define IER_TX_ENABLE (1<<1)  // 发送中断使能位
#define FCR 2                 // FIFO 控制寄存器
#define FCR_FIFO_ENABLE (1<<0)   // FIFO 使能位
#define FCR_FIFO_CLEAR (3<<1)    // 清空两个 FIFO 的内容
#define ISR 2                 // 中断状态寄存器
#define LCR 3                 // 线路控制寄存器
#define LCR_EIGHT_BITS (3<<0)   // 8 位数据位，无奇偶校验
#define LCR_BAUD_LATCH (1<<7)   // 用于设置波特率的特殊模式
#define LSR 5                 // 线路状态寄存器
#define LSR_RX_READY (1<<0)     // 输入已准备好可以从 RHR 中读取
#define LSR_TX_IDLE (1<<5)      // THR 可以接受另一个字符发送

#define ReadReg(reg) (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

// 发送输出缓冲区。
struct spinlock uart_tx_lock;
#define UART_TX_BUF_SIZE 32
char uart_tx_buf[UART_TX_BUF_SIZE];
uint64 uart_tx_w; // 写入下一个字符到 uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE]
uint64 uart_tx_r; // 从 uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE] 读取下一个字符

extern volatile int panicked; // 来自 printf.c

void uartstart();

// 初始化 UART 设备
void
uartinit(void)
{
  // 禁用中断。
  WriteReg(IER, 0x00);

  // 设置特殊模式以设置波特率。
  WriteReg(LCR, LCR_BAUD_LATCH);

  // 波特率设置为 38.4K。
  WriteReg(0, 0x03); // LSB
  WriteReg(1, 0x00); // MSB

  // 离开设置波特率模式，
  // 并将数据位设置为 8 位，无奇偶校验。
  WriteReg(LCR, LCR_EIGHT_BITS);

  // 重置并启用 FIFO。
  WriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);

  // 启用发送和接收中断。
  WriteReg(IER, IER_TX_ENABLE | IER_RX_ENABLE);

  initlock(&uart_tx_lock, "uart");
}

// 将字符添加到输出缓冲区，并告诉 UART 开始发送（如果尚未开始）。
// 如果输出缓冲区已满，会阻塞。因为可能会阻塞，所以它不能从中断中调用；
// 它只适用于 write() 中使用。
void
uartputc(int c)
{
  acquire(&uart_tx_lock);

  if(panicked){
    for(;;)
      ;
  }

  while(1){
    if(uart_tx_w == uart_tx_r + UART_TX_BUF_SIZE){
      // 缓冲区已满。
      // 等待 uartstart() 在缓冲区中打开空间。
      sleep(&uart_tx_r, &uart_tx_lock);
    } else {
      uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE] = c;
      uart_tx_w += 1;
      uartstart();
      release(&uart_tx_lock);
      return;
    }
  }
}

// 不使用中断的 uartputc() 的备用版本，
// 用于内核的 printf() 和回显字符。它会旋转等待 UART 的输出寄存器为空。
void
uartputc_sync(int c)
{
  push_off();

  if(panicked){
    for(;;)
      ;
  }

  // 等待 LSR 中设置 Transmit Holding Empty。
  while((ReadReg(LSR) & LSR_TX_IDLE) == 0)
    ;
  WriteReg(THR, c);

  pop_off();
}

// 如果 UART 空闲，并且在发送缓冲区中有一个字符等待，发送它。
// 调用者必须持有 uart_tx_lock。
// 从上半部和下半部调用。
void
uartstart()
{
  while(1){
    if(uart_tx_w == uart_tx_r){
      // 发送缓冲区为空。
      return;
    }
    
    if((ReadReg(LSR) & LSR_TX_IDLE) == 0){
      // UART 发送保持寄存器已满，
      // 所以我们不能给它提供另一个字节。
      // 当它准备好接受新字节时，它会发生中断。
      return;
    }
    
    int c = uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE];
    uart_tx_r += 1;
    
    // 可能 uartputc() 在缓冲区中等待空间。
    wakeup(&uart_tx_r);
    
    WriteReg(THR, c);
  }
}

// 从 UART 读取一个输入字符。
// 如果没有等待的字符，返回 -1。
int
uartgetc(void)
{
  if(ReadReg(LSR) & 0x01){
    // 输入数据已准备好。
    return ReadReg(RHR);
  } else {
    return -1;
  }
}

// 处理 uart 中断，引发原因是有输入到达，或 uart 准备好发送更多输出，
// 或者两者兼有。从 trap.c 调用。
void
uartintr(void)
{
  // 读取并处理传入字符。
  while(1){
    int c = uartgetc();
    if(c == -1)
      break;
    consoleintr(c);
  }

  // 发送缓冲区中的字符。
  acquire(&uart_tx_lock);
  uartstart();
  release(&uart_tx_lock);
}
