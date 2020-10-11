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

	// ����һ��SOCKADDR_IN�ṹ��ָ�����ն˵�ַ��Ϣ
	serverAddr.sin_family = AF_INET;   // ʹ��IP��ַ��
	serverAddr.sin_port = htons(PORT); // �˿ں�
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

	// 4. ��ʼ��һ���׽��ּ���fdSocket�����������׽��ַ���  
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
		// 5. ��fdSocket�Ŀ���fdRead����select����  
		readSet  = socketSet;
		writeSet = socketSet;

		// 6. ͬʱ����׽��ֵĿɶ���д��
		int   ret = select(0, &readSet, &writeSet, NULL, &time);//�������ó�ʱ��selectΪ����  
		if (ret > 0) {
			// 7. �Ƿ���ڿͻ��˵���������  
			if (FD_ISSET(socketListen, &readSet)) { 
				//��readset�л᷵���Ѿ����ù�listen���׽��� 
				if (socketSet.fd_count < FD_SETSIZE) {
					sockaddr_in addrRemote;
					int nAddrLen = sizeof(addrRemote);
					SOCKET sClient = accept(socketListen, (sockaddr*)&addrRemote, &nAddrLen);
					if (sClient != INVALID_SOCKET) {
						FD_SET(sClient, &socketSet);
						cout << "���յ��ͻ������ӣ�" << inet_ntoa(addrRemote.sin_addr) << endl;
					}
				}
				else {
					cout << "���������Ѵ����ޣ�" << endl;
					continue;
				}
			}

			for (int i = 0; i < socketSet.fd_count; i++) { 
				if (FD_ISSET(socketSet.fd_array[i], &readSet)) {
					// ����recv����������
					int nRecv = recv(socketSet.fd_array[i], buffer, 4096, 0);
					if (nRecv > 0) {
						buffer[nRecv] = 0;
						cout << "recv " << socketSet.fd_array[i] << ":" << buffer << endl;
					}
				}

				if (FD_ISSET(socketSet.fd_array[i], &writeSet)) {

					// ����send����������
					char buf[] = "�������ѽ��յ�����";
					int nRet = send(socketSet.fd_array[i], buf, strlen(buf) + 1, 0);
					if (nRet <= 0) {
						if (GetLastError() == WSAEWOULDBLOCK) {
							//do nothing  
						}
						else {
							cout << "�ͻ���" << socketSet.fd_array[i] << "�Ͽ�����" << endl;
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