#include "DeformMeshVertexFactory.h"
#include "Materials/Material.h"
#include "RHIUtilities.h"


FDeformMeshVertexFactory::FDeformMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
	: FLocalVertexFactory(InFeatureLevel, "FDeformMeshVertexFactory")
{
	//We're not interested in Manual vertex fetch so we disable it 
	bSupportsManualVertexFetch = false;
}

bool FDeformMeshVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	if ((Parameters.MaterialParameters.MaterialDomain == MD_Surface &&
		Parameters.MaterialParameters.ShadingModels == MSM_Unlit) ||
		Parameters.MaterialParameters.bIsDefaultMaterial)
	{
		return true;
	}
	return false;
}

void FDeformMeshVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	const bool ContainsManualVertexFetch = OutEnvironment.GetDefinitions().Contains("MANUAL_VERTEX_FETCH");
	if (!ContainsManualVertexFetch)
	{
		OutEnvironment.SetDefine(TEXT("MANUAL_VERTEX_FETCH"), TEXT("0"));
	}

	OutEnvironment.SetDefine(TEXT("DEFORM_MESH"), TEXT("1"));
}

void FDeformMeshVertexFactory::InitRHI()
{
	// Check if this vertex factory has a valid feature level that is supported by the current platform
	check(HasValidFeatureLevel());
	
	//The vertex declaration element lists (Nothing but an array of FVertexElement)
	FVertexDeclarationElementList Elements; //Used for the Default vertex stream
	FVertexDeclarationElementList PosOnlyElements; // Used for the PositionOnly vertex stream
	
	if (Data.PositionComponent.VertexBuffer != NULL)
	{
		//We add the position stream component to both elemnt lists
		Elements.Add(AccessStreamComponent(Data.PositionComponent, 0));
		PosOnlyElements.Add(AccessStreamComponent(Data.PositionComponent, 0, EVertexInputStreamType::PositionOnly));
	}

	//Initialize the Position Only vertex declaration which will be used in the depth pass
	InitDeclaration(PosOnlyElements, EVertexInputStreamType::PositionOnly);
	
	//We add all the available texcoords to the default element list, that's all what we'll need for unlit shading
	if (Data.TextureCoordinates.Num())
	{
		const int32 BaseTexCoordAttribute = 4;
		for (int32 CoordinateIndex = 0; CoordinateIndex < Data.TextureCoordinates.Num(); CoordinateIndex++)
		{
			Elements.Add(AccessStreamComponent(
				Data.TextureCoordinates[CoordinateIndex],
				BaseTexCoordAttribute + CoordinateIndex
			));
		}

		for (int32 CoordinateIndex = Data.TextureCoordinates.Num(); CoordinateIndex < MAX_STATIC_TEXCOORDS / 2; CoordinateIndex++)
		{
			Elements.Add(AccessStreamComponent(
				Data.TextureCoordinates[Data.TextureCoordinates.Num() - 1],
				BaseTexCoordAttribute + CoordinateIndex
			));
		}
	}

	check(Streams.Num() > 0);

	InitDeclaration(Elements);
	check(IsValidRef(GetDeclaration()));
}
