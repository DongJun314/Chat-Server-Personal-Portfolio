#pragma once
#include "Headers.h"
#include "Defines.h"

class ThreadPool final
{
public:
    explicit ThreadPool(size_t _numThreads);
    ~ThreadPool();

public:
    void Enqueue(const SOCKET& _clientSocket);

private:
    void HandleClientSocket(const SOCKET& _clientSocket);
    bool HandleNicknameRegisterLoop(const SOCKET& _clientSocket);
    bool RecvNicknamePacket(const SOCKET& _clientSocket, PACKET_NICKNAME_REGISTER_RECV& _tOutPacket);
    void ProcessNickname(const PACKET_NICKNAME_REGISTER_RECV& _tInPacket, PACKET_NICKNAME_REGISTER_SEND& _tOutPacket);
    bool SendNicknameResponse(const SOCKET& _clientSocket, const PACKET_NICKNAME_REGISTER_SEND& _tSendPacket);
    std::string GetExitClientNickName(const SOCKET& _clientSocket);

    void HandleChatLoop(const SOCKET& _clientSocket);
    void BroadcastMessage(const SOCKET& _clientSocket, const std::array<char, MAX_RECV_SIZE>& _arrRecvBuffer, const uint32_t& _iSizeMsg);
    void RemoveClient(const SOCKET& _clientSocket);


    // Recv
    bool RecvExact(const SOCKET& _clientSocket, char* _szBuffer, uint32_t _iSize);
    bool RecvPacketFromClient(const SOCKET& _clientSocket, std::array<char, MAX_RECV_SIZE>& _arrRecvBuffer, PACKET_CHAT_MSG_RECV& _tRecvPacket);

    // Send
    bool SendExact(const SOCKET& _clientSocket, const char* _szBuffer, const uint32_t& _iSize);
    bool SendPacketToClient(const SOCKET& _clientSocket, const std::array<char, MAX_RECV_SIZE>& _arrSendBuffer, const uint32_t& _iSizeMsg);

private:
    std::mutex m_mtxClientSockets;
    std::vector<SOCKET> m_vecClientSockets;
    uint32_t m_iCurClientCnt{ 0 };

    std::vector<std::thread> m_vecWorkers;   // 워커 스레드들
    std::queue<SOCKET> m_qTaskSockets;           // 작업 큐

    std::mutex m_mtxTaskQueue;                      // 큐 보호용 뮤텍스
    std::condition_variable m_cvTask;         // 작업 대기/신호
    std::atomic<bool> m_bStopFlag;             // 종료 요청 플래그

    // 모든 클라의 닉네임 
    std::unordered_set<std::string> m_usetNicknames;
   // std::mutex m_mtxNicknames;
};