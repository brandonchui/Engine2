/*
 * EngineApp.cpp
 */

#include "Runtime/EngineApp.h"
#include "Runtime/ECS.h"
#include "Application/Interfaces/IUI.h"
#include "Utilities/Interfaces/ILog.h"
#include "Utilities/Math/MathTypes.h"
#include "Utilities/RingBuffer.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"

#include "Graphics/FSL/defaults.h"
#include "../../Shaders/FSL/Global.srt.h"

EngineApp::EngineApp()
: pRenderer(NULL)
, pGraphicsQueue(NULL)
, pSwapChain(NULL)
, pDepthBuffer(NULL)
, pImageAcquiredSemaphore(NULL)
, pShader(NULL)
, pPipeline(NULL)
, pCubeShader(NULL)
, pCubePipeline(NULL)
, pWorld(NULL)
, pRenderQuery(NULL)
, pRenderDataArray(NULL)
, maxRenderDataCount(1000)
, gFontID(0)
, gFrameIndex(0)
, pDescriptorSetPersistent(NULL)
, pDescriptorSetPerFrame(NULL)
, pDescriptorSetPerObject(NULL)
, nextObjectBufferIndex(0)
{
	for (uint32_t i = 0; i < gDataBufferCount; ++i)
		pUniformBufferPerFrame[i] = NULL;

	for (uint32_t i = 0; i < gDataBufferCount; ++i)
		for (uint32_t j = 0; j < MAX_OBJECTS; ++j)
			pUniformBufferPerObject[i][j] = NULL;
}

EngineApp::~EngineApp()
{
}

bool EngineApp::Init()
{
	LOGF(LogLevel::eINFO, "EngineApp::Init");

	if (!initRendererInternal())
	{
		LOGF(LogLevel::eERROR, "Failed to initialize renderer");
		return false;
	}

	// Initialize root signature
	RootSignatureDesc rootDesc = {};
	rootDesc.pGraphicsFileName = "default.rootsig";
	rootDesc.pComputeFileName = nullptr;
	initRootSignature(pRenderer, &rootDesc);

	// Define and load fonts
	FontDesc font = {};
	font.pFontPath = "TitilliumText/TitilliumText-Bold.otf";
	fntDefineFonts(&font, 1, &gFontID);

	// Initialize font system
	FontSystemDesc fontRenderDesc = {};
	fontRenderDesc.pRenderer = pRenderer;
	if (!initFontSystem(&fontRenderDesc))
	{
		LOGF(LogLevel::eERROR, "Failed to initialize font system");
		return false;
	}

	// Initialize UI for the performance measures and other Forge widgets
	UserInterfaceDesc uiRenderDesc = {};
	uiRenderDesc.pRenderer = pRenderer;
	initUserInterface(&uiRenderDesc);

	// Initialize profiler
	ProfilerDesc profiler = {};
	profiler.pRenderer = pRenderer;
	initProfiler(&profiler);

	// Initialize GPU profiler
	gGpuProfileToken = initGpuProfiler(pRenderer, pGraphicsQueue, "Graphics");

	// Initialize profiler draw settings
	gFrameTimeDraw = {};
	gFrameTimeDraw.mFontColor = 0xff00ffff; // Cyan
	gFrameTimeDraw.mFontSize = 18.0f;
	gFrameTimeDraw.mFontID = gFontID;

	initEntityComponentSystem();

	pWorld = ecs_init();
	initECS(pWorld);

	pRenderDataArray = (MeshRenderData*)tf_malloc(maxRenderDataCount * sizeof(MeshRenderData));
	if (!pRenderDataArray)
	{
		LOGF(LogLevel::eERROR, "Failed to allocate render data array");
		return false;
	}

	RenderContext* ctx = ecs_singleton_ensure(pWorld, RenderContext);
	ctx->pRenderDataArray = pRenderDataArray;
	ctx->renderDataCount = 0;
	ctx->maxRenderData = maxRenderDataCount;
	ctx->pCmd = NULL;
	ctx->pRenderTarget = NULL;
	ctx->frameIndex = 0;
	ecs_singleton_modified(pWorld, RenderContext);

	LOGF(LogLevel::eINFO, "ECS world initialized with %d max render slots", maxRenderDataCount);

	return true;
}

void EngineApp::Exit()
{
	LOGF(LogLevel::eINFO, "EngineApp::Exit");

	if (pRenderDataArray)
	{
		tf_free(pRenderDataArray);
		pRenderDataArray = NULL;
	}

	if (pWorld)
	{
		ecs_fini(pWorld);
		pWorld = NULL;
		LOGF(LogLevel::eINFO, "ECS world cleaned up");
	}

	exitUserInterface();

	exitFontSystem();

	exitGpuProfiler(gGpuProfileToken);
	exitProfiler();

	exitRootSignature(pRenderer);
	exitRendererInternal();
}

bool EngineApp::Load(ReloadDesc* pReloadDesc)
{
	LOGF(LogLevel::eINFO, "EngineApp::Load");

	if (!pReloadDesc || pReloadDesc->mType & RELOAD_TYPE_SHADER)
	{
		loadShaders();

		if (!pDescriptorSetPersistent)
		{
			DescriptorSetDesc descPersistent = SRT_SET_DESC(SrtData, Persistent, 1, 0);
			addDescriptorSet(pRenderer, &descPersistent, &pDescriptorSetPersistent);
			LOGF(LogLevel::eINFO, "Persistent descriptor set created (empty)");
		}
	}

	if (!pReloadDesc || pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
	{
		SwapChainDesc swapChainDesc = {};
		swapChainDesc.mWindowHandle = pWindow->handle;
		swapChainDesc.mPresentQueueCount = 1;
		swapChainDesc.ppPresentQueues = &pGraphicsQueue;
		swapChainDesc.mWidth = mSettings.mWidth;
		swapChainDesc.mHeight = mSettings.mHeight;
		swapChainDesc.mImageCount = getRecommendedSwapchainImageCount(pRenderer, &pWindow->handle);
		swapChainDesc.mColorFormat = getSupportedSwapchainFormat(pRenderer, &swapChainDesc,
																 COLOR_SPACE_SDR_SRGB);
		swapChainDesc.mColorSpace = COLOR_SPACE_SDR_SRGB;
		swapChainDesc.mEnableVsync = mSettings.mVSyncEnabled;
		addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);

		RenderTargetDesc depthRT = {};
		depthRT.mArraySize = 1;
		depthRT.mClearValue.depth = 0.0f;
		depthRT.mClearValue.stencil = 0;
		depthRT.mDepth = 1;
		depthRT.mFormat = TinyImageFormat_D32_SFLOAT;
		depthRT.mStartState = RESOURCE_STATE_DEPTH_WRITE;
		depthRT.mHeight = pSwapChain->ppRenderTargets[0]->mHeight;
		depthRT.mWidth = pSwapChain->ppRenderTargets[0]->mWidth;
		depthRT.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
		depthRT.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
		depthRT.mFlags = TEXTURE_CREATION_FLAG_NONE;
		addRenderTarget(pRenderer, &depthRT, &pDepthBuffer);

		LOGF(LogLevel::eINFO, "Swapchain and depth buffer recreated: %dx%d", mSettings.mWidth,
			 mSettings.mHeight);
	}

	if (!pReloadDesc || pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
	{
		createPipeline();

		if (!pUniformBufferPerFrame[0])
		{
			createPerFrameUniformBuffer();
			createPerObjectBuffers(10);
			createPerObjectDescriptorSets(10);
			LOGF(LogLevel::eINFO, "Per-frame and per-object uniform buffers created");
		}
	}

	if (!pReloadDesc || pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
	{
		if (pSwapChain && pSwapChain->ppRenderTargets[0])
		{
			loadProfilerUI(pSwapChain->ppRenderTargets[0]->mWidth,
						   pSwapChain->ppRenderTargets[0]->mHeight);
		}

		UserInterfaceLoadDesc uiLoad = {};
		uiLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
		uiLoad.mHeight = mSettings.mHeight;
		uiLoad.mWidth = mSettings.mWidth;
		uiLoad.mLoadType = pReloadDesc
							   ? pReloadDesc->mType
							   : (ReloadType)(RELOAD_TYPE_RENDERTARGET | RELOAD_TYPE_RESIZE);
		loadUserInterface(&uiLoad);

		FontSystemLoadDesc fontLoad = {};
		fontLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
		fontLoad.mHeight = mSettings.mHeight;
		fontLoad.mWidth = mSettings.mWidth;
		fontLoad.mLoadType = pReloadDesc
								 ? pReloadDesc->mType
								 : (ReloadType)(RELOAD_TYPE_RENDERTARGET | RELOAD_TYPE_RESIZE);
		loadFontSystem(&fontLoad);
	}

	return true;
}

void EngineApp::Unload(ReloadDesc* pReloadDesc)
{
	LOGF(LogLevel::eINFO, "EngineApp::Unload");

	waitQueueIdle(pGraphicsQueue);

	if (!pReloadDesc || pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
	{
		unloadFontSystem(pReloadDesc ? pReloadDesc->mType : RELOAD_TYPE_ALL);
		unloadUserInterface(pReloadDesc ? pReloadDesc->mType : RELOAD_TYPE_ALL);
		unloadProfilerUI();
	}

	if (!pReloadDesc || pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
	{
		destroyPipeline();
	}

	if (!pReloadDesc || pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
	{
		if (pSwapChain)
		{
			removeSwapChain(pRenderer, pSwapChain);
			pSwapChain = nullptr;
		}
		if (pDepthBuffer)
		{
			removeRenderTarget(pRenderer, pDepthBuffer);
			pDepthBuffer = nullptr;
		}
		LOGF(LogLevel::eINFO, "Swapchain and depth buffer removed for resize");
	}

	if (!pReloadDesc || pReloadDesc->mType & RELOAD_TYPE_SHADER)
	{
		destroyPerObjectDescriptorSets();
		destroyPerFrameUniformBuffer();

		if (pDescriptorSetPersistent)
		{
			removeDescriptorSet(pRenderer, pDescriptorSetPersistent);
			pDescriptorSetPersistent = nullptr;
			LOGF(LogLevel::eINFO, "Persistent descriptor set destroyed");
		}

		unloadShaders();
	}
}

void EngineApp::Update(float deltaTime)
{
	if (pWorld)
	{
		RenderContext* ctx = ecs_singleton_ensure(pWorld, RenderContext);
		ctx->renderDataCount = 0;
		ecs_singleton_modified(pWorld, RenderContext);

		float dt = deltaTime > 0.0f ? deltaTime : 0.016f;

		ecs_progress(pWorld, dt);
	}
}

void EngineApp::Draw()
{
	uint32_t swapchainImageIndex;
	acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, NULL, &swapchainImageIndex);

	RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapchainImageIndex];

	GpuCmdRingElement elem = getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 1);

	FenceStatus fenceStatus;
	getFenceStatus(pRenderer, elem.pFence, &fenceStatus);
	if (fenceStatus == FENCE_STATUS_INCOMPLETE)
		waitForFences(pRenderer, 1, &elem.pFence);

	flipProfiler();

	resetCmdPool(pRenderer, elem.pCmdPool);

	Cmd* cmd = elem.pCmds[0];
	beginCmd(cmd);

	cmdBeginGpuFrameProfile(cmd, gGpuProfileToken);

	RenderTargetBarrier barriers[] = {
		{pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET},
	};
	cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

	BindRenderTargetsDesc bindRenderTargets = {};
	bindRenderTargets.mRenderTargetCount = 1;
	bindRenderTargets.mRenderTargets[0] = {pRenderTarget, LOAD_ACTION_CLEAR};
	bindRenderTargets.mDepthStencil = {pDepthBuffer, LOAD_ACTION_CLEAR};
	cmdBindRenderTargets(cmd, &bindRenderTargets);
	cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight,
				   0.0f, 1.0f);
	cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

	// Render all entities with mesh components
	if (pWorld && pPipeline && pRenderDataArray)
	{
		cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "ECS Render");

		// Get the render data count from the singleton
		// This was filled by FillRenderDataSystem during ecs_progress() in Update()
		const RenderContext* ctx = ecs_singleton_get(pWorld, RenderContext);
		uint32_t drawCount = ctx ? ctx->renderDataCount : 0;

		//LOGF(LogLevel::eINFO, "Draw: drawCount = %d, ctx = %p", drawCount, ctx);

		if (drawCount > 0)
		{
			Pipeline* lastBoundPipeline = NULL;

			for (uint32_t i = 0; i < drawCount; i++)
			{
				const MeshRenderData& renderData = pRenderDataArray[i];

				if (renderData.pPipeline && renderData.pPipeline != lastBoundPipeline)
				{
					cmdBindPipeline(cmd, renderData.pPipeline);
					lastBoundPipeline = renderData.pPipeline;

					if (pDescriptorSetPersistent)
					{
						cmdBindDescriptorSet(cmd, 0, pDescriptorSetPersistent);
					}

					if (pDescriptorSetPerFrame)
					{
						cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetPerFrame);
					}
				}

				if (pDescriptorSetPerObject)
				{
					cmdBindDescriptorSet(cmd, renderData.descriptorSetIndex,
										 pDescriptorSetPerObject);
				}

				Buffer* vb = renderData.pVertexBuffer;
				uint32_t stride = renderData.vertexStride;
				cmdBindVertexBuffer(cmd, 1, &vb, &stride, NULL);

				cmdDraw(cmd, renderData.vertexCount, 0);
			}

			static uint32_t lastLogFrame = 0;
			if (gFrameIndex - lastLogFrame > 60)
			{
				LOGF(LogLevel::eINFO, "Rendered %d entities via ECS", drawCount);
				lastLogFrame = gFrameIndex;
			}
		}

		cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
	}

	cmdBindRenderTargets(cmd, NULL);

	cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw UI");

	BindRenderTargetsDesc bindRenderTargetsUI = {};
	bindRenderTargetsUI.mRenderTargetCount = 1;
	bindRenderTargetsUI.mRenderTargets[0] = {pRenderTarget, LOAD_ACTION_LOAD};
	cmdBindRenderTargets(cmd, &bindRenderTargetsUI);

	gFrameTimeDraw.mFontColor = 0xff00ffff; // Cyan
	gFrameTimeDraw.mFontSize = 18.0f;
	gFrameTimeDraw.mFontID = gFontID;
	float2 txtSizePx = cmdDrawCpuProfile(cmd, float2(8.f, 15.f), &gFrameTimeDraw);
	cmdDrawGpuProfile(cmd, float2(8.f, txtSizePx.y + 75.f), gGpuProfileToken, &gFrameTimeDraw);

	cmdDrawUserInterface(cmd);

	cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
	cmdBindRenderTargets(cmd, NULL);

	barriers[0] = {pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT};
	cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

	cmdEndGpuFrameProfile(cmd, gGpuProfileToken);

	endCmd(cmd);

	FlushResourceUpdateDesc flushUpdateDesc = {};
	flushUpdateDesc.mNodeIndex = 0;
	flushResourceUpdates(&flushUpdateDesc);
	Semaphore* waitSemaphores[2] = {flushUpdateDesc.pOutSubmittedSemaphore,
									pImageAcquiredSemaphore};

	QueueSubmitDesc submitDesc = {};
	submitDesc.mCmdCount = 1;
	submitDesc.mSignalSemaphoreCount = 1;
	submitDesc.mWaitSemaphoreCount = waitSemaphores[0] ? 2 : 1;
	submitDesc.ppCmds = &cmd;
	submitDesc.ppSignalSemaphores = &elem.pSemaphore;
	submitDesc.ppWaitSemaphores = waitSemaphores;
	submitDesc.pSignalFence = elem.pFence;
	queueSubmit(pGraphicsQueue, &submitDesc);

	// Present
	QueuePresentDesc presentDesc = {};
	presentDesc.mIndex = (uint8_t)swapchainImageIndex;
	presentDesc.mWaitSemaphoreCount = 1;
	presentDesc.ppWaitSemaphores = &elem.pSemaphore;
	presentDesc.pSwapChain = pSwapChain;
	presentDesc.mSubmitDone = true;
	queuePresent(pGraphicsQueue, &presentDesc);

	gFrameIndex = (gFrameIndex + 1) % gDataBufferCount;
}

bool EngineApp::initRendererInternal()
{
	RendererDesc settings = {};
	settings.mShaderTarget = SHADER_TARGET_6_0;
	initGPUConfiguration(settings.pExtendedSettings);
	::initRenderer(GetName(), &settings, &pRenderer);
	if (!pRenderer)
		return false;

	QueueDesc queueDesc = {};
	queueDesc.mType = QUEUE_TYPE_GRAPHICS;
	queueDesc.mFlag = QUEUE_FLAG_NONE;
	initQueue(pRenderer, &queueDesc, &pGraphicsQueue);

	GpuCmdRingDesc cmdRingDesc = {};
	cmdRingDesc.pQueue = pGraphicsQueue;
	cmdRingDesc.mPoolCount = 2;
	cmdRingDesc.mCmdPerPoolCount = 1;
	cmdRingDesc.mAddSyncPrimitives = true;
	initGpuCmdRing(pRenderer, &cmdRingDesc, &gGraphicsCmdRing);

	initSemaphore(pRenderer, &pImageAcquiredSemaphore);

	initResourceLoaderInterface(pRenderer);

	return true;
}

void EngineApp::exitRendererInternal()
{
	waitQueueIdle(pGraphicsQueue);

	if (pImageAcquiredSemaphore)
	{
		exitSemaphore(pRenderer, pImageAcquiredSemaphore);
		pImageAcquiredSemaphore = NULL;
	}

	exitResourceLoaderInterface(pRenderer);

	exitGpuCmdRing(pRenderer, &gGraphicsCmdRing);

	if (pGraphicsQueue)
	{
		exitQueue(pRenderer, pGraphicsQueue);
		pGraphicsQueue = NULL;
	}

	if (pRenderer)
	{
		::exitRenderer(pRenderer);
		pRenderer = NULL;
	}

	exitGPUConfiguration();
}

void EngineApp::loadShaders()
{
	ShaderLoadDesc shaderDesc = {};
	shaderDesc.mVert.pFileName = "basic.vert";
	shaderDesc.mFrag.pFileName = "basic.frag";
	addShader(pRenderer, &shaderDesc, &pShader);

	if (pShader)
	{
		LOGF(LogLevel::eINFO, "Sprite shaders loaded successfully");
	}
	else
	{
		LOGF(LogLevel::eERROR, "Failed to load sprite shaders");
	}

	ShaderLoadDesc cubeShaderDesc = {};
	cubeShaderDesc.mVert.pFileName = "basic.vert";
	cubeShaderDesc.mFrag.pFileName = "cube.frag";
	addShader(pRenderer, &cubeShaderDesc, &pCubeShader);

	if (pCubeShader)
	{
		LOGF(LogLevel::eINFO, "Cube shaders loaded successfully");
	}
	else
	{
		LOGF(LogLevel::eERROR, "Failed to load cube shaders");
	}
}

void EngineApp::unloadShaders()
{
	if (pShader)
	{
		removeShader(pRenderer, pShader);
		pShader = NULL;
	}

	if (pCubeShader)
	{
		removeShader(pRenderer, pCubeShader);
		pCubeShader = NULL;
	}
}

void EngineApp::createPipeline()
{
	LOGF(LogLevel::eINFO, "createPipeline: Starting pipeline creation");
	LOGF(LogLevel::eINFO, "createPipeline: pShader = %p, pSwapChain = %p", pShader, pSwapChain);

	VertexLayout vertexLayout = {};
	vertexLayout.mBindingCount = 1;
	vertexLayout.mAttribCount = 2;

	vertexLayout.mBindings[0].mStride = sizeof(float) * 5; // 3 floats position + 2 floats UV
	vertexLayout.mBindings[0].mRate = VERTEX_BINDING_RATE_VERTEX;

	vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
	vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
	vertexLayout.mAttribs[0].mBinding = 0;
	vertexLayout.mAttribs[0].mLocation = 0;
	vertexLayout.mAttribs[0].mOffset = 0;

	vertexLayout.mAttribs[1].mSemantic = SEMANTIC_TEXCOORD0;
	vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32_SFLOAT;
	vertexLayout.mAttribs[1].mBinding = 0;
	vertexLayout.mAttribs[1].mLocation = 1;
	vertexLayout.mAttribs[1].mOffset = sizeof(float) * 3;

	RasterizerStateDesc rasterizerStateDesc = {};
	rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

	DepthStateDesc depthStateDesc = {};
	depthStateDesc.mDepthTest = true;
	depthStateDesc.mDepthWrite = true;
	depthStateDesc.mDepthFunc = CMP_GEQUAL;
	depthStateDesc.mStencilTest = false;
	depthStateDesc.mStencilReadMask = 0xFF;
	depthStateDesc.mStencilWriteMask = 0xFF;
	depthStateDesc.mStencilFrontFunc = CMP_ALWAYS;
	depthStateDesc.mStencilFrontFail = STENCIL_OP_KEEP;
	depthStateDesc.mDepthFrontFail = STENCIL_OP_KEEP;
	depthStateDesc.mStencilFrontPass = STENCIL_OP_KEEP;
	depthStateDesc.mStencilBackFunc = CMP_ALWAYS;
	depthStateDesc.mStencilBackFail = STENCIL_OP_KEEP;
	depthStateDesc.mDepthBackFail = STENCIL_OP_KEEP;
	depthStateDesc.mStencilBackPass = STENCIL_OP_KEEP;

	BlendStateDesc blendStateDesc = {};
	blendStateDesc.mSrcFactors[0] = BC_SRC_ALPHA;
	blendStateDesc.mDstFactors[0] = BC_ONE_MINUS_SRC_ALPHA;
	blendStateDesc.mSrcAlphaFactors[0] = BC_ONE;
	blendStateDesc.mDstAlphaFactors[0] = BC_ONE_MINUS_SRC_ALPHA;
	blendStateDesc.mColorWriteMasks[0] = COLOR_MASK_ALL;
	blendStateDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;
	blendStateDesc.mIndependentBlend = false;

	PipelineDesc pipelineDesc = {};
	pipelineDesc.mType = PIPELINE_TYPE_GRAPHICS;

	GraphicsPipelineDesc& pipelineSettings = pipelineDesc.mGraphicsDesc;
	pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
	pipelineSettings.mRenderTargetCount = 1;

	pipelineSettings.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
	pipelineSettings.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
	pipelineSettings.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;

	if (pDepthBuffer)
	{
		pipelineSettings.mDepthStencilFormat = pDepthBuffer->mFormat;
	}
	else
	{
		pipelineSettings.mDepthStencilFormat = TinyImageFormat_D32_SFLOAT;
	}

	// Create separate depth state for sprite
	DepthStateDesc spriteDepthStateDesc = depthStateDesc;
	spriteDepthStateDesc.mDepthWrite = false;

	pipelineSettings.pShaderProgram = pShader;
	pipelineSettings.pVertexLayout = &vertexLayout;
	pipelineSettings.pRasterizerState = &rasterizerStateDesc;
	pipelineSettings.pDepthState = &spriteDepthStateDesc;
	pipelineSettings.pBlendState = &blendStateDesc;

	addPipeline(pRenderer, &pipelineDesc, &pPipeline);

	BlendStateDesc opaqueBlendStateDesc = {};
	opaqueBlendStateDesc.mSrcFactors[0] = BC_ONE;
	opaqueBlendStateDesc.mDstFactors[0] = BC_ZERO;
	opaqueBlendStateDesc.mSrcAlphaFactors[0] = BC_ONE;
	opaqueBlendStateDesc.mDstAlphaFactors[0] = BC_ZERO;
	opaqueBlendStateDesc.mColorWriteMasks[0] = COLOR_MASK_ALL;
	opaqueBlendStateDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;
	opaqueBlendStateDesc.mIndependentBlend = false;

	PipelineDesc cubePipelineDesc = {};
	cubePipelineDesc.mType = PIPELINE_TYPE_GRAPHICS;

	GraphicsPipelineDesc& cubePipelineSettings = cubePipelineDesc.mGraphicsDesc;
	cubePipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
	cubePipelineSettings.mRenderTargetCount = 1;
	cubePipelineSettings.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
	cubePipelineSettings.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
	cubePipelineSettings.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;

	if (pDepthBuffer)
	{
		cubePipelineSettings.mDepthStencilFormat = pDepthBuffer->mFormat;
	}

	cubePipelineSettings.pShaderProgram = pCubeShader;
	cubePipelineSettings.pVertexLayout = &vertexLayout;
	cubePipelineSettings.pRasterizerState = &rasterizerStateDesc;
	cubePipelineSettings.pDepthState = &depthStateDesc;
	cubePipelineSettings.pBlendState = &opaqueBlendStateDesc;

	addPipeline(pRenderer, &cubePipelineDesc, &pCubePipeline);
}

void EngineApp::destroyPipeline()
{
	if (pPipeline)
	{
		removePipeline(pRenderer, pPipeline);
		pPipeline = NULL;
	}

	if (pCubePipeline)
	{
		removePipeline(pRenderer, pCubePipeline);
		pCubePipeline = NULL;
	}
}

Buffer* EngineApp::createMeshBuffer(const void* pVertexData, uint32_t dataSize)
{
	Buffer* pBuffer = NULL;

	BufferLoadDesc vbDesc = {};
	vbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	vbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	vbDesc.mDesc.mSize = dataSize;
	vbDesc.pData = pVertexData;
	vbDesc.ppBuffer = &pBuffer;
	addResource(&vbDesc, NULL);

	waitForAllResourceLoads();

	if (pBuffer)
	{
		LOGF(LogLevel::eINFO, "Mesh buffer created successfully, size: %d bytes", dataSize);
	}
	else
	{
		LOGF(LogLevel::eERROR, "Failed to create mesh buffer!");
	}

	return pBuffer;
}

void EngineApp::createPerFrameUniformBuffer()
{
	if (!pRenderer)
		return;

	BufferLoadDesc ubDesc = {};
	ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
	ubDesc.pData = NULL;

	for (uint32_t i = 0; i < gDataBufferCount; ++i)
	{
		ubDesc.mDesc.pName = "PerFrameUniformBuffer";
		ubDesc.mDesc.mSize = sizeof(mat4);
		ubDesc.ppBuffer = &pUniformBufferPerFrame[i];
		addResource(&ubDesc, NULL);
	}

	DescriptorSetDesc descPerFrame = SRT_SET_DESC(SrtData, PerFrame, gDataBufferCount, 0);
	addDescriptorSet(pRenderer, &descPerFrame, &pDescriptorSetPerFrame);

	waitForAllResourceLoads();

	for (uint32_t i = 0; i < gDataBufferCount; ++i)
	{
		DescriptorData params[1] = {};
		params[0].mIndex = SRT_RES_IDX(SrtData, PerFrame, gCamera);
		params[0].ppBuffers = &pUniformBufferPerFrame[i];
		updateDescriptorSet(pRenderer, i, pDescriptorSetPerFrame, 1, params);
	}
}

void EngineApp::destroyPerFrameUniformBuffer()
{
	if (pDescriptorSetPerFrame)
	{
		removeDescriptorSet(pRenderer, pDescriptorSetPerFrame);
		pDescriptorSetPerFrame = NULL;
	}

	for (uint32_t i = 0; i < gDataBufferCount; ++i)
	{
		if (pUniformBufferPerFrame[i])
		{
			removeResource(pUniformBufferPerFrame[i]);
			pUniformBufferPerFrame[i] = NULL;
		}
	}
}

void EngineApp::createPerObjectBuffers(uint32_t count)
{
	if (!pRenderer || count == 0 || count > MAX_OBJECTS)
		return;

	BufferLoadDesc ubDesc = {};
	ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
	ubDesc.mDesc.mSize = sizeof(mat4);
	ubDesc.pData = NULL;

	for (uint32_t i = 0; i < gDataBufferCount; ++i)
	{
		for (uint32_t j = 0; j < count; ++j)
		{
			ubDesc.mDesc.pName = "PerObjectUniformBuffer";
			ubDesc.ppBuffer = &pUniformBufferPerObject[i][j];
			addResource(&ubDesc, NULL);
		}
	}

	waitForAllResourceLoads();
}

void EngineApp::createPerObjectDescriptorSets(uint32_t count)
{
	if (!pRenderer || count == 0 || count > MAX_OBJECTS)
		return;

	DescriptorSetDesc descPerObject = SRT_SET_DESC(SrtData, PerDraw, count, 0);
	addDescriptorSet(pRenderer, &descPerObject, &pDescriptorSetPerObject);

	for (uint32_t objIndex = 0; objIndex < count; ++objIndex)
	{
		for (uint32_t frameIndex = 0; frameIndex < gDataBufferCount; ++frameIndex)
		{
			DescriptorData params[1] = {};
			params[0].mIndex = SRT_RES_IDX(SrtData, PerDraw, gObject);
			params[0].ppBuffers = &pUniformBufferPerObject[frameIndex][objIndex];

			uint32_t descriptorIndex = objIndex;
			updateDescriptorSet(pRenderer, descriptorIndex, pDescriptorSetPerObject, 1, params);
		}
	}
}

void EngineApp::destroyPerObjectDescriptorSets()
{
	if (pDescriptorSetPerObject)
	{
		removeDescriptorSet(pRenderer, pDescriptorSetPerObject);
		pDescriptorSetPerObject = NULL;
	}

	for (uint32_t i = 0; i < gDataBufferCount; ++i)
	{
		for (uint32_t j = 0; j < MAX_OBJECTS; ++j)
		{
			if (pUniformBufferPerObject[i][j])
			{
				removeResource(pUniformBufferPerObject[i][j]);
				pUniformBufferPerObject[i][j] = NULL;
			}
		}
	}
}

uint32_t EngineApp::allocateObjectBufferSlot()
{
	if (nextObjectBufferIndex >= MAX_OBJECTS)
	{
		LOGF(LogLevel::eERROR, "Full! Max: %d", MAX_OBJECTS);
		return 0;
	}

	uint32_t slot = nextObjectBufferIndex++;
	LOGF(LogLevel::eINFO, "Allocated object buffer slot %d", slot);
	return slot;
}

void EngineApp::uploadPerFrameData(const void* pData, size_t size)
{
	if (!pData || size == 0 || !pUniformBufferPerFrame[gFrameIndex])
		return;

	BufferUpdateDesc cbv = {pUniformBufferPerFrame[gFrameIndex]};
	beginUpdateResource(&cbv);
	memcpy(cbv.pMappedData, pData, size);
	endUpdateResource(&cbv);
}

void EngineApp::uploadPerObjectData(uint32_t objectIndex, const void* pData, size_t size)
{
	if (!pData || size == 0 || objectIndex >= MAX_OBJECTS ||
		!pUniformBufferPerObject[gFrameIndex][objectIndex])
		return;

	BufferUpdateDesc cbv = {pUniformBufferPerObject[gFrameIndex][objectIndex]};
	beginUpdateResource(&cbv);
	memcpy(cbv.pMappedData, pData, size);
	endUpdateResource(&cbv);
}

ecs_entity_t EngineApp::createMeshEntity(const MeshEntityDesc* pDesc)
{
	uint32_t descriptorIndex = allocateObjectBufferSlot();

	ecs_entity_t entity = ::createMeshEntity(pWorld, pDesc);

	MeshComponent* mesh = ecs_get_mut(pWorld, entity, MeshComponent);
	if (mesh)
	{
		mesh->descriptorSetIndex = descriptorIndex;
		ecs_modified(pWorld, entity, MeshComponent);
	}

	LOGF(LogLevel::eINFO, "Entity %llu allocated descriptor set index %d", entity, descriptorIndex);
	return entity;
}

void EngineApp::updateTransform(ecs_entity_t entity, const TransformDesc* pDesc)
{
	::updateTransform(pWorld, entity, pDesc);
}

uint32_t EngineApp::getRenderDataCount() const
{
	const RenderContext* ctx = ecs_singleton_get(pWorld, RenderContext);
	return ctx ? ctx->renderDataCount : 0;
}
