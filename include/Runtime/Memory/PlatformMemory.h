/*
 * PlatformMemory.h
 *
 * Platform virtual memory abstraction layer.
 * An interface for reserve/commit memory operations across different OSes.
 *
 */

#ifndef _PLATFORMMEMORY_H_
#define _PLATFORMMEMORY_H_

#include <stdint.h>

/**
	Reserves a region of virtual address space without committing physical memory.

	This allocates address space from the operating system but does not consume
	physical RAM until platformCommitMemory() is called on the reserved region.

	On Windows: Uses VirtualAlloc with MEM_RESERVE
	On macOS: TODO

	@param size Size in bytes to reserve

	@return Pointer to reserved memory region, or nullptr on failure

	@note The returned memory is NOT accessible until committed

	@see platformCommitMemory
	@see platformReleaseMemory
*/
void* platformReserveMemory(uint64_t size);

/**
	Commits physical memory to a previously reserved region.

	Backs a portion of reserved virtual memory with actual physical pages,
	making the memory accessible for read/write operations.

	On Windows: Uses VirtualAlloc with MEM_COMMIT
	On macOS: TODO

	@param pMemory Pointer within a reserved region that's page aligned
	@param size Number of bytes to commit, rounded to page size

	@return True if commit succeeded, false on failure

	@note Memory is zero initialized by the operating system

	@see platformReserveMemory
	@see platformDecommitMemory
*/
bool platformCommitMemory(void* pMemory, uint64_t size);

/**
	Decommits physical memory from a region.

	Removes the physical backing from virtual pages while keeping the
	address space reserved.

	On Windows: Uses VirtualFree with MEM_DECOMMIT
	On macOS: TODO

	@param pMemory Pointer to committed memory that's page aligned
	@param size Number of bytes to decommit, rounded to page size

	@note The virtual address space remains reserved

	@see platformCommitMemory
*/
void platformDecommitMemory(void* pMemory, uint64_t size);

/**
	Releases a reserved memory region back to the operating system.

	Frees both the virtual address space and any committed physical pages.

	On Windows: Uses VirtualFree with MEM_RELEASE
	On macOS: TODO

	@param pMemory Pointer to the base of the reserved region
	@param size Size of the region

	@note This releases the entire reservation, not partial regions

	@see platformReserveMemory
*/
void platformReleaseMemory(void* pMemory, uint64_t size);

/**
	Returns the operating system's memory page size.

	This is the minimum granularity for commit/decommit operations.

	Usually values:
	- Windows: 4096 bytes
	- macOS: TODO

	@return Page size in bytes
*/
uint64_t platformGetPageSize();

#endif // _PLATFORMMEMORY_H_
