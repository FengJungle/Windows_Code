#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <WinSock2.h>  

#define PORT 8001 ///端口号 
#define SERVER_IPADRESS "127.0.0.1" // 服务器IP地址，这里设为本机地址
#define BUFFER_SIZE 1024  
#pragma comment(lib, "WS2_32")  

int main()
{
	// 初始化socket dll  
	WSADATA wsaData;

	// MAKEWORD
	// 原型：#define MAKEWORD(a, b) ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
	// 作用：将两个byte型的a和b合并为一个word型，高8为是b，低8位是a；
	// 返回值：一个无符号的16位整形
	WORD socketVersion = MAKEWORD(2, 0);
	if (WSAStartup(socketVersion, &wsaData) != 0)
	{
		printf("[ERROR] Init socket dll error!\n");
		return -1;
	}

	// 创建socket  
	// AF_INET: IPv4 网络协议的套接字类型;
	// SOCK_STREAM:提供面向连接的稳定数据传输，即TCP协议;
	SOCKET c_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == c_Socket)
	{
		printf("[ERROR] Create Socket Error!");
		WSACleanup();
		system("pause");
		exit(1);
	}

	// 指定服务端的地址  
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = inet_addr(SERVER_IPADRESS);
	server_addr.sin_port = htons(PORT);

	// 建立连接
	if (SOCKET_ERROR == connect(c_Socket, (LPSOCKADDR)&server_addr, sizeof(server_addr)))
	{
		printf("[ERROR] Can Not Connect To Server IP!\n");
		closesocket(c_Socket);
		WSACleanup();
		return -1;
	}

	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);

	while (1) {
		// 向服务器发送消息
		memset(buffer, 0, BUFFER_SIZE);
		scanf("%s", buffer);
		if (send(c_Socket, buffer, BUFFER_SIZE, 0) < 0)
		{
			printf("[ERROR] Send message Failed with Error Code: 0x%x\n", WSAGetLastError());
			closesocket(c_Socket);
			WSACleanup();
			system("pause");
			return -1;
		}

		memset(buffer, 0, BUFFER_SIZE);
		int length = 0;
		if ((length = recv(c_Socket, buffer, BUFFER_SIZE, 0)) > 0)
		{
			printf("%s\n", buffer);
		}
	}


	closesocket(c_Socket);

	//释放winsock库  
	WSACleanup();

	system("pause");
	return 0;
}
