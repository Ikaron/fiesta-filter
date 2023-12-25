#include "FilterLib/TypeUtils.h"


const char* ServerTypeName(ServerType type)
{
	switch (type)
	{
	case ServerType::Login:
		return "Login";
	case ServerType::WorldManager:
		return "World Manager";
	case ServerType::Zone:
		return "Zone";
	default:
		return "Unknown Server";
	}
}