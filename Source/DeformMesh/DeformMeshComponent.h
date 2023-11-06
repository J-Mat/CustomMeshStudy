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



