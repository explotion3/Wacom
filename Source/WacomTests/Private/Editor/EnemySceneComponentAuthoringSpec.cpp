// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "Authoring/WacomBattleSceneEnemyHostAuthoring.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "PaperFlipbook.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomEnemySceneComponentAuthoringSpec
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
		Fixture.Enemy->EnemyId = TEXT("Enemy.ComponentAuthoring");
		for (const TPair<FName, FName>& Entry : TArray<TPair<FName, FName>>{
			{ TEXT("Head"), TEXT("ComponentAuthoring.Head") },
			{ TEXT("Body"), TEXT("ComponentAuthoring.Body") },
			{ TEXT("Tail"), TEXT("ComponentAuthoring.Tail") } })
		{
			UEnemyPartDefinition* Definition = NewObject<UEnemyPartDefinition>(
				GetTransientPackage(), NAME_None, RF_Transient);
			Definition->PartId = Entry.Value;
			Definition->MaxHp = 10;
			Fixture.Parts.Emplace(Definition);
			FEnemyPartSlot& Slot = Fixture.Enemy->Parts.AddDefaulted_GetRef();
			Slot.PartSlotId = Entry.Key;
			Slot.PartDef = Definition;
		}
		return Fixture;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEnemySceneComponentAuthoringSyncSpec,
	"Wacom.Editor.EnemyScene.ComponentAuthoring.SyncIsTypedAndIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEnemySceneComponentAuthoringSyncSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomEnemySceneComponentAuthoringSpec;
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
	if (!TestNotNull(TEXT("Host"), Host))
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

	const TArray<FWacomBattleSceneEnemyHostSyncResult> First =
		FWacomBattleSceneEnemyHostAuthoring::SyncPartsFromDefinition(
			TArray<AWacomBattleEnemyActor*>{ Host });
	if (!TestEqual(TEXT("One Host result"), First.Num(), 1))
	{
		return false;
	}
	TestTrue(TEXT("First sync changes Host"), First[0].bChanged);
	TestEqual(TEXT("Three Parts added"), First[0].AddedPartSlotIds.Num(), 3);
	TArray<UWacomBattleEnemyPartComponent*> Parts = Host->GetBattleEnemyPartComponents();
	TestEqual(TEXT("Three typed Part components"), Parts.Num(), 3);
	for (UWacomBattleEnemyPartComponent* Part : Parts)
	{
		if (!TestNotNull(TEXT("Part component"), Part))
		{
			continue;
		}
		const FWacomBattleEnemyPartRuntimeDebugView View = Part->GetRuntimeDebugView();
		TestEqual(TEXT("One default direct Flipbook layer"), View.FlipbookLayerCount, 1);
		TestEqual(TEXT("One default direct ImpactAnchor"), View.ImpactAnchorCount, 1);
		TArray<UWacomBattleEnemyPartFlipbookLayerComponent*> Layers;
		Host->GetComponents(Layers);
		for (UWacomBattleEnemyPartFlipbookLayerComponent* Layer : Layers)
		{
			if (Layer && Layer->GetAttachParent() == Part)
			{
				Layer->SetFlipbook(NewObject<UPaperFlipbook>(Host, NAME_None, RF_Transient));
			}
		}
	}
	Host->NotifyEnemySceneComponentTopologyChanged();
	FWacomBattleSceneEnemyHostAuthoringReport ReadyReport =
		FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host);
	TestTrue(TEXT("Populated typed hierarchy is Ready"), ReadyReport.bAuthoringReady);
	TestEqual(TEXT("Definition order is retained"), ReadyReport.AttachedPartSlotIds,
		TArray<FName>{ TEXT("Head"), TEXT("Body"), TEXT("Tail") });

	if (Parts.Num() == 3)
	{
		Parts[0]->SetRelativeLocation(FVector(96.0f, -6.0f, 16.0f));
		Parts[0]->SetBoxExtent(FVector(42.0f, 38.0f, 42.0f));
		Parts[0]->SetDerivedPartId(TEXT("Wrong.Head"));
	}
	const FVector LocationBefore = Parts.IsEmpty() ? FVector::ZeroVector : Parts[0]->GetRelativeLocation();
	const FVector ExtentBefore = Parts.IsEmpty() ? FVector::ZeroVector : Parts[0]->GetUnscaledBoxExtent();
	const TArray<FWacomBattleSceneEnemyHostSyncResult> IdentityFix =
		FWacomBattleSceneEnemyHostAuthoring::SyncPartsFromDefinition(
			TArray<AWacomBattleEnemyActor*>{ Host });
	TestEqual(TEXT("One derived PartId fixed"), IdentityFix[0].UpdatedPartSlotIds.Num(), 1);
	if (!Parts.IsEmpty())
	{
		TestEqual(TEXT("Part transform preserved"), Parts[0]->GetRelativeLocation(), LocationBefore);
		TestEqual(TEXT("Box extent preserved"), Parts[0]->GetUnscaledBoxExtent(), ExtentBefore);
	}

	const uint32 RevisionBefore = Host->GetEnemySceneComponentTopologyRevision();
	const int32 ComponentCountBefore = Host->GetComponents().Num();
	const TArray<FWacomBattleSceneEnemyHostSyncResult> Second =
		FWacomBattleSceneEnemyHostAuthoring::SyncPartsFromDefinition(
			TArray<AWacomBattleEnemyActor*>{ Host });
	TestFalse(TEXT("Second sync is idempotent"), Second[0].bChanged);
	TestEqual(TEXT("Second sync creates no components"),
		Host->GetComponents().Num(), ComponentCountBefore);
	TestEqual(TEXT("Second sync does not change topology revision"),
		Host->GetEnemySceneComponentTopologyRevision(), RevisionBefore);
	return true;
}
