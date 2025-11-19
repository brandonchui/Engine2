/*
 * AssetCache.cpp
 *
 */

#include "Runtime/AssetCache.h"
#include "Runtime/Memory/Arena.h"
#include "Core/SlotMap.h"
#include "Core/Handle.h"
#include "Utilities/Math/BStringHashMap.h"
#include "Utilities/ThirdParty/OpenSource/Nothings/stb_ds.h"
#include "Utilities/Interfaces/ILog.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"
#include <math.h>

///////////////////////////////////////////
// Lifecycle

AssetCache* createAssetCache(Arena* pArena, Renderer* pRenderer)
{
	if (!pArena || !pRenderer)
		return nullptr;

	AssetCache* assetCache = arenaPushStruct<AssetCache>(pArena);
	assetCache->pArena = pArena;
	assetCache->pRenderer = pRenderer;

	// Asset cache slotmaps
	assetCache->pTextures = slotMapCreate(pArena, sizeof(TextureData), alignof(TextureData), 256);
	assetCache->pMeshes = slotMapCreate(pArena, sizeof(MeshData), alignof(MeshData), 256);

	assetCache->pTextureCache = NULL;
	assetCache->pMeshCache = NULL;

	return assetCache;
}

void shutdownAssetCache(AssetCache* pCache)
{
	if (!pCache)
		return;

	// Unload all textures
	if (pCache->pTextures)
	{
		uint32_t textureCount = slotMapCount(pCache->pTextures);
		TextureData* pTextureData = (TextureData*)pCache->pTextures->pValues;
		for (uint32_t i = 0; i < textureCount; ++i)
		{
			if (pTextureData[i].pTexture)
				removeResource(pTextureData[i].pTexture);
		}
	}

	// Unload all meshes
	if (pCache->pMeshes)
	{
		uint32_t meshCount = slotMapCount(pCache->pMeshes);
		MeshData* pMeshData = (MeshData*)pCache->pMeshes->pValues;
		for (uint32_t i = 0; i < meshCount; ++i)
		{
			if (pMeshData[i].pVertexBuffer)
				removeResource(pMeshData[i].pVertexBuffer);
			if (pMeshData[i].pIndexBuffer)
				removeResource(pMeshData[i].pIndexBuffer);
		}
	}

	// Free bstring hashmaps and their keys
	if (pCache->pTextureCache)
	{
		for (ptrdiff_t i = 0; i < bhlen(pCache->pTextureCache); ++i)
		{
			bdestroy(&pCache->pTextureCache[i].key);
		}
		bhfree(pCache->pTextureCache);
	}

	if (pCache->pMeshCache)
	{
		for (ptrdiff_t i = 0; i < bhlen(pCache->pMeshCache); ++i)
		{
			bdestroy(&pCache->pMeshCache[i].key);
		}
		bhfree(pCache->pMeshCache);
	}
}

///////////////////////////////////////////
// Texture Loading

TextureHandle loadTexture(AssetCache* pCache, const char* path)
{
	if (!pCache || !path)
		return TextureHandle{HANDLE_INVALID_ID};

	bstring pathStr = bconstfromcstr(path);

	// Check if already cached
	if (bhgeti_unsafe(pCache->pTextureCache, pathStr) != -1)
	{
		TextureHandle cachedHandle = bhget(pCache->pTextureCache, pathStr);
		TextureData* pCachedData = slotMapGet<TextureData>(pCache->pTextures, cachedHandle.id);
		if (pCachedData)
		{
			pCachedData->refCount++;
		}
		return cachedHandle;
	}

	Texture* pTexture = nullptr;
	TextureLoadDesc loadDesc = {};
	loadDesc.pFileName = path;
	loadDesc.ppTexture = &pTexture;
	addResource(&loadDesc, NULL);

	waitForAllResourceLoads();

	if (!pTexture)
		return TextureHandle{HANDLE_INVALID_ID};

	TextureData texData = {};
	texData.pTexture = pTexture;
	texData.width = pTexture->mWidth;
	texData.height = pTexture->mHeight;
	texData.pathHash = (uint32_t)stbds_hash_string(path, 0);
	texData.refCount = 1;
	uint32_t handleId = slotMapInsert(pCache->pTextures, texData);
	TextureHandle handle = {handleId};

	// Cache path -> handle mapping
	bstring keyCopy = bdynfromcstr(path);
	bhput(pCache->pTextureCache, keyCopy, handle);

	return handle;
}

TextureData* getTexture(AssetCache* pCache, TextureHandle handle)
{
	if (!pCache)
		return nullptr;

	return slotMapGet<TextureData>(pCache->pTextures, handle.id);
}

void unloadTexture(AssetCache* pCache, TextureHandle handle)
{
	if (!pCache)
		return;

	TextureData* pData = slotMapGet<TextureData>(pCache->pTextures, handle.id);
	if (!pData)
		return;

	if (pData->refCount == 0)
		return;

	pData->refCount--;
	if (pData->refCount > 0)
		return;

	removeResource(pData->pTexture);

	slotMapRemove(pCache->pTextures, handle.id);

	for (ptrdiff_t i = 0; i < bhlen(pCache->pTextureCache); ++i)
	{
		if (pCache->pTextureCache[i].value.id == handle.id)
		{
			bdestroy(&pCache->pTextureCache[i].key);
			bhdel(pCache->pTextureCache, pCache->pTextureCache[i].key);
			break;
		}
	}
}

///////////////////////////////////////////
// Mesh Loading

MeshHandle loadMesh(AssetCache* pCache, const char* path)
{
	if (!pCache || !path)
		return MeshHandle{HANDLE_INVALID_ID};

	bstring pathStr = bconstfromcstr(path);

	// Check if already cached
	if (bhgeti_unsafe(pCache->pMeshCache, pathStr) != -1)
	{
		MeshHandle cachedHandle = bhget(pCache->pMeshCache, pathStr);
		MeshData* pCachedData = slotMapGet<MeshData>(pCache->pMeshes, cachedHandle.id);
		if (pCachedData)
		{
			pCachedData->refCount++;
		}
		return cachedHandle;
	}

	// TODO: Load mesh file (OBJ, GLTF, etc.)
	Buffer* pVertexBuffer = nullptr;
	Buffer* pIndexBuffer = nullptr;
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;
	uint32_t vertexStride = 0;

	MeshData meshData = {};
	meshData.pVertexBuffer = pVertexBuffer;
	meshData.pIndexBuffer = pIndexBuffer;
	meshData.vertexCount = vertexCount;
	meshData.indexCount = indexCount;
	meshData.vertexStride = vertexStride;
	meshData.pathHash = (uint32_t)stbds_hash_string(path, 0);
	meshData.refCount = 1;
	uint32_t handleId = slotMapInsert(pCache->pMeshes, meshData);
	MeshHandle handle = {handleId};

	// Cache path -> handle mapping
	bstring keyCopy = bdynfromcstr(path);
	bhput(pCache->pMeshCache, keyCopy, handle);

	return handle;
}

MeshData* getMesh(AssetCache* pCache, MeshHandle handle)
{
	if (!pCache)
		return nullptr;

	return slotMapGet<MeshData>(pCache->pMeshes, handle.id);
}

void unloadMesh(AssetCache* pCache, MeshHandle handle)
{
	if (!pCache)
		return;

	MeshData* pData = slotMapGet<MeshData>(pCache->pMeshes, handle.id);
	if (!pData)
		return;

	if (pData->refCount == 0)
		return;

	pData->refCount--;
	if (pData->refCount > 0)
		return;

	removeResource(pData->pVertexBuffer);
	if (pData->pIndexBuffer)
		removeResource(pData->pIndexBuffer);

	slotMapRemove(pCache->pMeshes, handle.id);

	// Remove from bstring hashmap cache
	for (ptrdiff_t i = 0; i < bhlen(pCache->pMeshCache); ++i)
	{
		if (pCache->pMeshCache[i].value.id == handle.id)
		{
			bdestroy(&pCache->pMeshCache[i].key);
			bhdel(pCache->pMeshCache, pCache->pMeshCache[i].key);
			break;
		}
	}
}

///////////////////////////////////////////
// Procedural Generation

MeshHandle createQuad(AssetCache* pCache, float width, float height)
{
	if (!pCache)
		return MeshHandle{HANDLE_INVALID_ID};

	struct Vertex
	{
		float position[3];
		float uv[2];
	};

	float halfW = width * 0.5f;
	float halfH = height * 0.5f;
	Vertex vertices[4] = {{{-halfW, -halfH, 0.0f}, {0.0f, 1.0f}},
						  {{halfW, -halfH, 0.0f}, {1.0f, 1.0f}},
						  {{-halfW, halfH, 0.0f}, {0.0f, 0.0f}},
						  {{halfW, halfH, 0.0f}, {1.0f, 0.0f}}};

	uint16_t indices[6] = {0, 1, 2, 2, 1, 3};

	Buffer* pVertexBuffer = nullptr;
	BufferLoadDesc vbDesc = {};
	vbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	vbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	vbDesc.mDesc.mSize = sizeof(vertices);
	vbDesc.pData = vertices;
	vbDesc.ppBuffer = &pVertexBuffer;
	addResource(&vbDesc, NULL);

	Buffer* pIndexBuffer = nullptr;
	BufferLoadDesc ibDesc = {};
	ibDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
	ibDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	ibDesc.mDesc.mSize = sizeof(indices);
	ibDesc.pData = indices;
	ibDesc.ppBuffer = &pIndexBuffer;
	addResource(&ibDesc, NULL);

	waitForAllResourceLoads();

	if (!pVertexBuffer || !pIndexBuffer)
		return MeshHandle{HANDLE_INVALID_ID};

	MeshData meshData = {};
	meshData.pVertexBuffer = pVertexBuffer;
	meshData.pIndexBuffer = pIndexBuffer;
	meshData.vertexCount = 4;
	meshData.indexCount = 6;
	meshData.vertexStride = sizeof(Vertex);
	meshData.pathHash = 0;
	meshData.refCount = 1;
	uint32_t handleId = slotMapInsert(pCache->pMeshes, meshData);
	MeshHandle handle = {handleId};

	return handle;
}

MeshHandle createCube(AssetCache* pCache, float size)
{
	if (!pCache)
		return MeshHandle{HANDLE_INVALID_ID};

	struct Vertex
	{
		float position[3];
		float uv[2];
	};

	float s = size * 0.5f;
	Vertex vertices[24] = {{{-s, -s, -s}, {0.0f, 1.0f}}, {{-s, s, -s}, {0.0f, 0.0f}},
						   {{s, s, -s}, {1.0f, 0.0f}},	 {{s, -s, -s}, {1.0f, 1.0f}},

						   {{s, -s, s}, {0.0f, 1.0f}},	 {{s, s, s}, {0.0f, 0.0f}},
						   {{-s, s, s}, {1.0f, 0.0f}},	 {{-s, -s, s}, {1.0f, 1.0f}},

						   {{-s, -s, s}, {0.0f, 1.0f}},	 {{-s, s, s}, {0.0f, 0.0f}},
						   {{-s, s, -s}, {1.0f, 0.0f}},	 {{-s, -s, -s}, {1.0f, 1.0f}},

						   {{s, -s, -s}, {0.0f, 1.0f}},	 {{s, s, -s}, {0.0f, 0.0f}},
						   {{s, s, s}, {1.0f, 0.0f}},	 {{s, -s, s}, {1.0f, 1.0f}},

						   {{-s, -s, s}, {0.0f, 1.0f}},	 {{s, -s, s}, {1.0f, 1.0f}},
						   {{s, -s, -s}, {1.0f, 0.0f}},	 {{-s, -s, -s}, {0.0f, 0.0f}},

						   {{-s, s, -s}, {0.0f, 1.0f}},	 {{s, s, -s}, {1.0f, 1.0f}},
						   {{s, s, s}, {1.0f, 0.0f}},	 {{-s, s, s}, {0.0f, 0.0f}}};

	uint16_t indices[36] = {0,	1,	2,	2,	3,	0,	4,	5,	6,	6,	7,	4,	8,	9,	10, 10, 11, 8,
							12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20};

	Buffer* pVertexBuffer = nullptr;
	BufferLoadDesc vbDesc = {};
	vbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	vbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	vbDesc.mDesc.mSize = sizeof(vertices);
	vbDesc.pData = vertices;
	vbDesc.ppBuffer = &pVertexBuffer;
	addResource(&vbDesc, NULL);

	Buffer* pIndexBuffer = nullptr;
	BufferLoadDesc ibDesc = {};
	ibDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
	ibDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	ibDesc.mDesc.mSize = sizeof(indices);
	ibDesc.pData = indices;
	ibDesc.ppBuffer = &pIndexBuffer;
	addResource(&ibDesc, NULL);

	waitForAllResourceLoads();

	if (!pVertexBuffer || !pIndexBuffer)
		return MeshHandle{HANDLE_INVALID_ID};

	MeshData meshData = {};
	meshData.pVertexBuffer = pVertexBuffer;
	meshData.pIndexBuffer = pIndexBuffer;
	meshData.vertexCount = 24;
	meshData.indexCount = 36;
	meshData.vertexStride = sizeof(Vertex);
	meshData.pathHash = 0;
	meshData.refCount = 1;
	uint32_t handleId = slotMapInsert(pCache->pMeshes, meshData);
	MeshHandle handle = {handleId};

	return handle;
}

MeshHandle createSphere(AssetCache* pCache, float radius, uint32_t segments)
{
	// Referencing the Frank Luna D3D12 book
	if (!pCache)
		return MeshHandle{HANDLE_INVALID_ID};

	struct Vertex
	{
		float position[3];
		float uv[2];
	};

	const uint32_t latitudes = segments;
	const uint32_t longitudes = segments;
	const uint32_t vertexCount = (latitudes + 1) * (longitudes + 1);
	const uint32_t indexCount = latitudes * longitudes * 6;

	Vertex* vertices = arenaPushArray<Vertex>(pCache->pArena, vertexCount);
	uint16_t* indices = arenaPushArray<uint16_t>(pCache->pArena, indexCount);

	const float pi = 3.14159265359f;
	uint32_t vertIdx = 0;
	for (uint32_t lat = 0; lat <= latitudes; ++lat)
	{
		float theta = (float)lat / (float)latitudes * pi;
		float sinTheta = sinf(theta);
		float cosTheta = cosf(theta);

		for (uint32_t lon = 0; lon <= longitudes; ++lon)
		{
			float phi = (float)lon / (float)longitudes * 2.0f * pi;
			float sinPhi = sinf(phi);
			float cosPhi = cosf(phi);

			// Spherical -> Cartesian
			vertices[vertIdx].position[0] = radius * sinTheta * cosPhi; // x
			vertices[vertIdx].position[1] = radius * cosTheta;			// y
			vertices[vertIdx].position[2] = radius * sinTheta * sinPhi; // z

			// UV
			vertices[vertIdx].uv[0] = (float)lon / (float)longitudes;
			vertices[vertIdx].uv[1] = (float)lat / (float)latitudes;

			++vertIdx;
		}
	}

	// Generate indices
	uint32_t idxIdx = 0;
	for (uint32_t lat = 0; lat < latitudes; ++lat)
	{
		for (uint32_t lon = 0; lon < longitudes; ++lon)
		{
			uint16_t current = (uint16_t)(lat * (longitudes + 1) + lon);
			uint16_t next = (uint16_t)(current + longitudes + 1);

			// First triangle
			indices[idxIdx++] = current;
			indices[idxIdx++] = next;
			indices[idxIdx++] = current + 1;

			// Second triangle
			indices[idxIdx++] = current + 1;
			indices[idxIdx++] = next;
			indices[idxIdx++] = next + 1;
		}
	}

	Buffer* pVertexBuffer = nullptr;
	BufferLoadDesc vbDesc = {};
	vbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	vbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	vbDesc.mDesc.mSize = vertexCount * sizeof(Vertex);
	vbDesc.pData = vertices;
	vbDesc.ppBuffer = &pVertexBuffer;
	addResource(&vbDesc, NULL);

	Buffer* pIndexBuffer = nullptr;
	BufferLoadDesc ibDesc = {};
	ibDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
	ibDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	ibDesc.mDesc.mSize = indexCount * sizeof(uint16_t);
	ibDesc.pData = indices;
	ibDesc.ppBuffer = &pIndexBuffer;
	addResource(&ibDesc, NULL);

	waitForAllResourceLoads();
	if (!pVertexBuffer || !pIndexBuffer)
		return MeshHandle{HANDLE_INVALID_ID};

	MeshData meshData = {};
	meshData.pVertexBuffer = pVertexBuffer;
	meshData.pIndexBuffer = pIndexBuffer;
	meshData.vertexCount = vertexCount;
	meshData.indexCount = indexCount;
	meshData.vertexStride = sizeof(Vertex);
	meshData.pathHash = 0;
	meshData.refCount = 1;
	uint32_t handleId = slotMapInsert(pCache->pMeshes, meshData);
	MeshHandle handle = {handleId};

	return handle;
}
