#include "kernel/param.h"
#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/riscv.h"
#include "user/user.h"

void ugetpid_test();
void pgaccess_test();

int
main(int argc, char *argv[])
{
  ugetpid_test();
  pgaccess_test();
  printf("pgtbltest: all tests succeeded\n");
  exit(0);
}

char *testname = "???";

void
err(char *why)
{
  printf("pgtbltest: %s failed: %s, pid=%d\n", testname, why, getpid());
  exit(1);
}

void
ugetpid_test()
{
  int i;

  printf("ugetpid_test starting\n"); // 打印测试开始信息
  testname = "ugetpid_test"; // 设置当前测试的名称

  // 循环进行多次测试
  for (i = 0; i < 64; i++) {
    int ret = fork(); // 创建一个子进程
    if (ret != 0) { // 如果是父进程
      wait(&ret); // 等待子进程结束
      if (ret != 0)
        exit(1); // 子进程出错，退出
      continue; // 继续下一个测试
    }
    if (getpid() != ugetpid()) // 在子进程中，比较getpid()和ugetpid()
      err("missmatched PID"); // 如果PID不匹配，报错
    exit(0); // 子进程正常结束
  }
  printf("ugetpid_test: OK\n"); // 所有测试成功通过，打印成功信息
}


void
pgaccess_test()
{
  char *buf;
  unsigned int abits; // 存储返回的访问位掩码
  printf("pgaccess_test starting\n");
  testname = "pgaccess_test";

  // 分配一块内存用于测试
  buf = malloc(32 * PGSIZE);
  if (pgaccess(buf, 32, &abits) < 0) // 调用pgaccess系统调用来检测访问情况
    err("pgaccess failed"); // 如果pgaccess失败，输出错误信息

  // 在测试内存的不同位置进行访问
  buf[PGSIZE * 1] += 1;
  buf[PGSIZE * 2] += 1;
  buf[PGSIZE * 30] += 1;

  // 再次调用pgaccess系统调用来检测访问情况
  if (pgaccess(buf, 32, &abits) < 0)
    err("pgaccess failed"); // 如果pgaccess失败，输出错误信息

  // 检查返回的访问位掩码是否与预期相符
  if (abits != ((1 << 1) | (1 << 2) | (1 << 30)))
    err("incorrect access bits set"); // 如果访问位掩码与预期不符，输出错误信息

  free(buf); // 释放测试用的内存
  printf("pgaccess_test: OK\n"); // 输出测试通过的信息
}
