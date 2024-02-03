#include "threadPoll.h"
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
#include <mysql/mysql.h>


#define SERVER_PORT 8888            // 服务器端口号,暂定为8888
#define SERVER_IP "172.16.157.11"   // 服务器ip,暂定为本机ip
#define NAME_SIZE 10                // 用户名长度
#define PASSWORD_SIZE 20            // 密码长度
#define MAX_FRIEND_NUM 10           // 最大好友数量
#define MAX_GROUP_NUM 10            // 最大群组数量
#define MAX_GROUP_MEMBERS_NUM 20    // 最大群组成员数量
#define CONTENT_SIZE 1024           // 信息内容长度
#define PATH_SIZE 256               // 文件路径长度
#define MAX_SQL_LEN 1024            // sql语句长度
#define MAX_LISENT_NUM 128          // 最大监听数
#define MAX_BUFFER_SIZE 1024        // 最大缓冲区大小
#define MIN_POLL_NUM 2              // 最小线程池数量
#define MAX_POLL_NUM 8              // 最大线程池数量
#define MAX_QUEUE_NUM 50            // 最大队列数量
#define TIME_LEN 80                //显示的时间长度

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

// 定义结构体来保存任务参数
typedef struct TaskArgs
{
    int client_fd;
    MYSQL *mysql;
} TaskArgs;

//socket初始化
static int inet_init(int *sockfd);
//数据库初始化和建立表
static int mysql_Table_Init(MYSQL **recvSQL);
/* 接收请求并分类/处理请求 */
void *handleRequest(void* arg);
/* 用户注册 */
static int userRegister(int client_fd, json_object *json, MYSQL *mysql);
/* 用户登录 */
static int userLogin(int client_fd, json_object *json,  MYSQL *mysql);
/* 数据库查询 */
static int sqlQuery(const char *sql, MYSQL *mysql, MYSQL_RES **res);
/* 获取用户的群组和好友列表 */
static int getUserInfo(const char *name, json_object *json,  MYSQL *mysql);
//获取当前时间
static char *getCurTime();


//socket初始化
static int inet_init(int *sockfd)
{
    /* 初始化服务 */ 
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket error");
        return SOCKET_ERROR;
    }
    // 启用端口复用
    int reuseaddr = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1)
    {
        perror("setsockopt error");
        close(server_fd);
        return SETSOCKOPT_ERROR;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* 绑定服务器信息 */
    int ret = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        perror("bind error");
        return CONNECT_ERROR;
    }

    /* 开始监听 */
    ret = listen(server_fd, 10);
    if (ret < 0)
    {
        perror("listen error");
        return CONNECT_ERROR;
    }
    printf("server start success\n");

    *sockfd = server_fd;
    return SUCCESS;
}

//数据库初始化和建立表
static int mysql_Table_Init(MYSQL **recvSQL)
{
    /* 初始化数据库 */
    MYSQL *mysql = mysql_init(NULL);
    if (mysql == NULL)
    {
        fprintf(stderr, "mysql_init failed: %s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }

    /* 连接数据库 */
    mysql_real_connect(mysql, "localhost", "root", "1234", "demo", 3306, NULL, 0);
    if (mysql == NULL)
    {
        fprintf(stderr, "mysql_real_connect failed: %s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    printf("database connect success\n");

    /* 设置字符集 */
    mysql_set_character_set(mysql, "utf8");
    /* 设置编码 */
    mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8");
    
    /* 建表 */
    char sql[MAX_SQL_LEN] = {0};
    sprintf(sql, "create table if not exists users(client_fd int primary key, name varchar(%d),\
    password varchar(%d))", NAME_SIZE, PASSWORD_SIZE);
    int sql_ret = mysql_query(mysql, sql);
    if (sql_ret != 0)
    {
        fprintf(stderr, "create table failed: %s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    memset(sql, 0, sizeof(sql));

    /* 好友关系表 */
    sprintf(sql, "create table if not exists friends(id int primary key auto_increment, \
    name varchar(%d), friend_name varchar(%d), messages_num int(2))", NAME_SIZE,NAME_SIZE);
    sql_ret = mysql_query(mysql, sql);
    if (sql_ret != 0)
    {
        fprintf(stderr, "create friends table failed: %s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }   
    memset(sql, 0, sizeof(sql));

    /* 群组表 */
    sprintf(sql, "create table if not exists chatgroups(id int primary key auto_increment,\
    groupMainName varchar(%d), group_name varchar(%d))", NAME_SIZE, NAME_SIZE);  // groups 这个表名不能用
    sql_ret = mysql_query(mysql, sql);
    if (sql_ret != 0)
    {
        fprintf(stderr, "create chatgroups table failed: %s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    memset(sql, 0, sizeof(sql));

    /* 群组成员表 */
    sprintf(sql, "create table if not exists group_members(id int primary key auto_increment,\
    group_name varchar(%d), member_name varchar(%d))", NAME_SIZE, NAME_SIZE);
    sql_ret = mysql_query(mysql, sql);
    if (sql_ret != 0)
    {
        fprintf(stderr, "create group_members table failed: %s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    memset(sql, 0, sizeof(sql));

    /* 在线用户表 */
    sprintf(sql, "create table if not exists online_users(id int primary key auto_increment,\
    name varchar(%d), client_fd int)", NAME_SIZE);
    sql_ret = mysql_query(mysql, sql);
    if (sql_ret != 0)
    {

        fprintf(stderr, "create online_users table failed: %s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    memset(sql, 0, sizeof(sql));

    /* 消息表 */
    sprintf(sql, "create table if not exists messages(id int primary key auto_increment, \
    sender_name varchar(%d), receiver_name varchar(%d), message varchar(%d), send_time datetime)", NAME_SIZE, NAME_SIZE, CONTENT_SIZE);
    sql_ret = mysql_query(mysql, sql);
    if (sql_ret != 0)
    {

        fprintf(stderr, "create messages table failed: %s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    memset(sql, 0, sizeof(sql));

    //添加触发器
    //自动增加未读消息数
    sprintf(sql, "create trigger if not exists after_message_insert after insert on messages \
    for each row \
    update friends set messages_num = messages_num + 1 where name = new.receiver_name \
    and friend_name = new.sender_name");
    sql_ret = mysql_query(mysql, sql);
    if (sql_ret != 0)
    {

        fprintf(stderr, "create trigger table failed: %s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    memset(sql, 0, sizeof(sql));
    
     /* 上线后清除未读消息数 */
    sprintf(sql, "create trigger if not exists after_online_insert after insert on online_users \
    for each row \
    update friends set messages_num = 0 where name = new.name");
    if(mysql_query(mysql, sql) != 0)
    {
        printf("create after_online_insert trigger error:%s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    memset(sql, 0, sizeof(sql));

    /* 将系统(SYSTEM)添加为新注册账号的单向好友 */
    sprintf(sql, "create trigger if not exists after_register_insert after insert on users \
    for each row \
    insert into friends(name, friend_name, messages_num) values(new.name, 'SYSTEM', 0)");
    if(mysql_query(mysql, sql) != 0)
    {
        printf("create after_register_insert trigger error:%s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    memset(sql, 0, sizeof(sql));

    /* 建群时默认把群主加到群成员表 */
    sprintf(sql, "create trigger if not exists after_groupMain_insert after insert on chatgroups \
    for each row \
    insert into group_members(group_name, member_name, messages_num) values(new.group_name, new.groupMainName, 0)");
    if(mysql_query(mysql, sql) != 0)
    {
        printf("create after_group_insert trigger error:%s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    memset(sql, 0, sizeof(sql));
    // printf("create trigger success\n");

    *recvSQL = mysql;
    return SUCCESS;
}

int main(int argc, char *argv[])
{
    //初始化线程池
    thread_poll_t poll;
    threadPollInit(&poll, MIN_POLL_NUM, MAX_POLL_NUM, MAX_QUEUE_NUM);

    int server_fd;
    int retser = inet_init(&server_fd);
    if(retser == CONNECT_ERROR)
    {
        printf("init error\n");
    }
    printf("inet_init success\n");
  
    MYSQL *mysql;
    int retsql = mysql_Table_Init(&mysql);
    if(retsql == DATABASE_ERROR)
    {
        printf("mysal_table_init error");
    } 
    printf("mysql_Table_Init success\n");

    /* 创建套接字 */
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    /* 接收请求 */
    socklen_t client_addr_len = sizeof(client_addr);
    /* 启动服务 */
    while (1)
    {
       
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0)
        {
            perror("accept error");
            continue;
        }
        printf("client connect success\n");
    
        TaskArgs *args = (TaskArgs *)malloc(sizeof(TaskArgs));
        if(args == NULL)
        {
            perror("TaskArgs malloc error");
            continue;
        }
        args->client_fd = client_fd;
        args->mysql = mysql;
        //添加到任务队列
        threadPollAddTask(&poll, handleRequest, (void*)args);   
    }
    /* 释放线程池 */
    threadPollDestroy(&poll);
    close(server_fd);
    return 0;
}

/* 处理请求 */
void *handleRequest(void* arg)
{
    /* 线程分离 */
    pthread_detach(pthread_self());
    int client_fd = ((TaskArgs*)arg)->client_fd;
    MYSQL *mysql = ((TaskArgs*)arg)->mysql;
    // pthread_t main_tid = pthread_self();
    // printf("Main Thread ID: %lu\n", main_tid);
    char recvJson[CONTENT_SIZE] = {0};
    while(1)
    {
        /* 接收json字符串 */
        int ret = recv(client_fd, recvJson, CONTENT_SIZE, 0);
        if (ret == -1)
        {
            perror("recv error");
            return NULL;
        }
        if(ret == 0)
        {
            #if 1
            char sql[MAX_SQL_LEN] = {0};
            sprintf(sql, "select name from online_users where client_fd='%d'", client_fd);
            printf("sql: %s\n", sql);
            MYSQL_RES *res = NULL;
            int sql_ret = sqlQuery(sql, mysql, &res);
            if (sql_ret != 0)
            {
                printf("sql query error\n");
            }
            MYSQL_ROW row =  mysql_fetch_row(res);
            if(row == NULL)
            {
                printf("在线人数为0\n");
                // break;
                return NULL;
            }
            else
            {
                const char *useName = row[0];
                printf("用户%s下线\n", useName);
                //删除
                char sql[MAX_SQL_LEN] = {0};
                sprintf(sql, "delete from online_users where name='%s'", useName);
                int sql_ret = mysql_query(mysql, sql);
                if (sql_ret != 0)
                {
                    fprintf(stderr, "sql query error:%s\n", mysql_error(mysql));
                    return NULL;
                }
            }
            #endif
            
            //释放参数
            if(arg != NULL)
            {
                free(arg);
                arg = NULL;
            }
            //释放线程
            close(client_fd);
            mysql_free_result(res);
            return NULL;
        }
        printf("recv json: %s\n", recvJson);
        /* 解析json */
        json_object *jobj = json_tokener_parse(recvJson);
        if (jobj == NULL)
        {
            printf("json parse error\n");
            return NULL;
        }
        /* 获取json中的type */
        json_object *type = json_object_object_get(jobj, "type");
        if (type == NULL)
        {
            printf("json type error\n");
            return NULL;
        }
        /* 获取json中的内容 */
        const char *typeStr = json_object_get_string(type);
        if (typeStr == NULL)
        {
            printf("json type error\n");
            return NULL;
        }
        /* 分类处理请求 */
        if(strcmp(typeStr, "register") == 0)
        {
            /* 注册 */
            /* 消除没用的请求类型*/
            json_object_object_del(jobj, "type");
            userRegister(client_fd, jobj, mysql);
        }
        else if(strcmp(typeStr, "login") == 0)
        {
            /* 登录 */
            /* 消除没用的请求类型*/
            json_object_object_del(jobj, "type");
            userLogin(client_fd, jobj, mysql);
        }
         else if(strcmp(typeStr, "private") == 0)
        {
            /* 私聊 */
            /* 消除没用的请求类型*/
            json_object_object_del(jobj, "type");
        }
        else if(strcmp(typeStr, "group") == 0)
        {
            /* 其他 */
            printf("json type error\n");
            return NULL;
        }
    }
    pthread_exit(NULL);
}

/* 注册 */
static int userRegister(int client_fd, json_object *json,  MYSQL *mysql)
{
    printf("注册\n");
    int cfd = client_fd;
    const char *name = json_object_get_string(json_object_object_get(json, "name"));
    const char *password = json_object_get_string(json_object_object_get(json, "password"));
    printf("name: %s\n", name);
    printf("password: %s\n", password);

    /* 返回用json */
    json_object *returnJson = json_object_new_object();

    /* 查询数据库 */
    char sql[MAX_SQL_LEN] = {0};
    sprintf(sql, "select * from users where name='%s'", name);
    printf("sql: %s\n", sql);
    MYSQL_RES *res = NULL;
    int sql_ret = sqlQuery(sql, mysql, &res);
    if (sql_ret != 0)
    {
        printf("sql query error\n");
        json_object_object_add(returnJson, "receipt", json_object_new_string("fail"));
        json_object_object_add(returnJson, "reason", json_object_new_string("数据库查询错误"));
    }
    else
    {
        /* 处理数据库查询结果 */
        int num_rows = mysql_num_rows(res);     // 行数
        if (num_rows > 0)
        {
            json_object_object_add(returnJson, "receipt", json_object_new_string("fail"));
            json_object_object_add(returnJson, "reason", json_object_new_string("用户名已存在"));
        }
        else
        {
            /* 插入数据库 */
            sprintf(sql, "insert into users(client_fd, name, password) values('%d', '%s', '%s')", cfd, name, password);
            sql_ret = mysql_query(mysql, sql);
            if (sql_ret != 0)
            {
                printf("sql insert error\n");
                json_object_object_add(returnJson, "receipt", json_object_new_string("fail"));
                json_object_object_add(returnJson, "reason", json_object_new_string("数据库插入错误"));
            }
            else
            {
                printf("sql: %s\n", sql);
                json_object_object_add(returnJson, "receipt", json_object_new_string("success"));
                json_object_object_add(returnJson, "name", json_object_new_string(name));
            }
        }
    }

    const char *sendJson = json_object_to_json_string(returnJson);
    printf("send json: %s\n", sendJson);
    /* 发送json */
    int ret = send(client_fd, sendJson, strlen(sendJson), 0);

    /* 释放json */
    json_object_put(json);
    json_object_put(returnJson);

    /* 释放结果集 */
    if (res != NULL)
    {
        mysql_free_result(res);
        res = NULL;
    }

    if (ret == -1)
    {
        perror("send error");
        return SEND_ERROR;
    }
    return SUCCESS;
}

/* 登录 */
static int userLogin(int client_fd, json_object *json,  MYSQL *mysql)
{
    printf("登录\n");
    /* 返回用json */
    json_object *returnJson = json_object_new_object();
    /* 获取json中的内容 */
    const char *name = json_object_get_string(json_object_object_get(json, "name"));
    const char *password = json_object_get_string(json_object_object_get(json, "password"));
    printf("name: %s\n", name);
    printf("password: %s\n", password);
    /* 查询数据库 */
    char sql[MAX_SQL_LEN] = {0};
    sprintf(sql, "select password from users where name='%s'", name);
    printf("sql: %s\n", sql);
    MYSQL_RES *res = NULL;
    int sql_ret = sqlQuery(sql, mysql, &res);
    if (sql_ret != 0)
    {
        printf("sql query error:%s\n", mysql_error(mysql));
        json_object_object_add(returnJson, "receipt", json_object_new_string("fail"));
        json_object_object_add(returnJson, "reason", json_object_new_string("数据库查询错误"));
    }
    else
    {
        /* 处理数据库查询结果 */
        int num_rows = mysql_num_rows(res);     // 行数
        if (num_rows > 0)
        {
            /* 判断密码是否正确 */
            MYSQL_ROW row = mysql_fetch_row(res);
            const char *dbPassword = row[0];
            if (strcmp(password, dbPassword) == 0)
            {
                /* 登录成功 */
                json_object_object_add(returnJson, "receipt", json_object_new_string("success"));
                json_object_object_add(returnJson, "name", json_object_new_string(name));
                getUserInfo(name, returnJson, mysql);
                
                //记录登录状态
                char sql[MAX_SQL_LEN] = {0};
                sprintf(sql, "insert into online_users(name, client_fd) values('%s', '%d')", name, client_fd);
                int insert_ret = sqlQuery(sql, mysql, &res);
                if (insert_ret != 0)
                {
                    printf("insert online failed");
                }
                printf("insert online success\n");

            }
            else
            {
                /* 密码错误 */
                json_object_object_add(returnJson, "receipt", json_object_new_string("fail"));
                json_object_object_add(returnJson, "reason", json_object_new_string("密码错误"));
            }
        }
        else
        {
            /* 用户名不存在 */
            json_object_object_add(returnJson, "receipt", json_object_new_string("fail"));
            json_object_object_add(returnJson, "reason", json_object_new_string("用户名不存在"));
        }
    }
    /* 发送json */
    const char *sendJson = json_object_to_json_string(returnJson);
    printf("send json: %s\n", sendJson);
    int ret = send(client_fd, sendJson, strlen(sendJson), 0);
    /* 释放json */
    json_object_put(json);
    json_object_put(returnJson);
    /* 释放结果集 */
    if (res != NULL)
    {
        mysql_free_result(res);
        res = NULL;
    }
    if (ret == -1)
    {
        perror("send error");
        return SEND_ERROR;
    }
    return SUCCESS;
}

/* 获取用户的群组和好友列表 */
static int getUserInfo(const char *name, json_object *json,  MYSQL *mysql)
{
    MYSQL_RES *res = NULL;
    char sql[MAX_SQL_LEN] = {0};
    /* 查好友列表 */
    sprintf(sql, "select friend_name,messages_num from friends where name='%s'", name);
    int sql_ret = sqlQuery(sql, mysql, &res);
    if (sql_ret != 0)
    {
        printf("sql query error:%s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    MYSQL_ROW row;
    int num_rows = mysql_num_rows(res);     // 行数
    int i = 0;
    json_object *friends = json_object_new_object();
    while ((row = mysql_fetch_row(res)))
    {
        json_object_object_add(friends, row[0], json_object_new_int(atoi(row[1])));
        i++;
        if (i == num_rows)
        {
            break;
        }
    }
    json_object_object_add(json, "friends", friends);
    // mysql_free_result(res);
    /* 释放结果集 */
    if (res != NULL)
    {
        mysql_free_result(res);
        res = NULL;
    }
    /* 查群组列表 */
    sprintf(sql, "select group_name from group_members where groupMainName='%s'", name);
    sql_ret = sqlQuery(sql, mysql, &res);
    if (sql_ret != 0)
    {
        fprintf(stderr, "insert failed: %s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    num_rows = mysql_num_rows(res);     // 行数
    i = 0;
    json_object *groups = json_object_new_array();
    while ((row = mysql_fetch_row(res)))
    {
        json_object *group = json_object_new_object();
        json_object_object_add(group, "name", json_object_new_string(row[1]));
        json_object_array_add(groups, group);
        i++;
        if (i == num_rows)
        {
            break;
        }
    }
    json_object_object_add(json, "groups", groups);
    /* 释放结果集 */
    if (res != NULL)
    {
        mysql_free_result(res);
        res = NULL;
    }
}

/* 数据库查询 */
static int sqlQuery(const char *sql, MYSQL *mysql, MYSQL_RES **res)
{
    int sql_ret = mysql_query(mysql, sql);
    if (sql_ret != 0)
    {
        fprintf(stderr, "insert failed: %s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    *res = mysql_store_result(mysql);
    if (*res == NULL)
    {
        fprintf(stderr, "insert failed: %s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    return SUCCESS;
}

/* 更新用户在线状态 */
// static int updateUserStatus(const char *name, int status, MYSQL *mysql)
static int updateUserStatus(int client_fd, json_object *json,  MYSQL *mysql)
{
    char sql[MAX_SQL_LEN] = {0};
    MYSQL_RES *res = NULL;
    
    /* 查询数据库 */
    // sprintf(sql, "insert  from online_users where name='%s'", );
    printf("sql: %s\n", sql);
    int sql_ret = sqlQuery(sql, mysql, &res);
    if (sql_ret != 0)
    {
        printf("sql query error:%s\n", mysql_error(mysql));
        return DATABASE_ERROR;
    }
    const char *name = json_object_get_string(json_object_object_get(json, "name"));
    sprintf(sql, "select '%s' from users", name);
    sql_ret = sqlQuery(sql, mysql, &res);

}

/* 好友私聊 */
static int privateChat(int client_fd, json_object *json,  MYSQL *mysql)
{
    //返回消息
    json_object *returnObj = json_object_new_object();
    //转发消息
    json_object *forwardObj = json_object_new_object();
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;

    //获取json信息
    const char * name = json_object_get_string(json_object_object_get(json, "name"));
    const char * friendName = json_object_get_string(json_object_object_get(json, "friendName"));
    const char * message = json_object_get_string(json_object_object_get(json, "message"));
    printf("name:%s friendname:%s \n message:%s\n", name, friendName, message);

    char sql[MAX_SQL_LEN] = {0};
    /* 查好友fd */
    sprintf(sql, "select client_fd from online_users where name='%s'", friendName);
    int sql_ret = sqlQuery(sql, mysql, &res);
    if (sql_ret != 0)
    {
        printf("sql query error:%s\n", mysql_error(mysql));
        return DATABASE_ERROR;
        //返回错误信息

    }
    else
    {
        int num_rows = mysql_num_rows(res);
        printf("num_rows:%d\n", num_rows);
        if(num_rows > 0)
        {
            row = mysql_fetch_row(res);//发送消息
            int friend_fd = atoi(row[0]);//获取好友的fd  将字符串转换为整型
            json_object_object_add(forwardObj, "type", json_object_new_string("private"));
            json_object_object_add(forwardObj, "name", json_object_new_string(name));
            json_object_object_add(forwardObj, "messge", json_object_new_string(message));
            json_object_object_add(forwardObj, "time", json_object_new_string(getCurTime()));

            const char *forwardObjStr = json_object_to_json_string(forwardObj);
            int retSend = send(friend_fd, forwardObjStr, strlen(forwardObjStr), 0);//转发给好友
            if(retSend == -1)
            {
                perror("send error");
                return DATABASE_ERROR;
            }
            else
            {
                
            }
            
        }
        
    }
}

//获取当前时间
static char *getCurTime()
{
    time_t currentTime;
    struct tm *timeinfo;
    static char timeString[TIME_LEN] = {0};//将timeString改为静态变量
    time(&currentTime);  //获取当前时间
    timeinfo = localtime(&currentTime);
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", timeinfo);
    return timeString;
}
