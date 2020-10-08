
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
using namespace std;
#include <vector>
#include <algorithm>

#include <WinSock2.h>
#pragma comment ( lib,"WS2_32.lib" )  

#define RECEIVER_ADDRESS "127.0.0.1"
#define PORT 8001
#define BUFFER_SIZE 1024

vector<SOCKET>g_Clients;

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

	// ����һ��SOCKADDR_IN�ṹ��ָ�����ն˵�ַ��Ϣ
	serverAddr.sin_family = AF_INET;   // ʹ��IP��ַ��
	serverAddr.sin_port = htons(PORT); // �˿ں�
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
	cout << "�¿ͻ��˼��룬 IP = " << inet_ntoa(client_addr.sin_addr) << endl;
	return sd;
}

// 1 NoBlock  0 Block
int SetSocketNoBlock(SOCKET sd, int IsNoBlock)
{
	u_long nNoBlock = IsNoBlock;
	if(ioctlsocket(sd, FIONBIO, &nNoBlock) == SOCKET_ERROR) {
		debugLog("SetSocketNoBlock -> ioctlsocket failed!");
		return SOCKET_ERROR;
	}
	return 0;
}

// �����ر����ӣ��ر�����ǰ�Ȱ�δ������ɵ����ݷ��ͳ�ȥ���ٰ�ȫ�ر�
BOOL CloseConnect(SOCKET sd, char* buffer, int len)
{
	if (buffer != NULL && len > 0) {
		if (SetSocketNoBlock(sd, 0) == SOCKET_ERROR) {
			debugLog("CloseConnect -> SetSocketNoBlock failed!");
			return FALSE;
		}
		int nSend = 0;
		while (nSend < len) {
			int nTemp = send(sd, &buffer[nSend], len - nSend, 0);
			if (nTemp > 0) {
				nSend += nTemp;
			}
			else if (nTemp == SOCKET_ERROR) {
				debugLog("CloseConnect -> send failed!");
				return FALSE;
			}
			else {
				debugLog("CloseConnect -> send �����쳣!");
				break;
			}
		}
	}

	if (shutdown(sd, SD_SEND) == SOCKET_ERROR) {
		debugLog("CloseConnect -> shutdown �����쳣!");
		return FALSE;
	}
	return TRUE;
}

BOOL ClientConFun(SOCKET sd) {
	char buffer[BUFFER_SIZE] = { 0 };
	int nRetByte = 0;

	// ѭ����������
	do {
		nRetByte = recv(sd, buffer, BUFFER_SIZE, 0);
		if (nRetByte == SOCKET_ERROR) {
			debugLog("ClientConFun -> recv failed!");
			return FALSE;
		}
		else {
			if (nRetByte != 0) {
				cout << "���յ�һ�����ݣ� " << buffer << endl;
				int nSend = 0;
				while (nSend < nRetByte) {
					// �ѽ��յ������ݷ��ص����Ͷ�
					int nTemp = send(sd, &buffer[nSend], nRetByte - nSend, 0);
					if (nTemp > 0) {
						nSend += nTemp;
					}
					else if (nTemp == SOCKET_ERROR) {
						debugLog("ClientConFun -> send failed!");
						return FALSE;
					}
					else {
						// Send����0�� ���ڴ�ʱsend<nRetByte�� �����ݻ�û���ͳ�ȥ����ʾ�ͻ�������ر���
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
	// ���ȷ���һ��TCP FIN �ֶΣ���Է������Ѿ�������ݷ���
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
			debugLog("CloseConnect ����ؽ������ݣ�");
			break;
		}
	} while (nRetyte != 0);

	if (SOCKET_ERROR == closesocket(sd)) {
		debugLog("CloseConnect -> closesocket error!");
		return FALSE;
	}
	return TRUE;
}


VOID TcpServerFun() {
	SOCKET hSocket = Bind_Listen(1);
	if (hSocket == INVALID_SOCKET) {
		debugLog("TcpServerFun -> Bind_Listen error!");
		return;
	}

	/*if (SOCKET_ERROR == SetSocketNoBlock(hSocket, 1)) {
		debugLog("TcpServerFun -> SetSocketNoBlock error!");
		return;
	}*/

	// socket������
	fd_set fdRead;
	fd_set fdWrite;
	fd_set fdException;

	while (1) {
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdException);
		FD_SET(hSocket, &fdRead);
		FD_SET(hSocket, &fdWrite);
		FD_SET(hSocket, &fdException);

		for (int i = 0; i < (int)g_Clients.size(); i++) {
			FD_SET(g_Clients[i], &fdRead);
		}

		int ret = select(0, &fdRead, &fdWrite, &fdException, NULL);
		if (ret < 0) {
			debugLog("TcpServerFun -> select error!");
			break;
		}

		if (FD_ISSET(hSocket, &fdRead)) {
			//FD_CLR(hSocket, &fdRead);

			// ���ܿͻ��˵�����
			// ���ؿͻ��˵��׽���
			SOCKET sd = AcceptConnection(hSocket);
			if (sd == INVALID_SOCKET) {
				debugLog("TcpServerFun -> AcceptConnection error!");
				break;
			}

			g_Clients.push_back(sd);
		
			// �ͻ��˴���
			for (int i = 0; i < fdRead.fd_count; i++) {
				if (ClientConFun(fdRead.fd_array[i]) == FALSE) {
					auto iter = find(g_Clients.begin(), g_Clients.end(), fdRead.fd_array[i]);
					if (iter != g_Clients.end()) {
						g_Clients.erase(iter);
					}
				}
			}

			//// �ر�һ���ͻ�������
			//if (CloseConnect(sd) == FALSE) {
			//	//break;
			//}
		}
	}

	for (int i = 0; i < (int)g_Clients.size(); i++) {
		closesocket(g_Clients[i]);
	}

	if (closesocket(hSocket) == SOCKET_ERROR) {
		debugLog("TcpServerFun -> closesocket error!");
		return;
	}
}

int main()
{
	// ��ʼ��
	InitSocket();

	// ҵ�����ݴ���
	TcpServerFun();

	// �ͷ�
	WSACleanup();

	return 0;
}