#include "kernel/types.h"
#include "user/user.h"

const int duration_pos = 1;
typedef enum {wrong_char, success_parse, toomany_char} cmd_parse;
cmd_parse parse_cmd(int argc, char** argv);

int 
main(int argc, char** argv){
    //参数不够时返回错误信息
    if(argc == 1){
        printf("Please enter the parameters!");
        exit(0);
    }
    else{
        cmd_parse parse_result;
        parse_result = parse_cmd(argc, argv);
        //如果解析结果是 toomany_char，表示命令行参数过多。
        if(parse_result == toomany_char){
            printf("Too many args! \n");
            exit(0);
        }
        //如果解析结果是 wrong_char，表示命令行参数中包含非数字字符。
        else if(parse_result == wrong_char){
            printf("Cannot input alphabet, number only \n");
            exit(0);
        }
        //如果以上条件都不满足，说明命令行参数正确。
        else{
            int duration = atoi(argv[duration_pos]);//使用 atoi 函数将 argv[duration_pos] 中的字符串转换为整数，并赋值给 duration 变量。
            sleep(duration);//使用 sleep 函数让程序暂停执行一段时间，持续时间由 duration 变量指定。
            exit(0);
        }
        
    }
}

cmd_parse
parse_cmd(int argc, char** argv){
    if(argc > 2){
        return toomany_char;
    }
    else {
        int i = 0;
        while (argv[duration_pos][i] != '\0')//它将遍历持续时间参数字符串（argv[duration_pos]）。循环会一直执行直到字符串中的字符为终止符 '\0'，表示字符串的结尾。
        {
            //用于检查当前字符是否为数字字符。
            if(!('0' <= argv[duration_pos][i] && argv[duration_pos][i] <= '9')){
                return wrong_char;
            }
            i++;
        }
        
    }
    return success_parse;
}
