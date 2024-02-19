#include "Client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <json-c/json.h>

/* 状态码 */
enum STATUS_CODE
{
    SUCCESS = 0,        // 成功
    NULL_PTR = -1,      // 空指针
    MALLOC_ERROR = -2,  // 内存分配失败
    ILLEGAL_ACCESS = -3,// 非法访问
    UNDERFLOW = -4,     // 下溢
    OVERFLOW = -5,      // 上溢
    SOCKET_ERROR = -6,  // socket错误
    CONNECT_ERROR = -7, // 连接错误
    SEND_ERROR = -8,    // 发送错误
    RECV_ERROR = -9,    // 接收错误
    FILE_ERROR = -10,   // 文件错误
    JSON_ERROR = -11,   // json错误
    OTHER_ERROR = -12,  // 其他错误

};

#define SERVER_PORT 8081      // 服务器端口号,暂定为8848
#define SERVER_IP "127.0.0.1"   // 服务器ip,暂定为本机ip
#define FILENAME_MAX_SIZE 256
#define READ_MAX_SIZE 1024      //读取数据最大数


int main()
{
     /* 创建套接字 */
   int fd = socket(AF_INET, SOCK_STREAM, 0);
   if ((fd == -1))
   {
        perror("socket error");
        return SOCKET_ERROR;
   }

   /* 与服务器进行连接 */
   struct sockaddr_in serverAddress;
   serverAddress.sin_family = AF_INET;
   serverAddress.sin_port = htons(SERVER_PORT);
   int ret = inet_pton(AF_INET, SERVER_IP, (void *)&(serverAddress.sin_addr.s_addr)); 
   if (ret == -1)
   {
        perror("inet_pton error");
        return ret;
   }
   ret = connect(fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
   if (ret == -1)
   {
        perror("connect error");
        return CONNECT_ERROR;
   }
   
   /* 向服务器发送下载文件的文件名 */
   /* 用来存放文件名 */
   char fileName[FILENAME_MAX_SIZE] = {"0"};
   while (1)
   {
        printf("请输入要下载的文件名:\n");
        scanf("%s",fileName);

        /* 将文件名发送到服务器 */
        ret = send(fd, fileName, sizeof(fileName), 0);
        if (ret < 0)
        {
            perror("send fileName error\n");
            exit(-1);
        }

        /* 等待服务器查询是否有此文件 */
        char recvfileName[FILENAME_MAX_SIZE] = {"0"};
        ret = recv(fd, recvfileName, sizeof(recvfileName), 0);
        if (ret < 0)
        {
            perror("recv feedback error\n");
            exit(-1);
        }
        if (strcmp(recvfileName, "success") == 0)
        {
            printf("存在此文件，正在下载\n");
            break;
        }
        else
        {
            printf("此文件不存在\n");
        }
   }
    /* 此时到这里证明文件存在，开始在客户端创建目标文件 */
    FILE *file = fopen(fileName, "wb");
    if (file == NULL) 
    {
        perror("fail to create file");
        close(fd);
        exit(1);
    }

    /* 开始读取目标文件 */
    char readBuffer[READ_MAX_SIZE];
    ssize_t bytes_recv;
    while ((bytes_recv = recv(fd, readBuffer, sizeof(readBuffer), 0)) > 0) 
    {
        fwrite(readBuffer, 1, bytes_recv, file);
    }

    /* 读完后关闭文件*/
    fclose(file);
    close(fd);
    printf("file download and save as %s\n", fileName);
   
    return 0;
}