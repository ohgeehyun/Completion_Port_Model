#pragma once
#include <WinSock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include "Macro.h"
#pragma comment(lib,"ws2_32.lib")

using namespace std;

void HandleError(const char* cause)
{
	int32_t errCode = ::WSAGetLastError();
	cout << "ErrorCode : " << errCode << endl;
}

const int32_t BUFSIZE = 1000;

struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUFSIZE] = {};
	int32_t recvBytes = 0;
	int32_t sendBytes = 0;
};

enum IO_TYPE
{
	READ,
	WRITE,
	ACCEPT,
	CONNECT,
};

struct OverlappedEx
{
	WSAOVERLAPPED overlapped = {};
	int32_t type = 0; //IO_TYPE ...
};


void CALLBACK RecvCallback(DWORD error, DWORD recvLen, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	cout << "Data Recv Len Callback =" << recvLen << endl;
	//TODO: 에코 서버일 경우 WSASend()등을 호출
}

void WorkerThreadMain(HANDLE iocpHandle)
{
	while (true)
	{
		DWORD bytesTransferred = 0;
		Session* session = nullptr;
		OverlappedEx* overlappedEx = nullptr;
		BOOL ret = ::GetQueuedCompletionStatus(iocpHandle, &bytesTransferred,
			(ULONG_PTR*)&session, (LPOVERLAPPED*)&overlappedEx, INFINITE);

		if (ret == FALSE || bytesTransferred == 0)
		{
			//TODO : 연결 끊김
			continue;
		}
		ASSERT_CRASH(overlappedEx->type == IO_TYPE::READ);

		cout << "Recv Data IOCP = " << bytesTransferred << endl;
		//다시 recv를 바로 예약하기위해 함수안에서 recv를 다시 호출해주어야한다.
		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUFSIZE;

		DWORD recvLen = 0;
		DWORD flags = 0;
		::WSARecv(session->socket, &wsaBuf, 1, &recvLen, &flags, &overlappedEx->overlapped, NULL);
	}
}

int main()
{

	WSAData wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "start up 에러" << endl;
		return 0;
	}


	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
		return 0;

	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	serverAddr.sin_port = ::htons(5252);

	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;

	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
		return 0;

	cout << "Accept" << endl;

	//IOCP (Completion Port) 모델
	// - APC큐 대신 Completion Port (쓰레드마다 있는건 아님 1개.) 중앙에서 관리하는 apc큐 같은느낌
	// - Alertable Wait 대신 CP 결과 처리를 GetQueuedCompletionStatus호출하여 처리
	// 쓰레드랑 궁합이 굉장히 좋다.

	//CreateIoCompletionPort //completionport를 만드는 것과 관찰하는것 2가지일이 가능한 함수
	//GetQueuedCompletionStatus //결과를 관찰하는 함수

	//CP 생성
	//completionport를 생성할때는 첫인자를 INVALID_HANDLE_VALUE 이후 나머지는 기본값으로 셋팅하여 만듬.
	HANDLE iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	//WorkerThreads
	vector<std::thread> threads;
	for (int32_t i = 0; i < 5; i++)
	{
		threads.push_back(thread([=]() {WorkerThreadMain(iocpHandle); }));
	}

	vector<Session*> sessionManager;

	//Main Thread = accpet담당
	while (true)
	{
		SOCKADDR_IN clientAddr;
		int32_t addrLen = sizeof(clientAddr);
		SOCKET clientSocket;

		clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
			return 0;

		Session* session = new Session{ clientSocket };
		session->socket = clientSocket;
		sessionManager.push_back(session);

		cout << "Client Connected !" << endl;

		//소켓을 만들었으니 CP에 등록
		::CreateIoCompletionPort((HANDLE)clientSocket, iocpHandle,/*key*/(ULONG_PTR)session,/*코어 갯수 0이면 최대값*/0);

		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUFSIZE;

		OverlappedEx* overlappedEx = new OverlappedEx;
		overlappedEx->type = IO_TYPE::READ;

		DWORD recvLen = 0;
		DWORD flags = 0;
		::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags, &overlappedEx->overlapped, NULL);
		//여기까지가 메인스레드
	}
	
	//윈속 종료
	::WSACleanup();
}
