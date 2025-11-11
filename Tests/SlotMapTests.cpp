#include "catch_amalgamated.hpp"
#include "Runtime/Memory/Arena.h"
#include "Core/SlotMap.h"

TEST_CASE("SlotMap template API works", "[slotmap][template]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	int value = 42;
	uint32_t handle = slotMapInsert(pMap, value);
	int* pValue = slotMapGet<int>(pMap, handle);

	REQUIRE(*pValue == 42);
	arenaRelease(pArena);
}

TEST_CASE("SlotMap null arena fails", "[slotmap][edge]")
{
	SlotMap* pMap = slotMapCreate(nullptr, sizeof(int), alignof(int), 16);
	REQUIRE(pMap == nullptr);
}

TEST_CASE("SlotMap zero value size fails", "[slotmap][edge]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, 0, alignof(int), 16);
	REQUIRE(pMap == nullptr);
	arenaRelease(pArena);
}

TEST_CASE("SlotMap zero capacity fails", "[slotmap][edge]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 0);
	REQUIRE(pMap == nullptr);
	arenaRelease(pArena);
}

TEST_CASE("SlotMap insert null map fails", "[slotmap][edge]")
{
	int value = 42;
	uint32_t handle = slotMapInsertImpl(nullptr, &value);
	REQUIRE(handle == HANDLE_INVALID_ID);
}

TEST_CASE("SlotMap insert null value fails", "[slotmap][edge]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);
	uint32_t handle = slotMapInsertImpl(pMap, nullptr);
	REQUIRE(handle == HANDLE_INVALID_ID);
	arenaRelease(pArena);
}

TEST_CASE("SlotMap get null map returns null", "[slotmap][edge]")
{
	void* pValue = slotMapGetImpl(nullptr, 0);
	REQUIRE(pValue == nullptr);
}

TEST_CASE("SlotMap get invalid handle returns null", "[slotmap][edge]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);
	void* pValue = slotMapGetImpl(pMap, HANDLE_INVALID_ID);
	REQUIRE(pValue == nullptr);
	arenaRelease(pArena);
}

TEST_CASE("SlotMap remove null map is safe", "[slotmap][edge]")
{
	slotMapRemove(nullptr, 0);
	REQUIRE(true);
}

TEST_CASE("SlotMap remove invalid handle is safe", "[slotmap][edge]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);
	slotMapRemove(pMap, HANDLE_INVALID_ID);
	REQUIRE(slotMapCount(pMap) == 0);
	arenaRelease(pArena);
}

TEST_CASE("SlotMap isValid null map returns false", "[slotmap][edge]")
{
	REQUIRE(!slotMapIsValid(nullptr, 0));
}

TEST_CASE("SlotMap count null map returns zero", "[slotmap][edge]")
{
	REQUIRE(slotMapCount(nullptr) == 0);
}

TEST_CASE("SlotMap capacity null map returns zero", "[slotmap][edge]")
{
	REQUIRE(slotMapCapacity(nullptr) == 0);
}

TEST_CASE("Handle invalid ID is not valid", "[handle]")
{
	REQUIRE(!handleIsValid(HANDLE_INVALID_ID));
}

TEST_CASE("Handle make and extract index", "[handle]")
{
	uint32_t handle = handleMake(12345, 7);
	REQUIRE(handleIndex(handle) == 12345);
	REQUIRE(handleGeneration(handle) == 7);
}

TEST_CASE("Handle index mask works correctly", "[handle]")
{
	uint32_t handle = handleMake(0x00FFFFFF, 0);
	REQUIRE(handleIndex(handle) == 0x00FFFFFF);
}

TEST_CASE("Handle generation mask works correctly", "[handle]")
{
	uint32_t handle = handleMake(0, 0xFF);
	REQUIRE(handleGeneration(handle) == 0xFF);
}

TEST_CASE("Handle generation wraps at 256", "[handle]")
{
	uint32_t handle = handleMake(100, 256);
	REQUIRE(handleGeneration(handle) == 0);
}

TEST_CASE("SlotMap basic creation", "[slotmap]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);
	REQUIRE(pMap != nullptr);
	REQUIRE(slotMapCount(pMap) == 0);
	REQUIRE(slotMapCapacity(pMap) == 16);
	arenaRelease(pArena);
}

TEST_CASE("SlotMap insert and get single item", "[slotmap]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	int value = 42;
	uint32_t handle = slotMapInsert(pMap, value);
	REQUIRE(handleIsValid(handle));
	REQUIRE(slotMapCount(pMap) == 1);
	REQUIRE(slotMapIsValid(pMap, handle));

	int* pValue = slotMapGet<int>(pMap, handle);
	REQUIRE(pValue != nullptr);
	REQUIRE(*pValue == 42);

	arenaRelease(pArena);
}

TEST_CASE("SlotMap multiple inserts maintain values", "[slotmap]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	uint32_t handles[10];
	for (int i = 0; i < 10; i++)
	{
		int value = i * 10;
		handles[i] = slotMapInsert(pMap, value);
		REQUIRE(handleIsValid(handles[i]));
	}

	REQUIRE(slotMapCount(pMap) == 10);

	for (int i = 0; i < 10; i++)
	{
		int* pValue = slotMapGet<int>(pMap, handles[i]);
		REQUIRE(pValue != nullptr);
		REQUIRE(*pValue == i * 10);
	}

	arenaRelease(pArena);
}

TEST_CASE("SlotMap remove makes handle invalid", "[slotmap]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	int value = 100;
	uint32_t handle = slotMapInsert(pMap, value);
	REQUIRE(slotMapIsValid(pMap, handle));

	slotMapRemove(pMap, handle);
	REQUIRE(slotMapCount(pMap) == 0);
	REQUIRE(!slotMapIsValid(pMap, handle));

	int* pValue = slotMapGet<int>(pMap, handle);
	REQUIRE(pValue == nullptr);

	arenaRelease(pArena);
}

TEST_CASE("SlotMap generation increments on remove", "[slotmap][generation]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	int value1 = 42;
	uint32_t handle1 = slotMapInsert(pMap, value1);
	uint32_t index1 = handleIndex(handle1);
	uint32_t gen1 = handleGeneration(handle1);

	slotMapRemove(pMap, handle1);

	int value2 = 99;
	uint32_t handle2 = slotMapInsert(pMap, value2);
	uint32_t index2 = handleIndex(handle2);
	uint32_t gen2 = handleGeneration(handle2);

	REQUIRE(index1 == index2);
	REQUIRE(gen2 == gen1 + 1);
	REQUIRE(!slotMapIsValid(pMap, handle1));
	REQUIRE(slotMapIsValid(pMap, handle2));

	arenaRelease(pArena);
}

TEST_CASE("SlotMap stale handle rejected after slot reuse", "[slotmap][generation]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	int value1 = 10;
	uint32_t handleA = slotMapInsert(pMap, value1);

	int value2 = 20;
	uint32_t handleB = slotMapInsert(pMap, value2);

	slotMapRemove(pMap, handleA);

	int value3 = 30;
	uint32_t handleC = slotMapInsert(pMap, value3);

	int* pValueA = slotMapGet<int>(pMap, handleA);
	REQUIRE(pValueA == nullptr);

	int* pValueB = slotMapGet<int>(pMap, handleB);
	REQUIRE(pValueB != nullptr);
	REQUIRE(*pValueB == 20);

	int* pValueC = slotMapGet<int>(pMap, handleC);
	REQUIRE(pValueC != nullptr);
	REQUIRE(*pValueC == 30);

	arenaRelease(pArena);
}

TEST_CASE("SlotMap generation wraparound survives", "[slotmap][generation]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	int value = 1;
	uint32_t handle = slotMapInsert(pMap, value);

	for (int i = 0; i < 300; i++)
	{
		slotMapRemove(pMap, handle);
		handle = slotMapInsert(pMap, value);
	}

	REQUIRE(slotMapIsValid(pMap, handle));
	int* pValue = slotMapGet<int>(pMap, handle);
	REQUIRE(pValue != nullptr);

	arenaRelease(pArena);
}

TEST_CASE("SlotMap grows when capacity exceeded", "[slotmap][growth]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 4);
	REQUIRE(slotMapCapacity(pMap) == 4);

	uint32_t handles[10];
	for (int i = 0; i < 10; i++)
	{
		int value = i;
		handles[i] = slotMapInsert(pMap, value);
	}

	REQUIRE(slotMapCount(pMap) == 10);
	REQUIRE(slotMapCapacity(pMap) >= 10);

	for (int i = 0; i < 10; i++)
	{
		int* pValue = slotMapGet<int>(pMap, handles[i]);
		REQUIRE(pValue != nullptr);
		REQUIRE(*pValue == i);
	}

	arenaRelease(pArena);
}

TEST_CASE("SlotMap handles survive growth", "[slotmap][growth]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 2);

	int value1 = 100;
	uint32_t handle1 = slotMapInsert(pMap, value1);

	int value2 = 200;
	uint32_t handle2 = slotMapInsert(pMap, value2);

	int value3 = 300;
	uint32_t handle3 = slotMapInsert(pMap, value3);

	REQUIRE(slotMapCapacity(pMap) >= 3);

	int* pValue1 = slotMapGet<int>(pMap, handle1);
	REQUIRE(pValue1 != nullptr);
	REQUIRE(*pValue1 == 100);

	int* pValue2 = slotMapGet<int>(pMap, handle2);
	REQUIRE(pValue2 != nullptr);
	REQUIRE(*pValue2 == 200);

	int* pValue3 = slotMapGet<int>(pMap, handle3);
	REQUIRE(pValue3 != nullptr);
	REQUIRE(*pValue3 == 300);

	arenaRelease(pArena);
}

TEST_CASE("SlotMap remove middle element maintains others", "[slotmap][remove]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	uint32_t handles[5];
	for (int i = 0; i < 5; i++)
	{
		int value = i * 10;
		handles[i] = slotMapInsert(pMap, value);
	}

	slotMapRemove(pMap, handles[2]);
	REQUIRE(slotMapCount(pMap) == 4);
	REQUIRE(!slotMapIsValid(pMap, handles[2]));

	for (int i = 0; i < 5; i++)
	{
		if (i == 2)
			continue;
		int* pValue = slotMapGet<int>(pMap, handles[i]);
		REQUIRE(pValue != nullptr);
		REQUIRE(*pValue == i * 10);
	}

	arenaRelease(pArena);
}

TEST_CASE("SlotMap multiple removes in sequence", "[slotmap][remove]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	uint32_t handles[10];
	for (int i = 0; i < 10; i++)
	{
		int value = i;
		handles[i] = slotMapInsert(pMap, value);
	}

	slotMapRemove(pMap, handles[3]);
	slotMapRemove(pMap, handles[7]);
	slotMapRemove(pMap, handles[1]);

	REQUIRE(slotMapCount(pMap) == 7);
	REQUIRE(!slotMapIsValid(pMap, handles[1]));
	REQUIRE(!slotMapIsValid(pMap, handles[3]));
	REQUIRE(!slotMapIsValid(pMap, handles[7]));

	arenaRelease(pArena);
}

TEST_CASE("SlotMap double remove is safe", "[slotmap][edge]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	int value = 42;
	uint32_t handle = slotMapInsert(pMap, value);

	slotMapRemove(pMap, handle);
	REQUIRE(slotMapCount(pMap) == 0);

	slotMapRemove(pMap, handle);
	REQUIRE(slotMapCount(pMap) == 0);

	arenaRelease(pArena);
}

TEST_CASE("SlotMap with large struct", "[slotmap]")
{
	struct LargeStruct
	{
		int data[100];
		float values[50];
		char name[256];
	};

	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(LargeStruct), alignof(LargeStruct), 16);

	LargeStruct large = {};
	large.data[0] = 999;
	large.values[0] = 3.14f;
	large.name[0] = 'X';

	uint32_t handle = slotMapInsert(pMap, large);
	LargeStruct* pLarge = slotMapGet<LargeStruct>(pMap, handle);

	REQUIRE(pLarge != nullptr);
	REQUIRE(pLarge->data[0] == 999);
	REQUIRE(pLarge->values[0] == 3.14f);
	REQUIRE(pLarge->name[0] == 'X');

	arenaRelease(pArena);
}

TEST_CASE("SlotMap stress - many inserts", "[slotmap][stress]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	for (int i = 0; i < 1000; i++)
	{
		int value = i;
		uint32_t handle = slotMapInsert(pMap, value);
		REQUIRE(handleIsValid(handle));
	}

	REQUIRE(slotMapCount(pMap) == 1000);
	arenaRelease(pArena);
}

TEST_CASE("SlotMap stress - insert remove pattern", "[slotmap][stress]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	uint32_t handles[100];
	for (int i = 0; i < 100; i++)
	{
		int value = i;
		handles[i] = slotMapInsert(pMap, value);
	}

	for (int i = 0; i < 50; i++)
	{
		slotMapRemove(pMap, handles[i * 2]);
	}

	REQUIRE(slotMapCount(pMap) == 50);

	for (int i = 0; i < 50; i++)
	{
		int value = i + 1000;
		uint32_t handle = slotMapInsert(pMap, value);
		REQUIRE(handleIsValid(handle));
	}

	REQUIRE(slotMapCount(pMap) == 100);
	arenaRelease(pArena);
}

TEST_CASE("SlotMap stress - random operations", "[slotmap][stress]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 8);

	uint32_t handles[200];
	int activeCount = 0;

	for (int i = 0; i < 200; i++)
	{
		int value = i;
		handles[i] = slotMapInsert(pMap, value);
		activeCount++;
	}

	for (int i = 0; i < 200; i += 3)
	{
		if (slotMapIsValid(pMap, handles[i]))
		{
			slotMapRemove(pMap, handles[i]);
			activeCount--;
		}
	}

	REQUIRE(slotMapCount(pMap) == (uint32_t)activeCount);

	for (int i = 0; i < 50; i++)
	{
		int value = 9999 + i;
		uint32_t handle = slotMapInsert(pMap, value);
		REQUIRE(handleIsValid(handle));
		activeCount++;
	}

	REQUIRE(slotMapCount(pMap) == (uint32_t)activeCount);
	arenaRelease(pArena);
}

TEST_CASE("SlotMap get with out of bounds index", "[slotmap][edge]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	uint32_t bogusHandle = handleMake(9999, 0);
	void* pValue = slotMapGetImpl(pMap, bogusHandle);
	REQUIRE(pValue == nullptr);

	arenaRelease(pArena);
}

TEST_CASE("SlotMap isValid with out of bounds index", "[slotmap][edge]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	uint32_t bogusHandle = handleMake(9999, 0);
	REQUIRE(!slotMapIsValid(pMap, bogusHandle));

	arenaRelease(pArena);
}

TEST_CASE("SlotMap remove with out of bounds index", "[slotmap][edge]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	uint32_t bogusHandle = handleMake(9999, 123);
	slotMapRemove(pMap, bogusHandle);
	REQUIRE(slotMapCount(pMap) == 0);

	arenaRelease(pArena);
}

TEST_CASE("SlotMap with TextureHandle", "[slotmap][handles]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pTextures = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	int texData = 42;
	uint32_t handleId = slotMapInsert(pTextures, texData);
	TextureHandle texHandle = {handleId};

	REQUIRE(texHandle.id == handleId);
	REQUIRE(handleIsValid(texHandle.id));
	REQUIRE(slotMapIsValid(pTextures, texHandle.id));

	int* pData = slotMapGet<int>(pTextures, texHandle.id);
	REQUIRE(pData != nullptr);
	REQUIRE(*pData == 42);

	arenaRelease(pArena);
}

TEST_CASE("SlotMap with all typed handles", "[slotmap][handles]")
{
	Arena* pArena = arenaCreate();
	SlotMap* pMap = slotMapCreate(pArena, sizeof(int), alignof(int), 16);

	int value = 100;
	uint32_t handleId = slotMapInsert(pMap, value);

	TextureHandle tex = {handleId};
	MeshHandle mesh = {handleId};
	MaterialHandle mat = {handleId};
	ShaderHandle shader = {handleId};

	REQUIRE(tex.id == handleId);
	REQUIRE(mesh.id == handleId);
	REQUIRE(mat.id == handleId);
	REQUIRE(shader.id == handleId);

	arenaRelease(pArena);
}

TEST_CASE("SlotMap invalid typed handles", "[slotmap][handles]")
{
	TextureHandle tex = INVALID_TEXTURE_HANDLE;
	MeshHandle mesh = INVALID_MESH_HANDLE;
	MaterialHandle mat = INVALID_MATERIAL_HANDLE;
	ShaderHandle shader = INVALID_SHADER_HANDLE;

	REQUIRE(!handleIsValid(tex.id));
	REQUIRE(!handleIsValid(mesh.id));
	REQUIRE(!handleIsValid(mat.id));
	REQUIRE(!handleIsValid(shader.id));
}
