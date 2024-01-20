#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <json-c/json.h>


#define SERVER_PORT 8888        // 服务器端口号,暂定为8888
#define SERVER_IP "172.16.157.11"   // 服务器ip,暂定为本机ip
#define NAME_SIZE 10            // 用户名长度
#define PASSWORD_SIZE 20        // 密码长度
#define MAX_FRIEND_NUM 10       // 最大好友数量
#define MAX_GROUP_NUM 10        // 最大群组数量
#define MAX_GROUP_MEMBERS_NUM 20// 最大群组成员数量
#define CONTENT_SIZE 1024       // 信息内容长度
#define PATH_SIZE 256           // 文件路径长度

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

int main(int argc, char *argv[])
{
    /* 初始化服务 */ 
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket error");
        return SOCKET_ERROR;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    int ret = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        perror("bind error");
        return CONNECT_ERROR;
    }
    ret = listen(server_fd, 10);
    if (ret < 0)
    {
        perror("listen error");
        return CONNECT_ERROR;
    }
    printf("server start success\n");
    /* 启动服务 */
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0)
        {
            perror("accept error");
            continue;
        }
        printf("client connect success\n");
        /* 处理客户端请求 */
        char buffer[1024];
        while (1)
        {
            int ret = recv(client_fd, buffer, sizeof(buffer), 0);
            if (ret < 0)
            {
                perror("recv error");
                break;
            }
            else if (ret == 0)
            {
                printf("client disconnect\n");
                /* 客户端断开连接 */
                close(client_fd);
                break;
            }
            else
            {
                printf("receive message: %s\n", buffer);
                /* 处理客户端请求 */
                /* ... */
                /* 发送响应 */
            }
        }
    }

    return 0;
}