// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"
#include "Components/WacomRunEncounterSceneBindingComponent.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Snapshots/BattleSnapshot.h"
#include "Testing/WacomEncounterSceneLifecycleAutomationTestView.h"
#include "Testing/WacomEnemySceneRuntimeAutomationTestView.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "PaperFlipbook.h"

namespace WacomEncounterSceneLifecycleSpec
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

	UEnemyDefinition* MakeEnemyDefinition(
		UObject* Outer,
		const FName EnemyId,
		const FName PartId)
	{
		UEnemyPartDefinition* PartDefinition =
			NewObject<UEnemyPartDefinition>(Outer);
		PartDefinition->PartId = PartId;
		PartDefinition->MaxHp = 10;
		UEnemyDefinition* EnemyDefinition =
			NewObject<UEnemyDefinition>(Outer);
		EnemyDefinition->EnemyId = EnemyId;
		FEnemyPartSlot& Slot =
			EnemyDefinition->Parts.AddDefaulted_GetRef();
		Slot.PartSlotId = TEXT("Body");
		Slot.PartDef = PartDefinition;
		return EnemyDefinition;
	}

	AWacomBattleEnemyActor* SpawnHost(
		UWorld& World,
		UEnemyDefinition& EnemyDefinition,
		const FName EnemySlotId,
		UWacomBattleEnemyPartComponent*& OutPart,
		UWacomBattleEnemyPartFlipbookLayerComponent*& OutVisual)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		AWacomBattleEnemyActor* Host =
			World.SpawnActor<AWacomBattleEnemyActor>(
				AWacomBattleEnemyActor::StaticClass(),
				FTransform::Identity,
				SpawnParams);
		if (!Host)
		{
			return nullptr;
		}
		Host->EnemyDefinition = &EnemyDefinition;
		Host->EnemySlotId = EnemySlotId;
		OutPart = NewObject<UWacomBattleEnemyPartComponent>(
			Host, TEXT("Part_Body"), RF_Transient);
		Host->AddInstanceComponent(OutPart);
		OutPart->SetupAttachment(Host->GetRootComponent());
		OutPart->PartSlotId = TEXT("Body");
		OutPart->SetDerivedPartId(
			EnemyDefinition.Parts[0].PartDef->PartId);
		OutPart->RegisterComponent();

		OutVisual =
			NewObject<UWacomBattleEnemyPartFlipbookLayerComponent>(
				Host, TEXT("Visual_Body_Main"), RF_Transient);
		Host->AddInstanceComponent(OutVisual);
		OutVisual->SetupAttachment(OutPart);
		OutVisual->LayerId = TEXT("Lifecycle.Body.Main");
		OutVisual->SetFlipbook(
			NewObject<UPaperFlipbook>(Host, NAME_None, RF_Transient));
		OutVisual->RegisterComponent();
		Host->NotifyEnemySceneComponentTopologyChanged();
		return Host;
	}

	void PrimeRuntimePart(
		AWacomBattleEnemyActor& Host,
		UWacomBattleEnemyPartComponent& Part,
		const FName EncounterId,
		const FName EnemySlotId)
	{
		FEnemyPartSnapshot PartSnapshot;
		PartSnapshot.InstanceId = FGuid::NewGuid();
		PartSnapshot.EncounterId = EncounterId;
		PartSnapshot.EnemySlotId = EnemySlotId;
		PartSnapshot.PartSlotId = Part.PartSlotId;
		PartSnapshot.CurrentHp = 10;
		PartSnapshot.MaxHp = 10;
		FEnemySnapshot EnemySnapshot;
		EnemySnapshot.EncounterId = EncounterId;
		EnemySnapshot.EnemySlotId = EnemySlotId;
		EnemySnapshot.Parts = { PartSnapshot };
		FBattleSnapshot Snapshot;
		Snapshot.EncounterId = EncounterId;
		Snapshot.Enemies = { EnemySnapshot };
		FWacomEnemySceneRuntimeAutomationTestView::InitializeBinding(
			Host, EncounterId, EnemySlotId);
		FWacomEnemySceneRuntimeAutomationTestView::SyncPart(
			Host, Part, Snapshot);
		FWacomEnemySceneRuntimeAutomationTestView::
			SetRegisteredAndTargetable(Host, Part, true, true);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEncounterSceneLifecycleRetiresMappedHostsAfterBarrierSpec,
	"Wacom.App.RunEncounter.SceneLifecycle.RetiresMappedHostsAfterBarrier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEncounterSceneLifecycleRetiresMappedHostsAfterBarrierSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomEncounterSceneLifecycleSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomRunMapNodeAnchorActor* Anchor =
		World->SpawnActor<AWacomRunMapNodeAnchorActor>(
			AWacomRunMapNodeAnchorActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	UWacomRunEncounterSceneBindingComponent* Binding =
		Anchor
			? NewObject<UWacomRunEncounterSceneBindingComponent>(
				Anchor, TEXT("EncounterSceneBinding"), RF_Transient)
			: nullptr;
	UWacomBattleEnemyPartComponent* Part = nullptr;
	UWacomBattleEnemyPartFlipbookLayerComponent* Visual = nullptr;
	UWacomBattleEnemyPartComponent* ExtraPart = nullptr;
	UWacomBattleEnemyPartFlipbookLayerComponent* ExtraVisual = nullptr;
	AWacomBattleEnemyActor* Host = nullptr;
	AWacomBattleEnemyActor* ExtraHost = nullptr;
	ON_SCOPE_EXIT
	{
		for (AActor* Actor : TArray<AActor*>{ Host, ExtraHost, Anchor })
		{
			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}
	};
	if (!TestNotNull(TEXT("Encounter Anchor"), Anchor)
		|| !TestNotNull(TEXT("Encounter Binding"), Binding))
	{
		return false;
	}
	Anchor->NodeId = TEXT("Encounter.SceneLifecycle");
	Anchor->AddInstanceComponent(Binding);
	Binding->RegisterComponent();

	UEnemyDefinition* Enemy = MakeEnemyDefinition(
		Anchor, TEXT("Enemy.Lifecycle"), TEXT("Part.Lifecycle.Body"));
	Host = SpawnHost(*World, *Enemy, TEXT("Enemy"), Part, Visual);
	ExtraHost = SpawnHost(
		*World, *Enemy, TEXT("Extra"), ExtraPart, ExtraVisual);
	if (!TestNotNull(TEXT("Mapped Host"), Host)
		|| !TestNotNull(TEXT("Mapped Part"), Part)
		|| !TestNotNull(TEXT("Mapped Visual"), Visual)
		|| !TestNotNull(TEXT("Unbound Host"), ExtraHost))
	{
		return false;
	}
	PrimeRuntimePart(
		*Host, *Part, TEXT("Encounter.SceneLifecycle"), TEXT("Enemy"));
	const uint32 TopologyRevision =
		Host->GetEnemySceneComponentTopologyRevision();
	UWacomBattleEnemyPartFlipbookLayerComponent* const OriginalVisual =
		Visual;

	UEncounterDefinition* Encounter =
		NewObject<UEncounterDefinition>(Anchor);
	Encounter->EncounterDefinitionId = TEXT("Encounter.SceneLifecycle");
	FEncounterEnemySlot& EncounterSlot =
		Encounter->EnemySlots.AddDefaulted_GetRef();
	EncounterSlot.EnemySlotId = TEXT("Enemy");
	EncounterSlot.EnemyDefinition = Enemy;
	FWacomBattleSceneEnemyHostSlot& HostSlot =
		Binding->SceneEnemyHostSlots.AddDefaulted_GetRef();
	HostSlot.EnemySlotId = TEXT("Enemy");
	HostSlot.SceneEnemyHost = Host;

	Binding->BeginResolvedEncounterSceneRetirement();
	TestTrue(TEXT("Begin marks retirement pending"),
		Binding->IsRetirementPending());
	TestFalse(TEXT("Begin keeps mapped Host visible"), Host->IsHidden());
	const FWacomBattleEnemyPartRuntimeDebugView ActiveView =
		Part->GetRuntimeDebugView();
	TestFalse(TEXT("Begin keeps Part runtime active"),
		ActiveView.bRuntimeRetired);

	Binding->CompleteResolvedEncounterSceneRetirement(*Encounter);
	TestTrue(TEXT("Binding records completed retirement"),
		Binding->IsRetirementCompleted());
	TestTrue(TEXT("Mapped Host is retired"),
		Host->IsRuntimeEncounterPresentationRetired());
	TestTrue(TEXT("Mapped Host is hidden"), Host->IsHidden());
	TestFalse(TEXT("Mapped Host collision is disabled"),
		Host->GetActorEnableCollision());
	const FWacomBattleEnemyPartRuntimeDebugView RetiredView =
		Part->GetRuntimeDebugView();
	TestTrue(TEXT("Part runtime is retired"), RetiredView.bRuntimeRetired);
	TestFalse(TEXT("Part binding is cleared"),
		RetiredView.bBoundToSnapshot);
	TestFalse(TEXT("Unbound Host is not retired"),
		ExtraHost->IsRuntimeEncounterPresentationRetired());
	TestEqual(TEXT("Visual component is preserved"),
		Visual, OriginalVisual);
	TestEqual(TEXT("Topology revision is unchanged"),
		Host->GetEnemySceneComponentTopologyRevision(),
		TopologyRevision);

	Binding->CompleteResolvedEncounterSceneRetirement(*Encounter);
	TestTrue(TEXT("Repeated retirement is idempotent"),
		Host->IsRuntimeEncounterPresentationRetired());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEncounterSceneLifecycleRetirementPolicySpec,
	"Wacom.App.RunEncounter.SceneLifecycle.RetirementPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEncounterSceneLifecycleRetirementPolicySpec::RunTest(
	const FString& /*Parameters*/)
{
	using FPolicy = FWacomEncounterSceneLifecycleAutomationTestView;
	TestTrue(TEXT("Victory retires resolved Encounter"),
		FPolicy::ShouldRetireResolvedEncounterScene(
			true, EBattleOutcome::Victory, false));
	TestFalse(TEXT("Withdraw preserves Encounter scene"),
		FPolicy::ShouldRetireResolvedEncounterScene(
			true, EBattleOutcome::Victory, true));
	TestFalse(TEXT("Defeat preserves Encounter scene"),
		FPolicy::ShouldRetireResolvedEncounterScene(
			true, EBattleOutcome::Defeat, false));
	TestFalse(TEXT("Settlement failure preserves Encounter scene"),
		FPolicy::ShouldRetireResolvedEncounterScene(
			false, EBattleOutcome::Victory, false));
	return true;
}
