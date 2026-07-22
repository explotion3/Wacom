// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Components/WidgetComponent.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "Snapshots/BattleSnapshot.h"
#include "Testing/WacomEnemySceneRuntimeAutomationTestView.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomEnemySceneRuntimePerformanceSpec
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

	UWacomBattleEnemyPartComponent* AddPart(
		AWacomBattleEnemyActor& Host,
		FName PartSlotId,
		FName PartId)
	{
		UWacomBattleEnemyPartComponent* Part = NewObject<UWacomBattleEnemyPartComponent>(
			&Host, TEXT("Part_Performance"), RF_Transient | RF_Transactional);
		Host.AddInstanceComponent(Part);
		Part->CreationMethod = EComponentCreationMethod::Instance;
		Part->SetupAttachment(Host.GetRootComponent());
		Part->PartSlotId = PartSlotId;
		Part->SetDerivedPartId(PartId);
		Part->RegisterComponent();
		Host.NotifyEnemySceneComponentTopologyChanged();
		return Part;
	}

	FBattleSnapshot BuildSnapshot(
		const UEnemyDefinition& EnemyDefinition,
		const UEnemyPartDefinition& PartDefinition,
		const FGuid& InstanceId)
	{
		FBattleSnapshot Snapshot;
		Snapshot.Version = 7;
		Snapshot.EncounterId = TEXT("Encounter.Performance");
		Snapshot.Phase = EBattlePhase::PlayerAction;
		FEnemySnapshot& Enemy = Snapshot.Enemies.AddDefaulted_GetRef();
		Enemy.Definition = &EnemyDefinition;
		Enemy.EncounterId = Snapshot.EncounterId;
		Enemy.EnemySlotId = TEXT("Enemy");
		FEnemyPartSnapshot& Part = Enemy.Parts.AddDefaulted_GetRef();
		Part.InstanceId = InstanceId;
		Part.Definition = &PartDefinition;
		Part.EncounterId = Snapshot.EncounterId;
		Part.EnemySlotId = Enemy.EnemySlotId;
		Part.PartSlotId = TEXT("Body");
		Part.Identity = FBattlePartSlotIdentity::Make(
			Part.EncounterId, Part.EnemySlotId, Part.PartSlotId);
		Part.CurrentInitiative = 3;
		Part.CurrentIntentId = TEXT("Performance.Attack");
		Part.CurrentHp = 12;
		Part.MaxHp = 12;
		return Snapshot;
	}

	int32 CountWidgetComponents(const AWacomBattleEnemyActor& Host)
	{
		TInlineComponentArray<UWidgetComponent*> Components;
		Host.GetComponents(Components);
		return Components.Num();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEnemySceneRuntimeSnapshotAndTargetPreviewSpec,
	"Wacom.UI.Battle.EnemyScene.RuntimePerformance.SnapshotAndTargetPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEnemySceneRuntimeSnapshotAndTargetPreviewSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomEnemySceneRuntimePerformanceSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	TStrongObjectPtr<UEnemyPartDefinition> PartDefinition(NewObject<UEnemyPartDefinition>(
		GetTransientPackage(), NAME_None, RF_Transient));
	PartDefinition->PartId = TEXT("Performance.Body");
	PartDefinition->MaxHp = 12;
	TStrongObjectPtr<UEnemyDefinition> EnemyDefinition(NewObject<UEnemyDefinition>(
		GetTransientPackage(), NAME_None, RF_Transient));
	EnemyDefinition->EnemyId = TEXT("Enemy.Performance");
	FEnemyPartSlot& Slot = EnemyDefinition->Parts.AddDefaulted_GetRef();
	Slot.PartSlotId = TEXT("Body");
	Slot.PartDef = PartDefinition.Get();

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

	Host->EnemyDefinition = EnemyDefinition.Get();
	Host->EnemySlotId = TEXT("Enemy");
	UWacomBattleEnemyPartComponent* Part = AddPart(
		*Host, TEXT("Body"), PartDefinition->PartId);
	if (!TestNotNull(TEXT("Enemy Part"), Part))
	{
		return false;
	}

	FWacomEnemySceneRuntimeAutomationTestView::InitializeBinding(
		*Host, TEXT("Encounter.Performance"), TEXT("Enemy"));
	const FBattleSnapshot Snapshot = BuildSnapshot(
		*EnemyDefinition, *PartDefinition, FGuid::NewGuid());
	TestTrue(TEXT("First snapshot binds the part"),
		FWacomEnemySceneRuntimeAutomationTestView::SyncPart(*Host, *Part, Snapshot));
	FWacomEnemySceneRuntimeAutomationTestView::SetRegisteredAndTargetable(
		*Host, *Part, true, true);

	const FWacomBattleEnemyPartRuntimeDebugView First = Part->GetRuntimeDebugView();
	TestEqual(TEXT("First snapshot applies once"), First.SnapshotApplyCount, 1);
	const int32 InitialWidgetComponentCount = CountWidgetComponents(*Host);

	TestTrue(TEXT("Repeated snapshot remains bound"),
		FWacomEnemySceneRuntimeAutomationTestView::SyncPart(*Host, *Part, Snapshot));
	const FWacomBattleEnemyPartRuntimeDebugView Repeated = Part->GetRuntimeDebugView();
	TestEqual(TEXT("Repeated facts do not apply again"), Repeated.SnapshotApplyCount, 1);
	TestEqual(TEXT("Repeated facts hit the no-op path"), Repeated.SnapshotNoOpCount, 1);

	FWacomEnemySceneRuntimeAutomationTestView::SetHoverPrediction(*Host, *Part);
	TestEqual(TEXT("Hover does not create an independent prediction widget component"),
		CountWidgetComponents(*Host), InitialWidgetComponentCount);

	FWacomEnemySceneRuntimeAutomationTestView::SetHoverPrediction(*Host, *Part);
	TestEqual(TEXT("Repeated hover keeps the scene component topology stable"),
		CountWidgetComponents(*Host), InitialWidgetComponentCount);

	FWacomEnemySceneRuntimeAutomationTestView::ClearHoverPrediction(*Host, *Part);
	TestEqual(TEXT("Clearing hover keeps the scene component topology stable"),
		CountWidgetComponents(*Host), InitialWidgetComponentCount);

	FWacomEnemySceneRuntimeAutomationTestView::SetDragTargetPreview(
		*Host, *Part, EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	TestEqual(TEXT("Drag target feedback remains Niagara-only"),
		CountWidgetComponents(*Host), InitialWidgetComponentCount);
	FWacomEnemySceneRuntimeAutomationTestView::ClearDragTargetPreview(*Host, *Part);

	Host->RetireRuntimeEncounterPresentation();
	TestEqual(TEXT("Retirement has no prediction widget component to destroy"),
		CountWidgetComponents(*Host), InitialWidgetComponentCount);
	return true;
}
