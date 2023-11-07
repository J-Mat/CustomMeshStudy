#include "DeformMeshComponent.h"

FDeformMeshSceneProxy::FDeformMeshSceneProxy(UDeformMeshComponent* Component)
	: FPrimitiveSceneProxy(Component)
	, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
{
}

void FDeformMeshSceneProxy::UpdateDeformTransform_RenderThread(int32 SectionIndex, FMatrix Transform)
{
	check(IsInRenderingThread());
	if (SectionIndex < Sections.Num() &&
		Sections[SectionIndex] != nullptr)
	{
		DeformTransforms[SectionIndex] = Transform;
		//Mark as dirty
		bDeformTransformsDirty = true;
	}
}

void FDeformMeshSceneProxy::SetSectionVisibility_RenderThread(int32 SectionIndex, bool bNewVisibility)
{
	check(IsInRenderingThread());
	if (SectionIndex < Sections.Num() &&
		Sections[SectionIndex] != nullptr)
	{
		Sections[SectionIndex]->bSectionVisible = bNewVisibility;
	}
}

void FDeformMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	// Set up wireframe material (if needed)
	const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

	FColoredMaterialRenderProxy* WireframeMaterialInstance = NULL;
	if (bWireframe)
	{
		WireframeMaterialInstance = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : NULL,
			FLinearColor(0, 0.5f, 1.f)
		);

		Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
	}


	// Iterate over sections
	for (const FDeformMeshSectionProxy* Section : Sections)
	{
		if (Section != nullptr && Section->bSectionVisible)
		{
			//Get the section's material, or the wireframe material if we're rendering in wireframe mode
			FMaterialRenderProxy* MaterialProxy = bWireframe ? WireframeMaterialInstance : Section->Material->GetRenderProxy();
			
			// For each view..
			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				//Check if our mesh is visible from this view
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];
					// Allocate a mesh batch and get a ref to the first element
					FMeshBatch& Mesh = Collector.AllocateMesh();
					FMeshBatchElement& BatchElement = Mesh.Elements[0];
					//Fill this batch element with the mesh section's render data
					BatchElement.IndexBuffer = &Section->IndexBuffer;
					Mesh.bWireframe = bWireframe;
					Mesh.VertexFactory = &Section->VertexFactory;
					Mesh.MaterialRenderProxy = MaterialProxy;
					
					//The LocalVertexFactory uses a uniform buffer to pass primitve data like the local to world transform for this frame and for the previous one
					//Most of this data can be fetched using the helper function below
					bool bHasPrecomputedVolumetricLightmap;
					FMatrix PreviousLocalToWorld;
					int32 SingleCaptureIndex;
					bool bOutputVelocity;
					GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);
					//Alloate a temporary primitive uniform buffer, fill it with the data and set it in the batch element
					FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
					DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), true, bHasPrecomputedVolumetricLightmap, DrawsVelocity(), bOutputVelocity);
					BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;
					BatchElement.PrimitiveIdMode = PrimID_DynamicPrimitiveShaderData;


					//Additional data 
					BatchElement.FirstIndex = 0;
					BatchElement.NumPrimitives = Section->IndexBuffer.GetNumIndices() / 3;
					BatchElement.MinVertexIndex = 0;
					BatchElement.MaxVertexIndex = Section->MaxVertexIndex;
					Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
					Mesh.Type = PT_TriangleList;
					Mesh.DepthPriorityGroup = SDPG_World;
					Mesh.bCanApplyViewModeOverrides = false;
					
					//Add the batch to the collector
					Collector.AddMesh(ViewIndex, Mesh);
				}
			}
		}
	}
}

FPrimitiveViewRelevance FDeformMeshSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bDynamicRelevance = true;
	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
	MaterialRelevance.SetPrimitiveViewRelevance(Result);
	Result.bVelocityRelevance = IsMovable() && Result.bOpaque && Result.bRenderInMainPass;
	return Result;
}

bool FDeformMeshSceneProxy::CanBeOccluded() const
{
	return !MaterialRelevance.bDisableDepthTest;
}

uint32 FDeformMeshSceneProxy::GetMemoryFootprint(void) const
{
	return(sizeof(*this) + GetAllocatedSize());
}

uint32 FDeformMeshSceneProxy::GetAllocatedSize(void) const
{
	return(FPrimitiveSceneProxy::GetAllocatedSize());
}

void UDeformMeshComponent::CreateMeshSection(int32 SectionIndex, UStaticMesh* Mesh, const FTransform& Transform)
{
	// Ensure sections array is long enough
	if (SectionIndex >= DeformMeshSections.Num())
	{
		DeformMeshSections.SetNum(SectionIndex + 1, false);
	}

	// Reset this section (in case it already existed)
	FDeformMeshSection& NewSection = DeformMeshSections[SectionIndex];
	NewSection.Reset();
	
	// Fill in the mesh section with the needed data
	// I'm assuming that the StaticMesh has only one section and I'm only using that
	NewSection.StaticMesh = Mesh;
	NewSection.DeformTransform = Transform.ToMatrixWithScale().GetTransposed();
	
	//Update the local bound using the bounds of the static mesh that we're adding
	//I'm not taking in consideration the deformation here, if the deformation cause the mesh to go outside its bounds
	NewSection.StaticMesh->CalculateExtendedBounds();
	NewSection.SectionLocalBox += NewSection.StaticMesh->GetBoundingBox();

	//Add this sections' material to the list of the component's materials, with the same index as the section
	SetMaterial(SectionIndex, NewSection.StaticMesh->GetMaterial(0));


	UpdateLocalBounds(); // Update overall bounds
	MarkRenderStateDirty(); // New section requires recreating scene proxy
}

/// <summary>
/// Update the Transform Matrix that we use to deform the mesh
/// The update of the state in the game thread is simple, but for the scene proxy update, we need to enqueue a render command
/// </summary>
/// <param name="SectionIndex"> The index for the section that we want to update its DeformTransform </param>
/// <param name="Transform"> The new Transform Matrix </param>
void UDeformMeshComponent::UpdateMeshSectionTransform(int32 SectionIndex, const FTransform& Transform)
{
	if (SectionIndex < DeformMeshSections.Num())
	{
		//Set game thread state
		const FMatrix TransformMatrix = Transform.ToMatrixWithScale().GetTransposed();
		DeformMeshSections[SectionIndex].DeformTransform = TransformMatrix;
		DeformMeshSections[SectionIndex].SectionLocalBox += DeformMeshSections[SectionIndex].StaticMesh->GetBoundingBox().TransformBy(Transform);
	
		if (SceneProxy)
		{
			// Enqueue command to modify render thread info
			FDeformMeshSceneProxy* DeformMeshSceneProxy = (FDeformMeshSceneProxy*)SceneProxy;
			ENQUEUE_RENDER_COMMAND(FDeformMeshTransformsUpdate)(
				[DeformMeshSceneProxy, SectionIndex, TransformMatrix](FRHICommandListImmediate& RHICmdList)
				{
					DeformMeshSceneProxy->UpdateDeformTransform_RenderThread(SectionIndex, TransformMatrix);
				});
		}
		UpdateLocalBounds();		 // Update overall bounds
		MarkRenderTransformDirty();  // Need to send new bounds to render thread
	}
}

void UDeformMeshComponent::FinishTransformsUpdate()
{
}

void UDeformMeshComponent::ClearMeshSection(int32 SectionIndex)
{
}

void UDeformMeshComponent::ClearAllMeshSections()
{
}

void UDeformMeshComponent::SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility)
{
}

bool UDeformMeshComponent::IsMeshSectionVisible(int32 SectionIndex) const
{
	return false;
}

int32 UDeformMeshComponent::GetNumSections() const
{
	return int32();
}

FDeformMeshSection* UDeformMeshComponent::GetDeformMeshSection(int32 SectionIndex)
{
	return nullptr;
}

void UDeformMeshComponent::SetDeformMeshSection(int32 SectionIndex, const FDeformMeshSection& Section)
{
}

FPrimitiveSceneProxy* UDeformMeshComponent::CreateSceneProxy()
{
	if (!SceneProxy)
		return new FDeformMeshSceneProxy(this);
	else
		return SceneProxy;
}

int32 UDeformMeshComponent::GetNumMaterials() const
{
	return DeformMeshSections.Num();
}

FBoxSphereBounds UDeformMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds Ret(LocalBounds.TransformBy(LocalToWorld));

	Ret.BoxExtent *= BoundsScale;
	Ret.SphereRadius *= BoundsScale;

	return Ret;
}

void UDeformMeshComponent::UpdateLocalBounds()
{
	FBox LocalBox(ForceInit);

	for (const FDeformMeshSection& Section : DeformMeshSections)
	{
		LocalBox += Section.SectionLocalBox;
	}
	
	LocalBounds = LocalBox.IsValid ? FBoxSphereBounds(LocalBox) : FBoxSphereBounds(FVector(0, 0, 0), FVector(0, 0, 0), 0); // fallback to reset box sphere bounds
	
	// Update global bounds
	UpdateBounds();
	// Need to send to render thread
	MarkRenderTransformDirty();
}
