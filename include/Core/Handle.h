/*
 * Handle.h
 *
 * Handle wrappers for resource types.

 * Handles are 32-bit integers containing:
 *   - LSB 0-23  (24 bits): Slot index
 *   - MSB 24-31 (8 bits):  Generation
 *
 * Only 255 generations, but should be enough.
 */

#ifndef _HANDLE_H_
#define _HANDLE_H_

#include <stdint.h>

#define HANDLE_INDEX_BITS 24
#define HANDLE_GENERATION_BITS 8
#define HANDLE_INDEX_MASK 0x00FFFFFF
#define HANDLE_GENERATION_MASK 0xFF
#define HANDLE_INVALID_ID 0xFFFFFFFF

///////////////////////////////////////////
// Typed Handle Wrappers

/**
	Handle for texture resources.

	Type safe wrapper around a 32-bit handle ID.

	@see INVALID_TEXTURE_HANDLE
*/
struct TextureHandle
{
	uint32_t id;
};

/**
	Handle for mesh resources.

	Type safe wrapper around a 32-bit handle ID.
*/
struct MeshHandle
{
	uint32_t id;
};

/**
	Handle for Material resources.

	Type safe wrapper around a 32-bit handle ID.
*/
struct MaterialHandle
{
	uint32_t id;
};

/**
	Handle for shader resources.

	Type safe wrapper around a 32-bit handle ID.
*/
struct ShaderHandle
{
	uint32_t id;
};

///////////////////////////////////////////
// Handle Manipulation Functions

/**
	Extracts the slot index from a handle.

	Returns the lower 24 bits of the handle, where the index is.

	@param id Handle to extract index from

	@return Slot index

	@note Always check generation counter to detect stale handles

	Example:
	@code
	uint32_t handle = handleMake(42, 1);
	uint32_t index = handleIndex(handle);  // Returns 42
	@endcode

	@see handleGeneration
	@see handleMake
*/
inline uint32_t handleIndex(uint32_t id)
{
	return id & HANDLE_INDEX_MASK;
}

/**
	Extracts the generation counter from a handle.

	Returns the upper 8 bits of the handle, which represents the generation.

	@param id Handle to extract generation from

	@return Generation counter

	Example:
	@code
	uint32_t handle = handleMake(42, 7);
	uint32_t gen = handleGeneration(handle);  // Returns 7
	@endcode

	@see handleIndex
	@see handleMake
*/
inline uint32_t handleGeneration(uint32_t id)
{
	return (id >> HANDLE_INDEX_BITS) & HANDLE_GENERATION_MASK;
}

/**
	Creates a handle from an index and generation counter.

	Does some bit manipulation to pack the index and generation in
	a single 32 bit integer.

	@param index Slot index
	@param generation Generation counter

	@return Packed handle value

	Example:
	@code
	uint32_t handle = handleMake(1234, 5);
	assert(handleIndex(handle) == 1234);
	assert(handleGeneration(handle) == 5);
	@endcode

	@see handleIndex
	@see handleGeneration
*/
inline uint32_t handleMake(uint32_t index, uint32_t generation)
{
	return (index & HANDLE_INDEX_MASK) |
		   ((generation & HANDLE_GENERATION_MASK) << HANDLE_INDEX_BITS);
}

/**
	Checks if a handle has a valid format.

	Tests whether the handle is not equal to the invalid const value.
	Just a format check, does not verify resource existence.

	@param id Handle to check

	@return true if handle is not HANDLE_INVALID_ID, false otherwise

	@note This only checks the handle format, not resource existence, use
	slotMapIsValid() for that.

	Example:
	@code
	uint32_t handle = getEntityHandle();
	if (handleIsValid(handle)) {
		// Handle format is valid, but resource may still not exist
		if (slotMapIsValid(entityMap, handle)) {
			// Now we know the resource actually exists
		}
	}
	@endcode

	@see slotMapIsValid
	@see HANDLE_INVALID_ID
*/
inline bool handleIsValid(uint32_t id)
{
	return id != HANDLE_INVALID_ID;
}

///////////////////////////////////////////
// Invalid Handle Constants

/**
	Represents an invalid texture handle.

	Use to initialize texture handles that don't point
	to any resource, or to represent texture loading failures.

	Example:
	@code
	TextureHandle tex = INVALID_TEXTURE_HANDLE;
	if (fileExists("texture.png")) {
		tex.id = loadTexture("texture.png");
	}
	@endcode

	@see HANDLE_INVALID_ID
*/
static const TextureHandle INVALID_TEXTURE_HANDLE = {HANDLE_INVALID_ID};

/**
	Represents an invalid mesh handle.

	Use this constant to initialize mesh handles that don't point
	to any resource, or to represent mesh loading failures.

	Example:
	@code
	MeshHandle mesh = INVALID_MESH_HANDLE;
	if (fileExists("model.obj")) {
		mesh.id = loadMesh("model.obj");
	}
	@endcode

	@see HANDLE_INVALID_ID
*/
static const MeshHandle INVALID_MESH_HANDLE = {HANDLE_INVALID_ID};

/**
	Represents an invalid material handle.

	Use this constant to initialize material handles that don't point
	to any resource, or to represent material loading failures.

	Example:
	@code
	MaterialHandle mat = INVALID_MATERIAL_HANDLE;
	if (fileExists("material.mat")) {
		mat.id = loadMaterial("material.mat");
	}
	@endcode

	@see HANDLE_INVALID_ID
*/
static const MaterialHandle INVALID_MATERIAL_HANDLE = {HANDLE_INVALID_ID};

/**
	Represents an invalid shader handle.

	Use this constant to initialize shader handles that don't point
	to any resource, or to represent shader loading failures.

	Example:
	@code
	ShaderHandle shader = INVALID_SHADER_HANDLE;
	if (fileExists("shader.hlsl")) {
		shader.id = loadShader("shader.hlsl");
	}
	@endcode

	@see HANDLE_INVALID_ID
*/
static const ShaderHandle INVALID_SHADER_HANDLE = {HANDLE_INVALID_ID};

#endif // _HANDLE_H_
