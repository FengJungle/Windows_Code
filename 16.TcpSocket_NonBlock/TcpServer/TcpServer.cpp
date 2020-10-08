#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <WinSock2.h>  

#define PORT 8001  
#define SERVER_IPADRESSADRESS "127.0.0.1"  
#define BUFFER_SIZE 1024 
#pragma comment(lib, "WS2_32")  

int main()
{
	int ret = 0;

	// 1. ��������ʼ��һ�������(����)�ĵ�ַ�ṹ  
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	// 2. ��ʼ��socket dll  
	WSADATA wsaData;
	WORD socketVersion = MAKEWORD(2, 0);
	if (WSAStartup(socketVersion, &wsaData) != 0)
	{
		printf("[ERROR] Init socket dll error!\n");
		return -1;
	}

	// 3. ����socket  
	SOCKET server_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == server_Socket)
	{
		printf("[ERROR] Create Socket Error!\n");
		WSACleanup();
		return -1;
	}

	// 4. ��socket�ͷ����(����)��ַ  
	if (SOCKET_ERROR == bind(server_Socket, (LPSOCKADDR)&server_addr, sizeof(server_addr)))
	{
		printf("[ERROR] Server Bind Failed: %d\n", WSAGetLastError());
		closesocket(server_Socket);
		WSACleanup();
		return -1;
	}

	// 5. ����  
	if (SOCKET_ERROR == listen(server_Socket, 10))
	{
		printf("[ERROR] Server Listen Failed: %d\n", WSAGetLastError());
		closesocket(server_Socket);
		WSACleanup();
		return -1;
	}

	printf("Listening To Client...\n");

	// 1: ������
	// 0: ����
	int iMode = 1;
	/*
	#define FIONBIO     _IOW('f', 126, u_long) // set/clear non-blocking i/o
	*/
	ret = ioctlsocket(server_Socket, FIONBIO, (u_long FAR*)&iMode);

	sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	SOCKET client_Socket;

	while (true) {
		// ������ʽ�����Ի�ֱ�ӷ���
		client_Socket = accept(server_Socket, (sockaddr *)&client_addr, &client_addr_len);
		if (INVALID_SOCKET == client_Socket)
		{
			int ErrorCode = WSAGetLastError();
			if (ErrorCode == WSAEWOULDBLOCK) {
				Sleep(100);
				continue;
			}
			else {
				printf("[ERROR] accept Failed: %d\n", WSAGetLastError());
				closesocket(server_Socket);
				WSACleanup();
				return -1;
			}
			break;
		}
		printf("Connect To Client, IP:%s\n", inet_ntoa(client_addr.sin_addr));
		break;
	}

	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);

	// ѭ�����տͻ�������
	while (1) {
		memset(buffer, 0, BUFFER_SIZE);
		ret = recv(client_Socket, buffer, BUFFER_SIZE, 0);
		if (SOCKET_ERROR == ret) {
			int errCode = WSAGetLastError();
			if (errCode == WSAEWOULDBLOCK) {
				Sleep(100);
				continue;
			}
			// ��ʱ���������ж�
			else if(errCode == WSAETIMEDOUT || errCode == WSAENETDOWN){
				printf("[ERROR] Server Receive Data Failed with Error Code:0x%x\n", errCode);
				closesocket(client_Socket);
				closesocket(server_Socket);
				WSACleanup();
				return -1;
			}
		}

		char sPrintBuf[BUFFER_SIZE];
		sprintf(sPrintBuf, "IP:%s,���յ�����Ϣ��%s\n", inet_ntoa(client_addr.sin_addr), buffer);
		printf(sPrintBuf);

		if (strcmp(buffer, "close") == 0) {
			ret = send(client_Socket, "close", strlen("close"), 0);
			break;
		}
		else {
			sprintf(sPrintBuf, "������Ѿ����յ��ͻ��˵���Ϣ��%s\n", buffer);
			ret = send(client_Socket, sPrintBuf, strlen(sPrintBuf), 0);
			if (SOCKET_ERROR == ret) {
				int errCode = WSAGetLastError();
				// �޷�������������׽����ϵĲ���
				if (errCode == WSAEWOULDBLOCK) {
					Sleep(100);
					continue;
				}
				else {
					printf("[ERROR] Server Receive Data Failed with Error Code:0x%x\n", errCode);
					closesocket(client_Socket);
					closesocket(server_Socket);
					WSACleanup();
					return -1;
				}
			}
		}
		//break;
	}

	closesocket(server_Socket);
	closesocket(client_Socket);
	// �ͷ�winsock��  
	WSACleanup();

	return 0;
}