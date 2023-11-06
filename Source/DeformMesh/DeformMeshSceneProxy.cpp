#include "DeformMeshSceneProxy.h"
#include "DeformMeshComponent.h"


FDeformMeshSceneProxy::FDeformMeshSceneProxy(UDeformMeshComponent* Component)
	: FPrimitiveSceneProxy(Component)
	, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
{
}
