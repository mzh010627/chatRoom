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
#define SERVER_IP "127.0.0.1"
int main() 
{
    int clientSocket;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];
    FILE *file;
    size_t bytesReceived;

    // 创建套接字
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    // 配置服务器地址和端口
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

    // 连接到服务器
    addr_size = sizeof(serverAddr);
    connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);

    // 接收服务器发送的文件的MD5值
    unsigned char serverMd5Digest[MD5_DIGEST_LENGTH];
    recv(clientSocket, serverMd5Digest, MD5_DIGEST_LENGTH, 0);

    // 打开文件以写入接收到的数据
    file = fopen("received_file.dat", "wb");
    if (file == NULL) 
    {
        printf("Error opening file.\n");
        return 1;
    }

    // 计算接收到的文件的MD5值
    unsigned char clientMd5Digest[MD5_DIGEST_LENGTH];
    MD5_CTX md5Context;
    MD5_Init(&md5Context);

    // 接收并写入文件数据
    while ((bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0)) > 0) 
    {
        fwrite(buffer, 1, bytesReceived, file);
        MD5_Update(&md5Context, buffer, bytesReceived);
    }

    MD5_Final(clientMd5Digest, &md5Context);

    // 比较MD5值是否匹配
    if (memcmp(serverMd5Digest, clientMd5Digest, MD5_DIGEST_LENGTH) == 0) 
    {
        printf("File transfer successful. MD5 matches.\n");
    } 
    else 
    {
        printf("File transfer failed. MD5 does not match.\n");
    }

    // 关闭文件和套接字
    fclose(file);
    close(clientSocket);

    return 0;
}
