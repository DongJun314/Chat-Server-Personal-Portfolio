#pragma once
#define MAX_RECV_SIZE 512
#define MAX_SEND_SIZE 512
#define MAX_NICKNAME_SIZE 32
#define MAX_CHAT_MSG_SIZE 256

enum class ELANGUAGE_TYPE : uint8_t
{
	KOREAN = 1,
	ENGLISH,
	CHINESE,
	JAPANESE,
	END
};

enum class EPACKET_TYPE : uint8_t
{
	NICKNAME_REGISTER_REQ,
	NICKNAME_REGISTER_ACK,
	CHAT_MESSAGE,
	END
};

enum class ECHAT_TYPE : uint8_t
{
	JOIN,
	LEAVE,
	MESSAGE,
	END
};


struct PACKET_HEADER
{
	uint32_t iSize;             // 전체 패킷 길이
	EPACKET_TYPE ePacketType = EPACKET_TYPE::END;         // 패킷 타입

	void Serialize(char* _szBuffer) const
	{
		uint32_t iSerializedSize = htonl(iSize);
		memcpy(_szBuffer, &iSerializedSize, sizeof(iSerializedSize));

		uint8_t iSerializedPacketTypeSize = static_cast<uint8_t>(ePacketType);
		memcpy(_szBuffer + sizeof(iSerializedSize), &iSerializedPacketTypeSize, sizeof(iSerializedPacketTypeSize));
	}

	void Deserialize(const char* _szBuffer)
	{
		uint32_t iDeserializeSize = 0;
		memcpy(&iDeserializeSize, _szBuffer, sizeof(iDeserializeSize));
		iSize = ntohl(iDeserializeSize);

		uint8_t  iDeserializeType = 0;
		memcpy(&iDeserializeType, _szBuffer + sizeof(iDeserializeSize), sizeof(iDeserializeType));
		ePacketType = static_cast<EPACKET_TYPE>(iDeserializeType);
	}
};

struct PACKET_NICKNAME_REGISTER_RECV
{
	PACKET_HEADER tHeader;
	std::array<char, MAX_NICKNAME_SIZE> arrNickName;

	void Serialize(char* _szBuffer) const
	{
		// 1) Header 직렬화
		tHeader.Serialize(_szBuffer);

		// 2) 닉네임 문자열 복사
		memcpy(_szBuffer + sizeof(PACKET_HEADER), arrNickName.data(), arrNickName.size());
	}

	void Deserialize(const char* _szBuffer)
	{
		// 1) Header 역직렬화
		tHeader.Deserialize(_szBuffer);

		// 2) 닉네임 문자열 복사
		memcpy(arrNickName.data(), _szBuffer + sizeof(PACKET_HEADER), arrNickName.size());
	}
};

struct PACKET_NICKNAME_REGISTER_SEND
{
	PACKET_HEADER tHeader;
	bool bSuccess;
	//std::array<char, MAX_ERROR_MSG_SIZE> arrErrorMsg;

	void Serialize(char* _szBuffer) const
	{
		// 1) Header 직렬화
		tHeader.Serialize(_szBuffer);

		// 2) 성공 여부
		uint8_t iSerializedSuccess = bSuccess ? 1 : 0;
		memcpy(_szBuffer + sizeof(PACKET_HEADER), &iSerializedSuccess, sizeof(iSerializedSuccess));

		// 3) 에러 메시지
		//memcpy(_szBuffer + sizeof(PACKET_HEADER) + sizeof(iSerializedSuccess), arrErrorMsg.data(), arrErrorMsg.size());
	}

	void Deserialize(const char* _szBuffer)
	{
		// 1) Header 역직렬화
		tHeader.Deserialize(_szBuffer);

		// 2) 성공 여부
		uint8_t iDeserializeSuccess = 0;
		memcpy(&iDeserializeSuccess, _szBuffer + sizeof(PACKET_HEADER), sizeof(iDeserializeSuccess));
		bSuccess = (iDeserializeSuccess != 0);

		// 3) 에러 메시지
		//memcpy(arrErrorMsg.data(), _szBuffer + sizeof(PACKET_HEADER) + sizeof(iDeserializeSuccess), arrErrorMsg.size());
	}
};


// 채팅용 패킷 
struct PACKET_CHAT_MSG_RECV
{
	PACKET_HEADER tHeader;
	std::array<char, MAX_NICKNAME_SIZE> arrNickName{};    // 내 닉네임
	std::array<char, MAX_CHAT_MSG_SIZE> arrMessage{};    // 보낼 메시지
	ECHAT_TYPE    eChatType;

	void Serialize(char* _szBuffer) const
	{
		// 1) Header 직렬화
		tHeader.Serialize(_szBuffer);

		// 2) 닉네임 문자열 복사
		memcpy(_szBuffer + sizeof(PACKET_HEADER), arrNickName.data(), arrNickName.size());

		// 3) 메시지 문자열 복사
		memcpy(_szBuffer + sizeof(PACKET_HEADER) + arrNickName.size(), arrMessage.data(), arrMessage.size());

		// 4) 이벤트 타입 직렬화 (1바이트)
		memcpy(_szBuffer + sizeof(PACKET_HEADER) + arrNickName.size() + arrMessage.size(), &eChatType, sizeof(eChatType));
	}

	void Deserialize(const char* _szBuffer)
	{
		// 1) Header 역직렬화
		tHeader.Deserialize(_szBuffer);

		// 2) 닉네임 문자열 복사
		memcpy(arrNickName.data(), _szBuffer + sizeof(PACKET_HEADER), arrNickName.size());

		// 3) 닉네임 문자열 복사
		memcpy(arrMessage.data(), _szBuffer + sizeof(PACKET_HEADER) + arrNickName.size(), arrMessage.size());

		// 4) 이벤트 타입 역직렬화
		uint8_t iDeserializedChatType = 0;
		memcpy(&iDeserializedChatType, _szBuffer + sizeof(PACKET_HEADER) + arrNickName.size() + arrMessage.size(), sizeof(iDeserializedChatType));
		eChatType = static_cast<ECHAT_TYPE>(iDeserializedChatType);
	}
};

struct PACKET_CHAT_MSG_SEND
{
	PACKET_HEADER tHeader;
	std::array<char, MAX_NICKNAME_SIZE> arrNickName{};
	std::array<char, MAX_CHAT_MSG_SIZE> arrMessage{};
	uint32_t      iUserCount;
	ECHAT_TYPE    eChatType;

	void Serialize(char* _szBuffer) const
	{
		// 1) Header 직렬화
		tHeader.Serialize(_szBuffer);

		// 2) 닉네임 문자열 복사
		memcpy(_szBuffer + sizeof(PACKET_HEADER), arrNickName.data(), arrNickName.size());

		// 3) 메시지 문자열 복사
		memcpy(_szBuffer + sizeof(PACKET_HEADER) + arrNickName.size(), arrMessage.data(), arrMessage.size());

		// 4) 유저 수 직렬화 (엔디안 변환)
		uint32_t iSerializedUserCount = htonl(iUserCount);
		memcpy(_szBuffer + sizeof(PACKET_HEADER) + arrNickName.size() + arrMessage.size(), &iSerializedUserCount, sizeof(iSerializedUserCount));

		// 5) 이벤트 타입 직렬화 (1바이트)
		memcpy(_szBuffer + sizeof(PACKET_HEADER) + arrNickName.size() + arrMessage.size() + sizeof(uint32_t), &eChatType, sizeof(eChatType));
	}

	void Deserialize(const char* _szBuffer)
	{
		// 1) Header 역직렬화
		tHeader.Deserialize(_szBuffer);

		// 2) 닉네임 문자열 복사
		memcpy(arrNickName.data(), _szBuffer + sizeof(PACKET_HEADER), arrNickName.size());

		// 3) 닉네임 문자열 복사
		memcpy(arrMessage.data(), _szBuffer + sizeof(PACKET_HEADER) + arrNickName.size(), arrMessage.size());

		// 4) 유저 수 역직렬화 (엔디안 변환)
		uint32_t iDeserializedUserCount = 0;
		memcpy(&iDeserializedUserCount, _szBuffer + sizeof(PACKET_HEADER) + arrNickName.size() + arrMessage.size(), sizeof(iDeserializedUserCount));
		iUserCount = ntohl(iDeserializedUserCount);

		// 5) 이벤트 타입 역직렬화
		uint8_t iDeserializedChatType = 0;
		memcpy(&iDeserializedChatType, _szBuffer + sizeof(PACKET_HEADER) + arrNickName.size() + arrMessage.size() + sizeof(uint32_t), sizeof(iDeserializedChatType));
		eChatType = static_cast<ECHAT_TYPE>(iDeserializedChatType);
	}
};