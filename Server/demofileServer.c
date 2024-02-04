#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<error.h>
#include<netinet/in.h>
#include<unistd.h>
#include <pthread.h>

#define SERVER_PORT 8081
#define MAX_LISTEN 128
#define BUFFER_SIZE 128
#define SQL_SIZE 1024
#define NAME_MAX_SIZE 20
#define PASSWORD_MAX_SIZE 20
#define CONTENT_SIZE 1024
#define FILENAME_MAX_SIZE 256
#define READ_MAX_SIZE 1024      //读取数据最大数

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
    DATABASE_ERROR = -13,// 数据库错误
    SETSOCKOPT_ERROR = -14,// 设置socket选项错误

};


void * download_file(void * arg)
{

    int acceptfd = *((int *)arg);//线程接收客户端的socket
    char fileName[FILENAME_MAX_SIZE];
    memset(fileName, 0, sizeof(fileName));
    /* 读取文件名 */
    ssize_t read_bytes;
    read_bytes = read(acceptfd,fileName,sizeof(fileName));//接收客户端请求下载的文件
    if(read_bytes < 0)
    {
        perror("fileName read error\n");
        close(acceptfd);
        return NULL;
    }

    FILE * file = fopen(fileName,"rb"); //打开客户端要求的文件的名字
    if(file == NULL)
    {
        char *feedback = "fail";
        send(acceptfd, feedback, sizeof(feedback), 0);
        perror("file opened error\n");
        printf("没有此文件\n");
        close(acceptfd);
        return NULL;
    }
    else
    {
        char *feedback = "success";
        send(acceptfd, feedback,sizeof(feedback), 0);
    }
    sleep(1);

    char buffer[READ_MAX_SIZE];
    memset(buffer, 0, sizeof(buffer));
    while((read_bytes = fread(buffer,1,sizeof(buffer),file)) > 0)
    {
        if(write(acceptfd,buffer,read_bytes) < 0)
        {
        perror(" server write error\n");
        break;
        }

    }

    fclose(file);
    close(acceptfd);
    printf("Download file success\n");
    return NULL;
}

int main()
{
    /* 创建套接字 */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("sockfd error\n");
        exit(-1);
    }

    /* 设置端口复用 */
    int remakePort = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&remakePort, sizeof(remakePort));
    if (ret == -1)
    {
        perror("sockfd error\n");
        return ret;
        exit(-1);
    }
    
    /* 设置结构体变量 */
    struct sockaddr_in localAddress;
    memset(&localAddress, 0, sizeof(localAddress));

    /* 地址族 */
    /* 选取ipv4*/
    localAddress.sin_family = AF_INET;

    /* 端口转换，需要转换成网络字节序，也就是大端 */
    localAddress.sin_port = htons(SERVER_PORT);

    /* IP地址也需要转换成网络字节序 */
    /* 可以接受任意的IP地址 */
    /* INADDR_ANY = 0x00000000 */
    localAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(sockfd, (struct sockaddr *)&localAddress, sizeof(localAddress));
    if (ret == -1)
    {
        perror("bind error\n");
        exit(-1);
    }

    /* 设置监听，有主动态转换为被动态 */
    ret = listen(sockfd, MAX_LISTEN);
    if (ret == -1)
    {
        perror("listen error\n");
        exit(-1);
    }

    while (1)
    {
        /* 建立通信 */
        /* 客户端结构体信息 */
        struct sockaddr_in clientAddress;
        memset(&clientAddress, 0, sizeof(clientAddress));
        socklen_t clientAddressLen = sizeof(clientAddress);
        int acceptfd = accept(sockfd, (struct sockaddr *)&clientAddress, &clientAddressLen);
        if (ret == -1)
        {
            perror("accept error\n");
            exit(-1);
        }
        pthread_t thread;
        if(pthread_create(&thread,NULL,download_file,&acceptfd) != 0)
        {
            perror("server thread error\n");
            exit(1);
            close(acceptfd);
        }
        pthread_detach(thread); //线程分离，线程结束后自动回收资源

                
    }
    /* 关闭套接字 */
    close(sockfd);

    return 0;
}