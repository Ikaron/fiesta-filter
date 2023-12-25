#pragma once

#include "TypeUtils.h"
#include "BaseHook.h"
#include "Hooks.h"

#include <span>
#include <array>
#include <vector>
#include <functional>
#include <memory>

template<typename T>
concept FilterHook = requires() {
	std::is_base_of<BaseHook, T>::value;
	{ T::FilterType() } -> std::same_as<FilterHookType>;
};

/*
requires(T, std::vector<char>&p, void* pData)
{
	{ T::FilterPacket(p, pData) } -> std::same_as<FilterHookResponse>;
	{ T::FilterType() } -> std::same_as<FilterHookType>;
	{ T::AllocatePerStreamData() } -> std::same_as<void*>;
	{ T::FreePerStreamData(pData) } -> std::same_as<void>;

	typename T::PerStreamData;
};
*/

class FilterManager;

class FilterHookEntry
{
private:
	template<FilterHook HookType>
	static BaseHook* New(FilterManager* pFilterManager, Filter* pFilter, FilterStream* pStream)
	{
		return new HookType(HookType::FilterType(), pFilterManager, pFilter, pStream);
	}

public:
	BaseHook* (*pFnNew)(FilterManager*, Filter*, FilterStream*);
	FilterHookType hookType;
	size_t index;

	consteval FilterHookEntry()
		: pFnNew(nullptr), hookType(FilterHookType::BothDirections), index(0) {}

	FilterHookEntry(const FilterHookEntry&) = delete;
	consteval FilterHookEntry(FilterHookEntry&&) = default;
	FilterHookEntry& operator=(const FilterHookEntry&) = delete;
	consteval FilterHookEntry& operator=(FilterHookEntry&&) = default;

	template<FilterHook HookType>
	consteval static FilterHookEntry MakeEntry(FilterHookType hookType, size_t index)
	{
		FilterHookEntry e;
		e.pFnNew = &New<HookType>;
		e.hookType = hookType;
		e.index = index;
		return e;
	}

	std::unique_ptr<BaseHook> MakeUnique(FilterManager* pFilterManager, Filter* pFilter, FilterStream* pStream) const
	{
		return std::unique_ptr<BaseHook>(pFnNew(pFilterManager, pFilter, pStream));
	}
};

class IFilterHooks
{
public:
	//virtual std::vector<void*> AllocatePerStreamData() const = 0;
	//virtual void FreePerStreamData(std::vector<void*>&) const = 0;
	virtual std::span<const FilterHookEntry> GetHooks() const = 0;
	virtual ServerType GetServerType() const = 0;

	/*void* GetPerStreamData(const std::vector<void*>& datas, const FilterHookEntry& entry) const
	{
		return datas[entry.index];
	}

	template<typename T>
	T* GetPerStreamData(const std::vector<void*>& datas, const FilterHookEntry& entry) const
	{
		return (T*)datas[entry.index];
	}*/
};

template<ServerType Type, FilterHook... Hooks>
class FilterHooks : public IFilterHooks
{
private:
	template<size_t IndexValue, FilterHook CurrentHook>
	struct HookWithIndex
	{
		static const size_t Index = IndexValue;
		using Hook = CurrentHook;
	};

	consteval static size_t EntryCount()
	{
		return sizeof...(Hooks) + 1;
	}

	using array_type = std::array<FilterHookEntry, EntryCount()>;

	/*template<size_t StartIndex, FilterHook CurrentHook, FilterHook... RemainingHooks>
	struct HooksWithIndex
	{
		using Hooks = decltype(std::tuple_cat(std::tuple<HookWithIndex<StartIndex, CurrentHook>>(), HooksWithIndex<StartIndex + 1, RemainingHooks...>::Hooks()));
	};
*/

	/*template<size_t StartIndex, FilterHook CurrentHook, FilterHook... RemainingHooks>
	consteval static auto MakeWithIndex()
	{
		if constexpr (sizeof...(RemainingHooks) == 0)
			return std::tuple<HookWithIndex<StartIndex, CurrentHook>>{};
		else
			return std::tuple_cat(std::tuple<HookWithIndex<StartIndex, CurrentHook>>{}, MakeWithIndex<StartIndex + 1, RemainingHooks...>);
	}

	using WithIndex = decltype(MakeWithIndex<0, Hooks...>());
	*/

	/*template<FilterHook CurrentHook>
	consteval static void BuildEntry(size_t& index)
	{
		hooks[index] = FilterHookEntry(&CurrentHook::FilterPacket, CurrentHook::FilterType(), index);
		++index;
	}*/

	template<size_t Index>
	consteval static void BuildEntry(array_type& entries)
	{
		return;
	}

	template<size_t Index, FilterHook CurrentHook, FilterHook... RemainingHooks>
	consteval static void BuildEntry(array_type& entries)
	{
		entries[Index] = FilterHookEntry::MakeEntry<CurrentHook>(CurrentHook::FilterType(), Index);
		BuildEntry<Index + 1, RemainingHooks...>(entries);
	}

	template<FilterHook Hook, bool isClientToServer>
	consteval static bool Matches()
	{
		const auto targetType = isClientToServer ? FilterHookType::ClientToServer : FilterHookType::ServerToClient;
		return Hook::FilterType() & targetType;
	}

	array_type hooks;

public:
	static const ServerType HookServerType = Type;

	consteval FilterHooks() : hooks()
	{
		if constexpr (Type == ServerType::Login)
			BuildEntry<0, LoginHook>(hooks);
		if constexpr (Type == ServerType::WorldManager)
			BuildEntry<0, WorldManagerHook>(hooks);
		if constexpr (Type == ServerType::Zone)
			BuildEntry<0, ZoneHook>(hooks);

		BuildEntry<1, Hooks...>(hooks);
	}

	FilterHooks(const FilterHooks&) = delete;
	constexpr FilterHooks(FilterHooks&&) = default;
	FilterHooks& operator=(const FilterHooks&) = delete;
	constexpr FilterHooks& operator=(FilterHooks&&) = default;

	virtual std::span<const FilterHookEntry> GetHooks() const
	{
		return std::span<const FilterHookEntry>(hooks.begin(), hooks.size());
	}

	virtual ServerType GetServerType() const
	{
		return HookServerType;
	}
};