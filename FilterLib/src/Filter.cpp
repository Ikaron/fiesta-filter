#include "FilterLib/FilterStream.h"
#include "FilterLib/FilterUtils.h"
#include "FilterLib/Filter.h"

#include <format>

#include <WinSock2.h>
#include <ws2tcpip.h>

bool Filter::ParseCommandLineArgs(int argc, const char** argv)
{
    if (!FilterUtils::ParseCommandLinePort(argc, argv, "bind-port", port))
        return false;

    std::string fallbackIP = "127.0.0.1";
    FilterUtils::SanitiseIP(fallbackIP);

    if (!FilterUtils::ParseCommandLineIP(argc, argv, "server-ip", targetIP, fallbackIP))
        return false;

    if (!FilterUtils::ParseCommandLinePort(argc, argv, "server-port", targetPort))
        return false;

    return true;
}

void Filter::Init()
{
    WSADATA wsaData;
    auto result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
        throw std::format("WSAStartup failed with error {0:#08x}", result);
}

void Filter::Uninit()
{
    WSACleanup();
}

void Filter::Start()
{
    addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* addr = NULL;
    auto result = getaddrinfo(NULL, std::format("{0}", port).c_str(), &hints, &addr);
    if (result != 0)
    {
        WSACleanup();
        throw std::format("Getting binding address failed with error {0:#08x}", result);
    }

    serverSocket = WSASocketW(addr->ai_family, addr->ai_socktype, addr->ai_protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (serverSocket == INVALID_SOCKET)
    {
        auto lastError = WSAGetLastError();
        freeaddrinfo(addr);
        WSACleanup();
        throw std::format("Socket creation failed with error {0:#08x}", lastError);
    }

    while (true)
    {
        result = bind(serverSocket, addr->ai_addr, (int)addr->ai_addrlen);
        if (result != SOCKET_ERROR)
            break;

        freeaddrinfo(addr);

        auto lastError = WSAGetLastError();
        if (allowDynamicPort && lastError == WSAEADDRINUSE)
        {
            ++port;
            auto result = getaddrinfo(NULL, std::format("{0}", port).c_str(), &hints, &addr);
            if (result != 0)
            {
                WSACleanup();
                throw std::format("Getting binding address failed with error {0:#08x}", result);
            }
            continue;
        }

        WSACleanup();
        throw std::format("Socket binding failed with error {0:#08x}", lastError);
    }

    freeaddrinfo(addr);
}

void Filter::ProcessLoop()
{
    auto result = listen(serverSocket, SOMAXCONN);
    if (result == SOCKET_ERROR)
    {
        auto lastError = WSAGetLastError();
        closesocket(serverSocket);
        WSACleanup();
        throw std::format("Socket listen failed with error {0:#08x}", lastError);
    }

    while (true)
    {
        auto clientSock = accept(serverSocket, NULL, NULL);
        if (clientSock == INVALID_SOCKET)
        {
            auto lastError = WSAGetLastError();
            CloseAllStreams();
            closesocket(serverSocket);
            WSACleanup();
            throw std::format("Socket accept failed with error {0:#08x}", lastError);
        }

        streams.push_back(std::move(std::make_unique<FilterStream>(pFilterManager, this, targetIP, targetPort, clientSock)));
    }
}

void Filter::Close()
{
    CloseAllStreams();
    if (serverSocket != INVALID_SOCKET)
    {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
}

void Filter::CloseAllStreams()
{
    for (auto& str : streams)
        str->Close();

    streams.clear();
}