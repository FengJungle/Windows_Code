# Windows_Code

This is a source code for windows PE test code.


*=========================== 01 ===========================  

Project Name: PETool_ParsePEFile

Description: Parse given PE file.


*=========================== 02 ===========================  

Project Name: PETool_AddSection

Description: Add a new section to the fileBuffer and save as a new file.


*=========================== 03  =========================== 

Project Name: DLL_Test

Description: Sample code to generate Dll and use dll.


*=========================== 04  ===========================  

Project Name: PETool_ExportTable

Description: Print the Export Directory.


*=========================== 05.  ===========================  

Project Name: PETool_ImportDirectory

Description: Parse and print the Import Directory of self-defined PE file.



*=========================== 06  =========================== 

Project Name: PETool_Relocation

Description: Print the Relocaton Directory.


*=========================== 07  =========================== 

Date: 2020-09-05

Project Name: PETool_ExpandLastSection

Description: Read original file and expand the last section, add 0x1000 in size, then save as a new file.


*=========================== 08  =========================== 

Date: 2020-09-05

Project Name: PETool_MergeAllSections

Description: Merge all sections to only one section.


*=========================== 09  =========================== 

Date: 2020-09-05

Project Name: PETool_MoveExportTableToNewSection

Description: Move exportDirectory to a new section.


*=========================== 11  =========================== 

Date: 2020-10-06

Project Name: TcpSocket_File

Description: WinSock network. Use TCP/IP to implement a Server/Client, client can receive any file from server.

Blog: https://blog.csdn.net/sinat_21107433/article/details/80869887


*=========================== 12  =========================== 

Date: 2020-10-07

Project Name: TcpSocket_Message

Description: WinSock network. Use TCP/IP to implement a Server/Client, client can send any message to server.


*=========================== 13  =========================== 

Date: 2020-10-08

Project Name: TcpSocket_MultiThread

Description: 基于WinSock的TCP通信模型，阻塞式模型并发模式，即每次客户端请求连接时，服务端开启一个线程处理。

服务端流程：

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

客户端流程：

/*
* 1. 先与服务端建立连接，返回对应的连接的套接字
*    SOCKET ConnectToServer()
* 2. 客户端发送数据到服务端
*    BOOL ClientSendFunc(SOCKET hSocket)
* 3. 关闭一个连接
*    BOOL CloseConnect(SOCEKT sd)
* 4. 客户端主体
*    VOID TcpClientFun()
*/


*=========================== 14  =========================== 

Date: 2020-10-08

Project Name: UdpSocket

Description: WinSock network. Use UDP/IP to implement a Server/Client, client can send any message to server.

Blog: https://blog.csdn.net/sinat_21107433/article/details/103537437


*=========================== 15  =========================== 

Date: 2020-10-08

Project Name: TcpSocket_BlockMode

Description: 基于WinSock的TCP通信模型，阻塞式模型迭代模式，即每次只服务一个连接，只有在服务完当前客户连接之后，才会继续服务下一个客户端连接。

服务端流程：

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

客户端流程：

/*
* 1. 先与服务端建立连接，返回对应的连接的套接字
*    SOCKET ConnectToServer()
* 2. 客户端发送数据到服务端
*    BOOL ClientSendFunc(SOCKET hSocket)
* 3. 关闭一个连接
*    BOOL CloseConnect(SOCEKT sd)
* 4. 客户端主体
*    VOID TcpClientFun()
*/


*=========================== 16  =========================== 

Date: 2020-10-08

Project Name: TcpSocket_NonBlockMode

Description: 基于WinSock的TCP通信模型，非阻塞式模型。


*=========================== 17 =========================== 

Date: 2020-10-11

Project Name: TcpSocket_Select

Description: 基于WinSock的TCP通信模型，非阻塞式模型, 采用select模型实现单线程下的并发模式。

select模型是一种比较常用的IO模型。利用该模型可以使Windows socket应用程序可以同时管理多个套接字。使用select模型，可以使当执行操作的套接字满足可读可写条件时，给应用程序发送通知。收到这个通知后，应用程序再去调用相应的Windows socket API去执行函数调用。     

Select模型的核心是select函数。调用select函数检查当前各个套接字的状态。根据函数的返回值判断套接字的可读可写性。然后调用相应的Windows Sockets API完成数据的发送、接收等。Select模型是Windows sockets中最常见的IO模型。它利用select函数实现IO 管理。通过对select函数的调用，应用程序可以判断套接字是否存在数据、能否向该套接字写入数据。   如：在调用recv函数之前，先调用select函数，如果系统没有可读数据那么select函数就会阻塞在这里。当系统存在可读或可写数据时，select函数返回，就可以调用recv函数接收数据了。










