#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/MeshComponent.h"
#include "Engine/StaticMesh.h"
#include "DeformMeshComponent.generated.h"

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
public:
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
};

