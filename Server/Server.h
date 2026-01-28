#pragma once
#include "Headers.h"
#include "ThreadPool.h"

class Server 
{
public:
    explicit Server(const std::string& _strIP, uint16_t _iPort, size_t _numThreads);
    ~Server();
   
public:
    bool Run();
    
private:
    bool InitWinsock();
    bool CreateSocket();
    bool Bind(const std::string& _strIP, uint16_t _iPort);
    bool Listen();
    void AcceptLoop();

private:
    SOCKET m_hSocket;
    ThreadPool m_ThreadPool;
    
    std::string m_strIP;
    uint16_t m_iPort;
};
