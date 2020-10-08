#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>

#include <WinSock2.h>
#pragma comment ( lib,"WS2_32.lib" )  

#define RECEIVER_ADDRESS "127.0.0.1"
#define PORT 8000

int main()
{
	WSAData wasData;
	SOCKET serverSocket;
	SOCKADDR_IN serverAddr;
	char recvBuf[1024] = { 0 };
	int  recvBufLength = 1024;

	SOCKADDR_IN clientAddr;
	memset(&clientAddr, 0, sizeof(clientAddr));
	int clientAddrLength = sizeof(clientAddr);
	char  senderInfo[30] = { 0 };
	int              ret = -1;

	// 初始化
	WSAStartup(MAKEWORD(2, 2), &wasData);

	// 创建套接字
	// 套接字类型—：UDP/IP-SOCK_DGRAM
	// 协议：UDP-IPPROTO_UDP
	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (SOCKET_ERROR == serverSocket) {
		printf("[ERROR] Create Socket Error, Error Code: 0x%x!", WSAGetLastError());
		WSACleanup();
		goto exit;
	}

	// 创建一个SOCKADDR_IN结构，指定接收端地址信息
	serverAddr.sin_family = AF_INET;   // 使用IP地址族
	serverAddr.sin_port = htons(PORT); // 端口号
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// bind将地址信息和套接字关联
	ret = bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (0 != ret) {
		printf("[ERROR] bind error with Error Code 0x%x\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		goto exit;
	}

	// 接收数据报
	while (1) {
		ret = recvfrom(serverSocket, recvBuf, recvBufLength, 0, (SOCKADDR*)&clientAddr, &clientAddrLength);
		if (ret > 0) {
			printf("recv info：%s\n", recvBuf);
		}
		else if (ret == -1) {
			printf("[ERROR] recvfrom error with Error Code 0x%x\n", WSAGetLastError());
			closesocket(serverSocket);
			WSACleanup();
			goto exit;
		}
		else {

		}
	}

exit:
	closesocket(serverSocket);
	system("pause");
	return 0;
}