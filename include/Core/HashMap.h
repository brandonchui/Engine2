/*
 * HashMap.h
 *
 * Hash map for string key to value storage.
 * TODO : Replace with arena implementation.
 */

#ifndef _HASHMAP_H_
#define _HASHMAP_H_

#include <stdint.h>

struct Arena;

/**
	Hash map structure.

	Currently just a wrapper around std::map implementation.

	@see hashMapCreate
	@see hashMapInsert
	@see hashMapGet
*/
struct HashMap
{
	void* pImpl;		///< Backend implementation (std::map)
	uint32_t valueSize; ///< Size of each value in bytes
};

///////////////////////////////////////////
// Core API

/**
	Creates a new hash map.

	Allocates and initializes a hash map with the specified value size.

	@param pArena Arena to allocate from
	@param valueSize Size of each stored value in bytes

	@return Pointer to the created hash map, or nullptr on failure

	@see hashMapInsert
	@see hashMapGet
*/
HashMap* hashMapCreate(Arena* pArena, uint32_t valueSize);

/**
	Destroys a hash map and frees all memory.

	@param pHashMap Hash map to destroy

	@see hashMapCreate
*/
void hashMapDestroy(HashMap* pHashMap);

void hashMapInsertImpl(HashMap* pHashMap, const char* key, const void* pValue);
void* hashMapGetImpl(HashMap* pHashMap, const char* key);

/**
	Checks if a key exists in the hash map.

	@param pHashMap Hash map to check
	@param key String key to look up

	@return true if key exists, false otherwise

	@see hashMapGet
*/
bool hashMapContains(HashMap* pHashMap, const char* key);

/**
	Removes a key-value pair from the hash map.

	@param pHashMap Hash map to remove from
	@param key String key to remove

	@note Safe to call with non-existent key (no-op)

	@see hashMapInsert
*/
void hashMapRemove(HashMap* pHashMap, const char* key);

/**
	Gets the current number of key-value pairs in the hash map.

	@param pHashMap Hash map to query

	@return Number of pairs currently stored

	@see hashMapCreate
*/
uint32_t hashMapCount(HashMap* pHashMap);

///////////////////////////////////////////
// Template Helpers

/**
	Inserts a key value pair into the hash map with type safety.

	Inserts a key value pair into the data structure. If the key
	already exists, its value is replaced.

	@tparam T Type of value to insert
	@param pHashMap Hash map to insert into
	@param key String key (will be copied)
	@param value Value to insert (passed by const reference)

	@note The value is copied into the hash map's internal storage

	Example:
	@code
	HashMap* pMap = hashMapCreate(pArena, sizeof(TextureHandle));

	hashMapInsert(pMap, "player.dds", playerTexHandle);
	hashMapInsert(pMap, "enemy.dds", enemyTexHandle);
	@endcode

	@see hashMapGet
	@see hashMapContains
*/
template <typename T>
inline void hashMapInsert(HashMap* pHashMap, const char* key, const T& value)
{
	hashMapInsertImpl(pHashMap, key, &value);
}

/**
	Retrieves a pointer to a value in the hash map with type safety.

	Get the value based on the provided key. Returns nullptr if the
	key is not found.

	@tparam T Type of value to retrieve
	@param pHashMap Hash map to get from
	@param key String key to look up

	@return Pointer to the value, or nullptr if key not found

	@note The returned pointer is valid until the key is removed

	Example:
	@code
	TextureHandle* pHandle = hashMapGet<TextureHandle>(pMap, "player.dds");
	if (pHandle) {
		// Use *pHandle
	}
	@endcode

	@see hashMapInsert
	@see hashMapContains
*/
template <typename T>
inline T* hashMapGet(HashMap* pHashMap, const char* key)
{
	return (T*)hashMapGetImpl(pHashMap, key);
}

#endif // _HASHMAP_H_
