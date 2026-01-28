#include "Server.h"
#include "Headers.h"
#include "Defines.h"
#include "ThreadPool.h"

using namespace std;

Server::Server(const string& _strIP, uint16_t _iPort, size_t _numThreads)
	:m_strIP(_strIP), m_iPort(_iPort), m_ThreadPool(_numThreads), m_hSocket(INVALID_SOCKET)
{
}

Server::~Server()
{
	if (m_hSocket != INVALID_SOCKET)
	{
		closesocket(m_hSocket);
	}
	WSACleanup();
}

bool Server::Run()
{
	if (!InitWinsock()) return false;
	if (!CreateSocket()) return false;
	if (!Bind(m_strIP, m_iPort)) return false;
	if (!Listen()) return false;

	AcceptLoop(); // 루프는 false/true 반환 안 하고 무한 대기
	return true;
}

bool Server::InitWinsock()
{
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		cerr << "WSAStartup failed: " << iResult << endl;
		return false;
	}
	return true;
}

bool Server::CreateSocket()
{
	m_hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_hSocket == INVALID_SOCKET)
	{
		cerr << "Socket creation failed: " << WSAGetLastError() << endl;
		return false;
	}
	return true;
}

bool Server::Bind(const string& _strIP, uint16_t _iPort)
{
	SOCKADDR_IN servAddr{};
	{
		servAddr.sin_family = AF_INET;
		servAddr.sin_port = htons(_iPort);

		if (inet_pton(AF_INET, _strIP.c_str(), &servAddr.sin_addr) <= 0)
		{
			cerr << "Invalid IP address: " << _strIP << endl;
			return false;
		}
	}

	int iResult = ::bind(m_hSocket, reinterpret_cast<SOCKADDR*>(&servAddr), sizeof(servAddr));
	if (iResult == SOCKET_ERROR)
	{
		cerr << "Bind failed: " << WSAGetLastError() << endl;
		closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		return false;
	}
	return true; 
}

bool Server::Listen()
{
	if (listen(m_hSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		cerr << "Listen failed: " << WSAGetLastError() << endl;
		closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		return false;
	}
	return true;
}

void Server::AcceptLoop()
{
	while (true)
	{
		SOCKADDR_IN clientAddr{};
		int iSizeClientAddr = sizeof(clientAddr);

		SOCKET clientSocket = accept(m_hSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &iSizeClientAddr);
		if (clientSocket == INVALID_SOCKET)
		{
			cerr << "Accept failed: " << WSAGetLastError() << endl;
			continue; // 그냥 다음 클라 기다림
		}

		// 클라이언트 소켓을 스레드풀에 전달
		m_ThreadPool.Enqueue(clientSocket);
	}
}