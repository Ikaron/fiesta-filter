#include "FilterLib/FilterStream.h"
#include "FilterLib/Filter.h"
#include "FilterLib/Logger.h"

#include <WS2tcpip.h>
#include <format>
#include <iostream>

static const unsigned char xorTable[] =
{
	0x07, 0x59, 0x69, 0x4A, 0x94, 0x11, 0x94, 0x85, 0x8C, 0x88, 0x05, 0xCB, 0xA0, 0x9E, 0xCD, 0x58,
	0x3A, 0x36, 0x5B, 0x1A, 0x6A, 0x16, 0xFE, 0xBD, 0xDF, 0x94, 0x02, 0xF8, 0x21, 0x96, 0xC8, 0xE9,
	0x9E, 0xF7, 0xBF, 0xBD, 0xCF, 0xCD, 0xB2, 0x7A, 0x00, 0x9F, 0x40, 0x22, 0xFC, 0x11, 0xF9, 0x0C,
	0x2E, 0x12, 0xFB, 0xA7, 0x74, 0x0A, 0x7D, 0x78, 0x40, 0x1E, 0x2C, 0xA0, 0x2D, 0x06, 0xCB, 0xA8,
	0xB9, 0x7E, 0xEF, 0xDE, 0x49, 0xEA, 0x4E, 0x13, 0x16, 0x16, 0x80, 0xF4, 0x3D, 0xC2, 0x9A, 0xD4,
	0x86, 0xD7, 0x94, 0x24, 0x17, 0xF4, 0xD6, 0x65, 0xBD, 0x3F, 0xDB, 0xE4, 0xE1, 0x0F, 0x50, 0xF6,
	0xEC, 0x7A, 0x9A, 0x0C, 0x27, 0x3D, 0x24, 0x66, 0xD3, 0x22, 0x68, 0x9C, 0x9A, 0x52, 0x0B, 0xE0,
	0xF9, 0xA5, 0x0B, 0x25, 0xDA, 0x80, 0x49, 0x0D, 0xFD, 0x3E, 0x77, 0xD1, 0x56, 0xA8, 0xB7, 0xF4,
	0x0F, 0x9B, 0xE8, 0x0F, 0x52, 0x47, 0xF5, 0x6F, 0x83, 0x20, 0x22, 0xDB, 0x0F, 0x0B, 0xB1, 0x43,
	0x85, 0xC1, 0xCB, 0xA4, 0x0B, 0x02, 0x19, 0xDF, 0xF0, 0x8B, 0xEC, 0xDB, 0x6C, 0x6D, 0x66, 0xAD,
	0x45, 0xBE, 0x89, 0x14, 0x7E, 0x2F, 0x89, 0x10, 0xB8, 0x93, 0x60, 0xD8, 0x60, 0xDE, 0xF6, 0xFE,
	0x6E, 0x9B, 0xCA, 0x06, 0xC1, 0x75, 0x95, 0x33, 0xCF, 0xC0, 0xB2, 0xE0, 0xCC, 0xA5, 0xCE, 0x12,
	0xF6, 0xE5, 0xB5, 0xB4, 0x26, 0xC5, 0xB2, 0x18, 0x4F, 0x2A, 0x5D, 0x26, 0x1B, 0x65, 0x4D, 0xF5,
	0x45, 0xC9, 0x84, 0x14, 0xDC, 0x7C, 0x12, 0x4B, 0x18, 0x9C, 0xC7, 0x24, 0xE7, 0x3C, 0x64, 0xFF,
	0xD6, 0x3A, 0x2C, 0xEE, 0x8C, 0x81, 0x49, 0x39, 0x6C, 0xB7, 0xDC, 0xBD, 0x94, 0xE2, 0x32, 0xF7,
	0xDD, 0x0A, 0xFC, 0x02, 0x01, 0x64, 0xEC, 0x4C, 0x94, 0x0A, 0xB1, 0x56, 0xF5, 0xC9, 0xA9, 0x34,
	0xDE, 0x0F, 0x38, 0x27, 0xBC, 0x81, 0x30, 0x0F, 0x7B, 0x38, 0x25, 0xFE, 0xE8, 0x3E, 0x29, 0xBA,
	0x55, 0x43, 0xBF, 0x6B, 0x9F, 0x1F, 0x8A, 0x49, 0x52, 0x18, 0x7F, 0x8A, 0xF8, 0x88, 0x24, 0x5C,
	0x4F, 0xE1, 0xA8, 0x30, 0x87, 0x8E, 0x50, 0x1F, 0x2F, 0xD1, 0x0C, 0xB4, 0xFD, 0x0A, 0xBC, 0xDC,
	0x12, 0x85, 0xE2, 0x52, 0xEE, 0x4A, 0x58, 0x38, 0xAB, 0xFF, 0xC6, 0x3D, 0xB9, 0x60, 0x64, 0x0A,
	0xB4, 0x50, 0xD5, 0x40, 0x89, 0x17, 0x9A, 0xD5, 0x85, 0xCF, 0xEC, 0x0D, 0x7E, 0x81, 0x7F, 0xE3,
	0xC3, 0x04, 0x01, 0x22, 0xEC, 0x27, 0xCC, 0xFA, 0x3E, 0x21, 0xA6, 0x54, 0xC8, 0xDE, 0x00, 0xB6,
	0xDF, 0x27, 0x9F, 0xF6, 0x25, 0x34, 0x07, 0x85, 0xBF, 0xA7, 0xA5, 0xA5, 0xE0, 0x83, 0x0C, 0x3D,
	0x5D, 0x20, 0x40, 0xAF, 0x60, 0xA3, 0x64, 0x56, 0xF3, 0x05, 0xC4, 0x1C, 0x7D, 0x37, 0x98, 0xC3,
	0xE8, 0x5A, 0x6E, 0x58, 0x85, 0xA4, 0x9A, 0x6B, 0x6A, 0xF4, 0xA3, 0x7B, 0x61, 0x9B, 0x09, 0x40,
	0x1E, 0x60, 0x4B, 0x32, 0xD9, 0x51, 0xA4, 0xFE, 0xF9, 0x5D, 0x4E, 0x4A, 0xFB, 0x4A, 0xD4, 0x7C,
	0x33, 0x02, 0x33, 0xD5, 0x9D, 0xCE, 0x5B, 0xAA, 0x5A, 0x7C, 0xD8, 0xF8, 0x05, 0xFA, 0x1F, 0x2B,
	0x8C, 0x72, 0x57, 0x50, 0xAE, 0x6C, 0x19, 0x89, 0xCA, 0x01, 0xFC, 0xFC, 0x29, 0x9B, 0x61, 0x12,
	0x68, 0x63, 0x65, 0x46, 0x26, 0xC4, 0x5B, 0x50, 0xAA, 0x2B, 0xBE, 0xEF, 0x9A, 0x79, 0x02, 0x23,
	0x75, 0x2C, 0x20, 0x13, 0xFD, 0xD9, 0x5A, 0x76, 0x23, 0xF1, 0x0B, 0xB5, 0xB8, 0x59, 0xF9, 0x9F,
	0x7A, 0xE6, 0x06, 0xE9, 0xA5, 0x3A, 0xB4, 0x50, 0xBF, 0x16, 0x58, 0x98, 0xB3, 0x9A, 0x6E, 0x36,
	0xEE, 0x8D, 0xEB
};

static const size_t xorTableLength = _countof(xorTable);

class EncryptionState
{
private:
	void Crypt(std::vector<char>& packet, uint16_t& xorPos)
	{
		for (auto i = 0; i < packet.size(); ++i)
		{
			packet[i] ^= xorTable[xorPos];
			xorPos = (xorPos + 1) % xorTableLength;
		}
	}
public:
	bool enableCrypt = false;
	uint16_t xorPosReceive = 0;
	uint16_t xorPosSend = 0;

	void Decrypt(std::vector<char>& packet)
	{
		Crypt(packet, xorPosReceive);
	}

	void Encrypt(std::vector<char>& packet)
	{
		Crypt(packet, xorPosSend);
	}
};

class StreamData
{
private:
	enum class PacketReadState
	{
		Unread,
		ReadZeroLength,
		ReadLengthByte1,
		ReadLength,
	};
	enum class ProcessDataResult
	{
		Success,
		NeedsDisconnect,
		NeedsBan,
	};
public:
	enum class ReceiveResult
	{
		Success,
		Closed,
		Error
	};
	bool isClientToServer;
	std::vector<char> buf;
	WSAOVERLAPPED overlapped;
	HANDLE hEvent = NULL;
	DWORD received = 0;

	PacketReadState packetRead = PacketReadState::Unread;
	uint16_t packetLength = 0;
	std::vector<char> packetBuf;
	std::vector<char> packetSendBuf;

	EncryptionState* pEncryptionState;

	StreamData(bool isClientToServer, size_t bufLen, EncryptionState* pEncryptionState) : isClientToServer(isClientToServer), buf(bufLen), pEncryptionState(pEncryptionState)
	{
		hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
		overlapped.hEvent = hEvent;
		packetBuf.reserve(bufLen);
		packetSendBuf.reserve(bufLen);
	}

	~StreamData() = default;
	StreamData(const StreamData&) = delete;
	StreamData(StreamData&&) = delete;
	StreamData& operator=(const StreamData&) = delete;
	StreamData& operator=(StreamData&&) = delete;

	ReceiveResult StartReceive(FilterStream** pStr)
	{
		while (true)
		{
			WSABUF wsaBuf;
			wsaBuf.buf = buf.data();
			wsaBuf.len = (ULONG) buf.size();
			DWORD flags = 0;
			auto result = WSARecv(isClientToServer ? (*pStr)->clientSock : (*pStr)->serverSock, &wsaBuf, 1, &received, &flags, &overlapped, NULL);

			if (result != SOCKET_ERROR)
			{ 
				if (received == 0)
					return ReceiveResult::Closed;
				if (!ProcessAsyncResult(pStr, true))
					return ReceiveResult::Error;
				continue;
			}
			else
			{
				auto lastError = WSAGetLastError();
				if (lastError == WSA_IO_PENDING)
					return ReceiveResult::Success;
				Log.WriteSingleLine(LogLevel::Error, std::format("Unable to StartReceive - Error {}", lastError));
				return ReceiveResult::Error;
			}
		}
	}

	ProcessDataResult ProcessData(FilterStream** pStr, size_t& readOffset)
	{
		if (packetRead == PacketReadState::Unread && received - readOffset >= 1)
		{
			packetLength = buf[readOffset++] & 0xff;
			if (packetLength != 0)
				packetRead = PacketReadState::ReadLength;
			else
				packetRead = PacketReadState::ReadZeroLength;
		}

		if (packetRead == PacketReadState::ReadZeroLength && received - readOffset >= 1)
		{
			packetLength |= buf[readOffset++] & 0xff;
			packetRead = PacketReadState::ReadLengthByte1;
		}

		if (packetRead == PacketReadState::ReadLengthByte1 && received - readOffset >= 1)
		{
			packetLength |= (buf[readOffset++] & 0xff) << 8;
			packetRead = PacketReadState::ReadLength;
		}

		if (packetRead != PacketReadState::ReadLength)
			return ProcessDataResult::Success;

		size_t leftToRead = packetLength - packetBuf.size();
		size_t ableToRead = received - readOffset;

		if (ableToRead == 0)
			return ProcessDataResult::Success;

		size_t toRead = (std::min)(leftToRead, ableToRead);
		size_t previousPacketRead = packetBuf.size();

		packetBuf.resize(previousPacketRead + toRead);
		memcpy(packetBuf.data() + previousPacketRead, buf.data() + readOffset, toRead);
		readOffset += toRead;

		if(packetBuf.size() != packetLength)
			return ProcessDataResult::Success;

		// We look for encryption packets
		if (!isClientToServer && packetBuf.size() == 4)
		{
			if (packetBuf[0] == 0x07 && packetBuf[1] == 0x08)
			{
				uint16_t xorPos = 0;
				xorPos |= packetBuf[2] & 0xFF;
				xorPos |= (((uint16_t) packetBuf[3]) << 8);

				pEncryptionState->enableCrypt = true;
				pEncryptionState->xorPosReceive = xorPos;
				pEncryptionState->xorPosSend = xorPos;
			}
		}

		if (isClientToServer && pEncryptionState->enableCrypt)
			pEncryptionState->Decrypt(packetBuf);

		FilterHookResponse filterResponse = FilterHookResponse::KeepPacket;

		for (const auto& hook : (*pStr)->vHooks)
		{
			if ((hook->hookType & (isClientToServer ? FilterHookType::ClientToServer : FilterHookType::ServerToClient)) == 0)
				continue;

			auto response = hook->FilterPacket(packetBuf, isClientToServer);

			if (response > filterResponse)
				filterResponse = response;
		}

		switch (filterResponse)
		{
		case FilterHookResponse::KeepPacket:
			break;
		case FilterHookResponse::RewritePacket:
			packetLength = (uint16_t) packetBuf.size();
			break;
		case FilterHookResponse::DropPacket:
			packetRead = PacketReadState::Unread;
			packetBuf.resize(0);
			return ProcessDataResult::Success;
		case FilterHookResponse::Disconnect:
			return ProcessDataResult::NeedsDisconnect;
		case FilterHookResponse::Ban:
			return ProcessDataResult::NeedsBan;
		}

		if (isClientToServer && pEncryptionState->enableCrypt)
			pEncryptionState->Encrypt(packetBuf);

		size_t writeOffset = 0;
		packetSendBuf.resize(0);
		if (packetLength <= 0xFF)
		{
			packetSendBuf.push_back((char) packetLength);
			writeOffset = 1;
		}
		else
		{
			packetSendBuf.push_back(0);
			packetSendBuf.push_back((char) (packetLength & 0xFF));
			packetSendBuf.push_back((char) (packetLength >> 8));
			writeOffset = 3;
		}

		packetSendBuf.resize(writeOffset + packetLength);
		memcpy(packetSendBuf.data() + writeOffset, packetBuf.data(), packetLength);

		packetRead = PacketReadState::Unread;
		packetBuf.resize(0);

		auto result = send(isClientToServer ? (*pStr)->serverSock : (*pStr)->clientSock, packetSendBuf.data(), (int) packetSendBuf.size(), 0);
		if (result == SOCKET_ERROR)
		{
			auto lastError = WSAGetLastError();
			if (isClientToServer)
				Log.WriteSingleLine(LogLevel::Error, std::format("Unable to write to server - Error {}", lastError));
			else
				Log.WriteSingleLine(LogLevel::Error, std::format("Unable to write to client - Error {}", lastError));
			return ProcessDataResult::NeedsDisconnect;
		}

		return ProcessDataResult::Success;
	}

	bool ProcessAsyncResult(FilterStream** pStr, bool immediate)
	{
		if (!immediate)
		{
			DWORD flags = 0;
			if (!WSAGetOverlappedResult(isClientToServer ? (*pStr)->clientSock : (*pStr)->serverSock, &overlapped, &received, FALSE, &flags))
			{
				Log.WriteSingleLine(LogLevel::Error, "Overlapped result not yet available!");
				return false;
			}
		}

		if (received == 0)
		{
			Log.WriteSingleLine(LogLevel::Status, std::format("Closing tunnel from {}:{} to {}.{}", (*pStr)->clientIP, (*pStr)->clientPort, (*pStr)->pFilter->GetTargetIP(), (*pStr)->pFilter->GetTargetPort()));
			return false;
		}

		size_t readOffset = 0;
		while (received - readOffset > 0)
		{
			auto result = ProcessData(pStr, readOffset);

			if (result == ProcessDataResult::NeedsBan)
			{
				Log.WriteSingleLine(LogLevel::Error, "We need to ban this client!");
			}

			if (result != ProcessDataResult::Success)
				return false;
		}

		return true;
	}
};

void FilterStream::Pipe(FilterStream** pStr)
{
	std::unique_ptr<EncryptionState> pEncryptionState = std::make_unique<EncryptionState>();

	const size_t bufLen = 4096;
	StreamData clientToServer(true, bufLen, pEncryptionState.get());
	StreamData serverToClient(false, bufLen, pEncryptionState.get());
	
	auto receiveResult = clientToServer.StartReceive(pStr);
	if (receiveResult != StreamData::ReceiveResult::Success)
	{
		if (receiveResult == StreamData::ReceiveResult::Error)
			Log.WriteSingleLine(LogLevel::Error, "Unable to read from client!");
		(*pStr)->CloseWithinThread();
		return;
	}

	receiveResult = serverToClient.StartReceive(pStr);
	if (receiveResult != StreamData::ReceiveResult::Success)
	{
		if (receiveResult == StreamData::ReceiveResult::Error)
			Log.WriteSingleLine(LogLevel::Error, "Unable to read from server!");
		(*pStr)->CloseWithinThread();
		return;
	}

	std::vector<QueuedPacket> injected;
	std::vector<char> injectBuf;

	while (!(*pStr)->stopSignal.load())
	{
		HANDLE handles[] = { clientToServer.hEvent, serverToClient.hEvent, (*pStr)->hInjectedPacketEvent };
		auto waitResult = WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);

		if (waitResult == WAIT_OBJECT_0)
		{
			if (!clientToServer.ProcessAsyncResult(pStr, false))
				break;

			receiveResult = clientToServer.StartReceive(pStr);
			if (receiveResult != StreamData::ReceiveResult::Success)
			{
				if (receiveResult == StreamData::ReceiveResult::Error)
					Log.WriteSingleLine(LogLevel::Error, "Unable to read from client!");
				break;
			}
		}
		else if (waitResult == WAIT_OBJECT_0 + 1)
		{
			if (!serverToClient.ProcessAsyncResult(pStr, false))
				break;

			receiveResult = serverToClient.StartReceive(pStr);
			if (receiveResult != StreamData::ReceiveResult::Success)
			{
				if (receiveResult == StreamData::ReceiveResult::Error)
					Log.WriteSingleLine(LogLevel::Error, "Unable to read from server!");
				break;
			}
		}
		else if (waitResult == WAIT_OBJECT_0 + 2)
		{
			(*pStr)->GrabInjectedPackets(injected);

			for (auto& p : injected)
			{
				if (p.isClientToServer && pEncryptionState->enableCrypt)
					pEncryptionState->Encrypt(p.data);

				size_t packetLength = p.data.size();
				size_t writeOffset = 0;
				injectBuf.resize(0);
				if (packetLength <= 0xFF)
				{
					injectBuf.push_back((char)packetLength);
					writeOffset = 1;
				}
				else
				{
					injectBuf.push_back(0);
					injectBuf.push_back((char)(packetLength & 0xFF));
					injectBuf.push_back((char)(packetLength >> 8));
					writeOffset = 3;
				}

				injectBuf.resize(writeOffset + packetLength);
				memcpy(injectBuf.data() + writeOffset, p.data.data(), packetLength);

				auto result = send(p.isClientToServer ? (*pStr)->serverSock : (*pStr)->clientSock, injectBuf.data(), (int)injectBuf.size(), 0);
				if (result == SOCKET_ERROR)
				{
					auto lastError = WSAGetLastError();
					if (p.isClientToServer)
						Log.WriteSingleLine(LogLevel::Error, std::format("Unable to write injected to server - Error {}", lastError));
					else
						Log.WriteSingleLine(LogLevel::Error, std::format("Unable to write injected to client - Error {}", lastError));
					break;
				}
			}
		}
		else
		{
			Log.WriteSingleLine(LogLevel::Error, "Unable to wait for reads!");
			break;
		}
	}

	Log.WriteSingleLine(LogLevel::Status, std::format("Closed tunnel from {}:{} to {}.{}", (*pStr)->clientIP, (*pStr)->clientPort, (*pStr)->pFilter->GetTargetIP(), (*pStr)->pFilter->GetTargetPort()));

	(*pStr)->CloseWithinThread();
}

FilterStream::FilterStream(FilterManager* pFilterManager, Filter* pFilter, const std::string& targetIP, uint16_t targetPort, SOCKET clientSock) : pFilterManager(pFilterManager), pFilter(pFilter), clientSock(clientSock), serverSock(INVALID_SOCKET), stopSignal(false), pThreadData(nullptr), clientIP(), clientPort(0), pUserData(nullptr)
{
	hInjectedPacketEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hInjectedPacketEvent == NULL)
	{
		Log.WriteSingleLine(LogLevel::Error, "Unable to create packet injection mutex!");
		Close();
		return;
	}

	addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	addrinfo* addr = NULL;
	auto result = getaddrinfo(targetIP.c_str(), std::format("{0}", targetPort).c_str(), &hints, &addr);
	if (result != 0)
	{
		Log.WriteSingleLine(LogLevel::Error, std::format("Unable to get address of target server {}:{} - Error {}", targetIP, targetPort, result));
		Close();
		return;
	}

	serverSock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (serverSock == INVALID_SOCKET)
	{
		auto lastError = WSAGetLastError();
		freeaddrinfo(addr);
		Log.WriteSingleLine(LogLevel::Error, std::format("Unable to create socket to target server {}:{} - Error {}", targetIP, targetPort, lastError));
		Close();
		return;
	}

	result = connect(serverSock, addr->ai_addr, (int) addr->ai_addrlen);
	if (result != 0)
	{
		freeaddrinfo(addr);
		Log.WriteSingleLine(LogLevel::Error, std::format("Unable to connect to target server {}:{} - Error {}", targetIP, targetPort, result));
		Close();
		return;
	}

	freeaddrinfo(addr);

	{
		struct sockaddr_in clientAddr;
		ZeroMemory(&clientAddr, sizeof(clientAddr));
		socklen_t len = sizeof(clientAddr);
		if (getpeername(clientSock, (struct sockaddr*)&clientAddr, &len) == 0)
		{
			char ip[47];
			if (inet_ntop(AF_INET, &clientAddr.sin_addr, ip, _countof(ip)))
				clientIP = std::string(ip);

			clientPort = ntohs(clientAddr.sin_port);
		}
	}

	Log.WriteSingleLine(LogLevel::Status, std::format("Starting tunnel from {}:{} to {}.{}", clientIP, clientPort, targetIP, targetPort));

	pThreadData = new FilterStream*(this);

	for (const auto& h : pFilter->Hooks()->GetHooks())
		vHooks.push_back(h.MakeUnique(pFilterManager, pFilter, this));

	threads.emplace_back([this] { Pipe(pThreadData); });
}

FilterStream::~FilterStream()
{
	Close();
}

const std::string& FilterStream::GetTargetIP() const
{
	return pFilter->GetTargetIP();
}

uint16_t FilterStream::GetTargetPort() const
{
	return pFilter->GetTargetPort();
}


void FilterStream::InjectPacket(const QueuedPacket& p)
{
	{
		std::lock_guard<std::mutex> lock(injectedPacketMutex);
		injectedPackets.emplace_back(p);
	}

	SetEvent(hInjectedPacketEvent);
}

void FilterStream::InjectPacket(QueuedPacket&& p)
{
	{
		std::lock_guard<std::mutex> lock(injectedPacketMutex);
		injectedPackets.emplace_back(std::move(p));
	}

	SetEvent(hInjectedPacketEvent);
}


void FilterStream::GrabInjectedPackets(std::vector<QueuedPacket>& buffer)
{
	buffer.clear();

	{
		std::lock_guard<std::mutex> lock(injectedPacketMutex);
		for (auto& p : injectedPackets)
			buffer.emplace_back(std::move(p));
		injectedPackets.clear();
	}
}

void FilterStream::Close()
{
	stopSignal = true;
	for (auto& t : threads)
		t.join();
	threads.clear();

	if (clientSock != INVALID_SOCKET)
	{
		shutdown(clientSock, SD_SEND);
		closesocket(clientSock);
		clientSock = INVALID_SOCKET;
	}

	if (serverSock != INVALID_SOCKET)
	{
		shutdown(serverSock, SD_SEND);
		closesocket(serverSock);
		serverSock = INVALID_SOCKET;
	}

	if (hInjectedPacketEvent != NULL)
	{
		CloseHandle(hInjectedPacketEvent);
		hInjectedPacketEvent = NULL;
	}

	if (pThreadData != nullptr)
	{
		delete pThreadData;
		pThreadData = nullptr;
	}
}

void FilterStream::CloseWithinThread()
{
	stopSignal = true;

	if (clientSock != INVALID_SOCKET)
	{
		shutdown(clientSock, SD_SEND);
		closesocket(clientSock);
		clientSock = INVALID_SOCKET;
	}

	if (serverSock != INVALID_SOCKET)
	{
		shutdown(serverSock, SD_SEND);
		closesocket(serverSock);
		serverSock = INVALID_SOCKET;
	}

	if (pThreadData != nullptr)
	{
		delete pThreadData;
		pThreadData = nullptr;
	}
}
