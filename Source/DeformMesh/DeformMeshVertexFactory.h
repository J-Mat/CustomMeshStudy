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

class FDeformMeshSceneProxy;

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
	
	/* Modify compilation environment so we can control which parts of the shader file are taken in consideration by the shader compiler */
	/* This is the equivaalent to manually setting preprocessor directives, so when compilation happens, only the code that we're interested in gets in the compiled shader*/
	/* Check LocalVertexFactory.ush */
	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

	/* This is the main method that we're interested in*/
	/* Here we can initialize our RHI resources, so we can decide what would be in the final streams and the vertex declaration*/
	/* In the LocalVertexFactory, 3 vertex declarations are initialized; PositionOnly, PositionAndNormalOnly, and the default, which is the one that will be used in the main rendering*/
	/* PositionOnly is mandatory if you're enabling depth passes, however we can get rid of the PositionAndNormal since we're not interested in shading and we're only supporting unlit materials*/
	virtual void InitRHI() override;


	/* No need to override the ReleaseRHI() method, since we're not crearting any additional resources*/
	/* The base FVertexFactory::ReleaseRHI() will empty the 3 vertex streams and release the 3 vertex declarations (Probably just decrement the ref count since a declaration is cached and can be used by multiple vertex factories)*/

	void SetTransformIndex(uint16 Index) { TransformIndex = Index; }
	void SetSceneProxy(FDeformMeshSceneProxy* Proxy) { SceneProxy = Proxy; }

private:
	//We need to pass this as a shader parameter, so we store it in the vertex factory and we use in the vertex factory shader parameters
	uint16 TransformIndex;
	//All the mesh sections proxies keep a pointer to the scene proxy of the component so they can access the unified SRV
	FDeformMeshSceneProxy* SceneProxy;

	friend class FDeformMeshVertexFactoryShaderParameters;
};

///////////////////////////////////////////////////////////////////////
// The DeformMesh Vertex factory shader parameters
/*
 * We can bind shader parameters here
 * There's two types of shader parameters: FShaderPrameter and FShaderResourcePramater
 * We can use the first to pass parameters like floats, integers, arrays
 * We can use the second to pass shader resources bindings, for example Structured Buffer, texture, samplerstate, etc
 * Actually that's how manual fetch is implmented; for each of the Vertex Buffers of the stream components, an SRV is created
 * That SRV can bound as a shader resource parameter and you can fetch the buffers using the SV_VertexID
*/
///////////////////////////////////////////////////////////////////////
