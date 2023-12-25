#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <span>
#include <istream>
#include <vector>

class FilterUtils
{
public:
	FilterUtils() = delete;

	static bool ParseIP(std::string_view str, uint32_t& ip);
	static bool ParsePort(std::string_view str, uint16_t& port);
	static std::string FormatIP(uint32_t ip);
	static bool SanitiseIP(std::string& ip);

	static std::string DumpHex(const char* pData, size_t length);

	enum DateTimeFormat
	{
		Years,
		Months,
		Days,
		Hours,
		Minutes,
		Seconds,
		Milliseconds
	};

	static std::string FormatDateTime(DateTimeFormat = DateTimeFormat::Milliseconds, bool forFileName = false);

	static void PrintCommandLineUsage();

	static bool ParseCommandLineStringOptional(int argc, const char** argv, std::string_view key, std::string& str, bool& present);
	static bool ParseCommandLineString(int argc, const char** argv, std::string_view key, std::string& str);
	static bool ParseCommandLineFlag(int argc, const char** argv, std::string_view key, bool& flag);
	static bool ParseCommandLineIP(int argc, const char** argv, std::string_view key, std::string& ip, std::string_view fallback = 0);
	static bool ParseCommandLinePort(int argc, const char** argv, std::string_view key, uint16_t& port, uint16_t fallback = 0);
	static bool ParseCommandLineOption(int argc, const char** argv, std::string_view key, std::span<std::string_view> pOptions, intptr_t& index, intptr_t fallback = -1);

	static int QuitWithWait();
	static int QuitWithUsage();
};

class FiestaConfigNation
{
public:
	std::string name;
};

class FiestaConfigWorld
{
public:
	int index;
	std::string name;
	std::string dataPath;
};

namespace FiestaConfigServerType
{
	enum FiestaConfigServerType
	{
		Account = 0,
		AccountLog = 1,
		WorldChar = 2,
		WorldGameLog = 3,
		Login = 4,
		WorldManager = 5,
		Zone = 6,
		OPTool = 8,
		Client = 20
	};
}

class FiestaConfigServerConnection
{
public:
	FiestaConfigServerType::FiestaConfigServerType fromServerType;
	std::string bindIP;
	uint16_t bindPort;
	size_t maxConnectionBacklog;
	size_t maxClients;
};

class FiestaConfigServerInfo
{
public:
	std::string name;
	FiestaConfigServerType::FiestaConfigServerType serverType;
	int worldId;
	int zoneId;
	std::vector<FiestaConfigServerConnection> connections;
};

namespace FiestaConfigODBCEndpoint
{
	enum FiestaConfigODBCEndpoint
	{
		Account = 0,
		AccountLog = 1,
		Statistics = 2,
		OPTool = 3,
		Character = 10,
		GameLog = 11
	};
}

class FiestaConfigODBCInfo
{
public:
	std::string name;
	FiestaConfigODBCEndpoint::FiestaConfigODBCEndpoint endpoint;
	int worldId;
	std::string connectionString;
	std::string initSQL;
};

class FiestaConfigTunnelBinding
{
public:
	int bindingId;
	std::string bindIP;
};

class FiestaConfigTunnelInfo
{
public:
	std::string name;
	FiestaConfigServerType::FiestaConfigServerType serverType;
	int worldId;
	int zoneId;
	int bindingId;
	uint16_t bindPort;
};

class FiestaConfig
{
public:
	FiestaConfigNation nation;
	std::vector<FiestaConfigWorld> worlds;
	std::vector<FiestaConfigServerInfo> servers;
	std::vector<FiestaConfigODBCInfo> odbcInfos;
	std::vector<FiestaConfigTunnelBinding> tunnelBindings;
	std::vector<FiestaConfigTunnelInfo> tunnelInfos;

	bool Parse(std::istream&);

	bool GetBindIPForTunnelBinding(int id, std::string& ip);
};