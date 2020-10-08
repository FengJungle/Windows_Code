#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <WinSock2.h>  

#define PORT 8001 ///�˿ں� 
#define SERVER_IPADRESS "127.0.0.1" // ������IP��ַ��������Ϊ������ַ
#define BUFFER_SIZE 1024  
#pragma comment(lib, "WS2_32")  

int main()
{
	int ret = 0;

	// ��ʼ��socket dll  
	WSADATA wsaData;

	// MAKEWORD
	// ԭ�ͣ�#define MAKEWORD(a, b) ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
	// ���ã�������byte�͵�a��b�ϲ�Ϊһ��word�ͣ���8Ϊ��b����8λ��a��
	// ����ֵ��һ���޷��ŵ�16λ����
	WORD socketVersion = MAKEWORD(2, 0);
	if (WSAStartup(socketVersion, &wsaData) != 0)
	{
		printf("[ERROR] Init socket dll error!\n");
		return -1;
	}

	// ����socket  
	// AF_INET: IPv4 ����Э����׽�������;
	// SOCK_STREAM:�ṩ�������ӵ��ȶ����ݴ��䣬��TCPЭ��;
	SOCKET c_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == c_Socket)
	{
		printf("[ERROR] Create Socket Error!");
		WSACleanup();
		system("pause");
		exit(1);
	}

	// 1: ������
	// 0: ����
	int iMode = 1;
	/*
	#define FIONBIO     _IOW('f', 126, u_long) // set/clear non-blocking i/o
	*/
	ret = ioctlsocket(c_Socket, FIONBIO, (u_long FAR*)&iMode);

	// ָ������˵ĵ�ַ  
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = inet_addr(SERVER_IPADRESS);
	server_addr.sin_port = htons(PORT);

	while (1) {
		// ��������
		ret = connect(c_Socket, (LPSOCKADDR)&server_addr, sizeof(server_addr));
		if (ret == SOCKET_ERROR) {
			int errCode = WSAGetLastError();
			if (errCode == WSAEWOULDBLOCK || errCode == WSAEINVAL) {
				Sleep(100);
				continue;
			}
			else if (errCode == WSAEISCONN) {
				break;
			}
			else {
				printf("[ERROR] Can Not Connect To Server!\n");
				closesocket(c_Socket);
				WSACleanup();
				return -1;
			}
		}
	}

	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);

	while (1) {
		// �������������Ϣ
		memset(buffer, 0, BUFFER_SIZE);
		scanf("%s", buffer);
		while (1) {
			ret = send(c_Socket, buffer, BUFFER_SIZE, 0);
			if (ret == SOCKET_ERROR) {
				int errCode = WSAGetLastError();
				if (errCode == WSAEWOULDBLOCK) {
					Sleep(100);
					continue;
				}
				else {
					printf("[ERROR] Send message Failed with Error Code: 0x%x\n", errCode);
					closesocket(c_Socket);
					WSACleanup();
					return -1;
				}
			}
			break;
		}

		while (1) {
			memset(buffer, 0, BUFFER_SIZE);
			ret = recv(c_Socket, buffer, BUFFER_SIZE, 0);
			if (ret == SOCKET_ERROR) {
				int errCode = WSAGetLastError();
				if (errCode == WSAEWOULDBLOCK) {
					Sleep(100);
					continue;
				}
				else if (errCode == WSAETIMEDOUT || errCode == WSAENETDOWN) {
					printf("[ERROR] recv message Failed with Error Code: 0x%x\n", errCode);
					closesocket(c_Socket);
					WSACleanup();
					return -1;
				}
				break;
			}
			break;
		}
		printf("Recv from Sever: %s\n", buffer);
	}


	closesocket(c_Socket);

	//�ͷ�winsock��  
	WSACleanup();

	system("pause");
	return 0;
}
