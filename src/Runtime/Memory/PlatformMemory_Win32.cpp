/*
 * PlatformMemory_Win32.cpp
 *
 * Windows implementation of virtual memory operations using VirtualAlloc/VirtualFree.
 */

#if defined(_WIN32)

#include "Runtime/Memory/PlatformMemory.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void* platformReserveMemory(uint64_t size)
{
	// Reserve virtual address space
	void* ptr = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
	return ptr;
}

bool platformCommitMemory(void* pMemory, uint64_t size)
{
	uint64_t pageSize = platformGetPageSize();
	uint64_t alignedSize = (size + pageSize - 1) & ~(pageSize - 1);

	void* result = VirtualAlloc(pMemory, alignedSize, MEM_COMMIT, PAGE_READWRITE);
	return result != nullptr;
}

void platformDecommitMemory(void* pMemory, uint64_t size)
{
	VirtualFree(pMemory, size, MEM_DECOMMIT);
}

void platformReleaseMemory(void* pMemory, uint64_t size)
{
	// Size is default 0
	VirtualFree(pMemory, 0, MEM_RELEASE);
}

uint64_t platformGetPageSize()
{
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	return (uint64_t)sysInfo.dwPageSize;
}

#endif
