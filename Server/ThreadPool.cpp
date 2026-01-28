#include "ThreadPool.h"
#include "Headers.h"
#include "Defines.h"

using namespace std;

ThreadPool::ThreadPool(size_t _numThreads)
    : m_bStopFlag(false)
{
    for (size_t i = 0; i < _numThreads; ++i)
    {
        m_vecWorkers.emplace_back([this, i] {
            while (true)
            {
                SOCKET clientSocket;

                //  임계구역 시작
                {
                    unique_lock<mutex> lock(this->m_mtxTaskQueue);

                    // 큐가 비어있으면 조건변수 기다림
                    this->m_cvTask.wait(lock, [this] {
                        return this->m_bStopFlag || !this->m_qTaskSockets.empty();
                        });

                    // 종료 플래그 + 큐 비었음 → 루프 종료
                    if (this->m_bStopFlag && this->m_qTaskSockets.empty())
                        return;

                    // 큐에서 작업 꺼내오기
                    clientSocket = this->m_qTaskSockets.front();
                    this->m_qTaskSockets.pop();
                } //  lock 스코프 끝나면 자동 unlock 

                //  뮤텍스 해제 후 작업 실행 (안전)
                HandleClientSocket(clientSocket);
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        unique_lock<mutex> lock(m_mtxTaskQueue);
        m_bStopFlag = true;
    }
    m_cvTask.notify_all(); // 모든 워커 깨움

    for (thread& worker : m_vecWorkers)
        worker.join();
}

void ThreadPool::Enqueue(const SOCKET& _clientSocket)
{
    {
        unique_lock<mutex> lock(m_mtxTaskQueue);
        m_qTaskSockets.push(_clientSocket);
    }
    {
        lock_guard<mutex> guard(m_mtxClientSockets);
        m_vecClientSockets.push_back(_clientSocket);
        cout << "Client connected: SOCKET=" << _clientSocket << endl;
    }
    m_cvTask.notify_one(); // 워커 하나 깨움
}
bool ThreadPool::RecvNicknamePacket(const SOCKET& _clientSocket, PACKET_NICKNAME_REGISTER_RECV& _tOutPacket)
{
    std::array<char, MAX_RECV_SIZE> arrBuffer{};

    if (!RecvExact(_clientSocket, arrBuffer.data(), sizeof(PACKET_HEADER)))
        return false;

    PACKET_HEADER tHeader{};
    tHeader.Deserialize(arrBuffer.data());

    if (tHeader.iSize > MAX_RECV_SIZE)
    {
        cerr << "Message too large! size=" << tHeader.iSize << endl;
        return false;
    }

    if (!RecvExact(_clientSocket, arrBuffer.data() + sizeof(PACKET_HEADER), tHeader.iSize - sizeof(PACKET_HEADER)))
        return false;

    _tOutPacket.Deserialize(arrBuffer.data());
    return true;
}

void ThreadPool::ProcessNickname(const PACKET_NICKNAME_REGISTER_RECV& _tInPacket, PACKET_NICKNAME_REGISTER_SEND& _tOutPacket)
{
    std::string strNickName(_tInPacket.arrNickName.data());

    _tOutPacket.tHeader.iSize = sizeof(PACKET_NICKNAME_REGISTER_SEND);
    _tOutPacket.tHeader.ePacketType = EPACKET_TYPE::NICKNAME_REGISTER_ACK;
    _tOutPacket.bSuccess = m_usetNicknames.insert(strNickName).second;
}

bool ThreadPool::SendNicknameResponse(const SOCKET& _clientSocket, const PACKET_NICKNAME_REGISTER_SEND& _tSendPacket)
{
    std::array<char, MAX_SEND_SIZE> arrSendBuffer{};
    _tSendPacket.Serialize(arrSendBuffer.data());
    return SendExact(_clientSocket, arrSendBuffer.data(), sizeof(PACKET_NICKNAME_REGISTER_SEND));
}

std::string ThreadPool::GetExitClientNickName(const SOCKET& _clientSocket)
{
    lock_guard<mutex> guard(m_mtxClientSockets);

    for (SOCKET curSocket : m_vecClientSockets)
    {
        if (curSocket == _clientSocket)
            continue;

        // 여기서 닉네임을 얻어오기
    }
    return std::string();
}

bool ThreadPool::HandleNicknameRegisterLoop(const SOCKET& _clientSocket)
{
    while (true)
    {
        PACKET_NICKNAME_REGISTER_RECV tRecvRequestPacket{};
        if (!RecvNicknamePacket(_clientSocket, tRecvRequestPacket))
            return false;

        PACKET_NICKNAME_REGISTER_SEND tSendRegisterPacket{};
        ProcessNickname(tRecvRequestPacket, tSendRegisterPacket);

        if (!SendNicknameResponse(_clientSocket, tSendRegisterPacket))
            return false;

        if (tSendRegisterPacket.bSuccess)
        {
            // 2) 입장 이벤트 브로드캐스트
            ++m_iCurClientCnt;
            array<char, MAX_SEND_SIZE> arrSendBuffer{};
            PACKET_CHAT_MSG_SEND tSendPacket{};
            {
                tSendPacket.tHeader.iSize = sizeof(PACKET_CHAT_MSG_SEND);
                tSendPacket.tHeader.ePacketType = EPACKET_TYPE::CHAT_MESSAGE;
                std::copy(tRecvRequestPacket.arrNickName.begin(), tRecvRequestPacket.arrNickName.end(), tSendPacket.arrNickName.data());
                tSendPacket.iUserCount = m_iCurClientCnt;
                tSendPacket.eChatType = ECHAT_TYPE::JOIN;
                tSendPacket.Serialize(arrSendBuffer.data());

                // Broadcast
                BroadcastMessage(_clientSocket, arrSendBuffer, sizeof(PACKET_CHAT_MSG_SEND));
            }
            return true;
        }
    }

    return false;
}


void ThreadPool::HandleChatLoop(const SOCKET& _clientSocket)
{
    // main chat loop
    while (true)
    {
        // 서버로부터 응답 받기
        array<char, MAX_RECV_SIZE> arrRecvBuffer{};
        PACKET_CHAT_MSG_RECV tRecvPacket{};
        if (!RecvPacketFromClient(_clientSocket, arrRecvBuffer, tRecvPacket))
        {
            // 퇴장 이벤트 브로드캐스트
            array<char, MAX_SEND_SIZE> arrSendBuffer{};
            PACKET_CHAT_MSG_SEND tSendPacket{};
            {
                tSendPacket.tHeader.iSize = sizeof(PACKET_CHAT_MSG_SEND);
                tSendPacket.tHeader.ePacketType = EPACKET_TYPE::CHAT_MESSAGE;
                std::copy(tRecvPacket.arrNickName.begin(), tRecvPacket.arrNickName.end(), tSendPacket.arrNickName.data());
                tSendPacket.iUserCount = --m_iCurClientCnt;
                tSendPacket.eChatType = ECHAT_TYPE::LEAVE;
                tSendPacket.Serialize(arrSendBuffer.data());
                
                // Broadcast
                //GetExitClientNickName(_clientSocket);
                BroadcastMessage(_clientSocket, arrSendBuffer, sizeof(PACKET_CHAT_MSG_SEND));
            }
            break;
        }
        else if (tRecvPacket.eChatType == ECHAT_TYPE::LEAVE)
        {
            // 퇴장 이벤트 브로드캐스트
            array<char, MAX_SEND_SIZE> arrSendBuffer{};
            PACKET_CHAT_MSG_SEND tSendPacket{};
            {
                tSendPacket.tHeader.iSize = sizeof(PACKET_CHAT_MSG_SEND);
                tSendPacket.tHeader.ePacketType = EPACKET_TYPE::CHAT_MESSAGE;
                std::copy(tRecvPacket.arrNickName.begin(), tRecvPacket.arrNickName.end(), tSendPacket.arrNickName.data());
                tSendPacket.iUserCount = --m_iCurClientCnt;
                tSendPacket.eChatType = ECHAT_TYPE::LEAVE;
                tSendPacket.Serialize(arrSendBuffer.data());

                // Broadcast
                //GetExitClientNickName(_clientSocket);
                BroadcastMessage(_clientSocket, arrSendBuffer, sizeof(PACKET_CHAT_MSG_SEND));
            }
            break;
        }

        // 클라로 응답 보내기
        array<char, MAX_SEND_SIZE> arrSendBuffer{};
        PACKET_CHAT_MSG_SEND tSendPacket{};
        {
            tSendPacket.tHeader.iSize = sizeof(PACKET_CHAT_MSG_SEND);
            tSendPacket.tHeader.ePacketType = EPACKET_TYPE::CHAT_MESSAGE;
            std::copy(tRecvPacket.arrNickName.begin(), tRecvPacket.arrNickName.end(), tSendPacket.arrNickName.data());
            std::copy(tRecvPacket.arrMessage.begin(), tRecvPacket.arrMessage.end(), tSendPacket.arrMessage.data());
            tSendPacket.iUserCount = m_iCurClientCnt;
            tSendPacket.eChatType = tRecvPacket.eChatType;
            tSendPacket.Serialize(arrSendBuffer.data());

            // Broadcast
            BroadcastMessage(_clientSocket, arrSendBuffer, sizeof(PACKET_CHAT_MSG_SEND));
        }    
    }
}

void ThreadPool::HandleClientSocket(const SOCKET& _clientSocket)
{
    // 1) 닉네임 등록 루프
    if (!HandleNicknameRegisterLoop(_clientSocket))
        return;

    // 2) 메인 채팅 루프
    HandleChatLoop(_clientSocket);

    // 3) 종료 처리
    RemoveClient(_clientSocket);
    return;
}

void ThreadPool::BroadcastMessage(const SOCKET& _clientSocket, const std::array<char, MAX_RECV_SIZE>& _arrRecvBuffer, const uint32_t& _iSizeMsg)
{
    lock_guard<mutex> guard(m_mtxClientSockets);

    for (SOCKET curSocket : m_vecClientSockets)
    {
        //if (curSocket == _clientSocket)
        //    continue;

        if (!SendPacketToClient(curSocket, _arrRecvBuffer, _iSizeMsg))
        {
            cerr << "Send failed to socket: " << curSocket << endl;
            continue; // 다음 클라로 넘어감
        }
    }
}

void ThreadPool::RemoveClient(const SOCKET& _clientSocket)
{
    lock_guard<mutex> guard(m_mtxClientSockets);
    m_vecClientSockets.erase(remove(m_vecClientSockets.begin(), m_vecClientSockets.end(), _clientSocket), m_vecClientSockets.end());
    closesocket(_clientSocket);
    cout << "Client disconnected: SOCKET=" << _clientSocket << endl;
}

bool ThreadPool::RecvExact(const SOCKET& _clientSocket, char* _szBuffer, uint32_t _iSize)
{
    int iTotalRecv = 0;
    while (iTotalRecv < _iSize)
    {
        int iReceived = recv(_clientSocket, _szBuffer + iTotalRecv, _iSize - iTotalRecv, 0);
        if (iReceived > 0)
        {
            iTotalRecv += iReceived;
            cout << "Bytes received: " << iReceived << endl;
        }
        else if (iReceived == 0)
        {
            cout << "Connection closing..." << endl;
            return false; // 연결 종료
        }
        else
        {
            cerr << "Receive failed: " << WSAGetLastError() << endl;
            return false; // 오류
        }
    }
    return true;
}

bool ThreadPool::RecvPacketFromClient(const SOCKET& _clientSocket, std::array<char, MAX_RECV_SIZE>& _arrRecvBuffer, PACKET_CHAT_MSG_RECV& _tRecvPacket)
{
    // 1) 헤더 먼저 받기
    if (!RecvExact(_clientSocket, _arrRecvBuffer.data(), sizeof(PACKET_HEADER)))
        return false;

    // 2) 헤더 역직렬화
    PACKET_HEADER tHeader{};
    tHeader.Deserialize(_arrRecvBuffer.data());

    // 사이즈 예외처리
    if (tHeader.iSize > MAX_RECV_SIZE)
    {
        cerr << "Message too large! size=" << tHeader.iSize << endl;
        return false;
    }

    // 3) 남은 본문 받기 (헤더 제외)
    if (!RecvExact(_clientSocket, _arrRecvBuffer.data() + sizeof(PACKET_HEADER), tHeader.iSize - sizeof(PACKET_HEADER)))
        return false;

    // 4) 본문 역질력화
    _tRecvPacket.Deserialize(_arrRecvBuffer.data());

    return true;
}

bool ThreadPool::SendExact(const SOCKET& _clientSocket, const char* _szBuffer, const uint32_t& _iSize)
{
    int iTotalSent = 0;
    while (iTotalSent < _iSize)
    {
        int iSent = send(_clientSocket, _szBuffer + iTotalSent, _iSize - iTotalSent, 0);
        if (iSent == SOCKET_ERROR)
        {
            cerr << "Send failed: " << WSAGetLastError() << endl;
            return false;
        }
        iTotalSent += iSent;
        cout << "Bytes sent: " << iSent << endl;
    }
    return true;
}

bool ThreadPool::SendPacketToClient(const SOCKET& _clientSocket, const std::array<char, MAX_RECV_SIZE>& _arrSendBuffer, const uint32_t& _iSizeMsg)
{
    if (_iSizeMsg > MAX_SEND_SIZE)
    {
        cerr << "Message too large to send! size=" << _iSizeMsg << endl;
        return false;
    }

    // 1) 본문
    if (!SendExact(_clientSocket, _arrSendBuffer.data(), _iSizeMsg))
        return false;

    return true;
}