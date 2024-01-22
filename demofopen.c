#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#define BUFFERSIZE 10

int main()
{
    char * str1 = "hello";
    char * str2 = "world";
    char  temp[50] ={0};
    /* 创建目录 */
    /* 判断路径是否存在 */
    if(access(str1, F_OK) == -1)
    {
        /* 不存在则创建 */
        printf("mkdir\n");
        mkdir(str1, 0777);
    }
    else
    {
        printf("exist\n");
    }
    /* 拼接路径 */ 
    // sleep(1);
    sprintf(temp, "%s/%s.txt", str1, str2);
    printf("%d\n", BUFFERSIZE << 1);
    
    FILE * fp = fopen(temp,"w+");
    fprintf(fp, "hello world");
    fclose(fp);
    return 0;
}