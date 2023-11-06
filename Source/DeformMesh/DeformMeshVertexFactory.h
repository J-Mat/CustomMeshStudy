#pragma once

#include "CoreMinimal.h"
#include "VertexFactory.h"
#include "LocalVertexFactory.h"
#include "ShaderParameters.h"
#include "RHIUtilities.h"
#include "RenderingThread.h"
#include "PrimitiveViewRelevance.h"
#include "StaticMeshResources.h"
#include "MeshMaterialShader.h"

class FDeformMeshVertexFactory : public FLocalVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FDeformMeshVertexFactory);
public:
	FDeformMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel);

	/* Should we cache the material's shadertype on this platform with this vertex factory? */
	/* Given these parameters, we can decide which permutations should be compiled for this vertex factory*/
	/* For example, we're only intersted in unlit materials, so we only return true when
	1 Material Domain is Surface
	2 Shading Model is Unlit
	* We also add the permutation for the default material, because if that's not found, the engine would crash
	* That's because the default material is the fallback for all other materials, so it needs to be compiled for all vertex factories
	*/
	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);

	/* This is the main method that we're interested in*/
	/* Here we can initialize our RHI resources, so we can decide what would be in the final streams and the vertex declaration*/
	/* In the LocalVertexFactory, 3 vertex declarations are initialized; PositionOnly, PositionAndNormalOnly, and the default, which is the one that will be used in the main rendering*/
	/* PositionOnly is mandatory if you're enabling depth passes, however we can get rid of the PositionAndNormal since we're not interested in shading and we're only supporting unlit materials*/
	virtual void InitRHI() override;


	void SetTransformIndex(uint16 Index) { TransformIndex = Index; }
	void SetSceneProxy(FDeformMeshSceneProxy* Proxy) { SceneProxy = Proxy; }
};
