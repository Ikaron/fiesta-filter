#pragma once

#include "FilterHook.h"
#include <vector>

class Filter;
class FilterStream;
class FilterUserData;

class BaseHook
{
protected:
	friend class FilterManager;

	FilterManager* pFilterManager;
	Filter* pFilter;
	FilterStream* pStream;
public:
	const FilterHookType hookType;

	BaseHook(FilterHookType hookType, FilterManager* pFilterManager, Filter* pFilter, FilterStream* pStream) : hookType(hookType), pFilterManager(pFilterManager), pFilter(pFilter), pStream(pStream) {}
	virtual ~BaseHook() {}

	virtual FilterHookResponse FilterPacket(std::vector<char>& packet, bool isClientToServer) = 0;
};