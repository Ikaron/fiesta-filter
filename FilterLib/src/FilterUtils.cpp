#include "FilterLib/FilterUtils.h"

#include <format>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iostream>
#include <conio.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

bool ParseOctet(std::string_view str, uint8_t& octet)
{
    octet = 0;
    char* pEnd;
    errno = 0;
    uint64_t val = strtoull(&str[0], &pEnd, 10);
    if (errno || (val == 0 && pEnd == &str[0]))
        return false;
    if (val > 0xFF)
        return false;
    octet = (uint8_t)val;
    return true;
}

bool FilterUtils::ParseIP(std::string_view str, uint32_t& ip)
{
    ip = 0;
    uint32_t shift = 24;
    size_t lastIndex = 0;
    for (auto i = 0; i < 4; ++i)
    {
        size_t index = str.find_first_of('.', lastIndex);
        if (index == std::string::npos)
        {
            if (i != 3)
            {
                ip = 0;
                return false;
            }

            index = str.length();
        }

        uint8_t octet;
        if (!ParseOctet(str.substr(lastIndex, index - lastIndex), octet))
        {
            ip = 0;
            return false;
        }

        ip |= ((uint32_t)octet << shift);
        shift -= 8;
        lastIndex = index + 1;
    }

    return true;
}

bool FilterUtils::ParsePort(std::string_view str, uint16_t& port)
{
    port = 0;
    char* pEnd;
    uint64_t val = strtoull(&str[0], &pEnd, 10);
    errno = 0;
    if (errno || (val == 0 && pEnd == &str[0]))
        return false;
    if (val > 0xFFFF)
        return false;
    port = (uint16_t)val;
    return true;
}

std::string FilterUtils::FormatIP(uint32_t ip)
{
    return std::format("{0:d}.{1:d}.{2:d}.{3:d}", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
}

bool FilterUtils::SanitiseIP(std::string& ip)
{
    uint32_t ip32;

    if (!ParseIP(ip, ip32))
    {
        ip.clear();
        return false;
    }

    ip = FormatIP(ip32);
    return true;
}

std::string FilterUtils::DumpHex(const char* pData, size_t length)
{
    if (length == 0)
        return {};

    std::stringstream ss;

    const size_t columnCount = 16;
    size_t rowCount = (length - 1) / columnCount + 1;

    for (size_t r = 0; r < rowCount; ++r)
    {
        for (size_t c = 0; c < columnCount; ++c)
        {
            if (c == columnCount / 2)
                ss << " ";

            size_t ind = r * columnCount + c;
            if (ind >= length)
            {
                ss << "   ";
                continue;
            }

            ss << std::format("{:02x} ", pData[ind] & 0xFF);
        }

        ss << "  ";

        for (size_t c = 0; c < columnCount; ++c)
        {
            if (c == columnCount / 2)
                ss << "  ";

            size_t ind = r * columnCount + c;
            if (ind >= length)
            {
                ss << " ";
                continue;
            }

            char d = pData[ind];
            if (d >= 0x20 && d <= 0x7E)
                ss << d;
            else
                ss << '.';
        }

        ss << std::endl;
    }

    return ss.str();
}

template<FilterUtils::DateTimeFormat fmt, char yearMonthDaySep, char dayHourSep, char hourMinSecSep, char msSep>
std::string FormatDateTime()
{
    std::stringstream ss;
    ss << std::setfill('0');
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    struct tm t;
    localtime_s(&t, &tt);
    if constexpr (fmt >= FilterUtils::DateTimeFormat::Years)
    {
        ss << std::setw(0) << (t.tm_year + 1900);
    }
    if constexpr (fmt >= FilterUtils::DateTimeFormat::Months)
    {
        ss << yearMonthDaySep;
        ss << std::setw(2) << (t.tm_mon + 1);
    }
    if constexpr(fmt >= FilterUtils::DateTimeFormat::Days)
    {
        ss << yearMonthDaySep;
        ss << std::setw(2) << t.tm_mday;
    }
    if constexpr (fmt >= FilterUtils::DateTimeFormat::Hours)
    {
        ss << dayHourSep;
        ss << std::setw(2) << t.tm_hour;
    }
    if constexpr (fmt >= FilterUtils::DateTimeFormat::Minutes)
    {
        ss << hourMinSecSep;
        ss << std::setw(2) << t.tm_min;
    }
    if constexpr (fmt >= FilterUtils::DateTimeFormat::Seconds)
    {
        ss << hourMinSecSep;
        ss << std::setw(2) << t.tm_sec;
    }
    if constexpr (fmt >= FilterUtils::DateTimeFormat::Milliseconds)
    {
        ss << msSep;
        auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
        auto fraction = now - seconds;
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(fraction);
        ss << std::setw(3) << milliseconds.count();
    }

    return ss.str();
}

std::string FilterUtils::FormatDateTime(DateTimeFormat fmt, bool forFileName)
{
    if (forFileName)
    {
        switch (fmt)
        {
        case DateTimeFormat::Years:
            return ::FormatDateTime<DateTimeFormat::Years, '-', ' ', '-', '_'>();
        case DateTimeFormat::Months:
            return ::FormatDateTime<DateTimeFormat::Months, '-', ' ', '-', '_'>();
        case DateTimeFormat::Days:
            return ::FormatDateTime<DateTimeFormat::Days, '-', ' ', '-', '_'>();
        case DateTimeFormat::Hours:
            return ::FormatDateTime<DateTimeFormat::Hours, '-', ' ', '-', '_'>();
        case DateTimeFormat::Minutes:
            return ::FormatDateTime<DateTimeFormat::Minutes, '-', ' ', '-', '_'>();
        case DateTimeFormat::Seconds:
            return ::FormatDateTime<DateTimeFormat::Seconds, '-', ' ', '-', '_'>();
        case DateTimeFormat::Milliseconds:
            return ::FormatDateTime<DateTimeFormat::Milliseconds, '-', ' ', '-', '_'>();
        default:
            return "";
        }
    }
    else
    {
        switch (fmt)
        {
        case DateTimeFormat::Years:
            return ::FormatDateTime<DateTimeFormat::Years, '-', ' ', ':', '.'>();
        case DateTimeFormat::Months:
            return ::FormatDateTime<DateTimeFormat::Months, '-', ' ', ':', '.'>();
        case DateTimeFormat::Days:
            return ::FormatDateTime<DateTimeFormat::Days, '-', ' ', ':', '.'>();
        case DateTimeFormat::Hours:
            return ::FormatDateTime<DateTimeFormat::Hours, '-', ' ', ':', '.'>();
        case DateTimeFormat::Minutes:
            return ::FormatDateTime<DateTimeFormat::Minutes, '-', ' ', ':', '.'>();
        case DateTimeFormat::Seconds:
            return ::FormatDateTime<DateTimeFormat::Seconds, '-', ' ', ':', '.'>();
        case DateTimeFormat::Milliseconds:
            return ::FormatDateTime<DateTimeFormat::Milliseconds, '-', ' ', ':', '.'>();
        default:
            return "";
        }
    }
}

void FilterUtils::PrintCommandLineUsage()
{
    std::cout << "Usage: Filter.exe --bind-port=1234 (--server-ip=123.123.123.123 - Default: 127.0.0.1) --server-port=2345 (--log-to-console=Info|Status|Warning|Error|None - Default: Status) (--log-to-file=Info|Status|Warning|Error|None - Default: Status)" << std::endl;
}

bool FilterUtils::ParseCommandLineStringOptional(int argc, const char** argv, std::string_view key, std::string& str, bool& present)
{
    for (auto i = 1; i < argc; ++i)
    {
        std::string_view arg(argv[i]);
        if (!arg.starts_with("--"))
            continue;

        auto index = arg.find_first_of('=');
        if (index == std::string::npos)
        {
            std::string_view argKey = arg.substr(2);
            if (argKey == key)
            {
                std::cout << "Invalid Argument: \"" << arg << "\" - Expected a value!" << std::endl;
                return false;
            }

            continue;
        }

        {
            std::string_view argKey = arg.substr(2, index - 2);
            if (argKey != key)
                continue;

            str = arg.substr(index + 1);
            present = true;
            return true;
        }
    }

    present = false;
    return true;
}

bool FilterUtils::ParseCommandLineString(int argc, const char** argv, std::string_view key, std::string& str)
{
    bool present;
    if (!ParseCommandLineStringOptional(argc, argv, key, str, present))
        return false;

    if (!present)
    {
        std::cout << "Missing Argument: \"" << key << "\"!" << std::endl;
        return false;
    }

    return true;
}

bool FilterUtils::ParseCommandLineFlag(int argc, const char** argv, std::string_view key, bool& flag)
{
    for (auto i = 1; i < argc; ++i)
    {
        std::string_view arg(argv[i]);
        if (!arg.starts_with("--"))
            continue;

        auto index = arg.find_first_of('=');
        if (index != std::string::npos)
        {
            std::string_view argKey = arg.substr(2, index - 2);
            if (argKey == key)
            {
                std::cout << "Invalid Argument: \"" << arg << "\" - Expected no value!" << std::endl;
                return false;
            }

            continue;
        }

        {
            std::string_view argKey = arg.substr(2);
            if (argKey != key)
                continue;

            flag = true;
            return true;
        }
    }

    flag = true;
    return true;
}

bool FilterUtils::ParseCommandLineIP(int argc, const char** argv, std::string_view key, std::string& ip, std::string_view fallback)
{
    bool present;
    if (!ParseCommandLineStringOptional(argc, argv, key, ip, present))
        return false;

    if (!present)
    {
        if (!fallback.empty())
        {
            ip = fallback;
            return true;
        }

        std::cout << "Missing Argument: \"" << key << "\"!" << std::endl;
        return false;
    }

    if (ip.length() == 0 || !SanitiseIP(ip))
    {
        std::cout << "Invalid Argument: \"" << key << "\" - Expected valid IP, got \"" << ip << "\"!" << std::endl;
        return false;
    }

    return true;
}

bool FilterUtils::ParseCommandLinePort(int argc, const char** argv, std::string_view key, uint16_t& port, uint16_t fallback)
{
    std::string str;
    bool present;
    if (!ParseCommandLineStringOptional(argc, argv, key, str, present))
        return false;

    if (!present)
    {
        if (fallback != 0)
        {
            port = fallback;
            return true;
        }

        std::cout << "Missing Argument: \"" << key << "\"!" << std::endl;
        return false;
    }

    if (str.length() == 0 || !ParsePort(str, port))
    {
        std::cout << "Invalid Argument: \"" << key << "\" - Expected valid port, got \"" << str << "\"!" << std::endl;
        return false;
    }

    return true;
}

bool FilterUtils::ParseCommandLineOption(int argc, const char** argv, std::string_view key, std::span<std::string_view> pOptions, intptr_t& index, intptr_t fallback)
{
    std::string str;
    bool present;
    if (!ParseCommandLineStringOptional(argc, argv, key, str, present))
        return false;

    if (!present)
    {
        if (fallback != -1)
        {
            index = fallback;
            return true;
        }

        std::cout << "Missing Argument: \"" << key << "\"!" << std::endl;
        return false;
    }

    if (str.length() == 0)
    {
        std::cout << "Invalid Argument: \"" << key << "\" - Expected valid option, got \"" << str << "\"!" << std::endl;
        return false;
    }

    for (auto i = 0; i < pOptions.size(); ++i)
    {
        const std::string_view& opt = pOptions[i];
        if (opt == str)
        {
            index = i;
            return true;
        }
    }

    std::cout << "Invalid Argument: \"" << key << "\" - Expected valid option, got \"" << str << "\"!" << std::endl;
    return false;

}

int FilterUtils::QuitWithWait()
{
    std::cout << "Press any key to continue..." << std::endl;
    while (!_kbhit())
        Sleep(1);
    return 1;
}

int FilterUtils::QuitWithUsage()
{
    PrintCommandLineUsage();
    return QuitWithWait();
}

bool ReadToken(std::istream& in, std::string& buf)
{
    buf.clear();
    bool isInToken = false;
    bool isInString = false;

    while (in)
    {
        char c = in.peek();
        if (!in || c == EOF)
            return isInToken && !isInString;

        if (!isInString && c == ';')
            return isInToken;

        if(isInToken && isInString && c == '"')
            isInString = false;

        if (!isInToken && !std::isspace(c))
        {
            isInToken = true;
            if (c == '"')
                isInString = true;
        }

        if (isInToken)
        {
            if (!isInString && (std::isspace(c) || c == ','))
            {
                in.get();
                return buf.size() > 0;
            }

            buf.push_back((char) c);
        }

        in.get();
    }

    return isInToken && !isInString;
}

bool ReadString(std::istream& in, std::string& buf)
{
    if (!ReadToken(in, buf))
        return false;

    if (buf.size() < 2)
        return false;

    if (buf[0] != '"' || buf[buf.size() - 1] != '"')
        return false;

    buf = buf.substr(1, buf.size() - 2);
    return true;
}

bool ReadInt(std::istream& in, std::string& buf, int& num)
{
    if (!ReadToken(in, buf))
        return false;

    if (buf.size() < 1)
        return false;

    char* pEnd = nullptr;
    errno = 0;
    auto parsed = strtoll(buf.data(), &pEnd, 10);
    if (pEnd == buf.data())
        return false;
    if (errno == ERANGE)
        return false;

    num = (int) parsed;
    return true;
}

size_t FindFirstNonWhitespace(std::string_view view)
{
    for (auto i = 0; i < view.size(); ++i)
    {
        auto c = view[i];
        if (!std::isspace(c))
            return i;
    }

    return std::string::npos;
}

bool FiestaConfig::Parse(std::istream& in)
{
    if (!in)
        return false;

    std::string token;
    std::string line;

    bool isInDefine = false;

    while (std::getline(in, line))
    {
        std::string_view data = line;

        std::istringstream ss { std::string(data) };
        if (!ReadToken(ss, token))
            continue;

        if (token == "#DEFINE")
        {
            // Log instead of throw
            if (isInDefine)
                throw "Duplicate #DEFINE !";

            isInDefine = true;
        }
        else if (token == "#ENDDEFINE")
        {
            // Log instead of throw
            if (!isInDefine)
                throw "#ENDDEFINE without a #DEFINE !";

            isInDefine = false;
        }
        else if (isInDefine)
            continue;
        else if (token == "NATION_NAME")
        {
            if (!ReadString(ss, token))
                return false;

            nation.name = token;

            // Check that this token only occurs once
        }
        else if (token == "WORLD_NAME")
        {
            FiestaConfigWorld wld;
            if (!ReadInt(ss, token, wld.index))
                return false;
            if (!ReadString(ss, token))
                return false;
            wld.name = token;
            if (!ReadString(ss, token))
                return false;
            wld.dataPath = token;
            worlds.push_back(std::move(wld));

            // Check that no world with this ID exists
        }
        else if (token == "SERVER_INFO")
        {
            if (!ReadString(ss, token))
                return false;
            std::string name = token;
            int serverTypeInt;
            if (!ReadInt(ss, token, serverTypeInt) || serverTypeInt < 0)
                return false;
            auto serverType = (FiestaConfigServerType::FiestaConfigServerType) serverTypeInt;
            int worldId;
            if (!ReadInt(ss, token, worldId) || worldId < 0)
                return false;
            int zoneId;
            if (!ReadInt(ss, token, zoneId) || zoneId < 0)
                return false;

            int fromServerTypeInt;
            if (!ReadInt(ss, token, fromServerTypeInt) || fromServerTypeInt < 0)
                return false;
            // Type cast with check!
            auto fromServerType = (FiestaConfigServerType::FiestaConfigServerType) fromServerTypeInt;
            if (!ReadString(ss, token))
                return false;
            std::string bindIP = token;
            int bindPort;
            if (!ReadInt(ss, token, bindPort) || bindPort < 0 || bindPort > 0xFFFF)
                return false;
            int connBacklog;
            if (!ReadInt(ss, token, connBacklog) || connBacklog < 0)
                return false;
            int maxConns;
            if (!ReadInt(ss, token, maxConns) || maxConns < 0)
                return false;

            FiestaConfigServerInfo* pServerInfo = nullptr;

            for (auto& serverInfo : servers)
            {
                bool nameMatches = serverInfo.name == name;

                if (serverInfo.serverType != serverType)
                {
                    if (nameMatches)
                        std::cout << "Server with name " << name << " already exists, but is of type " << serverInfo.serverType << ", not " << serverType << "!" << std::endl;
                    continue;
                }

                if (serverInfo.worldId != worldId)
                {
                    if (nameMatches)
                        std::cout << "Server with name " << name << " already exists, but is for world " << serverInfo.worldId << ", not " << worldId << "!" << std::endl;
                    continue;
                }

                if (serverInfo.zoneId != zoneId)
                {
                    if (nameMatches)
                        std::cout << "Server with name " << name << " already exists, but is for zone " << serverInfo.zoneId << ", not " << zoneId << "!" << std::endl;
                    continue;
                }

                if (!nameMatches)
                {
                    std::cout << "Server type " << serverType << ", world " << worldId << ", zone " << zoneId << " already exists, but called " << serverInfo.name << ", not " << name << "!" << std::endl;
                    return false;
                }

                pServerInfo = &serverInfo;
            }

            if (pServerInfo == nullptr)
            {
                FiestaConfigServerInfo serverInfo;
                serverInfo.name = name;
                serverInfo.serverType = serverType;
                serverInfo.worldId = worldId;
                serverInfo.zoneId = zoneId;
                servers.push_back(std::move(serverInfo));
                pServerInfo = &servers[servers.size() - 1];
            }

            FiestaConfigServerConnection conn;
            conn.fromServerType = fromServerType;
            conn.bindIP = bindIP;
            conn.bindPort = (uint16_t) bindPort;
            conn.maxConnectionBacklog = connBacklog;
            conn.maxClients = maxConns;

            pServerInfo->connections.push_back(std::move(conn));

            // Check that this conn is unique!
        }
        else if (token == "ODBC_INFO")
        {
            FiestaConfigODBCInfo odbc;

            if (!ReadString(ss, odbc.name))
                return false;

            int odbcTypeInt;
            if (!ReadInt(ss, token, odbcTypeInt) || odbcTypeInt < 0)
                return false;
            // Type cast with check!
            odbc.endpoint = (FiestaConfigODBCEndpoint::FiestaConfigODBCEndpoint) odbcTypeInt;
            if (!ReadInt(ss, token, odbc.worldId) || odbc.worldId < 0)
                return false;
            if (!ReadString(ss, odbc.connectionString))
                return false;
            if (!ReadString(ss, odbc.initSQL))
                return false;
            
            odbcInfos.push_back(std::move(odbc));
            // Check that there are no duplicates
        }
        else if (token == "TUNNEL_BINDING")
        {
            FiestaConfigTunnelBinding binding;

            if (!ReadInt(ss, token, binding.bindingId))
                return false;

            if (!ReadString(ss, binding.bindIP))
                return false;

            if (!FilterUtils::SanitiseIP(binding.bindIP))
                return false;

            tunnelBindings.push_back(std::move(binding));
            // More logging
        }
        else if (token == "TUNNEL_INFO")
        {
            FiestaConfigTunnelInfo tunnel;

            if (!ReadString(ss, tunnel.name))
                return false;

            int serverTypeInt;
            if (!ReadInt(ss, token, serverTypeInt) || serverTypeInt < 0)
                return false;
            tunnel.serverType = (FiestaConfigServerType::FiestaConfigServerType) serverTypeInt;
            if (!ReadInt(ss, token, tunnel.worldId) || tunnel.worldId < 0)
                return false;
            if (!ReadInt(ss, token, tunnel.zoneId) || tunnel.zoneId < 0)
                return false;
            if (!ReadInt(ss, token, tunnel.bindingId) || tunnel.bindingId < 0)
                return false;
            int bindPort;
            if (!ReadInt(ss, token, bindPort) || bindPort < 0 || bindPort > 0xFFFF)
                return false;
            tunnel.bindPort = (uint16_t) bindPort;

            tunnelInfos.push_back(std::move(tunnel));
            // Check names being correct
            // Check that there are no duplicates
            // Check that binding ids are valid
        }
        else if (token == "#END")
        {
            break;
        }
    }

    return true;
}

bool FiestaConfig::GetBindIPForTunnelBinding(int id, std::string& ip)
{
    for (auto& b : tunnelBindings)
    {
        if (b.bindingId == id)
        {
            ip = b.bindIP;
            return true;
        }
    }

    ip.clear();
    return false;
}