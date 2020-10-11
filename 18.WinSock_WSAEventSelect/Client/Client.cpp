#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<WINSOCK2.H>
#include <windows.h> 

#include<iostream>
#include<string>
using namespace std;

#pragma comment(lib,"WS2_32.lib")

#define SERVER_IPADRESS "127.0.0.1"
#define PORT 8001
#define BUFFER_SIZE 1024

VOID debugLog(const char* logStr) {
	cout << logStr << "Error Code: " << WSAGetLastError() << endl;
}

BOOL InitSocket()
{
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		cout << "InitSocket() failed!" << endl;
		return FALSE;
	}

	return TRUE;
}

SOCKET ConnectToServer()
{
	// 创建socket  
	// AF_INET: IPv4 网络协议的套接字类型;
	// SOCK_STREAM:提供面向连接的稳定数据传输，即TCP协议;
	SOCKET c_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == c_Socket)
	{
		debugLog("[ERROR] Create Socket Error!");
		return INVALID_SOCKET;
	}

	// 指定服务端的地址  
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = inet_addr(SERVER_IPADRESS);
	server_addr.sin_port = htons(PORT);

	// 建立连接
	if (SOCKET_ERROR == connect(c_Socket, (LPSOCKADDR)&server_addr, sizeof(server_addr)))
	{
		debugLog("[ERROR] Can Not Connect To Server IP!\n");
		closesocket(c_Socket);
		return INVALID_SOCKET;
	}
	return c_Socket;
}

int main()
{
	int ret = 0;;
	char buf[BUFFER_SIZE];

	// 1. Initial Socket环境
	if (FALSE == InitSocket()) {
		debugLog("InitSocket failed!");
		return -1;
	}

	// 2. Connect to server
	SOCKET socketHost = ConnectToServer();
	if (INVALID_SOCKET == socketHost) {
		debugLog("ConnectToServer failed!");
		return -1;
	}

	while (true) {
		//向服务器发送字符串
		cout << "input a string to send:" << endl;

		//接收输入的数据
		std::string str;
		std::cin >> str;

		ZeroMemory(buf, BUFFER_SIZE);
		strcpy(buf, str.c_str());
		if (strcmp(buf, "quit") == 0) {
			cout << "quit!" << endl;
			break;
		}

		while (true) {
			ret = send(socketHost, buf, strlen(buf), 0);
			if (SOCKET_ERROR == ret) {
				debugLog("send failed!");
				closesocket(socketHost);
				WSACleanup();
				return -1;
			}
			break;
		}
	}

	return 0;
}