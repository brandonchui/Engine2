/*
 * SlotMap.h
 *
 * Generational slot map for stable handle-based resource storage.
 * Provides O(1) insert/remove/lookup with automatic handle invalidation.
 */

#ifndef _SLOTMAP_H_
#define _SLOTMAP_H_

#include "Core/Handle.h"
#include <stdint.h>

struct Arena;

/**
	Generational slot map structure.

	A slot map provides stable handle-based access to densely packed elements
	with O(1) insert, remove, and lookup operations. Uses generational indices
	to detect use-after-free errors automatically.

	@note All memory is allocated from the provided arena
	@note Handles remain valid until the element is removed
	@note Uses swap-and-pop for removal to maintain dense packing

	@see slotMapCreate
	@see slotMapInsert
	@see slotMapGet
*/
struct SlotMap
{
	Arena* pArena;

	void* pValues;              // Dense array of values
	uint32_t* pIndices;         // Sparse-to-dense index mapping
	uint32_t* pGenerations;     // Generation counters for ABA protection
	uint32_t* pErase;           // Dense-to-sparse reverse mapping

	uint32_t capacity;          // Maximum number of slots
	uint32_t count;             // Current number of elements
	uint32_t freeHead;          // Head of free list
	uint32_t valueSize;         // Size of each value in bytes
	uint32_t valueAlign;        // Alignment requirement for values
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

	@note The slot map will automatically grow if capacity is exceeded
	@note Minimum alignment is 8 bytes

	@see slotMapInsert
	@see slotMapCount
	@see slotMapCapacity
*/
SlotMap* slotMapCreate(Arena* pArena, uint32_t valueSize, uint32_t valueAlign,
					   uint32_t initialCapacity);

/**
	Inserts a value into the slot map (implementation function).

	Internal function used by the template wrapper. Prefer using
	the type-safe slotMapInsert<T>() template instead.

	@param pSlotMap Slot map to insert into
	@param pValue Pointer to the value to insert

	@return Handle to the inserted element, or HANDLE_INVALID_ID on failure

	@note This is an internal implementation function
	@note Use slotMapInsert<T>() template for type safety

	@see slotMapInsert
*/
uint32_t slotMapInsertImpl(SlotMap* pSlotMap, const void* pValue);

/**
	Retrieves a value from the slot map (implementation function).

	Internal function used by the template wrapper. Prefer using
	the type-safe slotMapGet<T>() template instead.

	@param pSlotMap Slot map to get from
	@param handle Handle to the element

	@return Pointer to the element, or nullptr if handle is invalid

	@note This is an internal implementation function
	@note Use slotMapGet<T>() template for type safety

	@see slotMapGet
*/
void* slotMapGetImpl(SlotMap* pSlotMap, uint32_t handle);

/**
	Removes a value from the slot map (implementation function).

	Internal function used by the inline wrapper. Prefer using
	slotMapRemove() instead.

	@param pSlotMap Slot map to remove from
	@param handle Handle to the element to remove

	@note This is an internal implementation function
	@note Removal uses swap-and-pop, invalidating the handle
	@note The last element may be moved to fill the gap

	@see slotMapRemove
*/
void slotMapRemoveImpl(SlotMap* pSlotMap, uint32_t handle);

/**
	Checks if a handle is valid (implementation function).

	Internal function used by the inline wrapper. Prefer using
	slotMapIsValid() instead.

	@param pSlotMap Slot map to check
	@param handle Handle to validate

	@return true if handle is valid, false otherwise

	@note This is an internal implementation function

	@see slotMapIsValid
*/
bool slotMapIsValidImpl(SlotMap* pSlotMap, uint32_t handle);

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
	Inserts a value into the slot map with type safety.

	Type-safe wrapper around slotMapInsertImpl that automatically handles
	pointer conversion. Returns a generational handle that remains valid
	until the element is removed.

	@tparam T Type of value to insert
	@param pSlotMap Slot map to insert into
	@param value Value to insert (passed by const reference)

	@return Handle to the inserted element, or HANDLE_INVALID_ID on failure

	@note The slot map will automatically grow if needed
	@note The value is copied into the slot map's internal storage

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
	Retrieves a pointer to a value in the slot map with type safety.

	Type-safe wrapper around slotMapGetImpl that automatically casts
	to the correct type. Returns nullptr if the handle is invalid.

	@tparam T Type of value to retrieve
	@param pSlotMap Slot map to get from
	@param handle Handle to the element

	@return Pointer to the element, or nullptr if handle is invalid

	@note The returned pointer is valid until the element is removed
	@note Do not cache pointers across insertions/removals
	@note Removal operations may invalidate pointers due to swap-and-pop

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
	Uses swap-and-pop, so the last element may be moved to fill the gap.

	@param pSlotMap Slot map to remove from
	@param handle Handle to the element to remove

	@note After removal, the handle is permanently invalidated
	@note The generation counter is incremented to prevent ABA problems
	@note Removal is O(1) due to swap-and-pop strategy
	@note Safe to call multiple times with the same handle

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

	Validates both the handle format and the generation counter to
	detect use-after-free errors.

	@param pSlotMap Slot map to check
	@param handle Handle to validate

	@return true if handle points to a valid element, false otherwise

	@note This check is O(1) and very cheap
	@note Handles become invalid after removal or generation overflow

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
