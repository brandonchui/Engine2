/*
 * Arena.h
 *
 * Arena Allocator API
 */

#ifndef _ARENA_H_
#define _ARENA_H_

#include "Runtime/Memory/ArenaTypes.h"
#include "Runtime/RuntimeAPI.h"
// For memset
#include <cstring.h>

///////////////////////////////////////////
// Core API

/**
	Creates a new arena allocator.

	Allocates and initializes an arena with the specified parameters.
	If no parameters are provided, uses default reserve/commit sizes.

	@param pParams Arena creation parameters, or nullptr for defaults

	@return Pointer to the created arena, or nullptr on failure

	@see arenaRelease
*/
RUNTIME_API Arena* arenaCreate(const ArenaParams* pParams = nullptr);

/**
	Releases an arena and all its memory.

	Destroys the arena and frees all allocated blocks in the chain.
	All pointers allocated from this arena become invalid.

	@param pArena Arena to release

	@note After this call, pArena is invalid and must not be used

	@see arenaCreate
*/
RUNTIME_API void arenaRelease(Arena* pArena);

/**
	Allocates memory from the arena with specified alignment.

	Pushes a new allocation onto the arena with the requested size and
	alignment. Tries to chain if full (provided flag enables it).

	@param pArena Arena to allocate from
	@param size Number of bytes to allocate
	@param align Alignment requirement in bytes (power 2)

	@return Pointer to allocated memory, or nullptr on failure

	@note Memory is NOT zero initialized

	@see arenaPushStruct
	@see arenaPushArray
*/
RUNTIME_API void* arenaPush(Arena* pArena, uint64_t size, uint64_t align = 8);

/**
	Gets the current global position in the arena.

	Returns the cumulative byte offset across all blocks in the chain.
	This position can be saved and later restored with arenaPopTo().

	@param pArena Arena to query

	@return Current position in bytes from the start of the first block

	@see arenaPopTo
*/
RUNTIME_API uint64_t arenaGetPos(Arena* pArena);

/**
	Resets the arena to a previous position.

	Rewinds the allocation position to a previously saved point.
	Use when you want to save a position and want to return to it later.

	@param pArena Arena to reset
	@param pos Target position (see arenaGetPos)

	@note All pointers allocated after this position become invalid

	@see arenaGetPos
	@see arenaPop
*/
RUNTIME_API void arenaPopTo(Arena* pArena, uint64_t pos);

/**
	Pops a specified number of bytes from the end of the arena.

	Convenience function that rewinds the arena by a specific byte count.
	Use when you know he size of what you allocated and want to
	undo it.

	@param pArena Arena to pop from
	@param amount Number of bytes to free from the end

	@see arenaPopTo
*/
RUNTIME_API void arenaPop(Arena* pArena, uint64_t amount);

/**
	Clears the entire arena back to its initial state.

	Resets the arena to position 0, effectively freeing all allocations
	and releasing all chained blocks except the first.

	@param pArena Arena to clear

	@see arenaPopTo
*/
RUNTIME_API void arenaClear(Arena* pArena);

/**
	Begins a temporary arena scope. A scrach arena.

	Captures the current arena position for later restoration. Used for
	stack like temporary allocations that will be freed as a group.

	@param pArena Arena to create temp scope from

	@return ArenaTemp marker containing the saved position

	@see arenaTempEnd
*/
RUNTIME_API ArenaTemp arenaTempBegin(Arena* pArena);

/**
	Ends a temporary arena scope.

	Restores the arena to the position saved by arenaTempBegin, freeing
	all allocations made within the temporary scope.

	@param temp ArenaTemp marker from arenaTempBegin

	@see arenaTempBegin
*/
RUNTIME_API void arenaTempEnd(ArenaTemp temp);

///////////////////////////////////////////
// Template Helpers

/**
	Allocates and zero initializes a single struct from the arena.

	Type safe wrapper around arenaPush that automatically handles
	sizing and alignment. Memory is zero initialized.

	@tparam T Type to allocate (struct or class)
	@param pArena Arena to allocate from

	@return Pointer to zero initialized instance of T

	@note Uses alignof(T) with minimum 8 byte alignment
	@note Memory is zero-initialized via memset

	Example:
	@code
	Transform* pTransform = arenaPushStruct<Transform>(pArena);
	@endcode
*/
template <typename T>
inline T* arenaPushStruct(Arena* pArena)
{
	constexpr uint64_t align = alignof(T) >= 8 ? alignof(T) : 8;
	void* pMem = arenaPush(pArena, sizeof(T), align);
	return (T*)memset(pMem, 0, sizeof(T));
}

/**
	Allocates and zero initializes an array from the arena.

	Type safe wrapper for array allocation. Will zero initialize.

	@tparam T Element type
	@param pArena Arena to allocate from
	@param count Number of elements to allocate

	@return Pointer to zero initialized array of T[count]

	@note Uses alignof(T) with minimum 8 byte alignment.

	Example:
	@code
	Vertex* pVertices = arenaPushArray<Vertex>(pArena, 100);
	@endcode
*/
template <typename T>
inline T* arenaPushArray(Arena* pArena, uint64_t count)
{
	constexpr uint64_t align = alignof(T) >= 8 ? alignof(T) : 8;
	void* pMem = arenaPush(pArena, sizeof(T) * count, align);
	return (T*)memset(pMem, 0, sizeof(T) * count);
}

/**
	Allocates an array from the arena WITHOUT zero initialization.

	Type safe wrapper for array allocation. Memory is NOT initialized,
	which probably faster if plan to override immediately.

	@tparam T Element type
	@param pArena Arena to allocate from
	@param count Number of elements to allocate

	@return Pointer to uninitialized array of T[count]

	@note Uses alignof(T) with minimum 8 byte alignment
	@note Memory contents are undefined (no zeroed/memset)

	Example:
	@code
	float* pBuffer = arenaPushArrayNoZero<float>(pArena, 1000);
	// Fill with actual data immediately
	for (uint64_t i = 0; i < 1000; ++i) pBuffer[i] = compute(i);
	@endcode
*/
template <typename T>
inline T* arenaPushArrayNoZero(Arena* pArena, uint64_t count)
{
	constexpr uint64_t align = alignof(T) >= 8 ? alignof(T) : 8;
	return (T*)arenaPush(pArena, sizeof(T) * count, align);
}

///////////////////////////////////////////
// RAII Helpers

/**
	RAII wrapper for temporary arena scopes.

	Automatically captures arena position on construction and restores
	it on destruction. Uses C++ class/struct for scoping lifetime.
	Consider the ARENA_TEMP_SCOPE macro if you do not need variable name.

	@see arenaTempBegin
	@see arenaTempEnd

	Example:
	@code
	{
		ScopedArenaTemp temp(pArena);
		char* pTempBuffer = arenaPushArrayNoZero<char>(pArena, 1024);
		// ... use temp buffer
	} // Automatically freed here
	@endcode
*/
struct ScopedArenaTemp
{
	ArenaTemp temp;

	ScopedArenaTemp(Arena* pArena)
	: temp(arenaTempBegin(pArena))
	{
	}
	~ScopedArenaTemp() { arenaTempEnd(temp); }

	ScopedArenaTemp(const ScopedArenaTemp&) = delete;
	ScopedArenaTemp& operator=(const ScopedArenaTemp&) = delete;
};

/**
	Creates a scoped temporary arena with RAII cleanup.

	Macro that creates a named ScopedArenaTemp variable that automatically
	restores the arena when leaving scope.

	@param arena Arena pointer

	@see ScopedArenaTemp

	Example:
	@code
	ARENA_TEMP_SCOPE(pFrameArena);
	Vertex* pVerts = arenaPushArray<Vertex>(pFrameArena, 100);
	// ... use vertices
	// Automatically freed at end of scope
	@endcode
*/
#define ARENA_CONCAT_IMPL(a, b) a##b
#define ARENA_CONCAT(a, b) ARENA_CONCAT_IMPL(a, b)

#define ARENA_TEMP_SCOPE(arena) ScopedArenaTemp ARENA_CONCAT(_arena_temp_, __LINE__)(arena)

#endif // _ARENA_H_
