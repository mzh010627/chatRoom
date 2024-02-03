#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <unistd.h>
//gcc 编译 -lcrypto
//makefile 编译 make all

#define BUFFER_SIZE 1024

void calculateMD5(const char *filename, unsigned char *md5Digest) 
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL) 
    {
        printf("Error opening file.\n");
        return;
    }

    MD5_CTX md5Context;
    MD5_Init(&md5Context);

    unsigned char buffer[BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) 
    {
        MD5_Update(&md5Context, buffer, bytesRead);
    }

    MD5_Final(md5Digest, &md5Context);

    fclose(file);
}

int main() 
{
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];
    FILE *file;
    size_t bytesRead;

    // 创建套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // 配置服务器地址
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

    // 绑定套接字到指定端口
    bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    // 监听连接请求
    if (listen(serverSocket, 10) == 0)
        printf("Listening...\n");
    else
        printf("Error in listening.\n");

    // 接受客户端连接
    addr_size = sizeof(serverStorage);
    newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);

    // 打开要发送的文件
    file = fopen("large_file.dat", "rb");
    if (file == NULL) 
    {
        printf("Error opening file.\n");
        return 1;
    }

    // 计算文件的MD5值
    unsigned char md5Digest[MD5_DIGEST_LENGTH];
    calculateMD5("large_file.dat", md5Digest);

    // 发送文件的MD5值给客户端
    send(newSocket, md5Digest, MD5_DIGEST_LENGTH, 0);

    // 逐片读取并发送文件数据
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) 
    {
        send(newSocket, buffer, bytesRead, 0);
    }

    // 关闭文件和套接字
    fclose(file);
    close(newSocket);

    return 0;
}
