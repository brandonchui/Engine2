/*
 * HashMap.cpp
 *
 */

#include "Core/HashMap.h"
#include "Utilities/Interfaces/ILog.h"
#include <map>
#include <string>
#include <cstring>

// Temporary std::map-based implementation
// TODO: Replace with arena-based hash table
struct HashMapImpl
{
	std::map<std::string, uint8_t*> map;
	uint32_t valueSize;
	uint32_t count;

	HashMapImpl(uint32_t vs)
	: valueSize(vs)
	, count(0)
	{
	}

	~HashMapImpl()
	{
		for (auto& pair : map)
		{
			delete[] pair.second;
		}
	}
};

HashMap* hashMapCreate(Arena* pArena, uint32_t valueSize)
{
	(void)pArena;

	if (valueSize == 0)
		return nullptr;

	HashMap* pHashMap = new HashMap();
	if (!pHashMap)
		return nullptr;

	pHashMap->pImpl = new HashMapImpl(valueSize);
	if (!pHashMap->pImpl)
	{
		delete pHashMap;
		return nullptr;
	}

	pHashMap->valueSize = valueSize;
	return pHashMap;
}

void hashMapDestroy(HashMap* pHashMap)
{
	if (!pHashMap)
		return;

	if (pHashMap->pImpl)
	{
		HashMapImpl* pImpl = (HashMapImpl*)pHashMap->pImpl;
		delete pImpl;
	}

	delete pHashMap;
}

void hashMapInsertImpl(HashMap* pHashMap, const char* key, const void* pValue)
{
	if (!pHashMap || !key || !pValue)
		return;

	HashMapImpl* pImpl = (HashMapImpl*)pHashMap->pImpl;

	auto it = pImpl->map.find(key);
	if (it != pImpl->map.end())
	{
		std::memcpy(it->second, pValue, pImpl->valueSize);
	}
	else
	{
		uint8_t* pStorage = new uint8_t[pImpl->valueSize];
		std::memcpy(pStorage, pValue, pImpl->valueSize);

		pImpl->map[key] = pStorage;
		pImpl->count++;
	}
}

void* hashMapGetImpl(HashMap* pHashMap, const char* key)
{
	if (!pHashMap || !key)
		return nullptr;

	HashMapImpl* pImpl = (HashMapImpl*)pHashMap->pImpl;

	auto it = pImpl->map.find(key);
	if (it != pImpl->map.end())
	{
		return it->second;
	}

	return nullptr;
}

bool hashMapContains(HashMap* pHashMap, const char* key)
{
	if (!pHashMap || !key)
		return false;

	HashMapImpl* pImpl = (HashMapImpl*)pHashMap->pImpl;
	return pImpl->map.find(key) != pImpl->map.end();
}

void hashMapRemove(HashMap* pHashMap, const char* key)
{
	if (!pHashMap || !key)
		return;

	HashMapImpl* pImpl = (HashMapImpl*)pHashMap->pImpl;

	auto it = pImpl->map.find(key);
	if (it != pImpl->map.end())
	{
		delete[] it->second;
		pImpl->map.erase(it);
		pImpl->count--;
	}
}

uint32_t hashMapCount(HashMap* pHashMap)
{
	if (!pHashMap)
		return 0;

	HashMapImpl* pImpl = (HashMapImpl*)pHashMap->pImpl;
	return pImpl->count;
}
