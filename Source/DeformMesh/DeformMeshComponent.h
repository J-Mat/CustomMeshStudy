#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/MeshComponent.h"
#include "Engine/StaticMesh.h"
#include "DeformMeshComponent.generated.h"

class UDeformMeshComponent;
class FDeformMeshVertexFactory;

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

	void UpdateDeformTransform_RenderThread(int32 SectionIndex, FMatrix Transform);
	void SetSectionVisibility_RenderThread(int32 SectionIndex, bool bNewVisibility);

	/* Given the scene views and the visibility map, we add to the collector the relevant dynamic meshes that need to be rendered by this component*/
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const;

	virtual bool CanBeOccluded() const override;

	virtual uint32 GetMemoryFootprint(void) const;

	uint32 GetAllocatedSize(void) const;

	FShaderResourceViewRHIRef& GetDeformTransformsSRV() { return DeformTransformsSRV; }
private:
	/** Array of sections */
	TArray<FDeformMeshSectionProxy*> Sections;

	FMaterialRelevance MaterialRelevance;

	//The render thread array of transforms of all the sections
	//Individual updates of each section's deform transform will just update the entry in this array
	//Before binding the SRV, we update the content of the structured buffer with this updated array
	TArray<FMatrix> DeformTransforms;

	//The structured buffer that will contain all the deform transoform and going to be used as a shader resource
	FStructuredBufferRHIRef DeformTransformsSB;

	//The shader resource view of the structured buffer, this is what we bind to the vertex factory shader
	FShaderResourceViewRHIRef DeformTransformsSRV;

	//Whether the structured buffer needs to be updated or not
	bool bDeformTransformsDirty;
};


// Mesh section of the DeformMesh.A mesh section is a part of the mesh that is rendered with one material(1 material per section)*
USTRUCT()
struct FDeformMeshSection
{
	GENERATED_BODY()
public:
	/** The static mesh that holds the mesh data for this section */
	UPROPERTY()
	UStaticMesh* StaticMesh;
	
	/** The secondary transform matrix that we'll use to deform this mesh section*/
	UPROPERTY()
	FMatrix DeformTransform;
	
	/** Local bounding box of section */
	UPROPERTY()
	FBox SectionLocalBox;
	
	/** Should we display this section */
	UPROPERTY()
	bool bSectionVisible;
	
	FDeformMeshSection()
		: SectionLocalBox(ForceInit)
		, bSectionVisible(true)
	{}

	/** Reset this section, clear all mesh info. */
	void Reset()
	{
		StaticMesh = nullptr;
		SectionLocalBox.Init();
		bSectionVisible = true;
	}
};

/**
*	Component that allows you deform the vertices of a mesh by supplying a secondary deform transform
*/
UCLASS(hidecategories = (Object, LOD), meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class DEFORMMESH_API UDeformMeshComponent : public UMeshComponent
{
	GENERATED_BODY()
	friend class FDeformMeshSceneProxy;
public:
	void CreateMeshSection(int32 SectionIndex, UStaticMesh* Mesh, const FTransform& DeformTransform);

	void UpdateMeshSectionTransform(int32 SectionIndex, const FTransform& DeformTransform);

	void FinishTransformsUpdate();

	/** Clear a section of the DeformMesh. Other sections do not change index. */
	void ClearMeshSection(int32 SectionIndex);

	/** Clear all mesh sections and reset to empty state */
	void ClearAllMeshSections();

	/** Control visibility of a particular section */
	void SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility);

	/** Returns whether a particular section is currently visible */
	bool IsMeshSectionVisible(int32 SectionIndex) const;
	
	/** Returns number of sections currently created for this component */
	int32 GetNumSections() const;

	/**
	 *	Get pointer to internal data for one section of this Puzzle mesh component.
	 *	Note that pointer will becomes invalid if sections are added or removed.
	 */
	FDeformMeshSection* GetDeformMeshSection(int32 SectionIndex);

	/** Replace a section with new section geometry */
	void SetDeformMeshSection(int32 SectionIndex, const FDeformMeshSection& Section);

	//~ Begin UPrimitiveComponent Interface.
	/* PrimitiveComponents are SceneComponents that contain or generate some sort of geometry, generally to be rendered or used as collision data. (UE4 docs)
	* MeshComponents are primitive components, since they contain mesh data and render it
	* The most important method that we need to implement from the PrimitiveComponent interface is the CreateSceneProxy()
	* Any PrimitiveComponent has a scene proxy, which is the component's proxy in the render thread
	* Just like anything else on the game thread, we CAN'T just use it directly to issue render commands and create render resources
	* Instead, we create a proxy, and we delegate the render threads tasks to it.
	* PS: There's other methods (Collision related) from this interface that i'm not implementing, beacuse I'm only interested in the rendering
	*/
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;


	//~ Begin UMeshComponent Interface.
	/* MeshComponent is an abstract base for any component that is an instance of a renderable collection of triangles. (UE4 docs)
	*/
	virtual int32 GetNumMaterials() const override;
	//~ End UMeshComponent Interface.
private:
	//~ Begin USceneComponent Interface.
	/*A SceneComponent has a transform and supports attachment, but has no rendering or collision capabilities. (UE4 docs)
	* The PrimitiveComponent inherits from this class, and adds the rendering and collision capabilities
	* Any scene component, including our mesh component, has a transform and bounds
	* The transform is not managed directly here by the component, so we don't have to worry about that
	* But we need to manage the bounds of our component by implementing this method
	*/
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ Begin USceneComponent Interface.


	void UpdateLocalBounds();
	/** Array of sections of mesh */
	UPROPERTY()
	TArray<FDeformMeshSection> DeformMeshSections;

	/** Local space bounds of mesh */
	UPROPERTY()
	FBoxSphereBounds LocalBounds;
};

