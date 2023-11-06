#include "DeformMeshVertexFactory.h"
#include "Materials/Material.h"

FDeformMeshVertexFactory::FDeformMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
	: FLocalVertexFactory(InFeatureLevel, "FDeformMeshVertexFactory")
{
}

bool FDeformMeshVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	return false;
}

void FDeformMeshVertexFactory::InitRHI()
{
}
