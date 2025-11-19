/*
 * AssetCache.h
 *
 * Centralized asset loading and caching system
 */

#ifndef _ASSETCACHE_H_
#define _ASSETCACHE_H_

#include "Core/Handle.h"
#include "Utilities/ThirdParty/OpenSource/bstrlib/bstrlib.h"
#include <stdint.h>

struct Arena;
struct Renderer;
struct SlotMap;
struct Texture;
struct Buffer;

/**
	Texture resource data.

	Stores texture metadata and texture pointer. Uses The
	Forge API.
*/
struct TextureData
{
	Texture* pTexture; ///< Texture pointer
	uint32_t width;	   ///< Texture width in pixels
	uint32_t height;   ///< Texture height in pixels
	uint32_t pathHash; ///< Hash of file path
	uint32_t refCount; ///< Reference count for automatic cleanup
};

/**
	Mesh resource data.

	Stores mesh buffers and metadata for rendering.
*/
struct MeshData
{
	Buffer* pVertexBuffer; ///< Vertex buffer
	Buffer* pIndexBuffer;  ///< Index buffer
	uint32_t vertexCount;  ///< Number of vertices
	uint32_t indexCount;   ///< Number of indices
	uint32_t vertexStride; ///< Size of each vertex in bytes
	uint32_t pathHash;	   ///< Hash of file path
	uint32_t refCount;	   ///< Reference count for automatic cleanup
};

/**
	Asset cache structure.

	Centralized system for loading, caching, and managing game assets. Uses
	bstring from The Forge for path handling and caching.

	@see createAssetCache
	@see loadTexture
	@see loadMesh
*/
struct AssetCache
{
	Arena* pArena;		 ///< Arena for allocations
	Renderer* pRenderer; ///< The renderer

	SlotMap* pTextures; ///< Texture storage (handle -> TextureData)
	SlotMap* pMeshes;	///< Mesh storage (handle -> MeshData)

	struct
	{
		bstring key;
		TextureHandle value;
	}* pTextureCache; ///< Path -> TextureHandle
	struct
	{
		bstring key;
		MeshHandle value;
	}* pMeshCache; ///< Path -> MeshHandle
};

///////////////////////////////////////////
// Lifecycle

/**
	Creates a new asset cache.

	Initializes the asset cache with storage for game resources.

	@param pArena Arena to allocate from
	@param pRenderer Renderer for resource creation

	@return Pointer to the created asset cache, or nullptr on failure

	@see shutdownAssetCache
*/
AssetCache* createAssetCache(Arena* pArena, Renderer* pRenderer);

/**
	Shuts down the asset cache and frees all resources.

	Destroys all loaded textures and meshes, and releases memory.

	@param pCache Asset cache to shutdown

	@see createAssetCache
*/
void shutdownAssetCache(AssetCache* pCache);

///////////////////////////////////////////
// Texture Loading

/**
	Loads a texture from a file path.

	Loads the texture if not already cached, or returns the cached handle.

	@param pCache Asset cache
	@param path File path to the texture (e.g., "assets/player.dds")

	@return Handle to the loaded texture, or HANDLE_INVALID_ID on failure

	@note Supports DDS

	Example:
	@code
	TextureHandle tex = loadTexture(pCache, "assets/player.dds");
	if (handleIsValid(tex.id)) {
		TextureData* pTexData = getTexture(pCache, tex);
	}
	@endcode

	@see getTexture
	@see unloadTexture
*/
TextureHandle loadTexture(AssetCache* pCache, const char* path);

/**
	Gets texture data from a handle.

	Retrieves the TextureData associated with the handle.

	@param pCache Asset cache
	@param handle Texture handle

	@return Pointer to TextureData, or nullptr if handle is invalid

	@see loadTexture
*/
TextureData* getTexture(AssetCache* pCache, TextureHandle handle);

/**
	Unloads a texture by handle.

	Frees the texture resources and invalidates the handle.

	@param pCache Asset cache
	@param handle Texture handle to unload

	@see loadTexture
*/
void unloadTexture(AssetCache* pCache, TextureHandle handle);

///////////////////////////////////////////
// Mesh Loading

/**
	Loads a mesh from a file path.

	Loads the mesh if not already cached, or returns the cached handle.

	@param pCache Asset cache
	@param path File path to the mesh (e.g., "assets/cube.obj")

	@return Handle to the loaded mesh, or HANDLE_INVALID_ID on failure

	@see getMesh
	@see unloadMesh
*/
MeshHandle loadMesh(AssetCache* pCache, const char* path);

/**
	Gets mesh data from a handle.

	Retrieves the MeshData associated with the handle.

	@param pCache Asset cache
	@param handle Mesh handle

	@return Pointer to MeshData, or nullptr if handle is invalid

	@see loadMesh
*/
MeshData* getMesh(AssetCache* pCache, MeshHandle handle);

/**
	Unloads a mesh by handle.

	Frees the mesh resources and invalidates the handle.

	@param pCache Asset cache
	@param handle Mesh handle to unload

	@note Handle becomes invalid after this call

	@see loadMesh
*/
void unloadMesh(AssetCache* pCache, MeshHandle handle);

///////////////////////////////////////////
// Procedural Generation

/**
	Creates a quad mesh with specified dimensions.

	Generates a procedural quad mesh centered at origin.

	@param pCache Asset cache
	@param width Quad width
	@param height Quad height

	@return Handle to the created mesh, or HANDLE_INVALID_ID on failure

	@see createCube
	@see createSphere
*/
MeshHandle createQuad(AssetCache* pCache, float width, float height);

/**
	Creates a cube mesh with specified size.

	Generates a procedural cube mesh centered at origin.

	@param pCache Asset cache
	@param size Cube size (length of each edge)

	@return Handle to the created mesh, or HANDLE_INVALID_ID on failure

	@see createQuad
	@see createSphere
*/
MeshHandle createCube(AssetCache* pCache, float size);

/**
	Creates a sphere mesh with specified radius and segments.

	Generates a procedural UV sphere mesh centered at origin.

	@param pCache Asset cache
	@param radius Sphere radius
	@param segments Number of longitude/latitude segments

	@return Handle to the created mesh, or HANDLE_INVALID_ID on failure

	@see createQuad
	@see createCube
*/
MeshHandle createSphere(AssetCache* pCache, float radius, uint32_t segments);

#endif // _ASSETCACHE_H_
