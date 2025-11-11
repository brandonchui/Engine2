/*
 * SlotMap.cpp
 *
 */

#include "Core/SlotMap.h"
#include "Runtime/Memory/Arena.h"
#include "Utilities/Interfaces/ILog.h"
#include <assert.h>
#include <string.h>

static void slotMapGrow(SlotMap* pSlotMap)
{
	uint32_t newCapacity = pSlotMap->capacity * 2;

	void* pNewValues = arenaPush(pSlotMap->pArena, pSlotMap->valueSize * newCapacity,
								 pSlotMap->valueAlign);
	uint32_t* pNewIndices = arenaPushArray<uint32_t>(pSlotMap->pArena, newCapacity);
	uint32_t* pNewGenerations = arenaPushArray<uint32_t>(pSlotMap->pArena, newCapacity);
	uint32_t* pNewErase = arenaPushArray<uint32_t>(pSlotMap->pArena, newCapacity);

	if (!pNewValues || !pNewIndices || !pNewGenerations || !pNewErase)
	{
		LOGF(eERROR, "SlotMap: Failed to grow capacity from %u to %u", pSlotMap->capacity,
			 newCapacity);
		return;
	}

	memcpy(pNewValues, pSlotMap->pValues, pSlotMap->valueSize * pSlotMap->count);
	memcpy(pNewIndices, pSlotMap->pIndices, sizeof(uint32_t) * pSlotMap->capacity);
	memcpy(pNewGenerations, pSlotMap->pGenerations, sizeof(uint32_t) * pSlotMap->capacity);
	memcpy(pNewErase, pSlotMap->pErase, sizeof(uint32_t) * pSlotMap->count);

	for (uint32_t i = pSlotMap->capacity; i < newCapacity; i++)
	{
		pNewIndices[i] = UINT32_MAX;
		pNewGenerations[i] = 0;
	}

	pSlotMap->pValues = pNewValues;
	pSlotMap->pIndices = pNewIndices;
	pSlotMap->pGenerations = pNewGenerations;
	pSlotMap->pErase = pNewErase;
	pSlotMap->capacity = newCapacity;
}

SlotMap* slotMapCreate(Arena* pArena, uint32_t valueSize, uint32_t valueAlign,
					   uint32_t initialCapacity)
{
	if (!pArena || valueSize == 0 || initialCapacity == 0)
		return nullptr;

	SlotMap* pSlotMap = arenaPushStruct<SlotMap>(pArena);
	if (!pSlotMap)
		return nullptr;

	pSlotMap->pArena = pArena;
	pSlotMap->capacity = initialCapacity;
	pSlotMap->count = 0;
	pSlotMap->freeHead = UINT32_MAX;
	pSlotMap->valueSize = valueSize;
	pSlotMap->valueAlign = valueAlign >= 8 ? valueAlign : 8;

	pSlotMap->pValues = arenaPush(pArena, valueSize * initialCapacity, pSlotMap->valueAlign);
	pSlotMap->pIndices = arenaPushArray<uint32_t>(pArena, initialCapacity);
	pSlotMap->pGenerations = arenaPushArray<uint32_t>(pArena, initialCapacity);
	pSlotMap->pErase = arenaPushArray<uint32_t>(pArena, initialCapacity);

	if (!pSlotMap->pValues || !pSlotMap->pIndices || !pSlotMap->pGenerations || !pSlotMap->pErase)
	{
		LOGF(eERROR, "SlotMap: Failed to allocate arrays for capacity %u", initialCapacity);
		return nullptr;
	}

	for (uint32_t i = 0; i < initialCapacity; i++)
	{
		pSlotMap->pIndices[i] = UINT32_MAX;
		pSlotMap->pGenerations[i] = 0;
	}

	return pSlotMap;
}

uint32_t slotMapInsertImpl(SlotMap* pSlotMap, const void* pValue)
{
	if (!pSlotMap || !pValue)
		return HANDLE_INVALID_ID;

	if (pSlotMap->count >= pSlotMap->capacity)
	{
		slotMapGrow(pSlotMap);

		if (pSlotMap->count >= pSlotMap->capacity)
		{
			LOGF(eERROR, "SlotMap: Failed to grow, insertion failed");
			return HANDLE_INVALID_ID;
		}
	}

	uint32_t sparseIndex;

	if (pSlotMap->freeHead != UINT32_MAX)
	{
		sparseIndex = pSlotMap->freeHead;
		pSlotMap->freeHead = pSlotMap->pIndices[sparseIndex];
	}
	else
	{
		sparseIndex = pSlotMap->count;

		if (sparseIndex >= pSlotMap->capacity)
		{
			slotMapGrow(pSlotMap);
			if (sparseIndex >= pSlotMap->capacity)
			{
				LOGF(eERROR, "SlotMap: Out of sparse indices");
				return HANDLE_INVALID_ID;
			}
		}
	}

	uint32_t denseIndex = pSlotMap->count;
	pSlotMap->count++;

	pSlotMap->pIndices[sparseIndex] = denseIndex;
	pSlotMap->pErase[denseIndex] = sparseIndex;

	void* pDest = (uint8_t*)pSlotMap->pValues + (denseIndex * pSlotMap->valueSize);
	memcpy(pDest, pValue, pSlotMap->valueSize);

	uint32_t generation = pSlotMap->pGenerations[sparseIndex];
	return handleMake(sparseIndex, generation);
}

void* slotMapGetImpl(SlotMap* pSlotMap, uint32_t handle)
{
	if (!pSlotMap || !handleIsValid(handle))
		return nullptr;

	uint32_t sparseIndex = handleIndex(handle);
	uint32_t generation = handleGeneration(handle);

	if (sparseIndex >= pSlotMap->capacity)
		return nullptr;

	if (pSlotMap->pGenerations[sparseIndex] != generation)
		return nullptr;

	uint32_t denseIndex = pSlotMap->pIndices[sparseIndex];

	if (denseIndex == UINT32_MAX || denseIndex >= pSlotMap->count)
		return nullptr;

	return (uint8_t*)pSlotMap->pValues + (denseIndex * pSlotMap->valueSize);
}

void slotMapRemoveImpl(SlotMap* pSlotMap, uint32_t handle)
{
	if (!pSlotMap || !handleIsValid(handle))
		return;

	uint32_t sparseIndex = handleIndex(handle);
	uint32_t generation = handleGeneration(handle);

	if (sparseIndex >= pSlotMap->capacity)
		return;

	if (pSlotMap->pGenerations[sparseIndex] != generation)
		return;

	uint32_t denseIndex = pSlotMap->pIndices[sparseIndex];

	if (denseIndex == UINT32_MAX || denseIndex >= pSlotMap->count)
		return;

	uint32_t lastDenseIndex = pSlotMap->count - 1;

	if (denseIndex != lastDenseIndex)
	{
		void* pDest = (uint8_t*)pSlotMap->pValues + (denseIndex * pSlotMap->valueSize);
		void* pSrc = (uint8_t*)pSlotMap->pValues + (lastDenseIndex * pSlotMap->valueSize);
		memcpy(pDest, pSrc, pSlotMap->valueSize);

		uint32_t movedSparseIndex = pSlotMap->pErase[lastDenseIndex];
		pSlotMap->pIndices[movedSparseIndex] = denseIndex;
		pSlotMap->pErase[denseIndex] = movedSparseIndex;
	}

	pSlotMap->count--;

	pSlotMap->pGenerations[sparseIndex]++;
	pSlotMap->pIndices[sparseIndex] = pSlotMap->freeHead;
	pSlotMap->freeHead = sparseIndex;
}

bool slotMapIsValidImpl(SlotMap* pSlotMap, uint32_t handle)
{
	if (!pSlotMap || !handleIsValid(handle))
		return false;

	uint32_t sparseIndex = handleIndex(handle);
	uint32_t generation = handleGeneration(handle);

	if (sparseIndex >= pSlotMap->capacity)
		return false;

	if (pSlotMap->pGenerations[sparseIndex] != generation)
		return false;

	uint32_t denseIndex = pSlotMap->pIndices[sparseIndex];

	return denseIndex != UINT32_MAX && denseIndex < pSlotMap->count;
}

uint32_t slotMapCount(SlotMap* pSlotMap)
{
	return pSlotMap ? pSlotMap->count : 0;
}

uint32_t slotMapCapacity(SlotMap* pSlotMap)
{
	return pSlotMap ? pSlotMap->capacity : 0;
}
