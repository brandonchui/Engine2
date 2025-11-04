/*
 * EngineApp.h
 */

#ifndef _ENGINEAPP_H_
#define _ENGINEAPP_H_

#include "Runtime/RuntimeAPI.h"
#include "Application/Interfaces/IApp.h"
#include "Application/Interfaces/IProfiler.h"
#include "Application/Interfaces/IFont.h"
#include "Graphics/Interfaces/IGraphics.h"
#include "Utilities/RingBuffer.h"
#include "Game/ThirdParty/OpenSource/flecs/flecs.h"
#include "Runtime/ECS.h"

/**
	@class EngineApp

	Base application class for game development.

	EngineApp provides an interface for creating games by extending The Forge's
	IApp class. It manages the complete application lifecycle for many parts of
	the engine. Developers should extend this class and override lifecycle methods.

	@see
*/
class RUNTIME_API EngineApp : public IApp
{
public:
	/**
		Default constructor.

		Initializes all pointers to nullptr and sets up initial state.
	*/
	EngineApp();

	/**
		Virtual destructor.

		Ensures proper cleanup of derived classes (the game).
	*/
	virtual ~EngineApp();

	/**
		One time initialization called at application startup.

		Derived classes should call the base implementation before performing
		their own initialization so the engine subsystems are ready to use.

		@return True if initialization succeeded, false otherwise

		@see
	*/
	virtual bool Init() override;

	/**
		Cleanup and shutdown called at application termination.

		Destroys the ECS world, releases renderer resources, and performs final
		cleanups.

		@see
	*/
	virtual void Exit() override;

	/**
		Loads rendering resources when the window is created or resized.

		Once all initialization is complete, this function is called to load the
		swap chain, render targets, shaders, pipelines, and uniform buffers.

		@param pReloadDesc Descriptor containing reload information

		@return True if loading succeeded, false otherwise

		@see
	*/
	virtual bool Load(ReloadDesc* pReloadDesc) override;

	/**
		Unloads rendering resources before window resize or destruction.

		This function is called before window resize and before the
		application exits.

		@param pReloadDesc Descriptor containing reload information

		@see
	*/
	virtual void Unload(ReloadDesc* pReloadDesc) override;

	/**
		Updates game logic each frame.

		Processes input, updates game state, and advances ECS systems by calling
		ecs_progress(). Game specific updating logic should go here as well.

		@param deltaTime Time elapsed since last frame in seconds

		@note Called by The Forge every frame before Draw()

		@see
	*/
	virtual void Update(float deltaTime) override;

	/**
		Renders the current frame to the screen.

		All visuals are to be made in this virtual method.

		@see
	*/
	virtual void Draw() override;

	/**
		Returns the application name for window title and logging.

		@return Application name string

		@see
	*/
	virtual const char* GetName() override { return "EngineApp"; }

protected:
	Renderer* pRenderer;
	Queue* pGraphicsQueue;
	GpuCmdRing gGraphicsCmdRing;

	SwapChain* pSwapChain;
	Semaphore* pImageAcquiredSemaphore;

	Shader* pShader;
	Pipeline* pPipeline;

	ecs_world_t* pWorld;
	ecs_query_t* pRenderQuery;

	MeshRenderData* pRenderDataArray;
	uint32_t maxRenderDataCount;

	ProfileToken gGpuProfileToken;
	FontDrawDesc gFrameTimeDraw;
	uint32_t gFontID;

	uint32_t gFrameIndex;

	static const uint32_t gDataBufferCount = 2;

	Buffer* pUniformBufferPerFrame[gDataBufferCount];
	DescriptorSet* pDescriptorSetPerFrame;

	static const uint32_t MAX_OBJECTS = 100;
	Buffer* pUniformBufferPerObject[gDataBufferCount][MAX_OBJECTS];
	DescriptorSet* pDescriptorSetPerObject;
	uint32_t nextObjectBufferIndex;

	/**
		Uploads per frame uniform data to the GPU.

		Copies data to the per frame uniform buffer for the current frame. This
		buffer is shared by all objects and typically contains camera matrices,
		global lighting, and other scene wide params (for now anyway).

		@param pData Pointer to data to upload
		@param size Size of data in bytes

		@see
	*/
	void uploadPerFrameData(const void* pData, size_t size);

	/**
		Uploads per object uniform data to the GPU.

		Copies data to a specific object's uniform buffer for the current frame.

		@param objectIndex Index of the object buffer slot
		@param pData Pointer to data to upload
		@param size Size of data in bytes

		@see
	*/
	void uploadPerObjectData(uint32_t objectIndex, const void* pData, size_t size);

	/**
		Initializes the renderer and graphics queue.

		Creates the renderer instance, graphics queue, and command ring.

		@return True if initialization succeeded, false otherwise

		@note Called internally by Init()

		@see
	*/
	bool initRendererInternal();

	/**
		Destroys the renderer and releases graphics resources.

		This is called internally by Exit() and should not be called directly.

		@see
	*/
	void exitRendererInternal();

	/**
		Loads and compiles shaders from FSL source files.

		Loads vertex and fragment shaders from the Shaders/FSL directory. The
		shaders are compiled during the build process. This is called internally
		by Load().

		@note Called internally by Load()
		@note FSL files must be compiled by FSL compiler first

		@see
	*/
	void loadShaders();

	/**
		Unloads and releases shader resources.

		Destroys shader objects and releases GPU memory.

		@note Called internally by Unload()

		@see
	*/
	void unloadShaders();

	/**
		Creates the graphics pipeline with loaded shaders.

		Configures and creates the graphics pipeline state.
		This is called internally by Load().


		@see
	*/
	void createPipeline();

	/**
		Destroys the graphics pipeline.

		Releases the pipeline object and GPU resources.

		@note Called internally by Unload()

		@see
	*/
	void destroyPipeline();

	/**
		Creates a GPU vertex buffer from vertex data.

		Allocates a GPU buffer and uploads vertex data to it.

		@param pVertexData Pointer to vertex data array
		@param dataSize Total size of vertex data in bytes

		@return Pointer to the created buffer, or nullptr on failure

		@note The returned buffer must be manually destroyed when no longer needed

		@see
	*/
	Buffer* createMeshBuffer(const void* pVertexData, uint32_t dataSize);

	/**
		Creates a mesh entity with all required rendering components.

		Creates a new ECS entity with MeshComponent, TransformComponent,
		MaterialComponent, and RenderableTag. This is the primary way to add
		renderable objects to the scene. The function automatically allocates
		a descriptor set slot for the entity.

		Example:

		@code
		MeshEntityDesc desc = {};
		desc.pVertexBuffer = myVertexBuffer;
		desc.vertexCount = 6;
		desc.vertexStride = sizeof(Vertex);
		desc.pPipeline = pPipeline;
		desc.position = vec3(0.0f, 0.0f, 0.0f);
		desc.rotation = vec3(0.0f, 0.0f, 0.0f);
		desc.scale = vec3(1.0f, 1.0f, 1.0f);

		ecs_entity_t entity = createMeshEntity(&desc);
		@endcode

		@param pDesc Descriptor containing all mesh entity parameters

		@return Entity ID of the created mesh entity

		@see
	*/
	ecs_entity_t createMeshEntity(const MeshEntityDesc* pDesc);

	/**
		Updates an entity's transform component.

		Modifies the position, rotation, and scale of an existing entity and
		marks it as dirty to trigger matrix recalculation by TransformSystem.

		@param entity Entity ID of the entity to update
		@param pDesc Descriptor containing new transform parameters

		@see
	*/
	void updateTransform(ecs_entity_t entity, const TransformDesc* pDesc);

	/**
		Returns the number of entities prepared for rendering this frame.

		Queries the RenderContext to get the current count of entities in the
		render data array.

		@return Number of entities currently in the render data array

		@see
	*/
	uint32_t getRenderDataCount() const;

	/**
		Gets direct access to the ECS world from flecs.

		Returns a pointer to the underlying Flecs ECS world. This allows finer control
		if you know what to do.

		@return Pointer to the Flecs ECS world

		@warning Do not destroy or reinitialize the world manually

		@see
	*/
	ecs_world_t* getWorld() { return pWorld; }

private:
	/**
		Creates per frame uniform buffers for camera and scene data.

		Allocates double buffered uniform buffers for per frame data that is
		shared across all objects. Also creates the corresponding descriptor set.

		@see
	*/
	void createPerFrameUniformBuffer();

	/**
		Destroys per-frame uniform buffers and descriptor sets.

		Releases the per-frame uniform buffers and descriptor set.

		@note Called internally by Unload()

		@see
	*/
	void destroyPerFrameUniformBuffer();

	/**
		Creates perobject uniform buffers for transform data.

		Each object gets its own buffer for transform matrices and material
		parameters.

		@param count Number of object buffers to create (up to MAX_OBJECTS)

		@see
	*/
	void createPerObjectBuffers(uint32_t count);

	/**
		Creates descriptor sets for per object uniform buffers.

		Creates descriptor set instances for the specified number of objects and
		binding each to its corresponding uniform buffer. Watch for the MAX_OBJECTS

		@param count Number of descriptor sets to create

		@note Called internally by Load()

		@see
	*/
	void createPerObjectDescriptorSets(uint32_t count);

	/**
		Destroys per-object descriptor sets.

		Releases all per object descriptor sets.

		@note Called internally by Unload()

		@see
	*/
	void destroyPerObjectDescriptorSets();

public:
	/**
		Allocates a descriptor set slot for a new object.

		Returns the next available descriptor set index for per object uniform
		buffers.

		@return Index of the allocated descriptor set

		@see
	*/
	uint32_t allocateObjectBufferSlot();
};

#endif
