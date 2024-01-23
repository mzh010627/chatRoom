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
#define SERVER_IP "172.16.157.11"   // 服务器ip,暂定为本机ip
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

};

/* 接收请求并分类/处理请求 */
static int handleRequest(int client_fd, MYSQL *mysql);
/* 用户注册 */
static int userRegister(int client_fd, json_object *json, MYSQL *mysql);
/* 用户登录 */
static int userLogin(int client_fd);
/* 数据库查询 */
static int sqlQuery(const char *sql, MYSQL *mysql, MYSQL_RES **res);

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

    /* 初始化数据库 */
    MYSQL *mysql = mysql_init(NULL);
    if (mysql == NULL)
    {
        perror("mysql_init error");
        return DATABASE_ERROR;
    }

    /* 连接数据库 */
    mysql_real_connect(mysql, "localhost", "root", "52671314", "test", 3306, NULL, 0);
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
    sprintf(sql, "create table if not exists users(id int primary key auto_increment, name varchar(20), password varchar(20))");
    int sql_ret = mysql_query(mysql, sql);
    if (sql_ret != 0)
    {
        perror("create table error");
        return DATABASE_ERROR;
    }
    printf("create table success\n");


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

        /* 处理请求 */
        handleRequest(client_fd,mysql);
        break;
    }
    close(server_fd);
    return 0;
}

/* 处理请求 */
static int handleRequest(int client_fd, MYSQL *mysql)
{
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
            printf("client disconnect\n");
            close(client_fd);
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
            userLogin(client_fd);
        }
        else
        {
            /* 其他 */
            printf("json type error\n");
            return JSON_ERROR;
        }


    }

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
        int num_fields = mysql_num_fields(res); // 列数
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

    if (ret == -1)
    {
        perror("send error");
        return SEND_ERROR;
    }
    return SUCCESS;
}

static int userLogin(int client_fd)
{
    return 0;
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