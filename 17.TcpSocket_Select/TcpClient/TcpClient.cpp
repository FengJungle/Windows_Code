#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
using namespace std;

#include <WinSock2.h>
#pragma comment ( lib,"WS2_32.lib" )  

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
		debugLog("InitSocket() failed!");
		return FALSE;
	}

	return TRUE;
}

SOCKET ConnectToServer()
{
	// ����socket  
	// AF_INET: IPv4 ����Э����׽�������;
	// SOCK_STREAM:�ṩ�������ӵ��ȶ����ݴ��䣬��TCPЭ��;
	SOCKET c_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == c_Socket)
	{
		debugLog("[ERROR] Create Socket Error!");
		return INVALID_SOCKET;
	}

	// ָ������˵ĵ�ַ  
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = inet_addr(SERVER_IPADRESS);
	server_addr.sin_port = htons(PORT);

	// ��������
	if (SOCKET_ERROR == connect(c_Socket, (LPSOCKADDR)&server_addr, sizeof(server_addr)))
	{
		debugLog("[ERROR] Can Not Connect To Server IP!\n");
		closesocket(c_Socket);
		return INVALID_SOCKET;
	}
	return c_Socket;
}

BOOL ClientSendFunc(SOCKET ServerSocket)
{
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);

	while (1) {
		// �������������Ϣ
		memset(buffer, 0, BUFFER_SIZE);
		if (gets_s(buffer, BUFFER_SIZE)) {
			if (send(ServerSocket, buffer, BUFFER_SIZE, 0) < 0)
			{
				debugLog("[ERROR] MyTcpClientFun -> send error!");
				closesocket(ServerSocket);
				return FALSE;
			}

			memset(buffer, 0, BUFFER_SIZE);
			int length = 0;
			if ((length = recv(ServerSocket, buffer, BUFFER_SIZE, 0)) > 0)
			{
				cout << buffer << endl;
			}
		}
	}
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

VOID TcpClientFun()
{
	while (1) {
		SOCKET ServerSocket = ConnectToServer();
		if (INVALID_SOCKET == ServerSocket) {
			debugLog("MyTcpClientFun -> ConnectToServer error!");
			//break;
		}

		if (FALSE == ClientSendFunc(ServerSocket)) {
			debugLog("MyTcpClientFun -> ClientSendFunc error!");
			//break;
		}

		if (CloseConnect(ServerSocket) == FALSE) {
			//break;
		}

		if (closesocket(ServerSocket) == SOCKET_ERROR) {
			debugLog("MyTcpClientFun -> closesocket error!");
			return;
		}
	}
}

int main()
{
	// ��ʼ��
	InitSocket();

	// ҵ�����ݴ���
	TcpClientFun();

	// �ͷ�
	WSACleanup();

	return 0;
}