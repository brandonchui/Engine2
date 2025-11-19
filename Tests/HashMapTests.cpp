#include "catch_amalgamated.hpp"
#include "Runtime/Memory/Arena.h"
#include "Core/HashMap.h"
#include "Core/Handle.h"

TEST_CASE("HashMap template API works", "[hashmap][template]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));

	hashMapInsert(pMap, "test", 42);
	int* pValue = hashMapGet<int>(pMap, "test");

	REQUIRE(pValue != nullptr);
	REQUIRE(*pValue == 42);

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap zero value size fails", "[hashmap][edge]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, 0);
	REQUIRE(pMap == nullptr);
	arenaRelease(pArena);
}

TEST_CASE("HashMap insert null map is safe", "[hashmap][edge]")
{
	int value = 42;
	hashMapInsertImpl(nullptr, "test", &value);
	REQUIRE(true);
}

TEST_CASE("HashMap insert null key is safe", "[hashmap][edge]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));
	int value = 42;
	hashMapInsertImpl(pMap, nullptr, &value);
	REQUIRE(hashMapCount(pMap) == 0);
	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap insert null value is safe", "[hashmap][edge]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));
	hashMapInsertImpl(pMap, "test", nullptr);
	REQUIRE(hashMapCount(pMap) == 0);
	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap get null map returns null", "[hashmap][edge]")
{
	void* pValue = hashMapGetImpl(nullptr, "test");
	REQUIRE(pValue == nullptr);
}

TEST_CASE("HashMap get null key returns null", "[hashmap][edge]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));
	void* pValue = hashMapGetImpl(pMap, nullptr);
	REQUIRE(pValue == nullptr);
	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap get non-existent key returns null", "[hashmap][edge]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));
	int* pValue = hashMapGet<int>(pMap, "nonexistent");
	REQUIRE(pValue == nullptr);
	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap contains null map returns false", "[hashmap][edge]")
{
	REQUIRE(!hashMapContains(nullptr, "test"));
}

TEST_CASE("HashMap contains null key returns false", "[hashmap][edge]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));
	REQUIRE(!hashMapContains(pMap, nullptr));
	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap remove null map is safe", "[hashmap][edge]")
{
	hashMapRemove(nullptr, "test");
	REQUIRE(true);
}

TEST_CASE("HashMap remove null key is safe", "[hashmap][edge]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));
	hashMapRemove(pMap, nullptr);
	REQUIRE(true);
	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap count null map returns zero", "[hashmap][edge]")
{
	REQUIRE(hashMapCount(nullptr) == 0);
}

TEST_CASE("HashMap basic creation", "[hashmap]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));
	REQUIRE(pMap != nullptr);
	REQUIRE(hashMapCount(pMap) == 0);
	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap insert and get single item", "[hashmap]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));

	int value = 42;
	hashMapInsert(pMap, "answer", value);
	REQUIRE(hashMapCount(pMap) == 1);
	REQUIRE(hashMapContains(pMap, "answer"));

	int* pValue = hashMapGet<int>(pMap, "answer");
	REQUIRE(pValue != nullptr);
	REQUIRE(*pValue == 42);

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap multiple inserts maintain values", "[hashmap]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));

	const char* keys[] = {"one", "two", "three", "four", "five"};
	int values[] = {1, 2, 3, 4, 5};

	for (int i = 0; i < 5; i++)
	{
		hashMapInsert(pMap, keys[i], values[i]);
	}

	REQUIRE(hashMapCount(pMap) == 5);

	for (int i = 0; i < 5; i++)
	{
		REQUIRE(hashMapContains(pMap, keys[i]));
		int* pValue = hashMapGet<int>(pMap, keys[i]);
		REQUIRE(pValue != nullptr);
		REQUIRE(*pValue == values[i]);
	}

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap remove existing key", "[hashmap]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));

	int value = 100;
	hashMapInsert(pMap, "remove_me", value);
	REQUIRE(hashMapContains(pMap, "remove_me"));
	REQUIRE(hashMapCount(pMap) == 1);

	hashMapRemove(pMap, "remove_me");
	REQUIRE(hashMapCount(pMap) == 0);
	REQUIRE(!hashMapContains(pMap, "remove_me"));

	int* pValue = hashMapGet<int>(pMap, "remove_me");
	REQUIRE(pValue == nullptr);

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap remove non-existent key is safe", "[hashmap]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));

	hashMapRemove(pMap, "nonexistent");
	REQUIRE(hashMapCount(pMap) == 0);

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap replace existing key updates value", "[hashmap]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));

	hashMapInsert(pMap, "key", 42);
	REQUIRE(hashMapCount(pMap) == 1);

	int* pValue = hashMapGet<int>(pMap, "key");
	REQUIRE(*pValue == 42);

	hashMapInsert(pMap, "key", 100);
	REQUIRE(hashMapCount(pMap) == 1); // Count should not change

	pValue = hashMapGet<int>(pMap, "key");
	REQUIRE(*pValue == 100); // Value should be updated

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap with TextureHandle values", "[hashmap][handle]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(TextureHandle));

	TextureHandle handle1{handleMake(10, 0)};
	TextureHandle handle2{handleMake(20, 0)};

	hashMapInsert(pMap, "texture1.dds", handle1);
	hashMapInsert(pMap, "texture2.dds", handle2);

	REQUIRE(hashMapCount(pMap) == 2);

	TextureHandle* pH1 = hashMapGet<TextureHandle>(pMap, "texture1.dds");
	TextureHandle* pH2 = hashMapGet<TextureHandle>(pMap, "texture2.dds");

	REQUIRE(pH1 != nullptr);
	REQUIRE(pH2 != nullptr);
	REQUIRE(pH1->id == handle1.id);
	REQUIRE(pH2->id == handle2.id);

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap with MeshHandle values", "[hashmap][handle]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(MeshHandle));

	MeshHandle mesh1{handleMake(5, 1)};
	MeshHandle mesh2{handleMake(15, 2)};

	hashMapInsert(pMap, "cube.obj", mesh1);
	hashMapInsert(pMap, "sphere.obj", mesh2);

	REQUIRE(hashMapContains(pMap, "cube.obj"));
	REQUIRE(hashMapContains(pMap, "sphere.obj"));
	REQUIRE(!hashMapContains(pMap, "cone.obj"));

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap empty string key works", "[hashmap]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));

	int value = 999;
	hashMapInsert(pMap, "", value);

	REQUIRE(hashMapContains(pMap, ""));
	int* pValue = hashMapGet<int>(pMap, "");
	REQUIRE(pValue != nullptr);
	REQUIRE(*pValue == 999);

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap long key works", "[hashmap]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));

	const char* longKey = "this_is_a_very_long_key_name_that_should_still_work_correctly_in_the_hash_map";
	int value = 123;
	hashMapInsert(pMap, longKey, value);

	REQUIRE(hashMapContains(pMap, longKey));
	int* pValue = hashMapGet<int>(pMap, longKey);
	REQUIRE(*pValue == 123);

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap path-like keys work", "[hashmap]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(uint32_t));

	hashMapInsert<uint32_t>(pMap, "assets/textures/player.dds", 1);
	hashMapInsert<uint32_t>(pMap, "assets/textures/enemy.dds", 2);
	hashMapInsert<uint32_t>(pMap, "assets/meshes/cube.obj", 3);

	REQUIRE(hashMapCount(pMap) == 3);
	REQUIRE(hashMapContains(pMap, "assets/textures/player.dds"));
	REQUIRE(hashMapContains(pMap, "assets/meshes/cube.obj"));
	REQUIRE(!hashMapContains(pMap, "assets/textures/boss.dds"));

	uint32_t* pId = hashMapGet<uint32_t>(pMap, "assets/textures/enemy.dds");
	REQUIRE(*pId == 2);

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap similar keys are distinct", "[hashmap]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));

	hashMapInsert(pMap, "test", 1);
	hashMapInsert(pMap, "test1", 2);
	hashMapInsert(pMap, "test2", 3);
	hashMapInsert(pMap, "testing", 4);

	REQUIRE(hashMapCount(pMap) == 4);

	REQUIRE(*hashMapGet<int>(pMap, "test") == 1);
	REQUIRE(*hashMapGet<int>(pMap, "test1") == 2);
	REQUIRE(*hashMapGet<int>(pMap, "test2") == 3);
	REQUIRE(*hashMapGet<int>(pMap, "testing") == 4);

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap remove from multiple keys", "[hashmap]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));

	hashMapInsert(pMap, "a", 1);
	hashMapInsert(pMap, "b", 2);
	hashMapInsert(pMap, "c", 3);
	hashMapInsert(pMap, "d", 4);
	hashMapInsert(pMap, "e", 5);

	REQUIRE(hashMapCount(pMap) == 5);

	hashMapRemove(pMap, "c");
	REQUIRE(hashMapCount(pMap) == 4);
	REQUIRE(!hashMapContains(pMap, "c"));
	REQUIRE(hashMapContains(pMap, "a"));
	REQUIRE(hashMapContains(pMap, "e"));

	hashMapRemove(pMap, "a");
	hashMapRemove(pMap, "e");
	REQUIRE(hashMapCount(pMap) == 2);

	REQUIRE(hashMapContains(pMap, "b"));
	REQUIRE(hashMapContains(pMap, "d"));

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap stress test - many inserts", "[hashmap][stress]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));

	const int count = 100;
	char keyBuffer[64];

	for (int i = 0; i < count; i++)
	{
		snprintf(keyBuffer, sizeof(keyBuffer), "key_%d", i);
		hashMapInsert(pMap, keyBuffer, i * 10);
	}

	REQUIRE(hashMapCount(pMap) == count);

	for (int i = 0; i < count; i++)
	{
		snprintf(keyBuffer, sizeof(keyBuffer), "key_%d", i);
		REQUIRE(hashMapContains(pMap, keyBuffer));
		int* pValue = hashMapGet<int>(pMap, keyBuffer);
		REQUIRE(*pValue == i * 10);
	}

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap stress test - interleaved insert/remove", "[hashmap][stress]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));

	char keyBuffer[64];

	// Insert 50 items
	for (int i = 0; i < 50; i++)
	{
		snprintf(keyBuffer, sizeof(keyBuffer), "item_%d", i);
		hashMapInsert(pMap, keyBuffer, i);
	}

	REQUIRE(hashMapCount(pMap) == 50);

	// Remove every other item
	for (int i = 0; i < 50; i += 2)
	{
		snprintf(keyBuffer, sizeof(keyBuffer), "item_%d", i);
		hashMapRemove(pMap, keyBuffer);
	}

	REQUIRE(hashMapCount(pMap) == 25);

	// Verify remaining items
	for (int i = 1; i < 50; i += 2)
	{
		snprintf(keyBuffer, sizeof(keyBuffer), "item_%d", i);
		REQUIRE(hashMapContains(pMap, keyBuffer));
		int* pValue = hashMapGet<int>(pMap, keyBuffer);
		REQUIRE(*pValue == i);
	}

	// Verify removed items
	for (int i = 0; i < 50; i += 2)
	{
		snprintf(keyBuffer, sizeof(keyBuffer), "item_%d", i);
		REQUIRE(!hashMapContains(pMap, keyBuffer));
	}

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap destroy null map is safe", "[hashmap][edge]")
{
	hashMapDestroy(nullptr);
	REQUIRE(true);
}

TEST_CASE("HashMap with struct values", "[hashmap]")
{
	struct TestStruct
	{
		int id;
		float value;
		char name[16];
	};

	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(TestStruct));

	TestStruct s1 = {42, 3.14f, "first"};
	TestStruct s2 = {100, 2.71f, "second"};

	hashMapInsert(pMap, "struct1", s1);
	hashMapInsert(pMap, "struct2", s2);

	TestStruct* pS1 = hashMapGet<TestStruct>(pMap, "struct1");
	TestStruct* pS2 = hashMapGet<TestStruct>(pMap, "struct2");

	REQUIRE(pS1->id == 42);
	REQUIRE(pS1->value == 3.14f);
	REQUIRE(strcmp(pS1->name, "first") == 0);

	REQUIRE(pS2->id == 100);
	REQUIRE(pS2->value == 2.71f);
	REQUIRE(strcmp(pS2->name, "second") == 0);

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}

TEST_CASE("HashMap case sensitive keys", "[hashmap]")
{
	Arena* pArena = arenaCreate();
	HashMap* pMap = hashMapCreate(pArena, sizeof(int));

	hashMapInsert(pMap, "test", 1);
	hashMapInsert(pMap, "Test", 2);
	hashMapInsert(pMap, "TEST", 3);

	REQUIRE(hashMapCount(pMap) == 3);

	REQUIRE(*hashMapGet<int>(pMap, "test") == 1);
	REQUIRE(*hashMapGet<int>(pMap, "Test") == 2);
	REQUIRE(*hashMapGet<int>(pMap, "TEST") == 3);

	hashMapDestroy(pMap);
	arenaRelease(pArena);
}
