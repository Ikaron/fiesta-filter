#include "FilterLib.h"

#include <vector>

class AutoAttackHook : public BaseHook
{
public:
	static consteval FilterHookType FilterType()
	{
		return FilterHookType::BothDirections;
	}

	using BaseHook::BaseHook;

	virtual FilterHookResponse FilterPacket(std::vector<char>& packet, bool isClientToServer)
	{

		PacketReader r(packet);

		if (!r.IsOpcode(0x2447))
			return FilterHookResponse::KeepPacket;

		PacketWriter w;
		w.SetOpcode(0x2011);
		static uint8_t num = 0;
		w.Write(num++);
		w.WriteShortStr("You smacked someone!");

		QueuedPacket p(false);
		w.CopyTo(p.data);
		pStream->InjectPacket(std::move(p));

		return FilterHookResponse::KeepPacket;
	}
};