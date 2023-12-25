#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <mutex>

#include "FilterStream.h"

#include <WinSock2.h>

class FilterManager;
class IFilterHooks;

class Filter
{
private:
	const IFilterHooks* pHooks;
	FilterManager* pFilterManager;

	std::string ip;
	uint16_t port;
	bool allowDynamicPort;

	std::string targetIP;
	uint16_t targetPort;

	SOCKET serverSocket;

	std::vector<std::unique_ptr<FilterStream>> streams;

	void CloseAllStreams();

public:
	Filter(const IFilterHooks* hooks, FilterManager* pFilterManager) : pFilterManager(pFilterManager), pHooks(hooks), ip("127.0.0.1"), port(0), targetIP(), targetPort(0), allowDynamicPort(false), serverSocket(INVALID_SOCKET) {}
	Filter(const IFilterHooks* hooks, FilterManager* pFilterManager, std::string ip, uint16_t port, std::string_view targetIP, uint16_t targetPort, bool allowDynamicPort = false) : pFilterManager(pFilterManager), pHooks(hooks), ip(std::move(ip)), port(port), targetIP(targetIP), targetPort(targetPort), allowDynamicPort(allowDynamicPort), serverSocket(INVALID_SOCKET) {}
	Filter(const Filter&) = delete;
	Filter(Filter&&) = delete;
	Filter& operator=(const Filter&) = delete;
	Filter& operator=(Filter&&) = delete;

	bool ParseCommandLineArgs(int argc, const char** argv);

	const std::string& GetIP() const { return ip; }
	uint16_t GetPort() const { return port; }
	const std::string& GetTargetIP() const { return targetIP; }
	uint16_t GetTargetPort() const { return targetPort; }

	const IFilterHooks* Hooks() { return pHooks; }

	static void Init();
	static void Uninit();
	void Start();
	void ProcessLoop();
	void Close();
};