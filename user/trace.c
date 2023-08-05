// 实现对特定系统调用的跟踪，同时运行指定的命令。

#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i;
  char *nargv[MAXARG];
  //参数个数不小于3个,确保系统调用号是合法数字
  if(argc < 3 || (argv[1][0] < '0' || argv[1][0] > '9')){
    fprintf(2, "Usage: %s mask command\n", argv[0]);
    exit(1);
  }
  //将第一个参数转换为整数,作为系统调用号传入trace函数---该系统调用函数需要我们提供对应的实现
  if (trace(atoi(argv[1])) < 0) {
    fprintf(2, "%s: trace failed\n", argv[0]);
    exit(1);
  }
  //nargv数组持有要追踪的命令
  for(i = 2; i < argc && i < MAXARG; i++){
    nargv[i-2] = argv[i];
  }
  //执行命令完成系统调用过程追踪
  exec(nargv[0], nargv);
  exit(0);
}
