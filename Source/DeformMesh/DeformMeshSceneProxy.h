#pragma once

#include "PrimitiveSceneProxy.h"
#include "Containers/ResourceArray.h"
#include "RenderResource.h"
#include "PrimitiveViewRelevance.h"

///////////////////////////////////////////////////////////////////////
// The Deform Mesh Component Scene Proxy
/*
 * Encapsulates the render thread data of the Deform Mesh Component
 * Read more about scene proxies: https://docs.unrealengine.com/en-US/API/Runtime/Engine/FPrimitiveSceneProxy/index.html
*/
///////////////////////////////////////////////////////////////////////
class FDeformMeshSceneProxy final : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	/* On construction of the Scene proxy, we'll copy all the needed data from the game thread mesh sections to create the needed render thread mesh sections' proxies*/
	/* We'll also create the structured buffer that will contain the deform transforms of all the sections*/
	FDeformMeshSceneProxy(UDeformMeshComponent* Component);
};
