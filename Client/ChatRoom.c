#include "ChatRoom.h"
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

#define SERVER_PORT 8888        // 服务器端口号,暂定为8888
#define SERVER_IP "127.0.0.1"   // 服务器ip,暂定为本机ip
#define NAME_SIZE 10            // 用户名长度
#define PASSWORD_SIZE 20        // 密码长度
#define MAX_FRIEND_NUM 10       // 最大好友数量
#define MAX_GROUP_NUM 10        // 最大群组数量
#define MAX_GROUP_MEMBERS_NUM 20// 最大群组成员数量
#define CONTENT_SIZE 1024       // 信息内容长度


/* 发送json到服务器 */
int SendJsonToServer(int fd, const char *json)
{
    int ret = 0;
    int len = strlen(json);
    int sendLen = 0;
    while (sendLen < len)
    {
        ret = send(fd, json + sendLen, len - sendLen, 0);
        if (ret < 0)
        {
            perror("send error");
            return ret;
        }
        sendLen += ret;
    }
    return SUCCESS;
}
/* 接收json */
int RecvJsonFromServer(int fd, char *json)
{
    int ret = 0;
    int len = 0;
    int recvLen = 0;
    while (recvLen < len)
    {
        ret = recv(fd, json + recvLen, len - recvLen, 0);
        if (ret < 0)
        {
            perror("recv error");
            return ret;
        }
        recvLen += ret;
        if (recvLen == len)
        {
            break;
        }
        if (recvLen > len)
        {
            perror("recv error");
            return ILLEGAL_ACCESS;
        }
        if (recvLen < len)
        {
            continue;
        }
    }
    return SUCCESS;
}


/* 聊天室初始化 */
int ChatRoomOLInit()
{
    /* 初始化与服务器的连接 */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket error");
        return SOCKET_ERROR;
    }
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    int ret = connect(fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret == -1)
    {
        perror("connect error");
        return CONNECT_ERROR;
    }


    printf("欢迎使用网络聊天室\n");
    printf("请输入要进行的功能:\na.登录\nb.注册\n其他.退出聊天室");
    char ch;
    scanf("%c", &ch);
    switch (ch)
    {
        case 'a':
            ChatRoomOLLogin(fd);
            break;
        case 'b':
            ChatRoomOLRegister(fd);
            break;
        default:
            ChatRoomOLExit();
            break;
    }


}

/* 聊天室退出 */
int ChatRoomOLExit()
{
    printf("感谢您的使用\n");
    return 0;
}

/* 聊天室注册 */
int ChatRoomOLRegister(int sockfd)
{
    printf("注册\n");
    printf("请输入账号:");
    char name[NAME_SIZE];
    scanf("%s", name);
    printf("请输入密码:");
    char password[NAME_SIZE];
    scanf("%s", password);

    /* 注册信息转化为json，发送给服务器 */
    json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "type", json_object_new_string("register"));
    json_object_object_add(jobj, "name", json_object_new_string(name));
    json_object_object_add(jobj, "password", json_object_new_string(password));
    char *json = json_object_to_json_string(jobj);
    SendJsonToServer(sockfd, json);
    free(json);
    return SUCCESS;
}

/* 聊天室登录 */
int ChatRoomOLLogin(int sockfd);

/* 添加好友 */
int ChatRoomOLAddFriend(int sockfd, const char *name);

/* 显示好友 */
int ChatRoomOLShowFriend(int sockfd);

/* 删除好友 */
int ChatRoomOLDelFriend(int sockfd, const char *name);

/* 私聊 */
int ChatRoomOLPrivateChat(int sockfd, const char *name);

/* 发起群聊 */
int ChatRoomOLAddGroupChat(int sockfd, const char *name);

/* 群聊 */
int ChatRoomOLGroupChat(int sockfd, const char *name);

/* 退出群聊 */
int ChatRoomOLExitGroupChat(int sockfd, const char *name);
