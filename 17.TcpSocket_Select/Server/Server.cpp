#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>  
#include <Windows.h>  
#include <stdio.h>  
#include <iostream>
using namespace std;

#pragma comment(lib,"Ws2_32.lib")  

#define RECEIVER_ADDRESS "127.0.0.1"
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

SOCKET Bind_Listen(int nBackLog) {

	SOCKET listenSocket;
	SOCKADDR_IN serverAddr;

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (SOCKET_ERROR == listenSocket) {
		debugLog("Bind_Listen -> socket failed!");
		return INVALID_SOCKET;
	}

	// 创建一个SOCKADDR_IN结构，指定接收端地址信息
	serverAddr.sin_family = AF_INET;   // 使用IP地址族
	serverAddr.sin_port = htons(PORT); // 端口号
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr))) {
		debugLog("Bind_Listen -> bind failed!");
		closesocket(listenSocket);
		return INVALID_SOCKET;
	}

	if (listen(listenSocket, nBackLog) == SOCKET_ERROR) {
		debugLog("Bind_Listen -> listen failed!");
		closesocket(listenSocket);
		return INVALID_SOCKET;
	}

	return listenSocket;
}

// 1 NoBlock  0 Block
int SetSocketNoBlock(SOCKET sd, int IsNoBlock)
{
	u_long nNoBlock = IsNoBlock;
	if (ioctlsocket(sd, FIONBIO, &nNoBlock) == SOCKET_ERROR) {
		debugLog("SetSocketNoBlock -> ioctlsocket failed!");
		return SOCKET_ERROR;
	}
	return 0;
}

int main() {

	// 1. Initial winsock
	if (FALSE == InitSocket()) {
		debugLog("InitSocket error!");
		return -1;
	}

	// 2. Bind and listen
	SOCKET socketListen = Bind_Listen(5); 
	if (SOCKET_ERROR == socketListen) {
		debugLog("socket failed!");
		return -1;
	}

	// 3. Set socket non-block
	if (SOCKET_ERROR == SetSocketNoBlock(socketListen, 1)) {
		debugLog("SetSocketNoBlock error!");
		return -1;
	}

	// 4. 初始化一个套接字集合fdSocket，并将监听套接字放入  
	fd_set    socketSet;
	fd_set    readSet;
	fd_set    writeSet;

	FD_ZERO(&socketSet);
	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);

	FD_SET(socketListen, &socketSet);

	TIMEVAL time = { 1,0 };
	char buffer[BUFFER_SIZE] = { 0 };

	while (true) {
		// 5. 将fdSocket的拷贝fdRead传给select函数  
		readSet  = socketSet;
		writeSet = socketSet;

		// 6. 同时检查套接字的可读可写性
		int   ret = select(0, &readSet, &writeSet, NULL, &time);//若不设置超时则select为阻塞  
		if (ret > 0) {
			// 7. 是否存在客户端的连接请求  
			if (FD_ISSET(socketListen, &readSet)) { 
				//在readset中会返回已经调用过listen的套接字 
				if (socketSet.fd_count < FD_SETSIZE) {
					sockaddr_in addrRemote;
					int nAddrLen = sizeof(addrRemote);
					SOCKET sClient = accept(socketListen, (sockaddr*)&addrRemote, &nAddrLen);
					if (sClient != INVALID_SOCKET) {
						FD_SET(sClient, &socketSet);
						cout << "接收到客户端连接：" << inet_ntoa(addrRemote.sin_addr) << endl;
					}
				}
				else {
					cout << "连接数量已达上限！" << endl;
					continue;
				}
			}

			for (int i = 0; i < socketSet.fd_count; i++) { 
				if (FD_ISSET(socketSet.fd_array[i], &readSet)) {
					// 调用recv，接收数据
					int nRecv = recv(socketSet.fd_array[i], buffer, 4096, 0);
					if (nRecv > 0) {
						buffer[nRecv] = 0;
						cout << "recv " << socketSet.fd_array[i] << ":" << buffer << endl;
					}
				}

				if (FD_ISSET(socketSet.fd_array[i], &writeSet)) {

					// 调用send，发送数据
					char buf[] = "服务器已接收到数据";
					int nRet = send(socketSet.fd_array[i], buf, strlen(buf) + 1, 0);
					if (nRet <= 0) {
						if (GetLastError() == WSAEWOULDBLOCK) {
							//do nothing  
						}
						else {
							cout << "客户端" << socketSet.fd_array[i] << "断开连接" << endl;
							closesocket(socketSet.fd_array[i]);
							FD_CLR(socketSet.fd_array[i], &socketSet);
						}
					}
				}
			}
		}
		else if (ret == 0) {
			cout << "time out!" << endl;
		}
		else {
			debugLog("select error!");
			Sleep(5000);
			break;
		}
		Sleep(100);
	}

	closesocket(socketListen);
	WSACleanup();

	return 0;
}