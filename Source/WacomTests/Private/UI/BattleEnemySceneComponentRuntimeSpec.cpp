// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Components/SceneComponent.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"
#include "Components/WacomBattleEnemyPartImpactAnchorComponent.h"
#include "Components/WacomBattleEnemyPartSpriteLayerComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomEnemySceneComponentRuntimeSpec
{
	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	struct FDefinitionFixture
	{
		TStrongObjectPtr<UEnemyDefinition> Enemy;
		TArray<TStrongObjectPtr<UEnemyPartDefinition>> Parts;
	};

	FDefinitionFixture MakeDefinition()
	{
		FDefinitionFixture Fixture;
		Fixture.Enemy.Reset(NewObject<UEnemyDefinition>(GetTransientPackage(), NAME_None, RF_Transient));
		Fixture.Enemy->EnemyId = TEXT("Enemy.ComponentRuntime");
		for (const TPair<FName, FName>& Entry : TArray<TPair<FName, FName>>{
			{ TEXT("Left"), TEXT("ComponentRuntime.Left") },
			{ TEXT("Core"), TEXT("ComponentRuntime.Core") } })
		{
			UEnemyPartDefinition* PartDefinition = NewObject<UEnemyPartDefinition>(
				GetTransientPackage(), NAME_None, RF_Transient);
			PartDefinition->PartId = Entry.Value;
			PartDefinition->MaxHp = 10;
			Fixture.Parts.Emplace(PartDefinition);
			FEnemyPartSlot& Slot = Fixture.Enemy->Parts.AddDefaulted_GetRef();
			Slot.PartSlotId = Entry.Key;
			Slot.PartDef = PartDefinition;
		}
		return Fixture;
	}

	template <typename TComponent>
	TComponent* AddSceneComponent(
		AWacomBattleEnemyActor& Host,
		USceneComponent& Parent,
		FName Name)
	{
		TComponent* Component = NewObject<TComponent>(
			&Host, Name, RF_Transient | RF_Transactional);
		Host.AddInstanceComponent(Component);
		Component->CreationMethod = EComponentCreationMethod::Instance;
		Component->SetupAttachment(&Parent);
		Component->RegisterComponent();
		return Component;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEnemySceneComponentRuntimeHierarchySpec,
	"Wacom.UI.Battle.EnemyScene.ComponentRuntime.TypedHierarchyPreservesAuthoredState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEnemySceneComponentRuntimeHierarchySpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomEnemySceneComponentRuntimeSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FDefinitionFixture Definition = MakeDefinition();
	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host = World->SpawnActor<AWacomBattleEnemyActor>(
		FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("Enemy Host"), Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Host->EnemyDefinition = Definition.Enemy.Get();
	UWacomBattleEnemyPartComponent* Core = AddSceneComponent<UWacomBattleEnemyPartComponent>(
		*Host, *Host->GetRootComponent(), TEXT("Part_Core"));
	UWacomBattleEnemyPartComponent* Left = AddSceneComponent<UWacomBattleEnemyPartComponent>(
		*Host, *Host->GetRootComponent(), TEXT("Part_Left"));
	Core->PartSlotId = TEXT("Core");
	Core->SetDerivedPartId(TEXT("ComponentRuntime.Core"));
	Left->PartSlotId = TEXT("Left");
	Left->SetDerivedPartId(TEXT("ComponentRuntime.Left"));
	Left->SetRelativeLocation(FVector(-88.0f, 8.0f, -6.0f));
	Left->SetBoxExtent(FVector(46.0f, 38.0f, 40.0f));

	UWacomBattleEnemyPartFlipbookLayerComponent* DirectFlipbook =
		AddSceneComponent<UWacomBattleEnemyPartFlipbookLayerComponent>(
			*Host, *Left, TEXT("Visual_Left_Main"));
	DirectFlipbook->LayerId = TEXT("ComponentRuntime.Left.Main");
	DirectFlipbook->SetFlipbook(NewObject<UPaperFlipbook>(Host, NAME_None, RF_Transient));
	DirectFlipbook->SetRelativeLocation(FVector(1.0f, 2.0f, 3.0f));
	DirectFlipbook->SetPlaybackPosition(0.04f, false);
	UWacomBattleEnemyPartImpactAnchorComponent* Anchor =
		AddSceneComponent<UWacomBattleEnemyPartImpactAnchorComponent>(
			*Host, *Left, TEXT("ImpactAnchor_Left"));
	Anchor->SetRelativeLocation(FVector(0.0f, 0.0f, 12.0f));

	USceneComponent* InvalidIntermediate = AddSceneComponent<USceneComponent>(
		*Host, *Left, TEXT("InvalidIntermediate"));
	UWacomBattleEnemyPartSpriteLayerComponent* IndirectSprite =
		AddSceneComponent<UWacomBattleEnemyPartSpriteLayerComponent>(
			*Host, *InvalidIntermediate, TEXT("IndirectVisual"));
	IndirectSprite->LayerId = TEXT("Must.Not.Be.Collected");
	IndirectSprite->SetSprite(NewObject<UPaperSprite>(Host, NAME_None, RF_Transient));

	Host->NotifyEnemySceneComponentTopologyChanged();
	const TArray<UWacomBattleEnemyPartComponent*> OrderedParts =
		Host->GetBattleEnemyPartComponents();
	TestEqual(TEXT("Definition order controls runtime part order"), OrderedParts.Num(), 2);
	if (OrderedParts.Num() == 2)
	{
		TestTrue(TEXT("Left is first despite creation order"), OrderedParts[0] == Left);
		TestTrue(TEXT("Core is second despite creation order"), OrderedParts[1] == Core);
	}

	const FWacomBattleEnemyPartRuntimeDebugView Before = Left->GetRuntimeDebugView();
	TestEqual(TEXT("Only direct flipbook child is collected"), Before.FlipbookLayerCount, 1);
	TestEqual(TEXT("Indirect sprite is rejected from runtime ownership"), Before.SpriteLayerCount, 0);
	TestEqual(TEXT("One direct impact anchor is collected"), Before.ImpactAnchorCount, 1);
	const uint32 RevisionBefore = Host->GetEnemySceneComponentTopologyRevision();
	const FVector PartLocationBefore = Left->GetRelativeLocation();
	const FVector ExtentBefore = Left->GetUnscaledBoxExtent();
	const FVector VisualLocationBefore = DirectFlipbook->GetRelativeLocation();
	const float PlaybackBefore = DirectFlipbook->GetPlaybackPosition();

	Host->NotifyEnemySceneComponentTopologyChanged();
	TestEqual(TEXT("Idempotent refresh keeps topology revision"),
		Host->GetEnemySceneComponentTopologyRevision(), RevisionBefore);
	TestEqual(TEXT("Part viewport transform is preserved"), Left->GetRelativeLocation(), PartLocationBefore);
	TestEqual(TEXT("Hit bounds are preserved"), Left->GetUnscaledBoxExtent(), ExtentBefore);
	TestEqual(TEXT("Visual viewport transform is preserved"),
		DirectFlipbook->GetRelativeLocation(), VisualLocationBefore);
	TestEqual(TEXT("Refresh does not reset flipbook progress"),
		DirectFlipbook->GetPlaybackPosition(), PlaybackBefore);
	return true;
}
