// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunTunnelPaperLayerActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AWacomRunTunnelPaperLayerActor::AWacomRunTunnelPaperLayerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	PaperPlaneComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PaperPlane"));
	SetRootComponent(PaperPlaneComponent);

	PaperPlaneComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PaperPlaneComponent->SetGenerateOverlapEvents(false);
	PaperPlaneComponent->SetCastShadow(false);
	PaperPlaneComponent->bCastDynamicShadow = false;
	PaperPlaneComponent->bCastStaticShadow = false;
	PaperPlaneComponent->SetMobility(EComponentMobility::Static);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		PaperPlaneComponent->SetStaticMesh(PlaneMesh.Object);
	}
}

void AWacomRunTunnelPaperLayerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bApplyMaterialOnConstruction)
	{
		RefreshPaperLayerMaterial();
	}
}

void AWacomRunTunnelPaperLayerActor::BeginPlay()
{
	Super::BeginPlay();

	RefreshPaperLayerMaterial();
}

void AWacomRunTunnelPaperLayerActor::RefreshPaperLayerMaterial()
{
	LastAppliedTextureIndex = INDEX_NONE;
	DynamicPaperMaterial = nullptr;

	if (!PaperPlaneComponent)
	{
		return;
	}

	UMaterialInterface* SourceMaterial = PaperMaterial;
	if (!SourceMaterial)
	{
		SourceMaterial = PaperPlaneComponent->GetMaterial(0);
	}

	if (!SourceMaterial)
	{
		return;
	}

	DynamicPaperMaterial = PaperPlaneComponent->CreateDynamicMaterialInstance(0, SourceMaterial);
	if (!DynamicPaperMaterial || TextureParameterName.IsNone())
	{
		return;
	}

	const int32 TextureIndex = ResolveTextureIndex();
	if (!PaperTextures.IsValidIndex(TextureIndex) || !PaperTextures[TextureIndex])
	{
		return;
	}

	DynamicPaperMaterial->SetTextureParameterValue(TextureParameterName, PaperTextures[TextureIndex]);
	LastAppliedTextureIndex = TextureIndex;
}

void AWacomRunTunnelPaperLayerActor::RerollTextureSelection()
{
	if (FixedTextureIndex < 0)
	{
		++TextureSelectionSeed;
	}

	RefreshPaperLayerMaterial();
}

int32 AWacomRunTunnelPaperLayerActor::ResolveTextureIndex() const
{
	if (PaperTextures.IsEmpty())
	{
		return INDEX_NONE;
	}

	if (FixedTextureIndex >= 0)
	{
		return PaperTextures.IsValidIndex(FixedTextureIndex) && PaperTextures[FixedTextureIndex]
			? FixedTextureIndex
			: INDEX_NONE;
	}

	TArray<int32, TInlineAllocator<8>> ValidIndices;
	for (int32 Index = 0; Index < PaperTextures.Num(); ++Index)
	{
		if (PaperTextures[Index])
		{
			ValidIndices.Add(Index);
		}
	}

	if (ValidIndices.IsEmpty())
	{
		return INDEX_NONE;
	}

	FRandomStream Stream(BuildStableRandomSeed());
	const int32 Pick = Stream.RandRange(0, ValidIndices.Num() - 1);
	return ValidIndices[Pick];
}

int32 AWacomRunTunnelPaperLayerActor::BuildStableRandomSeed() const
{
	uint32 Seed = static_cast<uint32>(TextureSelectionSeed);
	if (bSaltRandomWithActorName)
	{
		Seed = HashCombine(Seed, GetTypeHash(GetFName()));
	}

	return static_cast<int32>(Seed);
}
