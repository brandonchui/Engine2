/*
 * ArenaTypes.h
 *
 */

#ifndef _ARENATYPES_H_
#define _ARENATYPES_H_

#include <stdint.h>

/**
	Macros
 */
#define Kilobyte(n) ((n) * 1024)
#define Megabyte(n) (Kilobyte(n) * 1024)

#define ARENA_HEADER_SIZE 128			   ///< Size of Arena header in bytes
#define ARENA_DEFAULT_RESERVE Megabyte(64) ///< Default virtual reservation (64MB)
#define ARENA_DEFAULT_COMMIT Kilobyte(64)  ///< Default commit granularity (64KB)

/**
	Arena allocator flags.

	Allows to chain arena blocks to allow growth.
*/
enum ArenaFlags : uint32_t
{
	ArenaFlag_None = 0,
	ArenaFlag_NoChain = (1 << 0),
};

/**
	Arena block header and state.

	Each arena consists of one or more blocks chained together. This structure
	contains the metadata for a single block.

	@note 128 bytes for alignment and cache line
*/
struct Arena
{
	Arena* pPrev;		  ///< Previous block in chain via linked list
	Arena* pCurrent;	  ///< Current active block for allocation
	uint32_t flags;		  ///< ArenaFlags controlling behavior
	uint32_t _pad;		  ///< Padding
	uint64_t commitSize;  ///< How much memory to commit at a time
	uint64_t reserveSize; ///< How much virtual memory to reserve per block
	uint64_t basePos;	  ///< Global offset of this block in the arena chain
	uint64_t pos;		  ///< Current position within this current block
	uint64_t committed;	  ///< Total bytes committed in physical memory
	uint64_t reserved;	  ///< Total bytes reserved in virtual address
};
static_assert(sizeof(Arena) <= 128, "Arena header must fit in 128 bytes");

/**
	Arena creation parameters.

	Specifies the configuration for a new arena.
*/
struct ArenaParams
{
	uint32_t flags;		  ///< ArenaFlags
	uint64_t reserveSize; ///< Virtual memory reservation size per block (0 = default)
	uint64_t commitSize;  ///< Physical memory commit (0 = default)
	void* pBackingBuffer; ///< Optional preallocated buffer (nullptr = allocate new)
};

/**
	Temporary arena scope marker.

	Captures the current position of an arena for later restoration.
	Think of it like a stack frame for arena allocations.

	@see arenaTempBegin
	@see arenaTempEnd
*/
struct ArenaTemp
{
	Arena* pArena; ///< Arena being tracked
	uint64_t pos;  ///< Saved position to restore
};

#endif // _ARENATYPES_H_
