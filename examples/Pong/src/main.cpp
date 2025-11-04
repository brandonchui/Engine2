#include "Runtime/EngineApp.h"
#include "Runtime/ECS.h"
#include "Utilities/Interfaces/ILog.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"
#include "OS/Input/InputCommon.h"
#include "Utilities/Math/Random.h"

#include "../../../thirdparty/The-Forge/Common_3/Graphics/FSL/defaults.h"
#include "../../../Shaders/FSL/Global.srt.h"

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
	Buffer* pQuadBuffer = NULL;

	ecs_entity_t topWallEntity = 0;
	ecs_entity_t bottomWallEntity = 0;
	ecs_entity_t rightWallEntity = 0;
	ecs_entity_t paddleEntity = 0;
	ecs_entity_t ballEntity = 0;

	Camera cameraData = {};

	float paddleOffsetVertical = 0.0f;
	vec2 ballPosition = vec2(0, 0);
	vec2 ballVelocity = vec2(0, 0);

public:
	bool Init() override
	{
		if (!EngineApp::Init())
			return false;

		LOGF(LogLevel::eINFO, "Game initialized!");
		return true;
	}

	void Exit() override
	{
		if (topWallEntity && pWorld)
		{
			ecs_delete(pWorld, topWallEntity);
			topWallEntity = 0;
		}
		if (bottomWallEntity && pWorld)
		{
			ecs_delete(pWorld, bottomWallEntity);
			bottomWallEntity = 0;
		}
		if (rightWallEntity && pWorld)
		{
			ecs_delete(pWorld, rightWallEntity);
			rightWallEntity = 0;
		}
		if (paddleEntity && pWorld)
		{
			ecs_delete(pWorld, paddleEntity);
			paddleEntity = 0;
		}
		if (ballEntity && pWorld)
		{
			ecs_delete(pWorld, ballEntity);
			ballEntity = 0;
		}
		LOGF(LogLevel::eINFO, "Entities destroyed");

		if (pQuadBuffer)
		{
			removeResource(pQuadBuffer);
			pQuadBuffer = NULL;
		}
		LOGF(LogLevel::eINFO, "Mesh buffers destroyed");

		LOGF(LogLevel::eINFO, "Game shutting down");
		EngineApp::Exit();
	}

	bool Load(ReloadDesc* pReloadDesc) override
	{
		if (!EngineApp::Load(pReloadDesc))
			return false;

		if (!pQuadBuffer && pWorld)
		{
			struct Vertex
			{
				float position[3];
				float color[3];
			};

			float halfSize = 0.2f;
			Vertex quadVerts[] = {
				// Tri 1
				{{-halfSize, -halfSize, 0.0f}, {1.0f, 1.0f, 1.0f}},
				{{halfSize, -halfSize, 0.0f}, {1.0f, 1.0f, 1.0f}},
				{{-halfSize, halfSize, 0.0f}, {1.0f, 1.0f, 1.0f}},
				// Tri 2
				{{-halfSize, halfSize, 0.0f}, {1.0f, 1.0f, 1.0f}},
				{{halfSize, -halfSize, 0.0f}, {1.0f, 1.0f, 1.0f}},
				{{halfSize, halfSize, 0.0f}, {1.0f, 1.0f, 1.0f}},
			};

			pQuadBuffer = createMeshBuffer(quadVerts, sizeof(quadVerts));

			// Create entities with mesh components that all share the same quad mesh
			if (pQuadBuffer && pPipeline)
			{
				const uint32_t quadVertexCount = 6;
				const uint32_t vertexStride = sizeof(Vertex);

				MeshEntityDesc entityDesc = {};
				entityDesc.pVertexBuffer = pQuadBuffer;
				entityDesc.vertexCount = quadVertexCount;
				entityDesc.vertexStride = vertexStride;
				entityDesc.pPipeline = pPipeline;
				entityDesc.position = vec3(0, 0, 0);
				entityDesc.rotation = vec3(0, 0, 0);
				entityDesc.scale = vec3(1, 1, 1);

				topWallEntity = createMeshEntity(&entityDesc);
				bottomWallEntity = createMeshEntity(&entityDesc);
				rightWallEntity = createMeshEntity(&entityDesc);
				paddleEntity = createMeshEntity(&entityDesc);
				ballEntity = createMeshEntity(&entityDesc);

				// Spawn ball in middle of screen
				ballPosition = vec2((float)pSwapChain->ppRenderTargets[0]->mWidth / 2.0f,
									(float)pSwapChain->ppRenderTargets[0]->mHeight / 2.0f);
				ballVelocity = vec2(300.0f, 200.0f);
			}
		}

		LOGF(LogLevel::eINFO, "Game assets loaded");
		return true;
	}

	void Unload(ReloadDesc* pReloadDesc) override { EngineApp::Unload(pReloadDesc); }

	void Update(float deltaTime) override
	{
		float windowWidth = (float)pSwapChain->ppRenderTargets[0]->mWidth;
		float windowHeight = (float)pSwapChain->ppRenderTargets[0]->mHeight;

		const float wallThickness = windowHeight * 0.05f;

		// Top wall
		TransformDesc topWallTransform = {};
		topWallTransform.position = vec3(windowWidth / 2.0f, wallThickness / 2.0f, 0);
		topWallTransform.rotation = vec3(0, 0, 0);
		topWallTransform.scale = vec3(windowWidth / 0.4f, wallThickness / 0.4f, 1.0f);
		updateTransform(topWallEntity, &topWallTransform);

		// Bottom wall
		TransformDesc bottomWallTransform = {};
		bottomWallTransform.position = vec3(windowWidth / 2.0f,
											windowHeight - (wallThickness / 2.0f), 0);
		bottomWallTransform.rotation = vec3(0, 0, 0);
		bottomWallTransform.scale = vec3(windowWidth / 0.4f, wallThickness / 0.4f, 1.0f);
		updateTransform(bottomWallEntity, &bottomWallTransform);

		// Right wall
		float rightWallHeight = windowHeight - (2.0f * wallThickness);
		TransformDesc rightWallTransform = {};
		rightWallTransform.position = vec3(windowWidth - (wallThickness / 2.0f),
										   windowHeight / 2.0f, 0);
		rightWallTransform.rotation = vec3(0, 0, 0);
		rightWallTransform.scale = vec3(wallThickness / 0.4f, rightWallHeight / 0.4f, 1.0f);
		updateTransform(rightWallEntity, &rightWallTransform);

		// Movement
		float paddleSpeed = 300.0f;

		if (inputGetValue(0, K_W) || inputGetValue(0, K_UPARROW))
		{
			paddleOffsetVertical += paddleSpeed * deltaTime;
		}
		if (inputGetValue(0, K_S) || inputGetValue(0, K_DOWNARROW))
		{
			paddleOffsetVertical -= paddleSpeed * deltaTime;
		}

		// Paddle dimensions
		float paddleWidth = 20.0f;
		float paddleHeight = 100.0f;
		float paddleOffsetFromEdge = 50.0f;

		// Clamping to the wall offsets so doesn't go off screen
		float playAreaHeight = windowHeight - (2.0f * wallThickness);
		float maxOffset = (playAreaHeight / 2.0f) - (paddleHeight / 2.0f);
		if (paddleOffsetVertical > maxOffset)
			paddleOffsetVertical = maxOffset;
		if (paddleOffsetVertical < -maxOffset)
			paddleOffsetVertical = -maxOffset;

		// Position paddle in the center of the play area (between walls)
		float playAreaCenterY = wallThickness + (playAreaHeight / 2.0f);
		TransformDesc paddleTransform = {};
		paddleTransform.position = vec3(paddleOffsetFromEdge,
										playAreaCenterY - paddleOffsetVertical, 0);
		paddleTransform.rotation = vec3(0, 0, 0);
		paddleTransform.scale = vec3(paddleWidth / 0.4f, paddleHeight / 0.4f, 1.0f);
		updateTransform(paddleEntity, &paddleTransform);

		// BALL PHYSICS
		const float ballSize = 15.0f;

		// Update ball position
		ballPosition.setX(ballPosition.getX() + ballVelocity.getX() * deltaTime);
		ballPosition.setY(ballPosition.getY() + ballVelocity.getY() * deltaTime);

		// Ball collision with top/bottom walls
		if (ballPosition.getY() - (ballSize / 2.0f) < wallThickness)
		{
			ballPosition.setY(wallThickness + (ballSize / 2.0f));
			ballVelocity.setY(-ballVelocity.getY());
		}
		if (ballPosition.getY() + (ballSize / 2.0f) > windowHeight - wallThickness)
		{
			ballPosition.setY(windowHeight - wallThickness - (ballSize / 2.0f));
			ballVelocity.setY(-ballVelocity.getY());
		}

		// Ball collision with right wall
		if (ballPosition.getX() + (ballSize / 2.0f) > windowWidth - wallThickness)
		{
			ballPosition.setX(windowWidth - wallThickness - (ballSize / 2.0f));
			ballVelocity.setX(-ballVelocity.getX());
		}

		// AABB
		float paddleLeft = paddleOffsetFromEdge - (paddleWidth / 2.0f);
		float paddleRight = paddleOffsetFromEdge + (paddleWidth / 2.0f);
		float paddleTop = playAreaCenterY - paddleOffsetVertical - (paddleHeight / 2.0f);
		float paddleBottom = playAreaCenterY - paddleOffsetVertical + (paddleHeight / 2.0f);

		float ballLeft = ballPosition.getX() - (ballSize / 2.0f);
		float ballRight = ballPosition.getX() + (ballSize / 2.0f);
		float ballTop = ballPosition.getY() - (ballSize / 2.0f);
		float ballBottom = ballPosition.getY() + (ballSize / 2.0f);

		// Check if ball overlaps with paddle
		if (ballRight > paddleLeft && ballLeft < paddleRight && ballBottom > paddleTop &&
			ballTop < paddleBottom)
		{
			// Bounce ball back
			ballPosition.setX(paddleRight + (ballSize / 2.0f));
			ballVelocity.setX(-ballVelocity.getX());

			// Offset based on where it hit the paddle
			float hitOffset = (ballPosition.getY() - (playAreaCenterY - paddleOffsetVertical)) /
							  (paddleHeight / 2.0f);
			ballVelocity.setY(ballVelocity.getY() + hitOffset * 100.0f);
		}

		// Ball is out of bounds, so we reset it to the center
		if (ballPosition.getX() < 0)
		{
			ballPosition = vec2(windowWidth / 2.0f, windowHeight / 2.0f);
			ballVelocity = vec2(300.0f, 200.0f);
		}

		// Ball positions
		TransformDesc ballTransform = {};
		ballTransform.position = vec3(ballPosition.getX(), ballPosition.getY(), 0);
		ballTransform.rotation = vec3(0, 0, 0);
		ballTransform.scale = vec3(ballSize / 0.4f, ballSize / 0.4f, 1.0f);
		updateTransform(ballEntity, &ballTransform);

		EngineApp::Update(deltaTime);

		CameraMatrix projection = CameraMatrix::orthographic(0.0f, windowWidth, windowHeight, 0.0f,
															 -1.0f, 1.0f);

		// Upload per frame data
		cameraData.projView = projection;
		uploadPerFrameData(&cameraData, sizeof(cameraData));

		// Upload per object data
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

	const char* GetName() override { return "Pong"; }
};

DEFINE_APPLICATION_MAIN(MyGame)
