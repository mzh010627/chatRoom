#include <stdio.h>
#include <stdlib.h>
#include <openssl/md5.h>
//gcc 编译 -lcrypto

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
    unsigned char md5Digest[MD5_DIGEST_LENGTH];
    calculateMD5("my_file.txt", md5Digest);

    printf("MD5: ");
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) 
    {
        printf("%02x", md5Digest[i]);
    }
    printf("\n");

    return 0;
}
