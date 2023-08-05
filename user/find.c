#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

//Copy from Grep.c
char buf[1024];
int match(char*, char*);
int matchhere(char*, char*);
int matchstar(int, char*, char*);

int
match(char *re, char *text)
{
  if(re[0] == '^')//首先检查正则表达式re是否以 ^ 符号开始。
    return matchhere(re+1, text);//调用 matchhere 函数来检查剩余的正则表达式（去掉 ^ 后的部分）是否出现在 text 的开头。
  do{  // must look at empty string
    if(matchhere(re, text))
      return 1;
  }while(*text++ != '\0');//该循环用于检查空字符串的情况，即在文本的所有可能位置进行尝试匹配。
  return 0;
}

// matchhere: search for re at beginning of text
int matchhere(char *re, char *text)
{
  if(re[0] == '\0')//检查正则表达式 re 是否为空字符串，即是否已经到达正则表达式的结尾。
    return 1;//如果正则表达式 re 为空字符串，直接返回 1，表示匹配成功。空正则表达式表示空匹配，也就是在文本的任意位置都能找到匹配。
  if(re[1] == '*')
    return matchstar(re[0], re+2, text);
  if(re[0] == '$' && re[1] == '\0')
    return *text == '\0';
  if(*text!='\0' && (re[0]=='.' || re[0]==*text))
    return matchhere(re+1, text+1);
  return 0;
}

// matchstar: search for c*re at beginning of text
int matchstar(int c, char *re, char *text)
{
  do{  // a * matches zero or more instances
    if(matchhere(re, text))
      return 1;
  }while(*text!='\0' && (*text++==c || c=='.'));
  return 0;
}


/*
  find.c
*/
char* fmtname(char *path);
void find(char *path, char *re);

int 
main(int argc, char** argv){
    if(argc < 2){
      printf("Parameters are not enough\n");//只提供了程序名但没有其他参数
    }
    else{
      //在路径path下递归搜索文件 
      find(argv[1], argv[2]);//argv[1] 表示搜索的起始路径，argv[2] 表示用于匹配文件和目录名称的正则表达式。
    }
    exit(0);
}

// 对ls中的fmtname，去掉了空白字符串
// fmtname函数用于从路径中提取文件名并进行格式化。
// 如果文件名长度超过了 DIRSIZ，则直接返回原始文件名指针；
// 否则，将文件名复制到静态数组 buf 中，并返回指向该数组的指针，从而实现文件名的格式化处理。
char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)//用于找到路径中最后一个斜杠/后面的字符。p初始化为指向路径末尾的字符（\0，表示字符串结束），然后在每次循环迭代中向前移动，直到找到一个斜杠/或者回到路径的开头（p>=path）。
    ;
  p++;
  // printf("len of p: %d\n", strlen(p));
  if(strlen(p) >= DIRSIZ)
    return p;
  memset(buf, 0, sizeof(buf));
  memmove(buf, p, strlen(p));
  //memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void 
find(char *path, char *re){
  char buf[512], *p;//用于存储路径和文件/目录名称的临时字符串，而 p 用于在后续处理中指向 buf 中的字符。
  int fd;//用于存储打开文件的文件描述符。
  struct dirent de;//用于存储目录项的信息。dirent 结构体包含了文件或目录的名称和inode编号等信息。
  struct stat st;//用于存储文件或目录的状态信息。stat 结构体包含了文件类型、文件大小、inode编号等信息。
  //尝试打开指定路径path的文件或目录。open函数会返回一个文件描述符fd，用于后续对文件的读取操作。如果打开失败，那么会输出错误信息并直接返回。
  if((fd = open(path, 0)) < 0){
      fprintf(2, "find: cannot open %s\n", path);
      return;
  }
  //尝试获取文件或目录的状态信息，并将其存储在结构体 st 中。fstat函数会通过文件描述符fd获取文件或目录的状态信息。如果获取失败，那么会输出错误信息并关闭文件描述符 fd，然后直接返回。
  if(fstat(fd, &st) < 0){
      fprintf(2, "find: cannot stat %s\n", path);
      close(fd);
      return;
  }
  //根据获取到的文件类型 st.type 来进行不同的操作。st.type 表示文件的类型，可以是普通文件、目录、字符设备、块设备等。
  switch(st.type){
  //普通文件
  case T_FILE:
      if(match(re, fmtname(path)))//尝试使用正则表达式re匹配path的文件名（即去除路径后的文件名）。如果匹配成功，则打印该文件的路径。
          printf("%s\n", path);
      break;
  //目录
  case T_DIR:
      //判断检查当前路径 path 组合后是否超出了字符数组 buf 的大小。如果超过了，说明路径过长，无法继续搜索。在这种情况下，输出错误信息并退出当前目录的搜索。
      if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
          printf("find: path too long\n");
          break;
      }
      strcpy(buf, path);//将当前路径 path 复制到字符数组 buf 中。
      p = buf + strlen(buf);//将字符指针 p 指向字符数组 buf 中路径的末尾，即去除路径后的位置。
      *p++ = '/';//在 p 指向的位置添加一个斜杠 /，用于连接后续目录项的名称。
      //通过read函数从目录中读取目录项信息，并存储在结构体de中。每次循环迭代，都会读取一个目录项的信息。
      while(read(fd, &de, sizeof(de)) == sizeof(de)){
          if(de.inum == 0)//如果读取到的目录项的inode编号为0，表示该目录项无效，可能已被删除，因此跳过当前循环迭代。
              continue;
          memmove(p, de.name, DIRSIZ);//将目录项的名称复制到字符数组 buf 中的 p 指向的位置。
          p[DIRSIZ] = 0;//在目录项的名称末尾添加一个null字符，以确保字符数组buf中的字符串以null结尾。
          //尝试获取新的路径buf的状态信息，并将其存储在结构体st中。这样可以判断新的路径是文件还是目录，并获取其状态信息。
          if(stat(buf, &st) < 0){
              printf("find: cannot stat %s\n", buf);
              continue;
          }
          char* lstname = fmtname(buf);//调用fmtname函数，处理新的路径 buf 并返回去除路径后的文件/目录名称。
          //检查新的路径的文件/目录名称是否为.或..。如果是这两者之一，表示当前目录项为当前目录（.）或父级目录（..），因此跳过当前循环迭代，不再递归搜索。
          if(strcmp(".", lstname) == 0 || strcmp("..", lstname) == 0)
            continue;
          //递归调用find函数，将新的路径buf作为起始路径，继续进行搜索。这样可以深度优先地搜索整个目录树。
          else
            find(buf, re);
      }
      break;
  }
  close(fd);
}
