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

#define SERVER_PORT 8082      // 服务器端口号,暂定为8848
#define SERVER_IP "127.0.0.1"   // 服务器ip,暂定为本机ip
#define NAME_SIZE 10            // 用户名长度
#define PASSWORD_SIZE 20        // 密码长度
#define MAX_FRIEND_NUM 10       // 最大好友数量
#define MAX_GROUP_NUM 10        // 最大群组数量
#define MAX_GROUP_MEMBERS_NUM 20// 最大群组成员数量
#define CONTENT_SIZE 1024       // 信息内容长度
#define MAX_NAME_SIZE 20        //最大姓名长度

/* 好友列表打印函数 */
static int ChatRoomPrintFriends();

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
        ret = recv(fd, json, CONTENT_SIZE, 0);
        if (ret < 0)
        {
            perror("recv error");
            return ret;
        }
    
    return SUCCESS;
}


/* 聊天室初始化 */
int ChatRoomInit()
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
    /* 初始化成功反馈 */
   printf("欢迎来到聊天室\n");
   /* 开始进行功能选择 */

   while (1)
   {
        printf("请输入你的选项:\na.登录\nb.注册\nc.退出\n");
        char ch ;
        ch = getchar();
        /* 根据输入的选项跳到对应的接口 */
        switch (ch)
        {
        case 'a':
            ChatRoomLogin(fd);
            break;
        
        case 'b':
            ChatRoomRegister(fd);
            break;

        case 'c':
            ChatRoomExit();
            return 0;

        default:
            return SUCCESS;
        }
        getchar();
   }
}


/* 聊天室退出 */
int ChatRoomExit()
{
    printf("感谢您的使用\n");
    return 0;
}

/* 聊天室注册 */
int ChatRoomRegister(int sockfd)
{
    /* 创建用户姓名，密码数组 */
    char name[NAME_SIZE];
    memset(name, 0, sizeof(name));
    char password[PASSWORD_SIZE];
    memset(&password, 0, sizeof(password));

    printf("请输入要注册的用户名:");
    scanf("%s", name);

    printf("请输入要设置的密码:");
    scanf("%s", password);

    /* 将注册的信息通过json发送到服务器 */
    json_object * object =json_object_new_object();
    json_object_object_add(object, "type", json_object_new_string("register"));
    json_object_object_add(object, "name", json_object_new_string(name));
    json_object_object_add(object,"password",json_object_new_string(password));
    /* 将json对象转换成字符串 */
    const char * json = json_object_to_json_string(object);
    /* 发送到服务器 */
    SendJsonToServer(sockfd, json);

    /* 判断是否接受成功 */
    char recvjson[CONTENT_SIZE];
    memset(recvjson, 0, sizeof(recvjson));
    
    /* 接受服务器反馈 */
    RecvJsonFromServer(sockfd, recvjson); 

    printf("json:%s\n",recvjson);
    /* 将收到的json进行解析 */
    json_object * retjson = json_tokener_parse(recvjson);
    if (retjson == NULL)
    {
        printf("注册失败\n");
        /* 释放json对象 */
        json_object_put(retjson);
        json_object_put(object);
    }
    /* 解析收到的服务器注册反馈 */
    json_object * retJson = json_object_object_get(retjson, "receipt");
    if(retJson == NULL)
    {
        printf("break;\n");
        return 0;
    }
    const char * reception = json_object_get_string(retJson);
    if (strcmp(reception, "success") == 0)
    {
        printf("注册成功\n");
    }
    return SUCCESS;
}

/* 聊天室登录 */
int ChatRoomLogin(int sockfd)
{
    /* 创建账户信息 */
    char name[NAME_SIZE];
    memset(name, 0, sizeof(name));
    char password[PASSWORD_SIZE];
    memset(password, 0, sizeof(password));

    printf("请输入要登录的用户名:");
    scanf("%s", name);

    printf("请输入密码:");
    scanf("%s", password);

    /* 发送json登录信息给服务器 */
    struct json_object * login = json_object_new_object();
    json_object_object_add(login, "type", json_object_new_string("login"));
    json_object_object_add(login, "name", json_object_new_string(name));
    json_object_object_add(login, "password",json_object_new_string(password));
    /* 将json对象转换成字符串 */
    const char * json = json_object_to_json_string(login);
    printf("%s\n", json);
    /* 发送json给服务器 */
    SendJsonToServer(sockfd, json);
    printf("正在登陆中\n");

    /*等待服务器的查询反馈 */
    char recvjson[CONTENT_SIZE];
    memset(recvjson, 0, sizeof(recvjson));
    RecvJsonFromServer(sockfd, recvjson);

    /* 将收到的json进行解析 */
    json_object * retjson = json_tokener_parse(recvjson);
    if (retjson == NULL)
    {
        printf("登录失败\n");
        /* 释放json对象 */
        json_object_put(retjson);
        json_object_put(login);
    }
    /* 解析收到的服务器登录反馈 */
    const char *  reception = json_object_get_string(json_object_object_get(retjson, "receipt"));
    if (strcmp(reception, "success") == 0)
    {
        printf("登录成功\n");
    }
    printf("靓仔啊，欢迎进入聊天室\n");
    printf("--------------------\n");
    printf("|  a.显示好友列表  |\n");
    printf("|  b.显示群聊列表  |\n");
    printf("|  c.退出          |\n");
    printf("--------------------\n");
    
    while (1)
   {
        printf("请输入你要进行的选择：");
        char ch ;
        ch = getchar();
        /* 根据输入的选项跳到对应的接口 */
        switch (ch)
        {
        case 'a':
            ChatRoomShowFriend(sockfd, name);//显示好友列表
            break;
        
        case 'b':
            break;

        case 'c':
            ChatRoomExit();
            return 0;

        default:
            return SUCCESS;
        }
        getchar();
   }


    
}

/* 添加好友 */
int ChatRoomAddFriend(int sockfd, const char *friend_name)
{

}


/* 显示好友 */
int ChatRoomShowFriend(int sockfd, const char *name)
{
    ChatRoomPrintFriends(sockfd, name);
    if (ChatRoomPrintFriends(sockfd, name) != SUCCESS)
    {
        return SUCCESS;
    }
    while (1)
    {
        char ch;
        ch = getchar();
        switch (ch)
        {
            case 'a':
            {
                char friendName[MAX_NAME_SIZE];
                printf("请输入要添加的好友:");
                scanf("%s", friendName);
                ChatRoomAddFriend(sockfd, friendName);
                memset(friendName, 0, NAME_SIZE);
                break;
            }
            case 'b':
            {
                printf("请输入要删除的好友:");
                break;
            }
            case 'c':
            {
                printf("请输入要私聊的好友:");
                    break;
            }
            case 'd':
            {
                /* todo... */
                break;
            }
            default:
                return SUCCESS;
        }
    }
    
    return SUCCESS; 
}

/* 删除好友 */
int ChatRoomDelFriend(int sockfd, const char *name);
#if 0
/* 私聊 */
int ChatRoomPrivateChat(int sockfd, const char *name)
{
    /* 打开存储在本地文件的聊天记录 */

    while (1)
    {
        /* 创建一个消息内容数组，用来存储发送的消息 */
        char content[CONTENT_SIZE];
        memset(content, 0, sizeof(content));
        printf("请输入要发送的消息");
        scanf("%s", content);

        /* 将私聊的消息转化为json发送给服务器 */
        struct json_object * private = json_object_new_object();
        json_object_object_add(private, "type", json_object_new_string("private"));
        json_object_object_add(private, "name",json_object_new_string("name"));
        json_object_object_add(private, "content", json_object_new_string("content"));
        /* 将字符串转换成json对象 */
        const char * json = json_object_to_json_string(private);

        /* 发送json给服务器 */
        SendJsonToServer(sockfd, json);
        
    }
}


/* 发起群聊 */
int ChatRoomAddGroupChat(int sockfd, const char *name);

/* 群聊 */
int ChatRoomGroupChat(int sockfd, const char *name);

/* 退出群聊 */
int ChatRoomExitGroupChat(int sockfd, const char *name);
#endif

static int ChatRoomPrintFriends(int sockfd, const char *name)
{
    /* 好友列表 */
    /* 发送json打印好友列表信息给服务器 */
    struct json_object * showFriends = json_object_new_object();
    json_object_object_add(showFriends, "type", json_object_new_string("showFriends"));
    json_object_object_add(showFriends, "name", json_object_new_string(name));
    /* 将json对象转换成字符串 */
    const char * json = json_object_to_json_string(showFriends);
    /* 发送到服务器 */
    SendJsonToServer(sockfd, json);

    /*等待服务器的查询反馈 */
    char recvjson[CONTENT_SIZE];
    memset(recvjson, 0, sizeof(recvjson));

    /* 接受好友列表 */
    RecvJsonFromServer(sockfd, recvjson);

    int jsonLen = json_object_object_length(recvjson);
    
    if(jsonLen == 0)
    {
        printf("暂无好友\n");
        return ILLEGAL_ACCESS;
    }
    else
    {
        printf("正在打印中\n");
        json_object_object_foreach(recvjson, key, value)
        {
            const char *name = key;
            const int messages_num = json_object_get_int(value);
            if(messages_num > 0)
            {
                printf("%s(%d)\n", name, messages_num);
            }
            else
            {
                printf("%s\n", name);
            }
        }
        
    }
    
}
