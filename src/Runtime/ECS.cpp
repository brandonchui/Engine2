/*
 * ECS.cpp
 */

#include "Runtime/ECS.h"
#include "Utilities/Interfaces/ILog.h"

ECS_COMPONENT_DECLARE(MeshComponent);
ECS_COMPONENT_DECLARE(TransformComponent);
ECS_COMPONENT_DECLARE(MaterialComponent);
ECS_COMPONENT_DECLARE(RenderableTag);
ECS_COMPONENT_DECLARE(RenderContext);

// Transform system
// Updates world matrices from position/rotation/scale
void TransformSystem(ecs_iter_t* it)
{
	TransformComponent* transforms = ecs_field(it, TransformComponent, 0);

	for (int i = 0; i < it->count; i++)
	{
		if (transforms[i].dirty)
		{
			mat4 T = mat4::translation(transforms[i].position);
			mat4 R = mat4::rotationZYX(transforms[i].rotation);
			mat4 S = mat4::scale(transforms[i].scale);

			transforms[i].worldMatrix = T * R * S;
			transforms[i].dirty = false;
		}
	}
}

// Fill render data system
// Prepares data for GPU upload
void FillRenderDataSystem(ecs_iter_t* it)
{
	LOGF(LogLevel::eINFO, "FillRenderDataSystem called: %d entities, field_count=%d", it->count,
		 it->field_count);

	// 2 comp
	TransformComponent* transforms = ecs_field(it, TransformComponent, 0);
	MeshComponent* meshes = ecs_field(it, MeshComponent, 1);

	RenderContext* ctx = (RenderContext*)ecs_singleton_get(it->world, RenderContext);
	if (!ctx || !ctx->pRenderDataArray)
	{
		LOGF(LogLevel::eERROR, "FillRenderDataSystem: No render context! ctx=%p", ctx);
		return;
	}

	// Fill render data for each entity
	for (int i = 0; i < it->count; i++)
	{
		uint32_t renderIndex = ctx->renderDataCount;
		if (renderIndex >= ctx->maxRenderData)
		{
			LOGF(LogLevel::eWARNING, "FillRenderDataSystem: Render data buffer full!");
			break;
		}

		ctx->pRenderDataArray[renderIndex].modelMatrix = transforms[i].worldMatrix;
		ctx->pRenderDataArray[renderIndex].pVertexBuffer = meshes[i].pVertexBuffer;
		ctx->pRenderDataArray[renderIndex].vertexCount = meshes[i].vertexCount;
		ctx->pRenderDataArray[renderIndex].vertexStride = meshes[i].vertexStride;
		ctx->pRenderDataArray[renderIndex].descriptorSetIndex = meshes[i].descriptorSetIndex;
		ctx->renderDataCount++;

		LOGF(LogLevel::eINFO,
			 "FillRenderDataSystem: Added entity %d (descriptor %d), total count now %d", i,
			 meshes[i].descriptorSetIndex, ctx->renderDataCount);
	}
}

void initECS(ecs_world_t* world)
{
	ECS_COMPONENT_DEFINE(world, MeshComponent);
	ECS_COMPONENT_DEFINE(world, TransformComponent);
	ECS_COMPONENT_DEFINE(world, MaterialComponent);
	ECS_COMPONENT_DEFINE(world, RenderableTag);

	ECS_COMPONENT_DEFINE(world, RenderContext);

	ECS_SYSTEM(world, TransformSystem, EcsOnUpdate, TransformComponent);
	ECS_SYSTEM(world, FillRenderDataSystem, EcsPostUpdate, TransformComponent, MeshComponent);

	LOGF(LogLevel::eINFO, "ECS initialized!");
}

ecs_entity_t createMeshEntity(ecs_world_t* world, const MeshEntityDesc* pDesc)
{
	ecs_entity_t entity = ecs_new(world);

	MeshComponent mesh = {};
	mesh.pVertexBuffer = pDesc->pVertexBuffer;
	mesh.vertexCount = pDesc->vertexCount;
	mesh.vertexStride = pDesc->vertexStride;
	ecs_set(world, entity, MeshComponent, mesh);

	TransformComponent transform = {};
	transform.worldMatrix = mat4::identity();
	transform.position = pDesc->position;
	transform.rotation = pDesc->rotation;
	transform.scale = pDesc->scale;
	transform.dirty = true;
	ecs_set(world, entity, TransformComponent, transform);

	MaterialComponent material = {};
	material.pPipeline = pDesc->pPipeline;
	ecs_set(world, entity, MaterialComponent, material);

	RenderableTag tag = {};
	tag.visible = true;
	ecs_set(world, entity, RenderableTag, tag);

	LOGF(LogLevel::eINFO, "Created mesh entity %llu", entity);
	return entity;
}

void updateTransform(ecs_world_t* world, ecs_entity_t entity, const TransformDesc* pDesc)
{
	TransformComponent* transform = ecs_get_mut(world, entity, TransformComponent);
	if (transform)
	{
		transform->position = pDesc->position;
		transform->rotation = pDesc->rotation;
		transform->scale = pDesc->scale;
		transform->dirty = true;
	}
}