#include "DeformMeshComponent.h"


FPrimitiveSceneProxy* UDeformMeshComponent::CreateSceneProxy()
{
    return nullptr;
}

int32 UDeformMeshComponent::GetNumMaterials() const
{
    return int32();
}
