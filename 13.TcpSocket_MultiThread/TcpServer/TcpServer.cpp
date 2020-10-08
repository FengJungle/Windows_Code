
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
using namespace std;

#include <WinSock2.h>
#pragma comment ( lib,"WS2_32.lib" )  

#define RECEIVER_ADDRESS "127.0.0.1"
#define PORT 8001
#define BUFFER_SIZE 1024

// 阻塞式模式
// 每次只服务一个连接，只有在服务完当前客户连接之后，才会继续服务下一个客户端连接

/*
* 1. 先处理连接，绑定本地地址和监听
*    SOCKET Bind_Listen(int nBackLog)
* 2. 接收一个客户端连接并返回对应的连接的套接字
*    SOCKET AcceptConnection(SOCKET hSocket)
* 3. 处理一个客户端的连接，实现接收和发送数据
*    BOOL ClientConFun(SOCKET sd)
* 4. 关闭一个连接
*    BOOL CloseConnect(SOCEKT sd)
* 5. 服务器主体
*    VOID TcpServerFunc()
*/

// 阻塞式并发连接模式
// 通过多线程，可以同时服务多个连接，每一个线程处理一个客户端连接

VOID debugLog(const char* logStr) {
	cout << logStr << "Error Code: "<< WSAGetLastError() << endl;
}

BOOL InitSocket()
{
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		debugLog("InitSocket() failed!");
		return FALSE;
	}

	return TRUE;
}

SOCKET Bind_Listen(int nBackLog) {

	SOCKET serverSocket;
	SOCKADDR_IN serverAddr;

	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (SOCKET_ERROR == serverSocket) {
		debugLog("Bind_Listen -> socket failed!");
		return INVALID_SOCKET;
	}

	// 创建一个SOCKADDR_IN结构，指定接收端地址信息
	serverAddr.sin_family = AF_INET;   // 使用IP地址族
	serverAddr.sin_port = htons(PORT); // 端口号
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr))) {
		debugLog("Bind_Listen -> bind failed!");
		closesocket(serverSocket);
		return INVALID_SOCKET;
	}
	if (listen(serverSocket, nBackLog) == SOCKET_ERROR) {
		debugLog("Bind_Listen -> listen failed!");
		closesocket(serverSocket);
		return INVALID_SOCKET;
	}

	return serverSocket;
}

SOCKET AcceptConnection(SOCKET hSocket) {
	sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);

	SOCKET sd = accept(hSocket, (sockaddr *)&client_addr, &client_addr_len);
	if (INVALID_SOCKET == sd) {
		debugLog("AcceptConnection -> accept failed!");
		return INVALID_SOCKET;
	}
	return sd;
}

BOOL ClientConFun(SOCKET sd) {
	char buffer[BUFFER_SIZE] = { 0 };
	int nRetByte = 0;

	// 循环处理数据
	do {
		nRetByte = recv(sd, buffer, BUFFER_SIZE, 0);
		if (nRetByte == SOCKET_ERROR) {
			debugLog("ClientConFun -> recv failed!");
			return FALSE;
		}
		else {
			if (nRetByte != 0) {
				cout << "接收到一条数据： " << buffer << endl;
				int nSend = 0;
				while (nSend < nRetByte) {
					// 把接收到的数据返回到发送端
					int nTemp = send(sd, &buffer[nSend], nRetByte - nSend, 0);
					if (nTemp > 0) {
						nSend += nTemp;
					}
					else if (nTemp == SOCKET_ERROR) {
						debugLog("ClientConFun -> send failed!");
						return FALSE;
					}
					else {
						// Send返回0， 由于此时send<nRetByte， 即数据还没发送出去，表示客户端意外关闭了
						debugLog("ClientConFun -> send ->close error!");
						return FALSE;
					}
				}
			}
		}

	} while (nRetByte != 0);

	return TRUE;
}

BOOL CloseConnect(SOCKET sd) {
	// 首先发送一个TCP FIN 分段，向对方表明已经完成数据发送
	if (shutdown(sd, SD_SEND) == SOCKET_ERROR) {
		debugLog("CloseConnect -> shutdown error!");
		return FALSE;
	}

	char buffer[BUFFER_SIZE];
	int nRetyte = 0;
	do {
		nRetyte = recv(sd, buffer, BUFFER_SIZE, 0);
		if (nRetyte == SOCKET_ERROR) {
			debugLog("CloseConnect -> recv error!");
			break;
		}
		else if (nRetyte > 0) {
			debugLog("CloseConnect 错误地接收数据！");
			break;
		}
	} while (nRetyte != 0);

	if (SOCKET_ERROR == closesocket(sd)) {
		debugLog("CloseConnect -> closesocket error!");
		return FALSE;
	}
	return TRUE;
}

// 线程处理业务逻辑
DWORD WINAPI ClientThreadFun(LPVOID lpParam) 
{
	SOCKET sd = (SOCKET)lpParam;
	// 客户端处理
	if (ClientConFun(sd) == FALSE) {
		//break;
	}

	// 关闭一个客户端连接
	if (CloseConnect(sd) == FALSE) {
		//break;
	}

	return 0;
}

VOID TcpServerFun() {
	SOCKET hSocket = Bind_Listen(1);
	if (hSocket == INVALID_SOCKET) {
		debugLog("TcpServerFun -> Bind_Listen error!");
		return;
	}
	while (1) {
		// 返回客户端的套接字
		SOCKET sd = AcceptConnection(hSocket);
		if (sd == INVALID_SOCKET) {
			debugLog("TcpServerFun -> AcceptConnection error!");
			break;
		}
		
		// 当接收到客户端连接请求，为客户端开启一个线程
		DWORD dwThreadId;
		HANDLE hThread = CreateThread(
			NULL,
			NULL,
			(LPTHREAD_START_ROUTINE)ClientThreadFun,
			(LPVOID)sd,
			0,
			&dwThreadId
		);
		if (hThread == INVALID_HANDLE_VALUE) {
			debugLog("TcpServerFun -> CreateThread error!");
			break;
		}
		else {
			CloseHandle(hThread);
		}
	}

	if (closesocket(hSocket) == SOCKET_ERROR) {
		debugLog("TcpServerFun -> closesocket error!");
		return;
	}
}

int main()
{
	// 初始化
	InitSocket();

	// 业务数据处理
	TcpServerFun();

	// 释放
	WSACleanup();

	return 0;
}