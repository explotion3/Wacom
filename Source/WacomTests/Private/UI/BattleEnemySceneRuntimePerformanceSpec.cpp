// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "Snapshots/BattleSnapshot.h"
#include "Testing/WacomEnemySceneRuntimeAutomationTestView.h"
#include "UI/Battle/WacomBattleEnemyPartDragPredictionTypes.h"
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEnemySceneRuntimeSnapshotAndPredictionReuseSpec,
	"Wacom.UI.Battle.EnemyScene.RuntimePerformance.SnapshotAndPredictionReuse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEnemySceneRuntimeSnapshotAndPredictionReuseSpec::RunTest(
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
	TestFalse(TEXT("Hidden prediction does not create a component"),
		First.bPredictionWidgetCreated);

	TestTrue(TEXT("Repeated snapshot remains bound"),
		FWacomEnemySceneRuntimeAutomationTestView::SyncPart(*Host, *Part, Snapshot));
	const FWacomBattleEnemyPartRuntimeDebugView Repeated = Part->GetRuntimeDebugView();
	TestEqual(TEXT("Repeated facts do not apply again"), Repeated.SnapshotApplyCount, 1);
	TestEqual(TEXT("Repeated facts hit the no-op path"), Repeated.SnapshotNoOpCount, 1);

	FWacomBattleEnemyPartDragPredictionDebugInput HoverInput;
	FWacomEnemySceneRuntimeAutomationTestView::SetHoverPrediction(
		*Host, *Part, HoverInput);
	const FWacomBattleEnemyPartRuntimeDebugView Visible = Part->GetRuntimeDebugView();
	TestTrue(TEXT("Visible prediction lazily creates its component"),
		Visible.bPredictionWidgetCreated);
	TestTrue(TEXT("Prediction widget is visible during hover"),
		Visible.bPredictionWidgetVisible);
	TestEqual(TEXT("Prediction component is created once"),
		Visible.PredictionWidgetCreateCount, 1);

	FWacomEnemySceneRuntimeAutomationTestView::SetHoverPrediction(
		*Host, *Part, HoverInput);
	const FWacomBattleEnemyPartRuntimeDebugView Stable = Part->GetRuntimeDebugView();
	TestEqual(TEXT("Identical hover does not recreate prediction"),
		Stable.PredictionWidgetCreateCount, 1);
	TestEqual(TEXT("Identical hover does not reapply prediction"),
		Stable.PredictionWidgetApplyCount, Visible.PredictionWidgetApplyCount);

	FWacomEnemySceneRuntimeAutomationTestView::ClearHoverPrediction(*Host, *Part);
	const FWacomBattleEnemyPartRuntimeDebugView Hidden = Part->GetRuntimeDebugView();
	TestTrue(TEXT("Hidden prediction keeps the reusable component"),
		Hidden.bPredictionWidgetCreated);
	TestFalse(TEXT("Clearing hover hides prediction"), Hidden.bPredictionWidgetVisible);

	FWacomEnemySceneRuntimeAutomationTestView::SetHoverPrediction(
		*Host, *Part, HoverInput);
	const FWacomBattleEnemyPartRuntimeDebugView Reused = Part->GetRuntimeDebugView();
	TestEqual(TEXT("Second hover reuses the same prediction component"),
		Reused.PredictionWidgetCreateCount, 1);

	Host->RetireRuntimeEncounterPresentation();
	const FWacomBattleEnemyPartRuntimeDebugView Retired = Part->GetRuntimeDebugView();
	TestFalse(TEXT("Retirement destroys the transient prediction component"),
		Retired.bPredictionWidgetCreated);
	return true;
}
