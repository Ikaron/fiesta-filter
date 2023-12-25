#include "FilterLib.h"

#include <format>

FilterHookResponse LoginHook::FilterPacket(std::vector<char>& packet, bool isClientToServer)
{
	PacketReader r(packet);

	if (r.IsOpcode(0x0c5a)) // Login
	{
		if (!r.ReadFixedStr(cachedUserName, 256))
			return FilterHookResponse::Disconnect;

		if (!r.Skip(60))
			return FilterHookResponse::Disconnect;

		return FilterHookResponse::KeepPacket;
	}
	else if (r.IsOpcode(0x0c0a)) // Login Success
	{
		pStream->pUserData = pFilterManager->CreateUserData(cachedUserName);
		if (pStream->pUserData != nullptr)
		{
			FilterUserDataLock userData(pStream->pUserData);
			userData.IP() = pStream->GetClientIP();
		}

		return FilterHookResponse::KeepPacket;
	}
	else if (!r.IsOpcode(0x0c0c)) // Choose World
		return FilterHookResponse::KeepPacket;

	PacketWriter w;
	w.SetOpcode(0x0c0c);

	uint8_t status;
	if (!r.Read(status))
		return FilterHookResponse::Disconnect;
	w.Write(status);

	std::string ip;
	if (!r.ReadFixedStr(ip, 16))
		return FilterHookResponse::Disconnect;
	if (!FilterUtils::SanitiseIP(ip))
	{
		Log.WriteSingleLine(LogLevel::Error, std::format("Invalid IP: {}", ip));
		return FilterHookResponse::Disconnect;
	}

	w.WriteFixedStr(pFilter->GetIP(), 16);

	uint16_t port;
	if (!r.Read(port))
		return FilterHookResponse::Disconnect;

	auto t = pFilterManager->CreateTunnelIfRequired(ServerType::WorldManager, ip, port);
	w.Write(t->pFilter->GetPort());

	std::string binaryToken;
	binaryToken.resize(64);

	if (!r.Read(binaryToken.data(), binaryToken.size()))
		return FilterHookResponse::Disconnect;
	w.Write(binaryToken.data(), binaryToken.size());

	if (pStream->pUserData != nullptr)
	{
		FilterUserDataLock userData(pStream->pUserData);
		userData.BinaryToken() = binaryToken;
	}

	if (!Log.IsIgnored(LogLevel::Info))
	{
		auto lock = Log.Lock(LogLevel::Info);
		lock.WriteTimestamp();
		lock.WriteLine("We rewrote: ");
		lock.WriteLine(FilterUtils::DumpHex(packet.data(), packet.size()));
		lock.WriteLine("To: ");
		lock.WriteLine(FilterUtils::DumpHex(w.GetBuffer().data(), w.GetBuffer().size()));
	}

	w.CopyTo(packet);

	return FilterHookResponse::RewritePacket;
}


FilterHookResponse WorldManagerHook::FilterPacket(std::vector<char>& packet, bool isClientToServer)
{
	PacketReader r(packet);

	if (r.IsOpcode(0x0c0f)) // World Login
	{
		std::string userName;
		if (!r.ReadFixedStr(userName, 256))
			return FilterHookResponse::Disconnect;

		std::string binaryToken;
		binaryToken.resize(64);

		if (!r.Read(binaryToken.data(), binaryToken.size()))
			return FilterHookResponse::Disconnect;

		pStream->pUserData = pFilterManager->FindUserData<FilterManager::UserDataKeyType::UserName>(userName);

		return FilterHookResponse::KeepPacket;
	}
	else if (r.IsOpcode(0x0c14)) // Char List
	{
		if (!r.Read(charToken))
			return FilterHookResponse::Disconnect;
		uint8_t charCount;
		if (!r.Read(charCount))
			return FilterHookResponse::Disconnect;

		chars.clear();
		for (auto i = 0; i < charCount; ++i)
		{
			uint32_t charId;
			if (!r.Read(charId))
				return FilterHookResponse::Disconnect;
			std::string charName;
			if (!r.ReadFixedStr(charName, 20))
				return FilterHookResponse::Disconnect;
			if (!r.Skip(2)) // Random char info
				return FilterHookResponse::Disconnect;
			uint8_t slot;
			if (!r.Read(slot))
				return FilterHookResponse::Disconnect;
			if (!r.Skip(103)) // Random char info
				return FilterHookResponse::Disconnect;

			chars.resize(slot + 1);
			chars[slot] = charName;
		}

		return FilterHookResponse::KeepPacket;
	}
	else if (r.IsOpcode(0x1406)) // Create Char
	{
		uint8_t charCount;
		if (!r.Read(charCount))
			return FilterHookResponse::Disconnect;

		for (auto i = 0; i < charCount; ++i)
		{
			uint32_t charId;
			if (!r.Read(charId))
				return FilterHookResponse::Disconnect;
			std::string charName;
			if (!r.ReadFixedStr(charName, 20))
				return FilterHookResponse::Disconnect;
			if (!r.Skip(2)) // Random char info
				return FilterHookResponse::Disconnect;
			uint8_t slot;
			if (!r.Read(slot))
				return FilterHookResponse::Disconnect;
			if (!r.Skip(103)) // Random char info
				return FilterHookResponse::Disconnect;

			chars.resize(slot + 1);
			chars[slot] = charName;
		}

		return FilterHookResponse::KeepPacket;
	}
	else if (r.IsOpcode(0x140c)) // Delete char
	{
		uint8_t slot;
		if (!r.Read(slot))
			return FilterHookResponse::Disconnect;

		if (slot < chars.size())
			chars[slot].clear();

		return FilterHookResponse::KeepPacket;
	}
	else if (r.IsOpcode(0x1001)) // Char Select
	{
		uint8_t selected;
		if (!r.Read(selected) || selected >= chars.size())
			return FilterHookResponse::Disconnect;

		if (pStream->pUserData != nullptr)
		{
			FilterUserDataLock userData(pStream->pUserData);
			userData.CharToken() = charToken;
			userData.CharName() = chars[selected];
		}

		return FilterHookResponse::KeepPacket;
	}
	else if (!r.IsOpcode(0x1003))
		return FilterHookResponse::KeepPacket;

	PacketWriter w;
	w.SetOpcode(0x1003);

	std::string ip;
	if (!r.ReadFixedStr(ip, 16))
		return FilterHookResponse::Disconnect;
	if (!FilterUtils::SanitiseIP(ip))
	{
		Log.WriteSingleLine(LogLevel::Error, std::format("Invalid IP: {}", ip));
		return FilterHookResponse::Disconnect;
	}

	w.WriteFixedStr(pFilter->GetIP(), 16);

	uint16_t port;
	if (!r.Read(port))
		return FilterHookResponse::Disconnect;

	auto t = pFilterManager->CreateTunnelIfRequired(ServerType::Zone, ip, port);
	w.Write(t->pFilter->GetPort());

	if (!Log.IsIgnored(LogLevel::Info))
	{
		auto lock = Log.Lock(LogLevel::Info);
		lock.WriteTimestamp();
		lock.WriteLine("We rewrote: ");
		lock.WriteLine(FilterUtils::DumpHex(packet.data(), packet.size()));
		lock.WriteLine("To: ");
		lock.WriteLine(FilterUtils::DumpHex(w.GetBuffer().data(), w.GetBuffer().size()));
	}

	w.CopyTo(packet);

	return FilterHookResponse::RewritePacket;
}

FilterHookResponse ZoneHook::FilterPacket(std::vector<char>& packet, bool isClientToServer)
{
	PacketReader r(packet);

	if (r.IsOpcode(0x1801)) // Zone Login
	{
		uint16_t charToken;
		if (!r.Read(charToken))
			return FilterHookResponse::Disconnect;

		std::string charName;
		if (!r.ReadFixedStr(charName, 20))
			return FilterHookResponse::Disconnect;

		pStream->pUserData = pFilterManager->FindUserData(pStream->GetClientIP(), charToken, charName);

		return FilterHookResponse::KeepPacket;
	}
	/*else if (r.IsOpcode(0x2447))
	{
		const char* messages[] =
		{
			"Hi!",
			"These are multiple messages!",
			"Hope they work!"
		};

		for(auto i = 0; i < _countof(messages); ++i)
		{
			PacketWriter w;
			w.SetOpcode(0x2011);
			static uint8_t num = 0;
			w.Write(num++);
			w.WriteShortStr(messages[i]);

			QueuedPacket p(false);
			w.CopyTo(p.data);
			pStream->InjectPacket(std::move(p));
		}
		return FilterHookResponse::KeepPacket;
	}*/
	else if (!r.IsOpcode(0x180a))
		return FilterHookResponse::KeepPacket;

	PacketWriter w;
	w.SetOpcode(0x180a);

	size_t remaining = r.Remaining();
	if (remaining >= 20)
	{
		remaining -= 20;
		w.Write(&packet[r.CurrentPos()], remaining);
		r.Skip(remaining);
	}

	std::string ip;
	if (!r.ReadFixedStr(ip, 16))
		return FilterHookResponse::Disconnect;
	if (!FilterUtils::SanitiseIP(ip))
	{
		Log.WriteSingleLine(LogLevel::Error, std::format("Invalid IP: {}", ip));
		return FilterHookResponse::Disconnect;
	}

	w.WriteFixedStr(pFilter->GetIP(), 16);

	uint16_t port;
	if (!r.Read(port))
		return FilterHookResponse::Disconnect;

	auto t = pFilterManager->CreateTunnelIfRequired(ServerType::Zone, ip, port);
	w.Write(t->pFilter->GetPort());

	remaining = r.Remaining();
	if (remaining > 0)
	{
		w.Write(&packet[r.CurrentPos()], remaining);
		r.Skip(remaining);
	}

	if (!Log.IsIgnored(LogLevel::Info))
	{
		auto lock = Log.Lock(LogLevel::Info);
		lock.WriteTimestamp();
		lock.WriteLine("We rewrote: ");
		lock.WriteLine(FilterUtils::DumpHex(packet.data(), packet.size()));
		lock.WriteLine("To: ");
		lock.WriteLine(FilterUtils::DumpHex(w.GetBuffer().data(), w.GetBuffer().size()));
	}

	w.CopyTo(packet);

	return FilterHookResponse::RewritePacket;
}

FilterHookResponse PacketEchoHook::FilterPacket(std::vector<char>& packet, bool isClientToServer)
{
	if (Log.IsIgnored(LogLevel::Info))
		return FilterHookResponse::KeepPacket;

	std::string userName;
	if (pStream->pUserData != nullptr)
		userName = pStream->pUserData->userName;

	std::string user;
	if (!userName.empty())
		user = std::format("{} ({}:{})", userName, pStream->GetClientIP(), pStream->GetClientPort());
	else
		user = std::format("{}:{}", pStream->GetClientIP(), pStream->GetClientPort());

	std::string server = std::format("{} ({})", ServerTypeName(pFilter->Hooks()->GetServerType()), pStream->GetTargetPort());
	{
		auto lock = Log.Lock(LogLevel::Info);
		lock.WriteTimestamp();
		if (isClientToServer)
			lock.WriteLine(std::format("{} -> {} :", user, server));
		else
			lock.WriteLine(std::format("{} -> {} :", server, user));
		lock.WriteLine(FilterUtils::DumpHex(packet.data(), packet.size()));
	}

	return FilterHookResponse::KeepPacket;
}