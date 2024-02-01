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


#define SERVER_PORT 8888        // 服务器端口号,暂定为8888
#define SERVER_IP "172.18.188.222"   // 服务器ip,暂定为本机ip
#define NAME_SIZE 10            // 用户名长度
#define PASSWORD_SIZE 20        // 密码长度
#define MAX_FRIEND_NUM 10       // 最大好友数量
#define MAX_GROUP_NUM 10        // 最大群组数量
#define MAX_GROUP_MEMBERS_NUM 20// 最大群组成员数量
#define CONTENT_SIZE 1024       // 信息内容长度
#define PATH_SIZE 256           // 文件路径长度
#define MAX_SQL_LEN 1024        // sql语句长度

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

//socket初始化
static int inet_init(int *sockfd);
//数据库初始化和建立表
static int mysql_Table_Init(MYSQL **recvSQL);
/* 接收请求并分类/处理请求 */
static int handleRequest(int client_fd, MYSQL *mysql);
/* 用户注册 */
static int userRegister(int client_fd, json_object *json, MYSQL *mysql);
/* 用户登录 */
static int userLogin(int client_fd, json_object *json,  MYSQL *mysql);
/* 数据库查询 */
static int sqlQuery(const char *sql, MYSQL *mysql, MYSQL_RES **res);
/* 获取用户的群组和好友列表 */
static int getUserInfo(const char *name, json_object *json,  MYSQL *mysql);

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
        perror("mysql_init error");
        return DATABASE_ERROR;
    }

    /* 连接数据库 */
    mysql_real_connect(mysql, "localhost", "root", "1234", "test", 3306, NULL, 0);
    if (mysql == NULL)
    {
        perror("mysql_real_connect error");
        return DATABASE_ERROR;
    }
    printf("database connect success\n");
    /* 设置字符集 */
    mysql_set_character_set(mysql, "utf8");
    /* 设置编码 */
    mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8");
    
    /* 建表 */
    char sql[MAX_SQL_LEN] = {0};
    sprintf(sql, "create table if not exists users(id int primary key auto_increment, name varchar(%d), password varchar(%d))", NAME_SIZE, PASSWORD_SIZE);
    int sql_ret = mysql_query(mysql, sql);
    if (sql_ret != 0)
    {
        perror("create table error");
        return DATABASE_ERROR;
    }
    printf("create users table success\n");
    /* 好友关系表 */
    sprintf(sql, "create table if not exists friends(id int primary key auto_increment, name varchar(%d), friend_name varchar(%d), messages_num int(2))", NAME_SIZE,NAME_SIZE);
    sql_ret = mysql_query(mysql, sql);
    if (sql_ret != 0)
    {
        perror("create table error");
        return DATABASE_ERROR;
    }
    printf("create friends table success\n");
    /* 群组表 */
    sprintf(sql, "create table if not exists chatgroups(id int primary key auto_increment, groupMainName varchar(%d), group_name varchar(%d))", NAME_SIZE, NAME_SIZE);  // groups 这个表名不能用
    sql_ret = mysql_query(mysql, sql);
    if (sql_ret != 0)
    {
        perror("create table error");
        return DATABASE_ERROR;
    }
    printf("create groups table success\n");
    /* 群组成员表 */
    sprintf(sql, "create table if not exists group_members(id int primary key auto_increment, group_name varchar(%d), member_name varchar(%d))", NAME_SIZE, NAME_SIZE);
    sql_ret = mysql_query(mysql, sql);
    if (sql_ret != 0)
    {
        perror("create table error");
        return DATABASE_ERROR;
    }
    *recvSQL = mysql;
    return SUCCESS;
}

int main(int argc, char *argv[])
{
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
    // printf("mysql_Table_Init success\n");

    /* 启动服务 */
    while (1)
    {
        /* 创建套接字 */
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        /* 接收请求 */
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0)
        {
            perror("accept error");
            continue;
        }
        printf("client connect success\n");

        #if 0
        /* 处理请求 */
        handleRequest(client_fd,mysql);
        #else
        #endif
    }
    close(server_fd);
    return 0;
}

/* 处理请求 */
static int handleRequest(int client_fd, MYSQL *mysql)
{
    /* 线程分离 */
    pthread_detach(pthread_self());
    pthread_t main_tid = pthread_self();
    printf("Main Thread ID: %lu\n", main_tid);
    char recvJson[CONTENT_SIZE] = {0};
    while(1)
    {
        /* 接收json字符串 */
        int ret = recv(client_fd, recvJson, CONTENT_SIZE, 0);
        if (ret == -1)
        {
            perror("recv error");
            return RECV_ERROR;
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
            char *useName = row[0];
            #endif
            
            printf("%s下线....\n", useName);
            close(client_fd);
            mysql_free_result(res);
            return SUCCESS;
        }
        printf("recv json: %s\n", recvJson);
        /* 解析json */
        json_object *jobj = json_tokener_parse(recvJson);
        if (jobj == NULL)
        {
            printf("json parse error\n");
            return JSON_ERROR;
        }
        /* 获取json中的type */
        json_object *type = json_object_object_get(jobj, "type");
        if (type == NULL)
        {
            printf("json type error\n");
            return JSON_ERROR;
        }
        /* 获取json中的内容 */
        const char *typeStr = json_object_get_string(type);
        if (typeStr == NULL)
        {
            printf("json type error\n");
            return JSON_ERROR;
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
        else
        {
            /* 其他 */
            printf("json type error\n");
            return JSON_ERROR;
        }
    }
    pthread_exit(NULL);
}

/* 注册 */
static int userRegister(int client_fd, json_object *json,  MYSQL *mysql)
{
    printf("注册\n");
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
            sprintf(sql, "insert into users(name, password) values('%s', '%s')", name, password);
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
            /* 判断密码是否正确 */
            MYSQL_ROW row = mysql_fetch_row(res);
            const char *dbPassword = row[0];
            if (strcmp(password, dbPassword) == 0)
            {
                /* 登录成功 */
                json_object_object_add(returnJson, "receipt", json_object_new_string("success"));
                json_object_object_add(returnJson, "name", json_object_new_string(name));
                getUserInfo(name, returnJson, mysql);
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
        perror("sql query error");
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
    mysql_free_result(res);
    /* 查群组列表 */
    sprintf(sql, "select group_name from group_members where groupMainName='%s'", name);
    sql_ret = sqlQuery(sql, mysql, &res);
    if (sql_ret != 0)
    {
        perror("sql query error");
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
    mysql_free_result(res);
}

/* 数据库查询 */
static int sqlQuery(const char *sql, MYSQL *mysql, MYSQL_RES **res)
{
    int sql_ret = mysql_query(mysql, sql);
    if (sql_ret != 0)
    {
        perror("sql query error");
        return DATABASE_ERROR;
    }
    *res = mysql_store_result(mysql);
    if (*res == NULL)
    {
        perror("sql store result error");
        return DATABASE_ERROR;
    }
    return SUCCESS;
}