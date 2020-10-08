
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
using namespace std;

#include <WinSock2.h>
#pragma comment ( lib,"WS2_32.lib" )  

#define RECEIVER_ADDRESS "127.0.0.1"
#define PORT 8001
#define BUFFER_SIZE 1024

// ����ʽģʽ
// ÿ��ֻ����һ�����ӣ�ֻ���ڷ����굱ǰ�ͻ�����֮�󣬲Ż����������һ���ͻ�������

/*
* 1. �ȴ������ӣ��󶨱��ص�ַ�ͼ���
*    SOCKET Bind_Listen(int nBackLog)
* 2. ����һ���ͻ������Ӳ����ض�Ӧ�����ӵ��׽���
*    SOCKET AcceptConnection(SOCKET hSocket)
* 3. ����һ���ͻ��˵����ӣ�ʵ�ֽ��պͷ�������
*    BOOL ClientConFun(SOCKET sd)
* 4. �ر�һ������
*    BOOL CloseConnect(SOCEKT sd)
* 5. ����������
*    VOID TcpServerFunc()
*/

// ����ʽ��������ģʽ
// ͨ�����̣߳�����ͬʱ���������ӣ�ÿһ���̴߳���һ���ͻ�������

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
	return sd;
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

// �̴߳���ҵ���߼�
DWORD WINAPI ClientThreadFun(LPVOID lpParam) 
{
	SOCKET sd = (SOCKET)lpParam;
	// �ͻ��˴���
	if (ClientConFun(sd) == FALSE) {
		//break;
	}

	// �ر�һ���ͻ�������
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
		// ���ؿͻ��˵��׽���
		SOCKET sd = AcceptConnection(hSocket);
		if (sd == INVALID_SOCKET) {
			debugLog("TcpServerFun -> AcceptConnection error!");
			break;
		}
		
		// �����յ��ͻ�����������Ϊ�ͻ��˿���һ���߳�
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
	// ��ʼ��
	InitSocket();

	// ҵ�����ݴ���
	TcpServerFun();

	// �ͷ�
	WSACleanup();

	return 0;
}