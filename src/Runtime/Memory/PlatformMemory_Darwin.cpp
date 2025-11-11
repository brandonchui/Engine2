/*
 * PlatformMemory_Darwin.cpp
 *
 */

#if defined(__APPLE__)

#include "Runtime/Memory/PlatformMemory.h"

void* platformReserveMemory(uint64_t size)
{
	//TODO Need to implement
	return nullptr;
}

bool platformCommitMemory(void* pMemory, uint64_t size)
{
	//TODO Need to implement

	return false;
}

void platformDecommitMemory(void* pMemory, uint64_t size)
{
	//TODO Need to implement
}

void platformReleaseMemory(void* pMemory, uint64_t size)
{
	//TODO Need to implement
}

uint64_t platformGetPageSize()
{
	//TODO Need to implement

	return 4096;
}

#endif // __APPLE__
