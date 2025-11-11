/*
 * ECS.h
 */

#ifndef _ENGINE_ECS_H_
#define _ENGINE_ECS_H_

#include "Runtime/RuntimeAPI.h"
#include "Game/ThirdParty/OpenSource/flecs/flecs.h"
#include "Graphics/Interfaces/IGraphics.h"
#include "Utilities/Math/MathTypes.h"

struct MeshComponent;
struct TransformComponent;
struct MaterialComponent;
struct RenderableTag;
struct RenderContext;

extern ECS_COMPONENT_DECLARE(MeshComponent);
extern ECS_COMPONENT_DECLARE(TransformComponent);
extern ECS_COMPONENT_DECLARE(MaterialComponent);
extern ECS_COMPONENT_DECLARE(RenderableTag);
extern ECS_COMPONENT_DECLARE(RenderContext);

/**
	@struct MeshComponent

	Stores GPU mesh data for renderable entities.

	Contains mesh metadata required to render.
	The descriptorSetIndex maps to a per object uniform buffer slot.

	@note Index buffer not implemented yet

	@see
*/
struct MeshComponent
{
	Buffer* pVertexBuffer;
	uint32_t vertexCount;
	uint32_t vertexStride;
	uint32_t descriptorSetIndex;
};

/**
	@struct TransformComponent

	Defines transformation data for entities.

	This component stores both transformation, usually for some entity.

	@see
*/
struct TransformComponent
{
	mat4 worldMatrix;
	vec3 position;
	vec3 rotation;
	vec3 scale;
	bool dirty;
};

/**
	@struct MaterialComponent

	Specifies which graphics pipeline to use when rendering an entity.

	Defined the pipeline to use for rendering the entity per MaterialComponent.
	Good for reusing the same pipeline across multiple entities.

	@see
*/
struct MaterialComponent
{
	Pipeline* pPipeline;
};

/**
	@struct RenderableTag

	Controls whether an entity is included in rendering.

	This tag component marks entities as renderable. Just a simple flecs tag.

	@see
*/
struct RenderableTag
{
	bool visible;
};

/**
	@struct MeshRenderData

	CPU side rendering data extracted from ECS components per frame.

	This structure contains all information needed to issue a draw call for
	a single mesh entity. The FillRenderDataSystem populates an array of these
	structures during the PostUpdate phase, which is then consumed during the
	Draw phase. Decouples ECS from rendering logic.

	@warning Need to profile and optimize later if necessary

	@see
*/
struct MeshRenderData
{
	mat4 modelMatrix;
	Buffer* pVertexBuffer;
	uint32_t vertexCount;
	uint32_t vertexStride;
	uint32_t descriptorSetIndex;
	Pipeline* pPipeline;
};

/**
	@struct RenderContext

	Singleton ECS component providing shared rendering state and buffers.

	Provides shared access to rendering resources and the render data array.
	Systems use this to coordinate rendering operations and share state between
	update and draw	phases. The pRenderDataArray is populated by FillRenderDataSystem
	and consumed by EngineApp::Draw().

	@warning renderDataCount is reset to 0 each frame by FillRenderDataSystem

	@see
*/
struct RenderContext
{
	Cmd* pCmd;
	RenderTarget* pRenderTarget;
	uint32_t frameIndex;

	MeshRenderData* pRenderDataArray;
	uint32_t renderDataCount;
	uint32_t maxRenderData;
};

/**
	Transform system that updates world matrices from local transform data.

	This system runs during the EcsOnUpdate phase and processes all entities
	with TransformComponent. When the dirty flag is true, it rebuilds the
	matrices.

	@param it Flecs iterator containing entities with TransformComponent

	@see
*/
void TransformSystem(ecs_iter_t* it);

/**
	Render data collection system that prepares CPU-side render information.

	This system processes all entities with both TransformComponent and
	MeshComponent. It copies data from components into the RenderContext's
	pRenderDataArray, which is later consumed by EngineApp::Draw().

	@param it Flecs iterator containing entities with Transform and Mesh components

	@warning Temporary limit of 1000 entities

	@see
*/
void FillRenderDataSystem(ecs_iter_t* it);

/**
	@struct MeshEntityDesc

	Descriptor for creating mesh entities.

	A descriptor containing all parameters needed to initialize the createMeshEntity()

	@see
*/
struct MeshEntityDesc
{
	Buffer* pVertexBuffer;
	uint32_t vertexCount;
	uint32_t vertexStride;
	Pipeline* pPipeline;
	vec3 position;
	vec3 rotation;
	vec3 scale;
};

/**
	@struct TransformDesc

	Descriptor for updating entity transform parameters.

	Just a clear structure to pass position, rotation, and scale for
	updating transforms.


	@see
*/
struct TransformDesc
{
	vec3 position;
	vec3 rotation;
	vec3 scale;
};

/**
	Initializes the ECS world with rendering components and systems.

	Registers all ECS components and systems with the Flecs world. It also
	creates the singleton RenderContext and allocates the render data array.

	@param world Pointer to the Flecs ECS world to initialize

	@see
*/
RUNTIME_API void initECS(ecs_world_t* world);

/**
	Creates a mesh entity with all required rendering components.

	An internal function that will create ECS entities quickly and inits
	it with the needed components.

	@param world Pointer to the Flecs ECS world
	@param pDesc Descriptor containing all mesh entity parameters

	@return Entity ID of the created mesh entity

	@note Game code should use EngineApp::createMeshEntity() instead

	@see
*/
RUNTIME_API ecs_entity_t createMeshEntity(ecs_world_t* world, const MeshEntityDesc* pDesc);

/**
	Updates an entity's transform component with new position, rotation, and scale.

	An internal function that will update the transform component of an entity.

	@param world Pointer to the Flecs ECS world
	@param entity Entity ID of the entity to update
	@param pDesc Descriptor containing new transform parameters

	@note Game code should use EngineApp::updateTransform() instead

	@see
*/
RUNTIME_API void updateTransform(ecs_world_t* world, ecs_entity_t entity,
								 const TransformDesc* pDesc);

#endif