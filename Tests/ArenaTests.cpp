#include "catch_amalgamated.hpp"
#include "Runtime/Memory/Arena.h"
#include <cstring>
#include <vector>

TEST_CASE("Arena basic creation and release", "[arena]")
{
	Arena* pArena = arenaCreate();
	REQUIRE(pArena != nullptr);
	arenaRelease(pArena);
}

TEST_CASE("Arena null input handling", "[arena][edge-cases]")
{
	arenaRelease(nullptr);

	void* p1 = arenaPush(nullptr, 100, 8);
	REQUIRE(p1 == nullptr);

	Arena* pArena = arenaCreate();
	void* p2 = arenaPush(pArena, 0, 8);
	REQUIRE(p2 == nullptr);

	arenaRelease(pArena);
}

TEST_CASE("Arena alignment verification", "[arena][alignment]")
{
	Arena* pArena = arenaCreate();

	uint64_t alignments[] = {1, 2, 4, 8, 16, 32, 64, 128};
	for (uint64_t align : alignments)
	{
		void* p = arenaPush(pArena, 1, align);
		REQUIRE(p != nullptr);
		REQUIRE(((uintptr_t)p % align) == 0);
	}

	arenaRelease(pArena);
}

TEST_CASE("Arena alignment with various sizes", "[arena][alignment]")
{
	Arena* pArena = arenaCreate();

	struct TestCase
	{
		uint64_t size;
		uint64_t align;
	};
	TestCase cases[] = {{1, 16}, {7, 8}, {15, 32}, {33, 64}, {127, 128}, {1000, 16}, {4095, 64}};

	for (const TestCase& tc : cases)
	{
		void* p = arenaPush(pArena, tc.size, tc.align);
		REQUIRE(p != nullptr);
		REQUIRE(((uintptr_t)p % tc.align) == 0);
	}

	arenaRelease(pArena);
}

TEST_CASE("Arena sequential allocations don't overlap", "[arena][integrity]")
{
	Arena* pArena = arenaCreate();

	void* p1 = arenaPush(pArena, 100, 8);
	void* p2 = arenaPush(pArena, 200, 8);
	void* p3 = arenaPush(pArena, 300, 8);

	REQUIRE(p1 != nullptr);
	REQUIRE(p2 != nullptr);
	REQUIRE(p3 != nullptr);

	uintptr_t addr1 = (uintptr_t)p1;
	uintptr_t addr2 = (uintptr_t)p2;
	uintptr_t addr3 = (uintptr_t)p3;

	REQUIRE(addr2 >= addr1 + 100);
	REQUIRE(addr3 >= addr2 + 200);

	arenaRelease(pArena);
}

TEST_CASE("Arena data integrity after allocation", "[arena][integrity]")
{
	Arena* pArena = arenaCreate();

	uint8_t* p1 = (uint8_t*)arenaPush(pArena, 1000, 8);
	REQUIRE(p1 != nullptr);

	for (int i = 0; i < 1000; i++)
	{
		p1[i] = (uint8_t)(i & 0xFF);
	}

	uint8_t* p2 = (uint8_t*)arenaPush(pArena, 500, 8);
	REQUIRE(p2 != nullptr);

	for (int i = 0; i < 1000; i++)
	{
		REQUIRE(p1[i] == (uint8_t)(i & 0xFF));
	}

	arenaRelease(pArena);
}

TEST_CASE("Arena position tracking accuracy", "[arena][position]")
{
	Arena* pArena = arenaCreate();

	uint64_t pos1 = arenaGetPos(pArena);
	REQUIRE(pos1 == ARENA_HEADER_SIZE);

	arenaPush(pArena, 100, 8);
	uint64_t pos2 = arenaGetPos(pArena);
	REQUIRE(pos2 >= pos1 + 100);

	arenaPush(pArena, 256, 16);
	uint64_t pos3 = arenaGetPos(pArena);
	REQUIRE(pos3 >= pos2 + 256);

	arenaRelease(pArena);
}

TEST_CASE("Arena block chaining with large allocation", "[arena][chaining]")
{
	Arena* pArena = arenaCreate();

	uint64_t hugeSize = Megabyte(128);
	void* pHuge = arenaPush(pArena, hugeSize, 8);
	REQUIRE(pHuge != nullptr);

	memset(pHuge, 0xAB, (size_t)hugeSize);

	void* pSmall = arenaPush(pArena, 100, 8);
	REQUIRE(pSmall != nullptr);

	arenaRelease(pArena);
}

TEST_CASE("Arena block chaining with many allocations", "[arena][chaining]")
{
	Arena* pArena = arenaCreate();

	std::vector<void*> pointers;
	uint64_t totalAllocated = 0;

	for (int i = 0; i < 1000; i++)
	{
		void* p = arenaPush(pArena, Kilobyte(128), 8);
		REQUIRE(p != nullptr);
		pointers.push_back(p);
		totalAllocated += Kilobyte(128);
	}

	REQUIRE(totalAllocated > ARENA_DEFAULT_RESERVE);

	arenaRelease(pArena);
}

TEST_CASE("Arena custom parameters", "[arena][params]")
{
	ArenaParams params = {};
	params.reserveSize = Megabyte(32);
	params.commitSize = Kilobyte(32);
	params.flags = ArenaFlag_None;

	Arena* pArena = arenaCreate(&params);
	REQUIRE(pArena != nullptr);

	void* p = arenaPush(pArena, Kilobyte(16), 8);
	REQUIRE(p != nullptr);

	arenaRelease(pArena);
}

TEST_CASE("Arena NoChain flag prevents chaining", "[arena][flags]")
{
	ArenaParams params = {};
	params.reserveSize = Kilobyte(64);
	params.commitSize = Kilobyte(4);
	params.flags = ArenaFlag_NoChain;

	Arena* pArena = arenaCreate(&params);
	REQUIRE(pArena != nullptr);

	void* p1 = arenaPush(pArena, Kilobyte(32), 8);
	REQUIRE(p1 != nullptr);

	void* p2 = arenaPush(pArena, Kilobyte(64), 8);
	REQUIRE(p2 == nullptr);

	arenaRelease(pArena);
}

TEST_CASE("Arena popTo behavior", "[arena][pop]")
{
	Arena* pArena = arenaCreate();

	uint64_t pos1 = arenaGetPos(pArena);
	arenaPush(pArena, 1000, 8);

	uint64_t pos2 = arenaGetPos(pArena);
	arenaPush(pArena, 2000, 8);

	uint64_t pos3 = arenaGetPos(pArena);
	arenaPush(pArena, 3000, 8);

	uint64_t pos4 = arenaGetPos(pArena);

	arenaPopTo(pArena, pos3);
	REQUIRE(arenaGetPos(pArena) == pos3);

	arenaPopTo(pArena, pos2);
	REQUIRE(arenaGetPos(pArena) == pos2);

	arenaPopTo(pArena, pos1);
	REQUIRE(arenaGetPos(pArena) == pos1);

	arenaRelease(pArena);
}

TEST_CASE("Arena pop behavior", "[arena][pop]")
{
	Arena* pArena = arenaCreate();

	uint64_t startPos = arenaGetPos(pArena);
	arenaPush(pArena, 500, 8);
	uint64_t afterFirst = arenaGetPos(pArena);

	uint64_t allocated = afterFirst - startPos;
	arenaPop(pArena, allocated);

	REQUIRE(arenaGetPos(pArena) == startPos);

	arenaRelease(pArena);
}

TEST_CASE("Arena clear resets to initial state", "[arena][clear]")
{
	Arena* pArena = arenaCreate();

	uint64_t initialPos = arenaGetPos(pArena);

	arenaPush(pArena, Kilobyte(10), 8);
	arenaPush(pArena, Kilobyte(20), 8);
	arenaPush(pArena, Kilobyte(30), 8);

	REQUIRE(arenaGetPos(pArena) > initialPos);

	arenaClear(pArena);

	REQUIRE(arenaGetPos(pArena) == initialPos);

	arenaRelease(pArena);
}

TEST_CASE("Arena temp scope basic", "[arena][temp]")
{
	Arena* pArena = arenaCreate();

	uint64_t startPos = arenaGetPos(pArena);

	ArenaTemp temp = arenaTempBegin(pArena);
	arenaPush(pArena, 1000, 8);
	REQUIRE(arenaGetPos(pArena) > startPos);
	arenaTempEnd(temp);

	REQUIRE(arenaGetPos(pArena) == startPos);

	arenaRelease(pArena);
}

TEST_CASE("Arena nested temp scopes", "[arena][temp]")
{
	Arena* pArena = arenaCreate();

	uint64_t pos0 = arenaGetPos(pArena);

	ArenaTemp temp1 = arenaTempBegin(pArena);
	arenaPush(pArena, 1000, 8);
	uint64_t pos1 = arenaGetPos(pArena);

	ArenaTemp temp2 = arenaTempBegin(pArena);
	arenaPush(pArena, 2000, 8);
	uint64_t pos2 = arenaGetPos(pArena);

	ArenaTemp temp3 = arenaTempBegin(pArena);
	arenaPush(pArena, 3000, 8);
	uint64_t pos3 = arenaGetPos(pArena);

	REQUIRE(pos3 > pos2);
	REQUIRE(pos2 > pos1);
	REQUIRE(pos1 > pos0);

	arenaTempEnd(temp3);
	REQUIRE(arenaGetPos(pArena) == pos2);

	arenaTempEnd(temp2);
	REQUIRE(arenaGetPos(pArena) == pos1);

	arenaTempEnd(temp1);
	REQUIRE(arenaGetPos(pArena) == pos0);

	arenaRelease(pArena);
}

TEST_CASE("Arena temp scope with block chaining", "[arena][temp][chaining]")
{
	Arena* pArena = arenaCreate();

	uint64_t startPos = arenaGetPos(pArena);

	ArenaTemp temp = arenaTempBegin(pArena);

	for (int i = 0; i < 100; i++)
	{
		arenaPush(pArena, Megabyte(1), 8);
	}

	uint64_t afterAllocs = arenaGetPos(pArena);
	REQUIRE(afterAllocs > ARENA_DEFAULT_RESERVE);

	arenaTempEnd(temp);

	REQUIRE(arenaGetPos(pArena) == startPos);

	arenaRelease(pArena);
}

TEST_CASE("Arena template pushStruct", "[arena][templates]")
{
	Arena* pArena = arenaCreate();

	struct TestStruct
	{
		int x;
		int y;
		float z;
	};

	TestStruct* pStruct = arenaPushStruct<TestStruct>(pArena);
	REQUIRE(pStruct != nullptr);
	REQUIRE(pStruct->x == 0);
	REQUIRE(pStruct->y == 0);
	REQUIRE(pStruct->z == 0.0f);

	pStruct->x = 42;
	pStruct->y = 99;
	pStruct->z = 3.14f;

	REQUIRE(pStruct->x == 42);
	REQUIRE(pStruct->y == 99);
	REQUIRE(pStruct->z == 3.14f);

	arenaRelease(pArena);
}

TEST_CASE("Arena template pushArray", "[arena][templates]")
{
	Arena* pArena = arenaCreate();

	int* pInts = arenaPushArray<int>(pArena, 100);
	REQUIRE(pInts != nullptr);

	for (int i = 0; i < 100; i++)
	{
		REQUIRE(pInts[i] == 0);
	}

	for (int i = 0; i < 100; i++)
	{
		pInts[i] = i * 2;
	}

	for (int i = 0; i < 100; i++)
	{
		REQUIRE(pInts[i] == i * 2);
	}

	arenaRelease(pArena);
}

TEST_CASE("Arena template pushArrayNoZero", "[arena][templates]")
{
	Arena* pArena = arenaCreate();

	int* pInts = arenaPushArrayNoZero<int>(pArena, 50);
	REQUIRE(pInts != nullptr);

	for (int i = 0; i < 50; i++)
	{
		pInts[i] = i * 3;
	}

	for (int i = 0; i < 50; i++)
	{
		REQUIRE(pInts[i] == i * 3);
	}

	arenaRelease(pArena);
}

TEST_CASE("Arena template with aligned struct", "[arena][templates][alignment]")
{
	Arena* pArena = arenaCreate();

	struct alignas(64) AlignedStruct
	{
		uint64_t data[8];
	};

	AlignedStruct* pStruct = arenaPushStruct<AlignedStruct>(pArena);
	REQUIRE(pStruct != nullptr);
	REQUIRE(((uintptr_t)pStruct % 64) == 0);

	arenaRelease(pArena);
}

TEST_CASE("Arena stress test - many small allocations", "[arena][stress]")
{
	Arena* pArena = arenaCreate();

	for (int i = 0; i < 10000; i++)
	{
		void* p = arenaPush(pArena, 16, 8);
		REQUIRE(p != nullptr);
	}

	arenaRelease(pArena);
}

TEST_CASE("Arena stress test - random sizes", "[arena][stress]")
{
	Arena* pArena = arenaCreate();

	uint64_t sizes[] = {1, 7, 16, 33, 64, 127, 256, 511, 1024, 4095, 8192};

	for (int i = 0; i < 1000; i++)
	{
		uint64_t size = sizes[i % 11];
		void* p = arenaPush(pArena, size, 8);
		REQUIRE(p != nullptr);
	}

	arenaRelease(pArena);
}

TEST_CASE("Arena stress test - interleaved push and pop", "[arena][stress][pop]")
{
	Arena* pArena = arenaCreate();

	for (int i = 0; i < 100; i++)
	{
		uint64_t startPos = arenaGetPos(pArena);

		for (int j = 0; j < 50; j++)
		{
			arenaPush(pArena, Kilobyte(4), 8);
		}

		arenaPopTo(pArena, startPos);
		REQUIRE(arenaGetPos(pArena) == startPos);
	}

	arenaRelease(pArena);
}

TEST_CASE("Arena multiple arenas simultaneously", "[arena][multi]")
{
	Arena* pArena1 = arenaCreate();
	Arena* pArena2 = arenaCreate();
	Arena* pArena3 = arenaCreate();

	void* p1 = arenaPush(pArena1, 1000, 8);
	void* p2 = arenaPush(pArena2, 2000, 8);
	void* p3 = arenaPush(pArena3, 3000, 8);

	REQUIRE(p1 != nullptr);
	REQUIRE(p2 != nullptr);
	REQUIRE(p3 != nullptr);

	arenaRelease(pArena1);
	arenaRelease(pArena2);
	arenaRelease(pArena3);
}

TEST_CASE("Arena commit boundary crossing", "[arena][commit]")
{
	ArenaParams params = {};
	params.reserveSize = Megabyte(64);
	params.commitSize = Kilobyte(64);
	params.flags = ArenaFlag_None;

	Arena* pArena = arenaCreate(&params);

	for (int i = 0; i < 100; i++)
	{
		void* p = arenaPush(pArena, Kilobyte(16), 8);
		REQUIRE(p != nullptr);
		memset(p, 0xFF, Kilobyte(16));
	}

	arenaRelease(pArena);
}

TEST_CASE("Arena huge allocation", "[arena][huge]")
{
	Arena* pArena = arenaCreate();

	uint64_t hugeSize = Megabyte(256);
	void* pHuge = arenaPush(pArena, hugeSize, 8);
	REQUIRE(pHuge != nullptr);

	uint8_t* pBytes = (uint8_t*)pHuge;
	pBytes[0] = 0xAA;
	pBytes[hugeSize / 2] = 0xBB;
	pBytes[hugeSize - 1] = 0xCC;

	REQUIRE(pBytes[0] == 0xAA);
	REQUIRE(pBytes[hugeSize / 2] == 0xBB);
	REQUIRE(pBytes[hugeSize - 1] == 0xCC);

	arenaRelease(pArena);
}

TEST_CASE("Arena pop with underflow protection", "[arena][pop][edge-cases]")
{
	Arena* pArena = arenaCreate();

	arenaPush(pArena, 100, 8);

	arenaPop(pArena, 10000);

	uint64_t pos = arenaGetPos(pArena);
	REQUIRE(pos >= ARENA_HEADER_SIZE);

	arenaRelease(pArena);
}

TEST_CASE("Arena popTo below header size", "[arena][pop][edge-cases]")
{
	Arena* pArena = arenaCreate();

	arenaPush(pArena, 1000, 8);

	arenaPopTo(pArena, 0);

	uint64_t pos = arenaGetPos(pArena);
	REQUIRE(pos == ARENA_HEADER_SIZE);

	arenaRelease(pArena);
}

TEST_CASE("Arena data persistence across operations", "[arena][integrity]")
{
	Arena* pArena = arenaCreate();

	uint64_t* pData = arenaPushArray<uint64_t>(pArena, 1000);
	for (int i = 0; i < 1000; i++)
	{
		pData[i] = i * i;
	}

	uint64_t savedPos = arenaGetPos(pArena);

	ArenaTemp temp = arenaTempBegin(pArena);
	arenaPush(pArena, Kilobyte(100), 8);
	arenaTempEnd(temp);

	REQUIRE(arenaGetPos(pArena) == savedPos);

	for (int i = 0; i < 1000; i++)
	{
		REQUIRE(pData[i] == (uint64_t)(i * i));
	}

	arenaRelease(pArena);
}
