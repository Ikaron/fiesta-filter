#pragma once

#include "FilterHook.h"
#include "BaseHook.h"

#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <WinSock2.h>

class FilterStreamKey
{
public:
	const std::string clientIP;
	const uint16_t clientPort;

	const std::string serverIP;
	const uint16_t serverPort;

	FilterStreamKey(std::string_view clientIP, uint16_t clientPort, std::string_view serverIP, uint16_t serverPort) : clientIP(clientIP), clientPort(clientPort), serverIP(serverIP), serverPort(serverPort) {}
	FilterStreamKey(std::string&& clientIP, uint16_t clientPort, std::string&& serverIP, uint16_t serverPort) : clientIP(clientIP), clientPort(clientPort), serverIP(serverIP), serverPort(serverPort) {}
	FilterStreamKey(const FilterStreamKey&) = default;
	FilterStreamKey(FilterStreamKey&&) = default;
	FilterStreamKey& operator=(const FilterStreamKey&) = default;
	FilterStreamKey& operator=(FilterStreamKey&&) = default;
	~FilterStreamKey() = default;
};

template<> struct std::less<FilterStreamKey>
{
	bool operator() (const FilterStreamKey& lhs, const FilterStreamKey& rhs) const
	{
		if (lhs.clientIP < rhs.clientIP)
			return true;
		if (lhs.clientIP > rhs.clientIP)
			return false;

		if (lhs.clientPort < rhs.clientPort)
			return true;
		if (lhs.clientPort > rhs.clientPort)
			return false;

		if (lhs.serverIP < rhs.serverIP)
			return true;
		if (lhs.serverIP > rhs.serverIP)
			return false;

		if (lhs.serverPort < rhs.serverPort)
			return true;
		if (lhs.serverPort > rhs.serverPort)
			return false;

		return false;
	}
};

class Filter;
class FilterManager;
class StreamData;

class QueuedPacket
{
public:
	bool isClientToServer;
	std::vector<char> data;

	QueuedPacket(bool isClientToServer) : isClientToServer(isClientToServer), data(data) {}
	QueuedPacket(bool isClientToServer, const std::vector<char>& data) : isClientToServer(isClientToServer), data(data) {}
	QueuedPacket(const QueuedPacket&) = default;
	QueuedPacket(QueuedPacket&&) = default;
	QueuedPacket& operator=(const QueuedPacket&) = default;
	QueuedPacket& operator=(QueuedPacket&&) = default;
	~QueuedPacket() = default;
};

class FilterStream
{
private:
	friend class StreamData;

	FilterManager* pFilterManager;
	Filter* pFilter;
	SOCKET clientSock;
	SOCKET serverSock;
	std::vector<std::thread> threads;
	std::atomic<bool> stopSignal;

	FilterStream** pThreadData;
	std::vector<void*> perStreamDatas;

	std::string clientIP;
	uint16_t clientPort;

	std::vector<std::unique_ptr<BaseHook>> vHooks;

	std::mutex injectedPacketMutex;
	std::vector<QueuedPacket> injectedPackets;

	HANDLE hInjectedPacketEvent;

	static void Pipe(FilterStream** pStr);
	void CloseWithinThread();
public:
	FilterUserData* pUserData;

	FilterStream(FilterManager* pFilterManager, Filter* pFilter, const std::string& targetIP, uint16_t targetPort, SOCKET clientSock);
	~FilterStream();

	FilterStream(const FilterStream&) = delete;
	FilterStream(FilterStream&&) = delete;
	FilterStream& operator=(const FilterStream&) = delete;
	FilterStream& operator=(FilterStream&&) = delete;

	const std::string& GetClientIP() const { return clientIP; }
	uint16_t GetClientPort() const { return clientPort; }
	const std::string& GetTargetIP() const;
	uint16_t GetTargetPort() const;

	void GrabInjectedPackets(std::vector<QueuedPacket>&);

	void InjectPacket(const QueuedPacket&);
	void InjectPacket(QueuedPacket&&);

	void Close();
};