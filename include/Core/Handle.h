/*
 * Handle.h
 *
 * Generational handle system for type-safe resource references.
 *
 * Handles are 32-bit opaque values containing:
 *   - Bits 0-23  (24 bits): Slot index (16,777,216 possible slots)
 *   - Bits 24-31 (8 bits):  Generation counter (256 reuses per slot)
 *
 * The generation counter provides automatic detection of use-after-free
 * errors. Each time a slot is reused, its generation increments, invalidating
 * all old handles pointing to that slot.
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
	Type-safe handle for texture resources.

	Wraps a raw handle ID to provide compile-time type safety and prevent
	accidentally using a texture handle as a mesh handle, etc.

	@see INVALID_TEXTURE_HANDLE
*/
struct TextureHandle { uint32_t id; };

/**
	Type-safe handle for mesh resources.

	@see INVALID_MESH_HANDLE
*/
struct MeshHandle { uint32_t id; };

/**
	Type-safe handle for material resources.

	@see INVALID_MATERIAL_HANDLE
*/
struct MaterialHandle { uint32_t id; };

/**
	Type-safe handle for shader resources.

	@see INVALID_SHADER_HANDLE
*/
struct ShaderHandle { uint32_t id; };

///////////////////////////////////////////
// Handle Manipulation Functions

/**
	Extracts the slot index from a handle.

	Returns the lower 24 bits of the handle, which represent the slot index
	in the underlying storage (e.g., SlotMap sparse array).

	@param id Handle to extract index from

	@return Slot index (0 to 16,777,215)

	@note The index alone is not sufficient to validate a handle
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

	Returns the upper 8 bits of the handle, which represent how many times
	the slot has been reused. Increments each time the slot is freed.

	@param id Handle to extract generation from

	@return Generation counter (0 to 255)

	@note Generation wraps to 0 after 255, potentially causing ABA issues
	@note This is acceptable in practice as 256 reuses per slot is rare

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

	Packs the 24-bit index and 8-bit generation into a single 32-bit handle.
	This is the canonical way to construct handles.

	@param index Slot index (0 to 16,777,215, upper bits ignored)
	@param generation Generation counter (0 to 255, upper bits ignored)

	@return Packed handle value

	@note Values exceeding bit limits are automatically masked
	@note Index and generation can be extracted with handleIndex/handleGeneration

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
	return (index & HANDLE_INDEX_MASK) | ((generation & HANDLE_GENERATION_MASK) << HANDLE_INDEX_BITS);
}

/**
	Checks if a handle has a valid format.

	Tests whether the handle is not equal to the invalid sentinel value.
	This is a fast format check only - does not validate that the handle
	points to an existing resource.

	@param id Handle to check

	@return true if handle is not HANDLE_INVALID_ID, false otherwise

	@note This only checks the handle format, not resource existence
	@note Use slotMapIsValid() to fully validate against actual storage

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
	Sentinel value representing an invalid texture handle.

	Use this constant to initialize texture handles that don't point
	to any resource, or to represent texture loading failures.

	Example:
	@code
	TextureHandle tex = INVALID_TEXTURE_HANDLE;
	if (fileExists("texture.png")) {
		tex.id = loadTexture("texture.png");
	}
	@endcode
*/
static const TextureHandle INVALID_TEXTURE_HANDLE = { HANDLE_INVALID_ID };

/**
	Sentinel value representing an invalid mesh handle.

	@see INVALID_TEXTURE_HANDLE
*/
static const MeshHandle INVALID_MESH_HANDLE = { HANDLE_INVALID_ID };

/**
	Sentinel value representing an invalid material handle.

	@see INVALID_TEXTURE_HANDLE
*/
static const MaterialHandle INVALID_MATERIAL_HANDLE = { HANDLE_INVALID_ID };

/**
	Sentinel value representing an invalid shader handle.

	@see INVALID_TEXTURE_HANDLE
*/
static const ShaderHandle INVALID_SHADER_HANDLE = { HANDLE_INVALID_ID };

#endif // _HANDLE_H_
