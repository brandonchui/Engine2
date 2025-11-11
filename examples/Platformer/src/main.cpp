#include "Runtime/EngineApp.h"
#include "Runtime/ECS.h"
#include "Utilities/Interfaces/ILog.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"

#include "OS/Input/InputCommon.h"

#include "../../../thirdparty/The-Forge/Common_3/Graphics/FSL/defaults.h"
#include "../../../Shaders/FSL/Global.srt.h"

#include "JoltHelpers.h"

struct Camera
{
	CameraMatrix projView;
};

struct Object
{
	mat4 worldMat;
};

class MyGame : public EngineApp
{
private:
	vec3 heroPosition = vec3(-2.0f, 0.0f, 0.0f);

	Buffer* pQuadBuffer = NULL;
	Buffer* pCubeBuffer = NULL;
	ecs_entity_t quadEntity = 0;
	ecs_entity_t cubeEntity = 0;
	Camera cameraData = {};

	Texture* pSpriteTexture = NULL;
	Texture* pCubeTexture = NULL;
	Sampler* pSpriteSampler = NULL;

	JPH::TempAllocatorImpl* pTempAllocator = nullptr;
	JPH::JobSystemThreadPool* pJobSystem = nullptr;
	JPH::PhysicsSystem* pPhysicsSystem = nullptr;
	JPH::BodyID floorBodyID;
	JPH::BodyID heroBodyID;
	JPH::BodyID cubeBodyID;

	BPLayerInterfaceImpl broad_phase_layer_interface;
	ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
	ObjectLayerPairFilterImpl object_vs_object_layer_filter;

public:
	bool Init() override
	{
		if (!EngineApp::Init())
			return false;

		JPH::RegisterDefaultAllocator();
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		pTempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

		pJobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
												  2);

		pPhysicsSystem = CreatePhysicsSystem(broad_phase_layer_interface,
											 object_vs_broadphase_layer_filter,
											 object_vs_object_layer_filter);

		floorBodyID = CreateFloor(pPhysicsSystem, 50.0f, 1.0f, 50.0f, -2.0f);

		heroBodyID = CreateDynamicBox(pPhysicsSystem, 0.5f, 0.5f, 0.5f, -2.0f, 5.0f, 0.0f);

		JPH::BoxShapeSettings cube_shape_settings(JPH::Vec3(1.0f, 1.0f, 1.0f));
		JPH::ShapeSettings::ShapeResult cube_shape_result = cube_shape_settings.Create();
		JPH::ShapeRefC cube_shape = cube_shape_result.Get();

		JPH::BodyCreationSettings cube_settings(cube_shape, JPH::RVec3(2.0_r, 0.0_r, 0.0_r),
												JPH::Quat::sIdentity(), JPH::EMotionType::Static,
												Layers::NON_MOVING);

		JPH::Body* cube = pPhysicsSystem->GetBodyInterface().CreateBody(cube_settings);
		pPhysicsSystem->GetBodyInterface().AddBody(cube->GetID(), JPH::EActivation::DontActivate);
		cubeBodyID = cube->GetID();

		SamplerDesc samplerDesc = {};
		samplerDesc.mMinFilter = FILTER_LINEAR;
		samplerDesc.mMagFilter = FILTER_LINEAR;
		samplerDesc.mMipMapMode = MIPMAP_MODE_NEAREST;
		samplerDesc.mAddressU = ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerDesc.mAddressV = ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerDesc.mAddressW = ADDRESS_MODE_CLAMP_TO_EDGE;
		addSampler(pRenderer, &samplerDesc, &pSpriteSampler);

		LOGF(LogLevel::eINFO, "Platformer initialized!");
		return true;
	}

	void Exit() override
	{
		if (quadEntity && pWorld)
		{
			ecs_delete(pWorld, quadEntity);
			quadEntity = 0;
		}
		if (cubeEntity && pWorld)
		{
			ecs_delete(pWorld, cubeEntity);
			cubeEntity = 0;
		}
		LOGF(LogLevel::eINFO, "Entities destroyed");

		if (pQuadBuffer)
		{
			removeResource(pQuadBuffer);
			pQuadBuffer = NULL;
		}
		if (pCubeBuffer)
		{
			removeResource(pCubeBuffer);
			pCubeBuffer = NULL;
		}
		LOGF(LogLevel::eINFO, "Mesh buffers destroyed");

		if (pSpriteTexture)
		{
			removeResource(pSpriteTexture);
			pSpriteTexture = NULL;
		}
		if (pCubeTexture)
		{
			removeResource(pCubeTexture);
			pCubeTexture = NULL;
		}
		if (pSpriteSampler)
		{
			removeSampler(pRenderer, pSpriteSampler);
			pSpriteSampler = NULL;
		}
		LOGF(LogLevel::eINFO, "Texture resources destroyed");

		LOGF(LogLevel::eINFO, "Platformer shutting down");

		if (pPhysicsSystem)
		{
			pPhysicsSystem->GetBodyInterface().RemoveBody(heroBodyID);
			pPhysicsSystem->GetBodyInterface().DestroyBody(heroBodyID);
			pPhysicsSystem->GetBodyInterface().RemoveBody(floorBodyID);
			pPhysicsSystem->GetBodyInterface().DestroyBody(floorBodyID);
			pPhysicsSystem->GetBodyInterface().RemoveBody(cubeBodyID);
			pPhysicsSystem->GetBodyInterface().DestroyBody(cubeBodyID);
			delete pPhysicsSystem;
			pPhysicsSystem = nullptr;
		}

		if (pJobSystem)
		{
			delete pJobSystem;
			pJobSystem = nullptr;
		}

		if (pTempAllocator)
		{
			delete pTempAllocator;
			pTempAllocator = nullptr;
		}

		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
		LOGF(LogLevel::eINFO, "Jolt shut down");

		EngineApp::Exit();
	}

	bool Load(ReloadDesc* pReloadDesc) override
	{
		if (!EngineApp::Load(pReloadDesc))
			return false;

		if (!pSpriteTexture)
		{
			TextureLoadDesc textureDesc = {};
			textureDesc.pFileName = "Sprite.tex";
			textureDesc.ppTexture = &pSpriteTexture;
			textureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
			addResource(&textureDesc, NULL);
		}

		if (!pCubeTexture)
		{
			TextureLoadDesc cubeTextureDesc = {};
			cubeTextureDesc.pFileName = "CubeTexture.tex";
			cubeTextureDesc.ppTexture = &pCubeTexture;
			cubeTextureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
			addResource(&cubeTextureDesc, NULL);
		}

		if (!pSpriteTexture || !pCubeTexture)
		{
			waitForAllResourceLoads();

			if (pSpriteTexture && pCubeTexture && pSpriteSampler)
			{
				DescriptorData params[3] = {};
				params[0].mIndex = SRT_RES_IDX(SrtData, Persistent, gSpriteTexture);
				params[0].ppTextures = &pSpriteTexture;
				params[1].mIndex = SRT_RES_IDX(SrtData, Persistent, gCubeTexture);
				params[1].ppTextures = &pCubeTexture;
				params[2].mIndex = SRT_RES_IDX(SrtData, Persistent, gSpriteSampler);
				params[2].ppSamplers = &pSpriteSampler;
				updateDescriptorSet(pRenderer, 0, pDescriptorSetPersistent, 3, params);
			}
		}

		if (!pCubeBuffer && pWorld)
		{
			struct Vertex
			{
				float position[3];
				float uv[2];
			};

			Vertex cubeVerts[36] = {// Front face (-Z)
									{{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},
									{{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f}},
									{{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f}},
									{{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f}},
									{{1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},
									{{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},

									// Back face (+Z)
									{{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}},
									{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
									{{-1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
									{{-1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
									{{-1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}},
									{{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}},

									// Left face (-X)
									{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},
									{{-1.0f, 1.0f, -1.0f}, {1.0f, 0.0f}},
									{{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
									{{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
									{{-1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}},
									{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},

									// Right face (+X)
									{{1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}},
									{{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
									{{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f}},
									{{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f}},
									{{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},
									{{1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}},

									// Bottom face (-Y)
									{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},
									{{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f}},
									{{1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}},
									{{1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}},
									{{-1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}},
									{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},

									// Top face (+Y)
									{{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
									{{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
									{{1.0f, 1.0f, -1.0f}, {1.0f, 1.0f}},
									{{1.0f, 1.0f, -1.0f}, {1.0f, 1.0f}},
									{{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f}},
									{{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}};

			pCubeBuffer = createMeshBuffer(cubeVerts, sizeof(cubeVerts));

			if (pCubeBuffer && pCubePipeline)
			{
				const uint32_t cubeVertexCount = 36;
				const uint32_t vertexStride = sizeof(Vertex);

				float windowWidth = (float)pSwapChain->ppRenderTargets[0]->mWidth;
				float windowHeight = (float)pSwapChain->ppRenderTargets[0]->mHeight;

				MeshEntityDesc cubeEntityDesc = {};
				cubeEntityDesc.pVertexBuffer = pCubeBuffer;
				cubeEntityDesc.vertexCount = cubeVertexCount;
				cubeEntityDesc.vertexStride = vertexStride;
				cubeEntityDesc.pPipeline = pCubePipeline;
				cubeEntityDesc.position = vec3(2.0f, 0.0f, 0.0f);
				cubeEntityDesc.rotation = vec3(0, 0, 0);
				cubeEntityDesc.scale = vec3(1.0f, 1.0f, 1.0f);

				cubeEntity = createMeshEntity(&cubeEntityDesc);
			}
		}

		if (!pQuadBuffer && pWorld)
		{
			struct Vertex
			{
				float position[3];
				float uv[2];
			};

			Vertex quadVerts[] = {
				// Triangle 1
				{{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
				{{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
				{{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
				// Triangle 2
				{{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
				{{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
				{{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
			};

			pQuadBuffer = createMeshBuffer(quadVerts, sizeof(quadVerts));

			if (pQuadBuffer && pPipeline)
			{
				const uint32_t quadVertexCount = 6;
				const uint32_t vertexStride = sizeof(Vertex);

				float windowWidth = (float)pSwapChain->ppRenderTargets[0]->mWidth;
				float windowHeight = (float)pSwapChain->ppRenderTargets[0]->mHeight;
				float quadSize = 500.0f;

				MeshEntityDesc entityDesc = {};
				entityDesc.pVertexBuffer = pQuadBuffer;
				entityDesc.vertexCount = quadVertexCount;
				entityDesc.vertexStride = vertexStride;
				entityDesc.pPipeline = pPipeline;
				entityDesc.position = vec3(-2.0f, 0.0f, 0.0f);
				entityDesc.rotation = vec3(0, 0, 0);
				entityDesc.scale = vec3(1.0f, 1.0f, 1.0f);

				quadEntity = createMeshEntity(&entityDesc);
			}
		}
		LOGF(LogLevel::eINFO, "Platformer assets loaded");

		return true;
	}

	void Unload(ReloadDesc* pReloadDesc) override { EngineApp::Unload(pReloadDesc); }

	void Update(float deltaTime) override
	{
		if (pPhysicsSystem)
		{
			const float cDeltaTime = deltaTime > 0.0f ? deltaTime : 1.0f / 60.0f;
			pPhysicsSystem->Update(cDeltaTime, 1, pTempAllocator, pJobSystem);

			JPH::RVec3 heroPhysicsPos =
				pPhysicsSystem->GetBodyInterface().GetCenterOfMassPosition(heroBodyID);
			heroPosition = vec3((float)heroPhysicsPos.GetX(), (float)heroPhysicsPos.GetY(),
								(float)heroPhysicsPos.GetZ());
		}

		float windowWidth = (float)pSwapChain->ppRenderTargets[0]->mWidth;
		float windowHeight = (float)pSwapChain->ppRenderTargets[0]->mHeight;

		if (pPhysicsSystem)
		{
			float moveSpeed = 5.0f;

			JPH::Vec3 currentVel = pPhysicsSystem->GetBodyInterface().GetLinearVelocity(heroBodyID);

			JPH::Vec3 newVel(0, currentVel.GetY(), 0);
			if (inputGetValue(0, K_W))
				newVel.SetZ(-moveSpeed);
			if (inputGetValue(0, K_S))
				newVel.SetZ(newVel.GetZ() + moveSpeed);
			if (inputGetValue(0, K_A))
				newVel.SetX(moveSpeed);
			if (inputGetValue(0, K_D))
				newVel.SetX(newVel.GetX() - moveSpeed);

			if (inputGetValue(0, K_SPACE))
			{
				if (fabs(currentVel.GetY()) < 0.5f)
				{
					float jumpVelocity = 7.0f;
					newVel.SetY(jumpVelocity);
				}
			}

			pPhysicsSystem->GetBodyInterface().SetLinearVelocity(heroBodyID, newVel);
		}

		TransformDesc quadTransform = {};
		quadTransform.position = heroPosition;
		quadTransform.rotation = vec3(0, 0, 0);
		quadTransform.scale = vec3(1.0f, 1.0f, 1.0f);
		updateTransform(quadEntity, &quadTransform);

		TransformDesc cubeTransform = {};
		cubeTransform.position = vec3(2.0f, 0.0f, 0.0f);
		cubeTransform.rotation = vec3(0.0f, 0.0f, 0.0f);
		cubeTransform.scale = vec3(1.0f, 1.0f, 1.0f);
		updateTransform(cubeEntity, &cubeTransform);

		EngineApp::Update(deltaTime);

		// Camera
		float aspect = windowWidth / windowHeight;

		float fovVerticalDegrees = 60.0f;
		float fovVerticalRadians = fovVerticalDegrees * (PI / 180.0f);

		float fovHorizontalRadians = 2.0f * atanf(tanf(fovVerticalRadians / 2.0f) * aspect);

		float aspectInverse = 1.0f / aspect;

		float nearPlane = 0.1f;
		float farPlane = 100.0f;

		CameraMatrix projection = CameraMatrix::perspectiveReverseZ(
			fovHorizontalRadians, aspectInverse, nearPlane, farPlane);

		TransformComponent* heroTransform = ecs_get_mut(pWorld, quadEntity, TransformComponent);
		if (heroTransform)
		{
			heroPosition = heroTransform->position;
		}

		vec3 cameraOffset = vec3(0.0f, 5.0f, 10.0f);
		Point3 eye = Point3(heroPosition.getX() + cameraOffset.getX(),
							heroPosition.getY() + cameraOffset.getY(),
							heroPosition.getZ() + cameraOffset.getZ());
		Point3 lookAt = Point3(heroPosition.getX(), heroPosition.getY(), heroPosition.getZ());
		vec3 up = vec3(0.0f, 1.0f, 0.0f);

		mat4 viewMat4 = mat4::lookAtLH(eye, lookAt, up);

		CameraMatrix view;
		view.mCamera = viewMat4;

		cameraData.projView = projection * view;
		uploadPerFrameData(&cameraData, sizeof(cameraData));

		uint32_t entityCount = getRenderDataCount();
		for (uint32_t i = 0; i < entityCount; ++i)
		{
			const MeshRenderData& renderData = pRenderDataArray[i];

			Object objectData = {};
			objectData.worldMat = renderData.modelMatrix;

			uploadPerObjectData(renderData.descriptorSetIndex, &objectData, sizeof(objectData));
		}
	}

	void Draw() override { EngineApp::Draw(); }

	const char* GetName() override { return "Platformer"; }
};

DEFINE_APPLICATION_MAIN(MyGame)
