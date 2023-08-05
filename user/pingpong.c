#include "kernel/types.h"
#include "user/user.h"

int 
main(int argc, char** argv ){
    int pid;//保存 fork 函数创建的子进程的进程ID
    int parent_fd[2];//parent_fd[0]父进程到子进程管道的读端;parent_fd[1]父进程到子进程管道的写端
    int child_fd[2];//child_fd[0]子进程到父进程管道的读端;child_fd[1]子进程到父进程管道的写端
    char buf[20];//
    //为父子进程建立管道
    pipe(child_fd); 
    pipe(parent_fd);

    // Child Progress
    // 如果进程是子进程，则关闭父子进程之间的不需要的管道端，并处理从父进程接收到的数据，并向父进程发送响应。
    if((pid = fork()) == 0){//如果当前进程为子进程
        close(parent_fd[1]);//关闭父子进程之间的不需要的管道端
        read(parent_fd[0],buf, 4);//处理从父进程接收到的数据
        printf("%d: received %s\n",getpid(), buf);//输出处理结果
        close(child_fd[0]);//处理完后关闭通道
        write(child_fd[1], "pong", sizeof(buf));//子进程返回信号给父进程
        exit(0);//退出进程
    }
    // Parent Progress
    // 如果进程是父进程，则关闭父子进程之间的不需要的管道端，并处理从子进程接收到的数据，并向子进程发送 "ping" 消息。
    else{
        close(parent_fd[0]);//关闭父进程到子进程管道的读端
        write(parent_fd[1], "ping",4);//向子进程发送ping信号
        close(child_fd[1]);//关闭子进程到父进程管道的读端
        read(child_fd[0], buf, sizeof(buf));//子进程读父进程的信号
        printf("%d: received %s\n", getpid(), buf);//输出处理结果
        exit(0);//退出进程
    }
    
}
