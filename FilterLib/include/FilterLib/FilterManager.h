#pragma once

#include "Filter.h"
#include "FilterHook.h"
#include "FilterStream.h"
#include "Logger.h"
#include "FilterUtils.h"
#include "TypeUtils.h"
#include <thread>
#include <map>
#include <format>

class IDynamicData
{
protected:
	virtual void* GetData() = 0;
public:
	virtual ~IDynamicData() {}

	template<typename T>
	operator T& ()
	{
		return *(T*)GetData();
	}
};

template<typename T>
class DynamicData : public IDynamicData
{
private:
	T data;

protected:
	virtual void* GetData() { return (void*)&data; }

public:
	template<typename... Args>
	DynamicData(Args... args) : data(args...) {}

	template<typename... Args>
	static std::unique_ptr<IDynamicData> MakeUnique(Args... args)
	{
		return std::make_unique<DynamicData<T>>(args...);
	}
};

class FilterUserDataLock;

class FilterUserData
{
public:
	const std::string userName;

private:
	friend class FilterUserDataLock;
	std::mutex mutex;
	std::string token;
	std::string binaryToken;

	std::string ip;
	uint16_t charToken;
	std::string charName;

public:
	FilterUserData(std::string_view userName) : userName(userName), charToken(0) {}
};

class FilterUserDataLock
{
private:
	FilterUserData* pUserData;
	std::lock_guard<std::mutex> lock;
public:
	FilterUserDataLock(FilterUserData* pUserData) : pUserData(pUserData), lock(pUserData->mutex) {}
	FilterUserDataLock(const FilterUserDataLock&) = delete;
	FilterUserDataLock(FilterUserDataLock&&) = delete;
	FilterUserDataLock& operator=(const FilterUserDataLock&) = delete;
	FilterUserDataLock& operator=(FilterUserDataLock&&) = delete;

	std::string& Token() { return pUserData->token; }
	std::string& BinaryToken() { return pUserData->binaryToken; }

	std::string& IP() { return pUserData->ip; }
	uint16_t& CharToken() { return pUserData->charToken; }
	std::string& CharName() { return pUserData->charName; }
};

class FilterManager;

struct FilterTunnel
{
public:
	std::unique_ptr<Filter> pFilter;
	std::thread thread;

	FilterTunnel(const IFilterHooks* pFilterHooks, FilterManager* pFilterManager) : pFilter(std::make_unique<Filter>(pFilterHooks, pFilterManager))
	{
	}

	FilterTunnel(const IFilterHooks* pFilterHooks, FilterManager* pFilterManager, std::string bindIP, uint16_t bindPort, std::string_view serverIP, uint16_t serverPort, bool allowDynamicPort) : pFilter(std::make_unique<Filter>(pFilterHooks, pFilterManager, bindIP, bindPort, serverIP, serverPort, allowDynamicPort))
	{
	}

	bool ParseCommandLineArgs(int argc, const char** argv)
	{
		return pFilter->ParseCommandLineArgs(argc, argv);
	}

	void Start()
	{
		pFilter->Start();
		thread = std::thread([this]
			{
				pFilter->ProcessLoop();
				pFilter->Close();
			});
	}
};

class FilterBinding
{
public:
	std::string ip;
	uint16_t port;
	FilterBinding() : ip(), port(0) {}
	FilterBinding(const std::string& ip, uint16_t port) : ip(ip), port(port) {}
	FilterBinding(std::string&& ip, uint16_t port) : ip(std::move(ip)), port(port) {}
};

class FilterManager
{
private:
	bool isServer;
	bool allowDynamicPort;

	std::string loginIP;
	uint16_t loginPort = 0;

	std::map<ServerType, const IFilterHooks*> mapHooks;
	std::map<uint16_t, FilterBinding> mapServerPortToTunnelPort;

	std::mutex tunnelMutex;
	std::vector<std::unique_ptr<FilterTunnel>> tunnels;

	std::mutex userDataMutex;
	std::vector<std::unique_ptr<FilterUserData>> userData;

public:
	FilterManager() = default;
	FilterManager(const FilterManager&) = delete;
	FilterManager(FilterManager&&) = delete;
	FilterManager& operator=(const FilterManager&) = delete;
	FilterManager& operator=(FilterManager&&) = delete;
	~FilterManager() = default;

	bool ParseCommandLineArgs(int argc, const char** argv)
	{
		std::string serverConfigPath;
		if (!FilterUtils::ParseCommandLineStringOptional(argc, argv, "server-config", serverConfigPath, isServer))
			return FilterUtils::QuitWithUsage();

		FiestaConfig config;
		if (isServer)
		{
			std::ifstream configFile(serverConfigPath);

			if (!config.Parse(configFile))
			{
				Log.WriteSingleLine(LogLevel::Error, std::format("Unable to parse config file \"{}\"", serverConfigPath));
				return false;
			}

			for (const auto& t : config.tunnelInfos)
			{
				std::string bindIp;
				if (!config.GetBindIPForTunnelBinding(t.bindingId, bindIp))
					continue;

				for (const auto& s : config.servers)
				{
					if (s.serverType != t.serverType)
						continue;
					if (s.worldId != t.worldId)
						continue;
					if (s.zoneId != t.zoneId)
						continue;

					bool foundConnection = false;

					for (const auto& c : s.connections)
					{
						if (c.fromServerType == FiestaConfigServerType::Client)
						{
							mapServerPortToTunnelPort[c.bindPort] = FilterBinding(bindIp, t.bindPort);
							foundConnection = true;
							break;
						}
					}

					if (foundConnection)
						break;
				}
			}

			bool foundLogin = false;

			for (const auto& s : config.servers)
			{
				if (s.serverType != FiestaConfigServerType::Login)
					continue;
				if (s.worldId != 0)
					continue;
				if (s.zoneId != 0)
					continue;

				for (const auto& c : s.connections)
				{
					if (c.fromServerType == FiestaConfigServerType::Client)
					{
						loginIP = c.bindIP;
						loginPort = c.bindPort;
						foundLogin = true;
						break;
					}
				}

				if (foundLogin)
					break;
			}

			if (!foundLogin)
			{
				Log.WriteSingleLine(LogLevel::Error, "Login Server not found in server config!");
				return false;
			}

			allowDynamicPort = false;
		}
		else
			allowDynamicPort = true;

		return true;
	}

	void RegisterHooks(ServerType serverType, const IFilterHooks* pHooks)
	{
		mapHooks[serverType] = pHooks;
	}

private:
	FilterTunnel* CreateTunnelIfRequired(ServerType serverType, std::string_view serverIP, uint16_t serverPort, bool allowDynamicPort)
	{
		auto it = mapHooks.find(serverType);
		if (it == mapHooks.end())
			return nullptr;

		FilterBinding binding;
		auto it2 = mapServerPortToTunnelPort.find(serverPort);
		if (it2 != mapServerPortToTunnelPort.end())
			binding = it2->second;
		else
		{
			if (isServer)
			{
				Log.WriteSingleLine(LogLevel::Error, std::format("Unable to create {} tunnel to {}:{} - No binding found!", ServerTypeName(serverType), serverIP, serverIP, serverPort));
				return nullptr;
			}
			binding.ip = "127.0.0.1";
			// If we allow dynamic ports, we start at 1 above, to prevent infinite self-connects if a port is not used and we're redirecting to localhost
			binding.port = allowDynamicPort ? serverPort + 1 : serverPort;
		}

		bool reused = false;
		FilterTunnel* pTunnel = nullptr;

		{
			std::lock_guard<std::mutex> lock(tunnelMutex);

			for (auto& tunnel : tunnels)
			{
				if (tunnel->pFilter->GetTargetIP() == serverIP && tunnel->pFilter->GetTargetPort() == serverPort)
				{
					reused = true;
					pTunnel = tunnel.get();
					break;
				}
			}

			if (pTunnel == nullptr)
			{
				tunnels.emplace_back(std::make_unique<FilterTunnel>(it->second, this, binding.ip, binding.port, serverIP, serverPort, allowDynamicPort));
				pTunnel = tunnels[tunnels.size() - 1].get();

				pTunnel->Start();
			}
		}

		Log.WriteSingleLine(LogLevel::Status, std::format("{} {} Filter on Port {}, redirecting to {}:{}!", reused ? "Reusing" : "Starting", ServerTypeName(serverType), pTunnel->pFilter->GetPort(), serverIP, serverPort));

		return pTunnel;
	}

public:
	FilterTunnel* CreateTunnelIfRequired(ServerType serverType, std::string_view serverIP, uint16_t serverPort)
	{
		return CreateTunnelIfRequired(serverType, serverIP, serverPort, allowDynamicPort);
	}

	FilterTunnel* CreateTunnelIfRequired(ServerType serverType, int argc, const char** argv)
	{
		auto it = mapHooks.find(serverType);
		if (it == mapHooks.end())
			return nullptr;

		bool reused = false;
		FilterTunnel* pTunnel = nullptr;

		{
			std::lock_guard<std::mutex> lock(tunnelMutex);

			tunnels.emplace_back(std::make_unique<FilterTunnel>(it->second, this));
			pTunnel = tunnels[tunnels.size() - 1].get();
		}

		if(!pTunnel->ParseCommandLineArgs(argc, argv))
			return nullptr;

		pTunnel->Start();

		Log.WriteSingleLine(LogLevel::Status, std::format("Starting {} Filter on Port {}, redirecting to {}:{}!", ServerTypeName(serverType), pTunnel->pFilter->GetPort(), pTunnel->pFilter->GetTargetIP(), pTunnel->pFilter->GetTargetPort()));

		return pTunnel;
	}

	void Run(int argc, const char** argv)
	{
		if (isServer)
		{
			auto tunnel = CreateTunnelIfRequired(ServerType::Login, loginIP, loginPort, false);
			if (tunnel == nullptr)
				return;
			tunnel->thread.join();
		}
		else
		{
			auto tunnel = CreateTunnelIfRequired(ServerType::Login, argc, argv);
			if (tunnel == nullptr)
				return;
			tunnel->thread.join();
		}
	}

	enum class UserDataKeyType
	{
		UserName,
		Token,
		BinaryToken
	};

private:
	template<UserDataKeyType KeyType>
	FilterUserData* FindUserDataUnsafe(std::string_view key)
	{
		for (const auto& d : userData)
		{
			if constexpr (KeyType == UserDataKeyType::UserName)
				if (d->userName == key)
					return d.get();
			if constexpr (KeyType == UserDataKeyType::Token)
			{
				FilterUserDataLock userData(d.get());
				if (userData.Token() == key)
					return d.get();
			}
			if constexpr (KeyType == UserDataKeyType::BinaryToken)
			{
				FilterUserDataLock userData(d.get());
				if (userData.BinaryToken() == key)
					return d.get();
			}
		}
		return nullptr;
	}

public:
	FilterUserData* CreateUserData(std::string_view userName)
	{
		std::lock_guard<std::mutex> lock(userDataMutex);
		auto found = FindUserDataUnsafe<UserDataKeyType::UserName>(userName);
		if (found != nullptr)
			return found;

		userData.emplace_back(std::make_unique<FilterUserData>(userName));
		return userData[userData.size() - 1].get();
	}

	template<UserDataKeyType KeyType>
	FilterUserData* FindUserData(std::string_view key)
	{
		std::lock_guard<std::mutex> lock(userDataMutex);
		return FindUserDataUnsafe<KeyType>(key);
	}

	FilterUserData* FindUserData(std::string_view ip, uint16_t charToken, std::string_view charName)
	{
		std::lock_guard<std::mutex> lock(userDataMutex);
		for (const auto& d : userData)
		{
			FilterUserDataLock userData(d.get());
			if (userData.IP() == ip && userData.CharToken() == charToken && userData.CharName() == charName)
				return d.get();
		}

		return nullptr;
	}
};