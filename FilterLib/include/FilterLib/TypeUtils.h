#pragma once

enum class ServerType
{
	Login,
	WorldManager,
	Zone
};

const char* ServerTypeName(ServerType type);

enum FilterHookResponse
{
	KeepPacket = 0,
	RewritePacket = 1,
	DropPacket = 2,
	Disconnect = 3,
	Ban = 4
};

enum FilterHookType
{
	ClientToServer = 1 << 0,
	ServerToClient = 1 << 1,
	BothDirections = ClientToServer | ServerToClient
};