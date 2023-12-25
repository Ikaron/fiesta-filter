#pragma once

#include "TypeUtils.h"
#include "BaseHook.h"

#include <string>
#include <vector>

class LoginHook : public BaseHook
{
private:
	std::string cachedUserName;
public:
	static consteval FilterHookType FilterType()
	{
		return FilterHookType::BothDirections;
	}

	using BaseHook::BaseHook;

	virtual FilterHookResponse FilterPacket(std::vector<char>& packet, bool isClientToServer);
};

class WorldManagerHook : public BaseHook
{
private:
	uint16_t charToken = 0;
	std::vector<std::string> chars;
public:
	static consteval FilterHookType FilterType()
	{
		return FilterHookType::BothDirections;
	}

	using BaseHook::BaseHook;

	virtual FilterHookResponse FilterPacket(std::vector<char>& packet, bool isClientToServer);
};

class ZoneHook : public BaseHook
{
public:
	static consteval FilterHookType FilterType()
	{
		return FilterHookType::BothDirections;
	}

	using BaseHook::BaseHook;

	virtual FilterHookResponse FilterPacket(std::vector<char>& packet, bool isClientToServer);
};

class PacketEchoHook : public BaseHook
{
private:
	std::string cachedUserName;
public:
	static consteval FilterHookType FilterType()
	{
		return FilterHookType::BothDirections;
	}

	using BaseHook::BaseHook;

	virtual FilterHookResponse FilterPacket(std::vector<char>& packet, bool isClientToServer);
};

class DummyHook : public BaseHook
{
public:
	static consteval FilterHookType FilterType()
	{
		return FilterHookType::BothDirections;
	}

	using BaseHook::BaseHook;

	virtual FilterHookResponse FilterPacket(std::vector<char>& packet, bool isClientToServer)
	{
		return FilterHookResponse::KeepPacket;
	}
};