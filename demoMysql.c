#include <mysql/mysql.h>    // 编译时加 -lmysqlclient
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[])
{
    /* 连接数据库 */
    MYSQL *conn;    // 连接句柄
    conn = mysql_init(NULL);    // 初始化句柄, 参数是句柄; 返回值是句柄; 
    if(conn == NULL)
    {
        printf("mysql_init failed!\n");
        return -1;
    }
    
    mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8");    // 设置数据库
    mysql_real_connect(conn, "localhost", "root", "52671314", "test", 3306, NULL, 0);   // 连接数据库; 参数依次是 句柄, 主机名, 用户名, 密码, 数据库名, 端口号, 客户端地址, 是否开启ssl;
    if(conn == NULL)
    {
        printf("mysql_real_connect failed!\n");
        return -1;
    }
    printf("mysql_real_connect success!\n");
    /* 建表 */
    char sql[1024] = {0};   // 用于存放sql语句的字符串数组;
    sprintf(sql, "create table if not exists test(id int primary key auto_increment, name varchar(20))");   // 塞入sql语句;
    int ret = mysql_query(conn, sql);   // 执行sql语句; 参数是句柄和sql语句;
    if(ret != 0)
    {
        printf("mysql_query failed!\n");
        return -1;
    }
    printf("mysql_query success!\n");
    /* 插入数据 */
    sprintf(sql, "insert into test(name) values('test')");
    ret = mysql_query(conn, sql);
    if(ret != 0)
    {
        printf("mysql_query failed!\n");
        return -1;
    }
    printf("mysql_query success!\n");
    printf("pConn:%p\n", conn);
    /* 查询数据 */
    sprintf(sql, "select name from test where name = 'test';");
    ret = mysql_query(conn, sql);
    printf("ret:%d\n", ret);
    if(ret != 0)
    {
        printf("mysql_query failed!\n");
        return -1;
    }
    MYSQL_RES *res = mysql_store_result(conn);  // 获取结果集; 参数是句柄; 返回值是结果集的句柄;
    if(res == NULL)
    {
        printf("mysql_store_result failed!\n");
        return -1;
    }
    printf("mysql_store_result success!\n");
    printf("pConn:%p\n", conn);
    int num = mysql_num_rows(res);      // 获取结果集的行数; 参数是结果集的句柄; 返回值是代表结果集的行数的整数;
    int col = mysql_num_fields(res);    // 获取结果集的列数; 参数是结果集的句柄; 返回值是代表结果集的列数的整数;
    MYSQL_ROW row;                      // 用于存放结果集的一行数据;
    while((row = mysql_fetch_row(res)) != NULL)   // 获取结果集的一行数据; 参数是结果集的句柄; 返回值是结果集的一行数据的指针;
    {
        for(int i = 0; i < col; i++)
        {
            printf("%s\t", row[i]);
            if(i == col - 1)
            {
                printf("\n");
            }
        }
    }
    mysql_free_result(res);         // 释放结果集; 参数是结果集的句柄;
    /* 关闭连接 */      
    mysql_close(conn);              // 关闭连接; 参数是句柄;
    printf("mysql_close success!\n");
    /* 释放资源 */  
    mysql_library_end();            // 释放资源; 返回值是0;
    printf("mysql_library_end success!\n");
    /* 退出 */
    // system("pause");
    // exit(0);

    return 0;
}