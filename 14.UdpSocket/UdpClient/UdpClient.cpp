#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <WinSock2.h>
#pragma comment ( lib,"WS2_32.lib" )  

#define RECEIVER_ADDRESS "127.0.0.1"
#define PORT 8000
#define BUFFER_SIZE 1024

int main()
{
	WSAData wasData;
	SOCKET clientSocket;
	SOCKADDR_IN serverAddr;
	int            addrLength = 0;
	char sendBuf[BUFFER_SIZE] = { 0 };
	int         sendBufLength = BUFFER_SIZE;
	int                   ret = 0;

	// 初始化
	WSAStartup(MAKEWORD(2, 2), &wasData);

	// 创建套接字
	// 套接字类型—：UDP/IP-SOCK_DGRAM
	// 协议：UDP-IPPROTO_UDP
	clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (SOCKET_ERROR == clientSocket) {
		printf("Create Socket Error!");
		goto exit;
	}

	// 创建一个SOCKADDR_IN结构，指定接收端地址信息
	serverAddr.sin_family = AF_INET;   // 使用IP地址族
	serverAddr.sin_port = htons(PORT); // 端口号
	serverAddr.sin_addr.S_un.S_addr = inet_addr(RECEIVER_ADDRESS);

	while (1) {
		if (gets_s(sendBuf, BUFFER_SIZE)) {
			ret = sendto(clientSocket, sendBuf, sizeof(sendBuf), 0, (SOCKADDR *)&serverAddr, sizeof(serverAddr));
			if (0 == ret) {
				printf("fail to send\n");
			}
			printf("send info: %s\n", sendBuf);
			memset(sendBuf, 0, sizeof(sendBuf));
		}
	}

exit:
	closesocket(clientSocket);
	system("pause");
	return 0;
}