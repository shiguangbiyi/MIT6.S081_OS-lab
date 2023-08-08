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
  unsigned int abits;
  printf("pgaccess_test starting\n");
  testname = "pgaccess_test";
  buf = malloc(32 * PGSIZE);
  if (pgaccess(buf, 32, &abits) < 0)
    err("pgaccess failed");
  buf[PGSIZE * 1] += 1;
  buf[PGSIZE * 2] += 1;
  buf[PGSIZE * 30] += 1;
  if (pgaccess(buf, 32, &abits) < 0)
    err("pgaccess failed");
  if (abits != ((1 << 1) | (1 << 2) | (1 << 30)))
    err("incorrect access bits set");
  free(buf);
  printf("pgaccess_test: OK\n");
}
