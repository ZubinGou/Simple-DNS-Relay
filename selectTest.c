#include <stdio.h>
#include <winsock2.h>
#pragma comment (lib, "ws2_32.lib")  //加载 ws2_32.dll
#pragma warning(disable:4996)
#define BUF_SIZE 100

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    //创建套接字
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    //绑定套接字
    sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));  //每个字节都用0填充
    servAddr.sin_family = PF_INET;  //使用IPv4地址
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); //自动获取IP地址
    servAddr.sin_port = htons(1234);  //端口
    bind(sock, (SOCKADDR*)&servAddr, sizeof(SOCKADDR));

    //接收客户端请求
    SOCKADDR clntAddr;  //客户端地址信息
    int nSize = sizeof(SOCKADDR);
    char buffer[BUF_SIZE];  //缓冲区
    fd_set fdread;
    while (1) {
        FD_ZERO(&fdread);
        FD_SET(sock, &fdread);
        TIMEVAL tv;//设置超时等待时间
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int ret = select(0, &fdread, NULL, NULL, &tv);
        if (SOCKET_ERROR == ret)
        {
            printf("select error:%d.\n", WSAGetLastError());
        }
        if (ret > 0)
        {
            if (FD_ISSET(sock, &fdread))
            {
                ret = recv(sock, buffer, BUF_SIZE, 0);
                if (SOCKET_ERROR == ret)
                {
                    printf("recv error:%d.\n", WSAGetLastError());
                }
                else if (ret == 0)
                {
                    printf("recv,socket closed/\n");
                    break;
                }
                else
                {
                    //buffer[strlen(buffer)] = 0;
                    printf("receive data:\n%s\n", "??");
                }
            }
            
        }
        //int strLen = recvfrom(sock, buffer, BUF_SIZE, 0, &clntAddr, &nSize);
        //buffer[strLen] = 0;
        //printf("%s\n", buffer);
        //sendto(sock, buffer, strLen, 0, &clntAddr, nSize);
        printf("test\n");
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}