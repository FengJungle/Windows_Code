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
	SOCKET listenSocket = Bind_Listen(5);
	if (SOCKET_ERROR == listenSocket) {
		debugLog("socket failed!");
		return -1;
	}

	// 3. Set socket non-block
	if (SOCKET_ERROR == SetSocketNoBlock(listenSocket, 1)) {
		debugLog("SetSocketNoBlock error!");
		return -1;
	}

	WSAEVENT arr_Event[WSA_MAXIMUM_WAIT_EVENTS]; // 64, Maximum number of wait objects
	SOCKET   arr_Socket[WSA_MAXIMUM_WAIT_EVENTS];
	
	TCHAR buffer[BUFFER_SIZE] = { 0 };
	int      total_Event = 0;
	int              ret = 0;

	// 4. Create event for listen socket and select it
	WSAEVENT listenEvent = WSACreateEvent();
	ret = WSAEventSelect(
		listenSocket,        // socket
		listenEvent,         // WSAEvent
		FD_ACCEPT | FD_CLOSE // lNetworkEvents
	);
	if (SOCKET_ERROR == ret) {
		debugLog("WSAEventSelect failed!");
		return -1;
	}

	arr_Event[total_Event]  = listenEvent;
	arr_Socket[total_Event] = listenSocket;
	total_Event++;

	while (true) {
		// 5. Wait until any network event happen
		int dwIndex = WSAWaitForMultipleEvents(
			total_Event,  // _In_ DWORD cEvents
			arr_Event,    // _In_reads_(cEvents) const WSAEVENT FAR * lphEvents
			FALSE,        // _In_ BOOL fWaitAll
			100,          // _In_ DWORD dwTimeout, WSA_INFINIT表示无限期等待
			FALSE         // _In_ BOOL fAlertable
		);

		if (dwIndex == WSA_WAIT_TIMEOUT) {
			continue;
		}
		
		// 6. 枚举server socket上发生的网络事件

		/*
			typedef struct _WSANETWORKEVENTS {
				long lNetworkEvents;
				int iErrorCode[FD_MAX_EVENTS];
			} WSANETWORKEVENTS, FAR * LPWSANETWORKEVENTS;
		*/
		WSANETWORKEVENTS wsaNetWorkEvents = { 0 };

		ret = WSAEnumNetworkEvents(
			arr_Socket[dwIndex - WSA_WAIT_EVENT_0], 
			arr_Event[dwIndex - WSA_WAIT_EVENT_0], 
			&wsaNetWorkEvents
		);
		if (SOCKET_ERROR == ret) {
			debugLog("WSAEnumNetworkEvents failed!");
			continue;
		}
		else { // ret == 0
			// accept the client
			if (wsaNetWorkEvents.lNetworkEvents & FD_ACCEPT) {
				// check if netword error happen 
				if (wsaNetWorkEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
					debugLog("FD_ACCEPT_BIT error!");
					continue;
				}
				SOCKET clientSocket = accept(arr_Socket[dwIndex - WSA_WAIT_EVENT_0], NULL, NULL);
				if (INVALID_SOCKET == clientSocket) {
					debugLog("accept error!");
					continue;
				}
				cout << "新客户端连入......" << endl;

				WSAEVENT clientEvent = WSACreateEvent();
				ret = WSAEventSelect(
					clientSocket,                 // socket
					clientEvent,                  // WSAEvent
					FD_READ | FD_WRITE | FD_CLOSE // lNetworkEvents
				);
				if (SOCKET_ERROR == ret) {
					debugLog("WSAEventSelect failed!");
					return -1;
				}

				arr_Socket[total_Event] = clientSocket;
				arr_Event[total_Event]  = clientEvent;
				total_Event++;
			}
			// recv message
			else if (wsaNetWorkEvents.lNetworkEvents & FD_READ) {
				// check if netword error happen 
				if (wsaNetWorkEvents.iErrorCode[FD_READ_BIT] != 0) {
					debugLog("FD_READ_BIT error!");
					continue;
				}
				ret = recv(arr_Socket[dwIndex - WSA_WAIT_EVENT_0], (char*)buffer, BUFFER_SIZE, 0);
				if (ret > 0) {
					buffer[ret] = 0;
					cout << "recv : " << buffer << endl;
				}
			}
			// send message
			else if (wsaNetWorkEvents.lNetworkEvents & FD_WRITE) {
				// check if netword error happen 
				if (wsaNetWorkEvents.iErrorCode[FD_WRITE_BIT] != 0) {
					debugLog("FD_WRITE_BIT error!");
					continue;
				}
				ret = send(arr_Socket[dwIndex - WSA_WAIT_EVENT_0], (char*)("服务端已接收到消息"), strlen("服务端已接收到消息"), 0);
			}
			// close connect
			else if (wsaNetWorkEvents.lNetworkEvents & FD_CLOSE) {
				// check if netword error happen 
				if (wsaNetWorkEvents.iErrorCode[FD_CLOSE_BIT] != 0) {
					//debugLog("FD_CLOSE_BIT error!");

					// close socket and event
					closesocket(arr_Socket[dwIndex - WSA_WAIT_EVENT_0]);
					WSACloseEvent(arr_Event[dwIndex - WSA_WAIT_EVENT_0]);

					// remove closed socket and event from array
					if (dwIndex - WSA_WAIT_EVENT_0 != total_Event - 1) {
						arr_Socket[dwIndex - WSA_WAIT_EVENT_0] = arr_Socket[total_Event - WSA_WAIT_EVENT_0 - 1];
						arr_Event[dwIndex - WSA_WAIT_EVENT_0] = arr_Event[total_Event - WSA_WAIT_EVENT_0 - 1];
					}

					total_Event--;

					continue;
				}
			}
		}
	}

	closesocket(listenSocket);
	WSACloseEvent(listenEvent);
	WSACleanup();

	return 0;
}