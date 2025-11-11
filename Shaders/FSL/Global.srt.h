#pragma once

BEGIN_SRT_NO_AB(SrtData)
BEGIN_SRT_SET(Persistent)
	DECL_SAMPLER(Persistent, SamplerState, gSpriteSampler)
	DECL_TEXTURE(Persistent, Tex2D(float4), gSpriteTexture)
	DECL_TEXTURE(Persistent, Tex2D(float4), gCubeTexture)
END_SRT_SET(Persistent)

BEGIN_SRT_SET(PerFrame)
DECL_CBUFFER(PerFrame, CBUFFER(Camera), gCamera)
END_SRT_SET(PerFrame)

BEGIN_SRT_SET(PerDraw)
DECL_CBUFFER(PerDraw, CBUFFER(Object), gObject)
END_SRT_SET(PerDraw)
END_SRT(SrtData)
