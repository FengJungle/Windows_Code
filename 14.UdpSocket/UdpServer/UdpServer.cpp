#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>

#include <WinSock2.h>
#pragma comment ( lib,"WS2_32.lib" )  

#define RECEIVER_ADDRESS "127.0.0.1"
#define PORT 8000

int main()
{
	WSAData wasData;
	SOCKET serverSocket;
	SOCKADDR_IN serverAddr;
	char recvBuf[1024] = { 0 };
	int  recvBufLength = 1024;

	SOCKADDR_IN clientAddr;
	memset(&clientAddr, 0, sizeof(clientAddr));
	int clientAddrLength = sizeof(clientAddr);
	char  senderInfo[30] = { 0 };
	int              ret = -1;

	// ��ʼ��
	WSAStartup(MAKEWORD(2, 2), &wasData);

	// �����׽���
	// �׽������͡���UDP/IP-SOCK_DGRAM
	// Э�飺UDP-IPPROTO_UDP
	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (SOCKET_ERROR == serverSocket) {
		printf("[ERROR] Create Socket Error, Error Code: 0x%x!", WSAGetLastError());
		WSACleanup();
		goto exit;
	}

	// ����һ��SOCKADDR_IN�ṹ��ָ�����ն˵�ַ��Ϣ
	serverAddr.sin_family = AF_INET;   // ʹ��IP��ַ��
	serverAddr.sin_port = htons(PORT); // �˿ں�
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// bind����ַ��Ϣ���׽��ֹ���
	ret = bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (0 != ret) {
		printf("[ERROR] bind error with Error Code 0x%x\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		goto exit;
	}

	// �������ݱ�
	while (1) {
		ret = recvfrom(serverSocket, recvBuf, recvBufLength, 0, (SOCKADDR*)&clientAddr, &clientAddrLength);
		if (ret > 0) {
			printf("recv info��%s\n", recvBuf);
		}
		else if (ret == -1) {
			printf("[ERROR] recvfrom error with Error Code 0x%x\n", WSAGetLastError());
			closesocket(serverSocket);
			WSACleanup();
			goto exit;
		}
		else {

		}
	}

exit:
	closesocket(serverSocket);
	system("pause");
	return 0;
}