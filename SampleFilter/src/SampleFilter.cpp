#include "FilterLib.h"

#include <iostream>
#include <format>

#include "Hooks.hpp"

Logger Log;

// We set up a simple filter that logs ("echoes") all packets to cout and/or a log file (depending on command line arguments)
// It also sends a notice to a player whenever they auto attack an enemy

constexpr FilterHooks<ServerType::Login, PacketEchoHook> LoginHooks;
constexpr FilterHooks<ServerType::WorldManager, PacketEchoHook> WorldHooks;
constexpr FilterHooks<ServerType::Zone, PacketEchoHook, AutoAttackHook> ZoneHooks;

int main(int argc, const char** argv)
{
	if (!Log.ParseCommandLineArgs(argc, argv))
		return FilterUtils::QuitWithUsage();

	FilterManager filterManager;

	if (!filterManager.ParseCommandLineArgs(argc, argv))
		return FilterUtils::QuitWithUsage();

	filterManager.RegisterHooks(ServerType::Login, &LoginHooks);
	filterManager.RegisterHooks(ServerType::WorldManager, &WorldHooks);
	filterManager.RegisterHooks(ServerType::Zone, &ZoneHooks);

	try
	{
		Filter::Init();

		filterManager.Run(argc, argv);

		Filter::Uninit();
	}
	catch (std::string& err)
	{
		Log.WriteSingleLine(LogLevel::Error, std::format("Error: {}", err));
		return 1;
	}
	catch (...)
	{
		Log.WriteSingleLine(LogLevel::Error, "Unknown error!");
		return 1;
	}

	return 0;
}