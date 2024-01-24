#include "ChatRoom.h"
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
#define SERVER_IP "172.16.157.11"   // 服务器ip,暂定为本机ip
#define NAME_SIZE 10            // 用户名长度
#define PASSWORD_SIZE 20        // 密码长度
#define MAX_FRIEND_NUM 10       // 最大好友数量
#define MAX_GROUP_NUM 10        // 最大群组数量
#define MAX_GROUP_MEMBERS_NUM 20// 最大群组成员数量
#define CONTENT_SIZE 1024       // 信息内容长度
#define PATH_SIZE 256           // 文件路径长度


/* 静态声明 */
/* 登录成功的主界面 */
static int ChatRoomMain(int fd, json_object *json);

/* 发送json到服务器 */
static int SendJsonToServer(int fd, const char *json)
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
static int RecvJsonFromServer(int fd,  char *json)
{

    printf("开始接收json\n");
    int ret = recv(fd, json, CONTENT_SIZE, 0);
    if (ret < 0)
    {
        perror("recv error");
        return ret;
    }
    printf("json:%s\n",json);
    printf("接收json成功\n");
    return SUCCESS;
}

/* 拼接路径 */
static int JoinPath(char *path, const char *dir, const char *filename)
{
    int ret = 0;
    if (path == NULL || dir == NULL || filename == NULL)
    {
        return NULL_PTR;
    }
    strcpy(path, dir);
    strcat(path, "/");
    strcat(path, filename);
    return SUCCESS;
}


/* 聊天室初始化 */
int ChatRoomInit()
{
    /* 创建用户本地数据目录 */
    if(access("./usersData", F_OK) == -1)
    {
        if (mkdir("./usersData", 0777) == -1)
        {
            perror("mkdir error");
            return MALLOC_ERROR;
        }
    }
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
    while(1)
    {
        printf("请输入要进行的功能:\na.登录\nb.注册\n其他.退出聊天室\n");
        char ch;
        while ((ch = getchar()) == '\n');   // 读取一个非换行的字符
        while ((getchar()) != '\n');        // 吸收多余的字符
        printf("ch:%d\n",ch);
        switch (ch)
        {
            case 'a':
                ChatRoomLogin(fd);
                break;
            case 'b':
                ChatRoomRegister(fd);
                break;
            default:
                ChatRoomExit();
                close(fd);
                return SUCCESS;
        }
    }

}

/* 聊天室退出 */
int ChatRoomExit()
{
    /* todo... */
    printf("感谢您的使用\n");
    return 0;
}

/* 聊天室注册 */
int ChatRoomRegister(int sockfd)
{
    char name[NAME_SIZE] = {0};
    char password[PASSWORD_SIZE] = {0};

    printf("注册\n");
    printf("请输入账号:");
    scanf("%s", name);
    /* 不显示输入的密码 */
    strncpy(password, getpass("请输入密码:"), PASSWORD_SIZE);

    /* 注册信息转化为json，发送给服务器 */
    json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "type", json_object_new_string("register"));
    json_object_object_add(jobj, "name", json_object_new_string(name));
    json_object_object_add(jobj, "password", json_object_new_string(password));
    const char *json = json_object_to_json_string(jobj);

    /* 发送json */
    /*
        发送给服务器的信息：
            请求类型，账号，密码
    */
    SendJsonToServer(sockfd, json);


    /* 等待服务器响应 */
    printf("注册中 ");
    /*
        预期接收到的服务器信息：
        receipt:success/fail
        (success)
        name:自己的ID
        friends:
            name:待处理消息数
        groups:
            name:待处理消息数
        (fail)
        reason:失败原因

    */
    char retJson[CONTENT_SIZE] = {0};
    RecvJsonFromServer(sockfd, retJson);

    json_object *jreceipt = json_tokener_parse(retJson);
    if (jreceipt == NULL)
    {
        printf("注册失败\n");
        json_object_put(jreceipt);
        json_object_put(jobj);
        return JSON_ERROR;
    }

    const char *receipt = json_object_get_string(json_object_object_get(jreceipt,"receipt"));
    if (strcmp(receipt, "success") == 0)
    {
        printf("注册成功\n");
        json_object_put(jobj);
        json_object_object_del(jreceipt, "receipt");    // 删除掉多余的回执数据
        /* 初始化好友列表和群组列表 */
        json_object *friends = json_object_new_array();
        json_object *groups = json_object_new_array();
        json_object_object_add(jreceipt, "friends", friends);
        json_object_object_add(jreceipt, "groups", groups);
        ChatRoomMain(sockfd,jreceipt);  
    }
    else
    {
        const char *reason = json_object_get_string(json_object_object_get(jreceipt,"reason"));
        printf("注册失败:%s\n",reason);
        json_object_put(jobj);
        json_object_put(jreceipt);
    }

    return SUCCESS;
}

/* 聊天室登录 */
int ChatRoomLogin(int sockfd)
{
    char name[NAME_SIZE] = {0};
    char password[PASSWORD_SIZE] = {0};

    printf("登录\n");
    printf("请输入账号:");
    scanf("%s", name);
    /* 不显示输入的密码 */
    strncpy(password, getpass("请输入密码:"), PASSWORD_SIZE);
    /* 登录信息转化为json，发送给服务器 */
    json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "type", json_object_new_string("login"));
    json_object_object_add(jobj, "name", json_object_new_string(name));
    json_object_object_add(jobj, "password", json_object_new_string(password));
    const char *json = json_object_to_json_string(jobj);

    /*
        发送给服务器的信息：
            请求类型，账号，密码
    */
    SendJsonToServer(sockfd, json);

    /* 等待服务器响应 */
    printf("登录中 ");
    char retJson[CONTENT_SIZE] = {0};
    RecvJsonFromServer(sockfd, retJson);
    /*
        预期接收到的服务器信息：
        receipt:success/fail
        name:自己的ID
        friends:
            name:待处理消息数
        groups:
            name:待处理消息数

    */
    json_object *jreceipt = json_tokener_parse(retJson);

    if (jreceipt == NULL)
    {
        printf("登录失败\n");
        json_object_put(jreceipt);
        json_object_put(jobj);
        return JSON_ERROR;
    }

    const char *receipt = json_object_get_string(json_object_object_get(jreceipt,"receipt"));
    if (strcmp(receipt, "success") == 0)
    {
        printf("登录成功\n");
        json_object_put(jobj);
        json_object_object_del(jreceipt, "receipt");    // 删除掉多余的回执数据
        ChatRoomMain(sockfd,jreceipt);
    }
    else
    {
        const char *reason = json_object_get_string(json_object_object_get(jreceipt,"reason"));
        printf("登录失败:%s\n",reason);
        json_object_put(jreceipt);
        json_object_put(jobj);
        return SUCCESS;
    }
    return SUCCESS;
}


/* 添加好友 */
int ChatRoomAddFriend(int sockfd, const char *name, json_object *friends, const char *username)
{
    /* 添加好友信息转化为json，发送给服务器 */
    json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "type", json_object_new_string("addfriend"));
    json_object_object_add(jobj, "name", json_object_new_string(username));
    json_object_object_add(jobj, "friend", json_object_new_string(name));
    const char *json = json_object_to_json_string(jobj);
    /* 发送json */
    /*
        发送给服务器的信息：
            type:addfriend
            name:自己的ID
            friend:好友ID
    */
    SendJsonToServer(sockfd, json);

    /* 释放jobj */
    json_object_put(jobj);
    /* 将好友加入好友列表 */
    json_object_object_add(friends, name, json_object_new_int(0));
    /* 反馈 */
    printf("好友申请已发送\n");
    return SUCCESS;
}

/* 打印好友列表 */
static int ChatRoomPrintFriends(json_object *friends)
{
    printf("好友列表:\n");
    int jsonLen = json_object_object_length(friends);
    
    if(jsonLen == 0)
    {
        printf("暂无好友\n");
        return ILLEGAL_ACCESS;
    }
    else
    {
        // for(int idx = 0; idx < jsonArrayLen; idx++)
        // {
        //     json_object *friend = json_object_array_get_idx(friends, idx);
        //     const char *name = json_object_get_string(json_object_object_get(friend, "name"));
        //     const int messages_num = json_object_get_int(json_object_object_get(friend, "messages_num"));
        //     if(messages_num > 0)
        //     {
        //         printf("%s(%d)\n", name, messages_num);
        //     }
        //     else
        //     {
        //         printf("%s\n", name);
        //     }
        // }
        json_object_object_foreach(friends, key, value)
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
    return SUCCESS;
}

/* 显示好友 */
int ChatRoomShowFriends(int sockfd, json_object* friends, const char *username, const char * path)
{

    while(1)
    {   
        if (ChatRoomPrintFriends(friends) != SUCCESS)
        {
            return SUCCESS;
        }
        printf("a.添加好友\nb.删除好友\nc.私聊\nd.退出\n其他.返回上一级");
        char ch;
        char name[NAME_SIZE] = {0};
        while ((ch = getchar()) == '\n');   // 读取一个非换行的字符
        while ((getchar()) != '\n');        // 吸收多余的字符
        switch (ch)
        {
            case 'a':
            {
                printf("请输入要添加的好友:");
                scanf("%s", name);
                ChatRoomAddFriend(sockfd, name, friends, username);
                memset(name, 0, NAME_SIZE);
                break;
            }
            case 'b':
            {
                printf("请输入要删除的好友:");
                scanf("%s", name);
                ChatRoomDelFriend(sockfd, name, friends, username);
                memset(name, 0, NAME_SIZE);
                break;
            }
            case 'c':
            {
                printf("请输入要私聊的好友:");
                scanf("%s", name);
                /* 判断是否存在好友 */
                if(json_object_object_get(friends, name) == NULL)
                {
                    printf("好友不存在\n");
                    break;
                }
                /* 创建私聊的本地聊天记录文件 */
                char privateChatRecord[PATH_SIZE] = {0};
                JoinPath(privateChatRecord, path, name);
                /* 创建文件 */
                FILE *fp = fopen(privateChatRecord, "a+");
                if(fp == NULL)
                {
                    printf("创建文件失败\n");
                    break;
                }
                fclose(fp);
                ChatRoomPrivateChat(sockfd, name, friends,username,privateChatRecord);
                memset(name, 0, NAME_SIZE);
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
int ChatRoomDelFriend(int sockfd, const char *name, json_object *friends, const char *username)
{
    
}

/* 私聊 */
int ChatRoomPrivateChat(int sockfd, const char *name, json_object *friends, const char *username, const char * path)
{
    /* 打开私聊的本地聊天记录文件 */


    char content[CONTENT_SIZE] = {0};
    while(1)
    {
        printf("请输入要私聊的内容:");
        scanf("%s", content);
        /* 私聊信息转化为json，发送给服务器 */
        json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "type", json_object_new_string("private"));
        json_object_object_add(jobj, "name", json_object_new_string(name));
        json_object_object_add(jobj, "content", json_object_new_string(content));
        const char *json = json_object_to_json_string(jobj);
        /*
            发送给服务器的信息：
                type：private
                name：好友名
                content：私聊内容
        */
        SendJsonToServer(sockfd, json);
    }
}

/* 接收消息 */
int ChatRoomRecvMsg(int sockfd, json_object *friends)
{
    /*
        预期接收到的服务器信息：
            type:private/group
            name:发信人
            toname:收信人
            content:消息内容
            time:发送时间
    */
}

/* 发起群聊 */
int ChatRoomAddGroupChat(int sockfd, const char *name);

/* 显示群聊列表 */
int ChatRoomShowGroupChat(int sockfd, json_object *groups, const char *username)
{
    while(1)
    {

        printf("群组列表:\n");
        if(json_object_array_length(groups) == 0)
        {
            printf("暂无群组\n");
            return SUCCESS;
        }
        else
        {
            json_object_object_foreach(groups, key, value)
            {
                if(value != 0)
                {
                    printf("\t%s*\n", key);
                }
                else
                {
                    printf("\t[%s]\n", key);
                }
            }
        }
    }
}

/* 群聊 */
int ChatRoomGroupChat(int sockfd, const char *name);

/* 退出群聊 */
int ChatRoomExitGroupChat(int sockfd, const char *name);


/* 登录成功的主界面 */
static int ChatRoomMain(int fd, json_object *json)
{
    
    /* 用户名 */
    json_object *usernameJson = json_object_object_get(json, "name");
    if(usernameJson == NULL)
    {
        printf("json_object_object_get error\n");
        return JSON_ERROR;
    }
    const char *username = json_object_get_string(usernameJson);
    
    /* 创建用户本地数据目录 */
    char path[PATH_SIZE] = {0};
    JoinPath(path, "./usersData", username);
    if(access(path, F_OK) == -1)
    {
        if (mkdir(path, 0777) == -1)
        {
            perror("mkdir error");
            return MALLOC_ERROR;
        }
    }
    /* 好友列表 */
    json_object * friends = json_object_object_get(json, "friends");
    const char *friend = json_object_get_string(friends);
    printf("friend:%s\n",friend);
    /* 群组列表 */
    json_object * groups = json_object_object_get(json, "groups");
    const char *group = json_object_get_string(groups);
    printf("group:%s\n",group);
    while(1)
    {
        /* 显示好友列表和群组列表 */
        printf("a.显示好友列表\nb.显示群聊列表\ne.退出登录\n其他无效\n");
        char ch;
        while ((ch = getchar()) == '\n');   // 读取一个非换行的字符
        while ((getchar()) != '\n');        // 吸收多余的字符
        switch (ch)
        {
            case 'a':
                ChatRoomShowFriends(fd, friends,username, path);
                break;
            case 'b':
                ChatRoomShowGroupChat(fd, groups,username);
                break;
            case 'e':
                printf("退出登录\n");
                return SUCCESS;
                break;
            default:
                printf("无效操作\n");
        }
    }

    return SUCCESS;
}
