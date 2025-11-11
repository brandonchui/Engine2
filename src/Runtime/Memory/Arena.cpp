/*
 * Arena.cpp
 *
 */

#include "Runtime/Memory/Arena.h"
#include "Runtime/Memory/PlatformMemory.h"
#include "Utilities/Interfaces/ILog.h"
#include <cassert>

/**
	Aligns value to pow 2.

	@param value Value to align
	@param align Alignment

	@return Aligned value
*/
static inline uint64_t alignPow2(uint64_t value, uint64_t align)
{
	//Example:
	//  00001000 (8)
	//& 11111000 (~7)
	//  --------
	//  00001000 (8)
	assert((align > 0) && ((align & (align - 1)) == 0) && "Alignment must be a power of 2");
	return value + (align - 1) & ~(align - 1);
}

// ============================================================================
// Core Arena Functions
// ============================================================================

Arena* arenaCreate(const ArenaParams* pParams)
{
	uint64_t reserveSize = ARENA_DEFAULT_RESERVE;
	uint64_t commitSize = ARENA_DEFAULT_COMMIT;
	uint32_t flags = ArenaFlag_None;

	if (pParams)
	{
		if (pParams->reserveSize != 0)
			reserveSize = pParams->reserveSize;

		if (pParams->commitSize != 0)
			commitSize = pParams->commitSize;

		flags = pParams->flags;
	}

	// Get the address of our reserved memory.
	void* reservedMemoryPtr = platformReserveMemory(reserveSize);
	if (!reservedMemoryPtr)
	{
		LOGF(eERROR, "Arena creation failed: Unable to reserve memory");
		return nullptr;
	}

	// Commit physical memory for the initial block minimum of header size.
	// Recall the header is just the struct Arena variables (flags, pos, etc).
	uint64_t initialCommitSize = commitSize;

	if (initialCommitSize < ARENA_HEADER_SIZE)
		initialCommitSize = ARENA_HEADER_SIZE;

	if (!platformCommitMemory(reservedMemoryPtr, initialCommitSize))
	{
		LOGF(eERROR, "Arena creation failed: Unable to commit initial memory");
		platformReleaseMemory(reservedMemoryPtr, reserveSize);
		return nullptr;
	}

	// Initialize Arena struct IN the reserved memory.
	Arena* pArena = (Arena*)reservedMemoryPtr;
	pArena->pPrev = nullptr;
	pArena->pCurrent = pArena;
	pArena->flags = flags;
	pArena->_pad = 0;
	pArena->commitSize = commitSize;
	pArena->reserveSize = reserveSize;
	pArena->basePos = 0;
	pArena->pos = ARENA_HEADER_SIZE;
	pArena->committed = initialCommitSize;
	pArena->reserved = reserveSize;

	// The arena again is in the start of the reserved memory block. It
	// is okay to return now.
	return pArena;
}

void arenaRelease(Arena* pArena)
{
	if (!pArena)
		return;

	Arena* current = pArena->pCurrent;

	while (current)
	{
		Arena* prev = current->pPrev;
		platformReleaseMemory(current, current->reserved);
		current = prev;
	}
}

void* arenaPush(Arena* pArena, uint64_t size, uint64_t align)
{
	// NOTE align is default 8 bytes

	if (!pArena || size == 0)
		return nullptr;

	Arena* pCurrent = pArena->pCurrent;

	// We need to figure out that if we can fit the requested size in the
	// current arena block, or if we need to chain a new one.
	uint64_t posAligned = alignPow2(pCurrent->pos, align);
	uint64_t posNew = posAligned + size;

	if (posNew > pCurrent->reserved)
	{
		// Not enough space in current block, so chain it.
		if (pCurrent->flags & ArenaFlag_NoChain)
		{
			LOGF(eERROR, "Arena exhausted (NoChain flag set)");
			return nullptr;
		}

		uint64_t newReserveSize = pCurrent->reserveSize;
		uint64_t newCommitSize = pCurrent->commitSize;

		// If the allocation is huge, make the new block big enough to
		// accommodate it.
		if (size + ARENA_HEADER_SIZE > newReserveSize)
		{
			newReserveSize = alignPow2(size + ARENA_HEADER_SIZE, newCommitSize);
		}

		void* pNewBlock = platformReserveMemory(newReserveSize);
		if (!pNewBlock)
		{
			LOGF(eERROR, "Failed to reserve memory for new arena block");
			return nullptr;
		}

		uint64_t initialCommit = newCommitSize;
		uint64_t neededCommit = ARENA_HEADER_SIZE + size;
		if (neededCommit > initialCommit)
		{
			initialCommit = alignPow2(neededCommit, newCommitSize);
		}

		if (!platformCommitMemory(pNewBlock, initialCommit))
		{
			LOGF(eERROR, "Failed to commit memory for new arena block");
			platformReleaseMemory(pNewBlock, newReserveSize);
			return nullptr;
		}

		Arena* pNewArena = (Arena*)pNewBlock;
		pNewArena->pPrev = pCurrent;
		pNewArena->pCurrent = pNewArena;
		pNewArena->flags = pCurrent->flags;
		pNewArena->_pad = 0;
		pNewArena->commitSize = newCommitSize;
		pNewArena->reserveSize = newReserveSize;
		pNewArena->basePos = pCurrent->basePos + pCurrent->reserved;
		pNewArena->pos = ARENA_HEADER_SIZE;
		pNewArena->committed = initialCommit;
		pNewArena->reserved = newReserveSize;

		pArena->pCurrent = pNewArena;

		// Recursively allocate from the new block
		return arenaPush(pArena, size, align);
	}

	if (posNew > pCurrent->committed)
	{
		uint64_t commitTarget = alignPow2(posNew, pCurrent->commitSize);

		// Clamp to reserved size
		if (commitTarget > pCurrent->reserved)
		{
			commitTarget = pCurrent->reserved;
		}

		uint64_t commitAmount = commitTarget - pCurrent->committed;

		void* pCommitStart = (uint8_t*)pCurrent + pCurrent->committed;
		if (!platformCommitMemory(pCommitStart, commitAmount))
		{
			LOGF(eERROR, "Failed to commit additional memory");
			return nullptr;
		}

		pCurrent->committed = commitTarget;
	}

	void* pResult = (uint8_t*)pCurrent + posAligned;

	pCurrent->pos = posNew;

	return pResult;
}

uint64_t arenaGetPos(Arena* pArena)
{
	if (!pArena)
		return 0;

	Arena* pCurrent = pArena->pCurrent;

	// Global position is where this block starts + local position of block
	return pCurrent->basePos + pCurrent->pos;
}

void arenaPopTo(Arena* pArena, uint64_t targetPos)
{
	if (!pArena)
		return;

	if (targetPos < ARENA_HEADER_SIZE)
		targetPos = ARENA_HEADER_SIZE;

	// Walk back through the arena blocks to find the one containing targetPos
	Arena* current = pArena->pCurrent;

	while (current && current->basePos >= targetPos)
	{
		Arena* prev = current->pPrev;
		platformReleaseMemory(current, current->reserved);
		current = prev;
	}

	// We should never be at a nullptr state
	if (!current)
	{
		LOGF(eERROR, "arenaPopTo: invalid target position");
		return;
	}

	pArena->pCurrent = current;

	uint64_t localPos = targetPos - current->basePos;

	current->pos = localPos;
}

void arenaPop(Arena* pArena, uint64_t amount)
{
	if (!pArena)
		return;

	uint64_t currentPos = arenaGetPos(pArena);

	uint64_t targetPos = 0;
	if (amount < currentPos)
	{
		targetPos = currentPos - amount;
	}

	arenaPopTo(pArena, targetPos);
}

void arenaClear(Arena* pArena)
{
	arenaPopTo(pArena, 0);
}

ArenaTemp arenaTempBegin(Arena* pArena)
{
	ArenaTemp temp{};
	temp.pArena = pArena;
	temp.pos = arenaGetPos(pArena);
	return temp;
}

void arenaTempEnd(ArenaTemp temp)
{
	arenaPopTo(temp.pArena, temp.pos);
}
