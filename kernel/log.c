#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// log.c 文件实现了简单的日志系统，用于支持并发的文件系统系统调用。
//
// 日志事务包含多个文件系统系统调用的更新。只有在没有激活的文件系统系统调用时，
// 日志系统才进行提交。因此，不需要推理是否将未提交的系统调用的更新写入磁盘。
//
// 系统调用应该调用 begin_op() 和 end_op() 来标记它的开始和结束。
// 通常，begin_op() 只是增加了正在进行中的文件系统系统调用的计数并返回。
// 但如果它认为日志即将用完，它会睡眠，直到最后一个未完成的 end_op() 提交。
//
// 日志是一个包含磁盘块的物理重做日志。磁盘上的日志格式：
//   头块，包含块 A、B、C 等的块号
//   块 A
//   块 B
//   块 C
//   ...
// 日志追加是同步的。

// 日志头部的内容，用于磁盘上的头部块以及在内存中跟踪提交前的块号。
struct logheader {
  int n; // 当前日志中的块数
  int block[LOGSIZE]; // 日志中的块号
};

struct log {
  struct spinlock lock; // 日志锁
  int start; // 日志在磁盘上的起始块号
  int size; // 日志大小（块数）
  int outstanding; // 正在执行的 FS 系统调用数
  int committing;  // 在提交中
  int dev; // 日志所属设备号
  struct logheader lh; // 日志头部数据
};

struct log log;

static void recover_from_log(void);
static void commit();

// 初始化日志系统
void
initlog(int dev, struct superblock *sb)
{
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  initlock(&log.lock, "log");
  log.start = sb->logstart;
  log.size = sb->nlog;
  log.dev = dev;
  recover_from_log(); // 从磁盘中恢复
}

// 将日志中已提交的块复制到它们的目标位置
static void
install_trans(int recovering)
{
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *lbuf = bread(log.dev, log.start+tail+1); // 读取日志块
    struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // 读取目标块
    memmove(dbuf->data, lbuf->data, BSIZE);  // 复制块到目标位置
    bwrite(dbuf);  // 将目标块写入磁盘
    if(recovering == 0)
      bunpin(dbuf);
    brelse(lbuf);
    brelse(dbuf);
  }
}

// 从磁盘读取日志头部到内存中的日志头部
static void
read_head(void)
{
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  log.lh.n = lh->n;
  for (i = 0; i < log.lh.n; i++) {
    log.lh.block[i] = lh->block[i];
  }
  brelse(buf);
}

// 将内存中的日志头部写入磁盘。
// 这是当前事务真正提交的地方。
static void
write_head(void)
{
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = log.lh.n;
  for (i = 0; i < log.lh.n; i++) {
    hb->block[i] = log.lh.block[i];
  }
  bwrite(buf);
  brelse(buf);
}

// 从日志中恢复
static void
recover_from_log(void)
{
  read_head();
  install_trans(1); // 如果已提交，则从日志复制到磁盘
  log.lh.n = 0;
  write_head(); // 清空日志
}

// 在每个 FS 系统调用的开始处调用。
void
begin_op(void)
{
  acquire(&log.lock);
  while(1){
    if(log.committing){
      sleep(&log, &log.lock);
    } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
      // 此操作可能耗尽日志空间，等待提交。
      sleep(&log, &log.lock);
    } else {
      log.outstanding += 1;
      release(&log.lock);
      break;
    }
  }
}

// 在每个 FS 系统调用的结束处调用。
// 如果这是最后一个未完成的操作，则进行提交。
void
end_op(void)
{
  int do_commit = 0;

  acquire(&log.lock);
  log.outstanding -= 1;
  if(log.committing)
    panic("log.committing");
  if(log.outstanding == 0){
    do_commit = 1;
    log.committing = 1;
  } else {
    // begin_op() 可能在等待日志空间，
    // 并且减少了预留空间量。
    wakeup(&log);
  }
  release(&log.lock);

  if(do_commit){
    // 调用 commit() 时不持有锁，因为不能
    // 在持有锁时睡眠。
    commit();
    acquire(&log.lock);
    log.committing = 0;
    wakeup(&log);
    release(&log.lock);
  }
}

// 将修改后的块从缓存复制到日志。
static void
write_log(void)
{
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *to = bread(log.dev, log.start+tail+1); // 日志块
    struct buf *from = bread(log.dev, log.lh.block[tail]); // 缓存块
    memmove(to->data, from->data, BSIZE);
    bwrite(to);  // 写入日志
    brelse(from);
    brelse(to);
  }
}

// 提交日志
static void
commit()
{
  if (log.lh.n > 0) {
    write_log();     // 将缓存中的修改块写入日志
    write_head();    // 将头部写入磁盘 - 真正的提交
    install_trans(0); // 现在将写入的数据复制到目标位置
    log.lh.n = 0;
    write_head();    // 擦除日志中的事务
  }
}

// 调用者已经修改了 b->data 并完成了对缓冲区的使用。
// 通过增加 refcnt 记录块号并将其固定在缓存中。
// commit()/write_log() 将进行磁盘写入。
//
// log_write() 替换了 bwrite()；典型用法是：
//   bp = bread(...)
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
  int i;

  acquire(&log.lock);
  if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
    panic("too big a transaction");
  if (log.outstanding < 1)
    panic("log_write outside of trans");

  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.block[i] == b->blockno)   // 日志吸收
      break;
  }
  log.lh.block[i] = b->blockno;
  if (i == log.lh.n) {  // 添加新块到日志中？
    bpin(b);
    log.lh.n++;
  }
  release(&log.lock);
}
