#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<error.h>
#include<netinet/in.h>
#include<unistd.h>
#include<mysql/mysql.h> 
#include <json-c/json.h>

#define SERVER_PORT 8082
#define MAX_LISTEN 128
#define BUFFER_SIZE 128
#define SQL_SIZE 1024
#define NAME_MAX_SIZE 20
#define PASSWORD_MAX_SIZE 20
#define CONTENT_SIZE 1024

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

/* 处理请求函数 */
void * handleReques();

/* 数据库查询函数 */
static int sqlQuery();

/* 注册 */
static int userRegister();

/* 登录 */
static int userLogin();

/* 获取用户好友列表 */
static int getUserInfo();

/* 私聊 */
static int userPrivate();

/* 群聊 */
static int userGroup();

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

    /* 连接数据库 */
    /* 创建数据库句柄,用于连接数据库 */
    MYSQL * database;
    /* 初始化句柄 */
    database = mysql_init(NULL);
    if (database == NULL) 
    {
        perror("mysql_init error\n");
        exit(-1);
    }

    /* 连接数据库 */
    /* 参数：句柄、 主机号、 用户名、 密码、数据库名、 端口号、 客户端地址（一般设置为空）、 一般设置为0 */
    mysql_real_connect(database, "localhost", "root", "12345678", "test", 3306, NULL, 0);
    if (database == NULL)
    {
        printf("mysql_real_connect error:%s\n",mysql_error(database));
        return DATABASE_ERROR;
    }
    printf("database connect success\n");
    
    /* 创建用户id表 */
    char sql[SQL_SIZE];  //存放SQL语句
    memset(sql, 0, sizeof(sql));
    /* 将sql语句插入*/
    sprintf(sql, "create table if not exists user(name varchar(%d) PRIMARY KEY, password varchar(%d))", NAME_MAX_SIZE, PASSWORD_MAX_SIZE);

    printf("sql:%s\n",sql);
    /* 执行sql语句 */
    ret = mysql_query(database, sql);
    if (ret == -1)
    {
        printf("mysql_real_query error:%s\n",mysql_error(database));
        return DATABASE_ERROR;
    }
    printf("user table create success\n");

    /* 创建好友列表 */
    /* 将sql语句插入*/
    sprintf(sql, "create table if not exists friends(name varchar(%d) PRIMARY KEY, friends_name varchar(%d), message_num int(2))",NAME_MAX_SIZE, NAME_MAX_SIZE);
    printf("sql:%s\n",sql);
    /* 执行sql语句 */
    ret = mysql_query(database, sql);
    if (ret == -1)
    {
        printf("mysql_real_query error:%s\n",mysql_error(database));
        return DATABASE_ERROR;
    }
    printf("friends table create success\n");

    // /* 创建群聊表 */
    // char sql[SQL_SIZE];  //存放SQL语句
    // memset(sql, 0, sizeof(sql));
    // /* 将sql语句插入*/
    // sprintf(sql, "create table if not exists groupchat(name varchar(%d) PRIMARY KEY, friends name varchar(%d), message_num int(2))",NAME_MAX_SIZE, NAME_MAX_SIZE);

    // printf("sql:%s\n",sql);
    // /* 执行sql语句 */
    // ret = mysql_query(database, sql);
    // if (ret == -1)
    // {
    //     printf("mysql_real_query error:%s\n",mysql_error(database));
    //     return DATABASE_ERROR;
    // }
    // printf("group table create success\n");

    /* 开始进行功能请求的处理 */
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
        printf("accept success\n");
        printf("acceptfd:%d\n",acceptfd);
        /* 功能请求处理 */
        handleReques(acceptfd, database);
        
    }
    /* 关闭套接字 */
    close(sockfd);
    return 0;
}

/* 功能请求处理 */
void* handleReques(int acceptfd, MYSQL * database)
{
    char recvJson[CONTENT_SIZE];
    memset(recvJson, 0, sizeof(recvJson));
    while (1)
    {
        /* 开始接收客户端发过来的json */
        int ret = recv(acceptfd, recvJson, CONTENT_SIZE, 0);
        if (ret <= 0)
        {
            perror("recv error");
            close(acceptfd);
            return NULL;
        }
        printf("acceptfd:%d\n",acceptfd);
        printf("recvJson:%s\n", recvJson);
        /* 解析服务器发送过来的json */
        json_object * object = json_tokener_parse(recvJson);
        if (object == NULL)
        {
            perror("json tokener error\n");
            return NULL;
        }
        
        /* 获取键值 */
        json_object * type = json_object_object_get(object, "type");
        if (type == NULL)
        {
            perror("json type error\n");
            return NULL;
        }
        
        /* 将键值转换成字符串 */
        const char * typeStr = json_object_get_string(type);
        if (typeStr == NULL)
        {
            perror("json typeStr error\n");
            return NULL;
        }
        /* 判断请求类型 */
        /* 注册 */
        if (strcmp(typeStr, "register") == 0)
        {
            userRegister(acceptfd, object, database);
        }
        /* 登录 */
        else if(strcmp(typeStr, "login") == 0)
        {
            userLogin(acceptfd, object, database);
        }
         /* 查询好友列表发送给服务器 */
        else if(strcmp(typeStr, "showFriends") == 0)
        {
            getUserInfo(acceptfd, object, database);
        }
        /* 私聊 */
        else if(strcmp(typeStr, "private") == 0)
        {
            //userPrivate();
        }
        /* 群聊 */
        else if(strcmp(typeStr, "group") == 0)
        {
            //userGroup();
        }

         
    }   
}

/* 数据库查询函数 */
static int sqlQuery(const char * sql, MYSQL * database, MYSQL_RES **res)
{
    int sql_ret = mysql_query(database, sql);
    if (sql_ret != 0)
    {
        printf("sql: %s\n", sql);
        printf("sql_ret:%d\n", sql_ret);
        printf("sqlQuery error:%s\n", mysql_error(database));
        return DATABASE_ERROR;
    }
    /* mysql_store_result 是 MySQL C API 中的一个函数，用于将执行查询后返回的结果集存储在 MYSQL_RES 结构体中 */
    *res = mysql_store_result(database);
    if (*res == NULL)
    {
        printf("sql store result error:%s\n", mysql_error(database));
        return DATABASE_ERROR;
    }
    return SUCCESS;
}

/* 注册 */
static int userRegister(int acceptfd, json_object * json, MYSQL * database)
{
    /* 将收到的json转化为字符串 */
    const char * name = json_object_get_string(json_object_object_get(json, "name"));
    if (name == NULL)
    {
        perror("json_name get error\n");
        return JSON_ERROR;
    }
    const char *password = json_object_get_string(json_object_object_get(json, "password"));
    if (password == NULL)
    {
        perror("json_password get error\n");
        return JSON_ERROR;
    }
    printf("name: %s\n", name);
    printf("password: %s\n", password);

    /* 创建反馈返回json */
     json_object *returnJson = json_object_new_object();

    /* 查询数据库 */
    char sql[SQL_SIZE] = {0};
    sprintf(sql, "select * from user where name='%s'", name);
    printf("sql: %s\n", sql);
    MYSQL_RES *res = NULL;

    /* 数据库查询函数 */
    int sql_ret = sqlQuery(sql, database, &res);
    if (sql_ret != 0)
    {
        printf("sql query error:%s\n", mysql_error(database));
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
            sprintf(sql, "insert into user(name, password) values('%s', '%s')", name, password);
            sql_ret = mysql_query(database, sql);
            if (sql_ret != 0)
            {
                printf("sql insert error:%s\n", mysql_error(database));
                json_object_object_add(returnJson, "receipt", json_object_new_string("fail"));
                json_object_object_add(returnJson, "reason", json_object_new_string("数据库插入错误"));
            }
            else
            {
                printf("sql: %s\n", sql);
                json_object_object_add(returnJson, "receipt", json_object_new_string("success"));
                json_object_object_add(returnJson, "type", json_object_new_string("register"));
            }
        }
    }
    const char *sendJson = json_object_to_json_string(returnJson);
    printf("send json: %s\n", sendJson);
    /* 发送json */
    int ret = send(acceptfd, sendJson, strlen(sendJson), 0);
    printf("acceptfd:%d\n", acceptfd);
    printf("ret:%d\n",ret);
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
static int userLogin(int acceptfd, json_object * json, MYSQL * database)
{
    
    /* 将收到的json转化为字符串 */
    const char * name = json_object_get_string(json_object_object_get(json, "name"));
    if (name == NULL)
    {
        perror("json_name get error\n");
        return JSON_ERROR;
    }
    const char *password = json_object_get_string(json_object_object_get(json, "password"));
    if (password == NULL)
    {
        perror("json_password get error\n");
        return JSON_ERROR;
    }
    printf("name: %s\n", name);
    printf("password: %s\n", password);

    /* 创建反馈返回json */
     json_object *returnJson = json_object_new_object();

    /* 查询数据库 */
    char sql[SQL_SIZE] = {0};
    sprintf(sql, "select * from user where name='%s'", name);
    printf("sql: %s\n", sql);
    MYSQL_RES *res = NULL;
    int sql_ret = sqlQuery(sql, database, &res);//数据库查询函数
    if (sql_ret != 0)
    {
        printf("sql query error:%s\n", mysql_error(database));
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
            const char *dbPassword = row[1];
            printf("client:%s,Db:%s\n", password, dbPassword);
            if (strcmp(password, dbPassword) == 0)
            {
                /* 登录成功 */
                json_object_object_add(returnJson, "receipt", json_object_new_string("success"));
                json_object_object_add(returnJson, "type", json_object_new_string("login"));
                getUserInfo(acceptfd, returnJson, database);

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
    const char *sendJson = json_object_to_json_string(returnJson);
    printf("send json: %s\n", sendJson);
    /* 发送json */
    int ret = send(acceptfd, sendJson, strlen(sendJson), 0);
    printf("acceptfd:%d\n", acceptfd);
    printf("ret:%d\n",ret);
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

/* 获取好友列表 */
static int getUserInfo(int acceptfd, json_object *json,  MYSQL *database)
{
    /* 将收到的json转化为字符串 */
    const char * name = json_object_get_string(json_object_object_get(json, "name"));
    if (name == NULL)
    {
        perror("json_name get error\n");
        return JSON_ERROR;
    }
    MYSQL_RES *res = NULL;
    char sql[SQL_SIZE] = {0};
    /* 查好友列表 */
    sprintf(sql, "select friend_name,messages_num from friends where name='%s'", name);
    int sql_ret = sqlQuery(sql, database, &res);
    if (sql_ret != 0)
    {
        printf("sql query error:%s\n", mysql_error(database));
        return DATABASE_ERROR;
    }
    MYSQL_ROW row;
    int num_rows = mysql_num_rows(res);     // 行数
    int idx = 0;
    json_object *friends = json_object_new_object();
    while ((row = mysql_fetch_row(res)))
    {
        json_object_object_add(friends, row[0], json_object_new_int(atoi(row[1])));
        idx++;
        if (idx == num_rows)
        {
            break;
        }
    }
    const char * sendJson = json_object_to_json_string(friends);
    printf("send json :%s\n", sendJson);
    /* 发送json */
    int ret = send(acceptfd, sendJson, strlen(sendJson),0);
    /* 释放json */
    json_object_put(json);
    json_object_put(friends);
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