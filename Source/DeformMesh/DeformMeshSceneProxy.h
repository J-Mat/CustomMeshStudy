#pragma once

#include "CoreMinimal.h"
#include "PrimitiveSceneProxy.h"
#include "Containers/ResourceArray.h"
#include "RenderResource.h"
#include "PrimitiveViewRelevance.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "Containers/ResourceArray.h"
#include "DeformMeshVertexFactory.h"

class UDeformMeshComponent;


///////////////////////////////////////////////////////////////////////
// The Deform Mesh Component Mesh Section Proxy
/*
 * Stores the render thread data that it is needed to render one mesh section
 1 Vertex Data: Each mesh section creates an instance of the vertex factory(vertex streams and declarations), also each mesh section owns an index buffer
 2 Material : Contains a pointer to the material that will be used to render this section
 3 Other Data: Visibility, and the maximum vertex index.
*/
///////////////////////////////////////////////////////////////////////
class FDeformMeshSectionProxy
{
public:
	////////////////////////////////////////////////////////
	/* Material applied to this section */
	UMaterialInterface* Material;
	/* Index buffer for this section */
	FRawStaticIndexBuffer IndexBuffer;
	/* Vertex factory instance for this section */
	FDeformMeshVertexFactory VertexFactory;
	/* Whether this section is currently visible */
	bool bSectionVisible;
	/* Max vertix index is an info that is needed when rendering the mesh, so we cache it here so we don't have to pointer chase it later*/
	uint32 MaxVertexIndex;

	/* For each section, we'll create a vertex factory to store the per-instance mesh data*/
	FDeformMeshSectionProxy(ERHIFeatureLevel::Type InFeatureLevel)
		: Material(NULL)
		, VertexFactory(InFeatureLevel)
		, bSectionVisible(true)
	{}
};



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
private:
	/** Array of sections */
	TArray<FDeformMeshSectionProxy*> Sections;

	FMaterialRelevance MaterialRelevance;
};
