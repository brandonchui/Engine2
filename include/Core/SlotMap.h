/*
 * SlotMap.h
 *
 * Generational slot map for handle (index, generation) resource storage.
 */

#ifndef _SLOTMAP_H_
#define _SLOTMAP_H_

#include "Core/Handle.h"
#include <stdint.h>

struct Arena;

/**
	Generational slot map structure.

	A slot map is a data structure for handles of types.

	@see slotMapCreate
	@see slotMapInsert
	@see slotMapGet
*/
struct SlotMap
{
	Arena* pArena;

	void* pValues;              ///< Dense array of values
	uint32_t* pIndices;         ///< Sparse to dense index mapping
	uint32_t* pGenerations;     ///< Generation counters for slots
	uint32_t* pErase;           ///< Dense to sparse reverse mapping

	uint32_t capacity;          ///< Maximum number of slots
	uint32_t count;             ///< Current number of elements
	uint32_t freeHead;          ///< Head of free list
	uint32_t valueSize;         ///< Size of each value in bytes
	uint32_t valueAlign;        ///< Alignment requirement for values
};

///////////////////////////////////////////
// Core API

/**
	Creates a new slot map.

	Allocates and initializes a slot map with the specified parameters.
	All memory is allocated from the provided arena.

	@param pArena Arena to allocate from
	@param valueSize Size of each stored value in bytes
	@param valueAlign Alignment requirement for values
	@param initialCapacity Initial number of slots to allocate

	@return Pointer to the created slot map, or nullptr on failure

	@note Minimum alignment is 8 bytes

	@see slotMapInsert
	@see slotMapCount
	@see slotMapCapacity
*/
SlotMap* slotMapCreate(Arena* pArena, uint32_t valueSize, uint32_t valueAlign,
					   uint32_t initialCapacity);


///////////////////////////////////////////
// Implementation

uint32_t slotMapInsertImpl(SlotMap* pSlotMap, const void* pValue);
void* slotMapGetImpl(SlotMap* pSlotMap, uint32_t handle);
void slotMapRemoveImpl(SlotMap* pSlotMap, uint32_t handle);
bool slotMapIsValidImpl(SlotMap* pSlotMap, uint32_t handle);


///////////////////////////////////////////
// Functions

/**
	Gets the current number of elements in the slot map.

	@param pSlotMap Slot map to query

	@return Number of elements currently stored, or 0 if pSlotMap is nullptr

	@see slotMapCapacity
*/
uint32_t slotMapCount(SlotMap* pSlotMap);

/**
	Gets the current capacity of the slot map.

	Returns the maximum number of elements that can be stored before
	the slot map needs to grow.

	@param pSlotMap Slot map to query

	@return Current capacity, or 0 if pSlotMap is nullptr

	@see slotMapCount
*/
uint32_t slotMapCapacity(SlotMap* pSlotMap);

///////////////////////////////////////////
// Template Helpers

/**
	Inserts a value into the slot map.

	Inserts the value into the slot map and returns a handle to it.

	@tparam T Type of value to insert
	@param pSlotMap Slot map to insert into
	@param value Value to insert

	@return Handle to the inserted element, or HANDLE_INVALID_ID on failure


	Example:
	@code
	SlotMap* pMap = slotMapCreate(pArena, sizeof(Entity), alignof(Entity), 64);

	Entity entity = { ... };
	uint32_t handle = slotMapInsert(pMap, entity);

	if (handleIsValid(handle)) {
		// Handle is valid, element was inserted
	}
	@endcode

	@see slotMapGet
	@see slotMapRemove
	@see slotMapIsValid
*/
template <typename T>
inline uint32_t slotMapInsert(SlotMap* pSlotMap, const T& value)
{
	return slotMapInsertImpl(pSlotMap, &value);
}

/**
	Retrieves a pointer to a value in the slot map.

	Returns the pointer to the element associated with the handle.

	@tparam T Type of value to retrieve
	@param pSlotMap Slot map to get from
	@param handle Handle to the element

	@return Pointer to the element, or nullptr if handle is invalid

	Example:
	@code
	Entity* pEntity = slotMapGet<Entity>(pMap, handle);
	if (pEntity) {
		// Entity is valid, can be modified in-place
		pEntity->position.x += velocity.x * deltaTime;
	}
	@endcode

	@see slotMapInsert
	@see slotMapIsValid
*/
template <typename T>
inline T* slotMapGet(SlotMap* pSlotMap, uint32_t handle)
{
	return (T*)slotMapGetImpl(pSlotMap, handle);
}

/**
	Removes a value from the slot map.

	Removes the element associated with the handle and invalidates the handle.

	@param pSlotMap Slot map to remove from
	@param handle Handle to the element to remove

	Example:
	@code
	slotMapRemove(pMap, entityHandle);

	// Handle is now invalid
	assert(!slotMapIsValid(pMap, entityHandle));
	@endcode

	@see slotMapInsert
	@see slotMapIsValid
*/
inline void slotMapRemove(SlotMap* pSlotMap, uint32_t handle)
{
	slotMapRemoveImpl(pSlotMap, handle);
}

/**
	Checks if a handle is currently valid.

	Validates both the handle format and the generation counter to avoid
	use after free errors. 

	@param pSlotMap Slot map to check
	@param handle Handle to validate

	@return true if handle points to a valid element, false otherwise


	Example:
	@code
	if (slotMapIsValid(pMap, handle)) {
		Entity* pEntity = slotMapGet<Entity>(pMap, handle);
		// Safe to use pEntity
	}
	@endcode

	@see slotMapGet
	@see slotMapRemove
*/
inline bool slotMapIsValid(SlotMap* pSlotMap, uint32_t handle)
{
	return slotMapIsValidImpl(pSlotMap, handle);
}

#endif // _SLOTMAP_H_
