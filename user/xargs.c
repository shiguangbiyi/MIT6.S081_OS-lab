#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    int i;
    int j = 0;
    int k;
    int l,m = 0;
    char block[32]; // Block = it should contain arguments of first command
    char buf[32]; // buf is for moving from bloc to split
    
    char *p = buf;
    char *split[32]; // Split = it should contain arguments of xargs

    for(i = 1; i < argc; i++){ // 将命令行参数（不包括程序名称本身）存储到split数组中。
        split[j++] = argv[i];
    }
    //从标准输入读取数据，直到读取完所有数据或者发生错误。
    while( (k = read(0, block, sizeof(block))) > 0){
        //处理每个读取的字节。
        for(l = 0; l < k; l++){
            if(block[l] == '\n'){
                buf[m] = 0;
                m = 0;
                split[j++] = p;
                p = buf;
                split[j] = 0;
                j = argc - 1;
                if(fork() == 0){
                    exec(argv[1], split);
                }
                wait(0);
            }else if(block[l] == ' ') {
                buf[m++] = 0;
                split[j++] = p;
                p = &buf[m];
            }else {
                buf[m++] = block[l];
            }
        }
    }
    exit(0);
}
