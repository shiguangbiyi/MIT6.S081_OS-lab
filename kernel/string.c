#include "types.h"

// 将 dst 起始地址的 n 个字节设置为字符 c。
void*
memset(void *dst, int c, uint n)
{
  char *cdst = (char *) dst; // 将 void 指针转换为 char 指针。
  int i;
  for(i = 0; i < n; i++){
    cdst[i] = c; // 将字符 c 赋值给 dst 中的每个字节。
  }
  return dst; // 返回设置后的 dst 地址。
}

// 比较两个内存区域 v1 和 v2 的前 n 个字节是否相同。
// 如果相同，返回 0，否则返回 v1 和 v2 的第一个不同字节的差值。
int
memcmp(const void *v1, const void *v2, uint n)
{
  const uchar *s1, *s2;

  s1 = v1; // 将 v1 转换为 uchar 指针。
  s2 = v2; // 将 v2 转换为 uchar 指针。
  while(n-- > 0){
    if(*s1 != *s2) // 比较 s1 和 s2 指向的字节是否相同。
      return *s1 - *s2; // 返回第一个不同字节的差值。
    s1++, s2++; // 移动指针到下一个字节。
  }

  return 0; // 如果前 n 个字节都相同，则返回 0。
}

// 将 src 起始地址的 n 个字节复制到 dst 起始地址，允许 src 和 dst 内存区域重叠。
// 如果重叠的情况下，从高地址向低地址复制；否则从低地址向高地址复制。
void*
memmove(void *dst, const void *src, uint n)
{
  const char *s;
  char *d;

  if(n == 0)
    return dst; // 如果 n 为 0，直接返回 dst。

  s = src; // 将 src 转换为 char 指针。
  d = dst; // 将 dst 转换为 char 指针。
  if(s < d && s + n > d){ // 如果 src 和 dst 内存区域重叠。
    s += n; // 从后向前复制，指向 src 的最后一个字节。
    d += n; // 指向 dst 的最后一个字节。
    while(n-- > 0)
      *--d = *--s; // 从高地址向低地址复制每个字节。
  } else
    while(n-- > 0)
      *d++ = *s++; // 从低地址向高地址复制每个字节。

  return dst; // 返回复制后的 dst 地址。
}

// memcpy 函数是为了满足 GCC 编译器的要求，实际上和 memmove 功能相同。
// 推荐使用 memmove，因为它能够处理内存区域重叠的情况。
void*
memcpy(void *dst, const void *src, uint n)
{
  return memmove(dst, src, n);
}

// 比较两个字符串 p 和 q 的前 n 个字符是否相同。
// 如果相同，返回 0；如果 p 小于 q，则返回负数；如果 p 大于 q，则返回正数。
int
strncmp(const char *p, const char *q, uint n)
{
  while(n > 0 && *p && *p == *q) // 比较前 n 个字符是否相同。
    n--, p++, q++; // 移动指针到下一个字符。
  if(n == 0)
    return 0; // 如果前 n 个字符都相同，则返回 0。
  return (uchar)*p - (uchar)*q; // 返回第一个不同字符的差值。
}

// 将字符串 t 的前 n 个字符复制到字符串 s。
// 如果 t 的长度小于 n，则 s 的剩余位置填充 0。
char*
strncpy(char *s, const char *t, int n)
{
  char *os;

  os = s; // 保存 s 的起始地址。
  while(n-- > 0 && (*s++ = *t++) != 0)
    ; // 将 t 的前 n 个字符复制到 s，并确保 s 以 '\0' 结尾。
  while(n-- > 0)
    *s++ = 0; // 如果 t 的长度小于 n，则 s 剩余的位置填充 0。
  return os; // 返回复制后的 s 起始地址。
}

// 类似于 strncpy，但是保证字符串以 '\0' 结尾。
// 将字符串 t 的前 n 个字符复制到字符串 s，并确保以 '\0' 结尾。
char*
safestrcpy(char *s, const char *t, int n)
{
  char *os;

  os = s; // 保存 s 的起始地址。
  if(n <= 0)
    return os; // 如果 n 小于等于 0，直接返回 s。
  while(--n > 0 && (*s++ = *t++) != 0)
    ; // 将 t 的前 n 个字符复制到 s，并确保 s 以 '\0' 结尾。
  *s = 0; // 确保字符串 s 以 '\0' 结尾。
  return os; // 返回复制后的 s 起始地址。
}

// 计算字符串 s 的长度（不包括 '\0' 结尾字符）。
int
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++) // 从头开始遍历字符串，直到遇到 '\0' 结尾字符。
    ;
  return n; // 返回字符串长度。
}
