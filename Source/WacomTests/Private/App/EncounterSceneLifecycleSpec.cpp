// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/BattleTriggerActor.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Snapshots/BattleSnapshot.h"
#include "Testing/WacomEncounterSceneLifecycleAutomationTestView.h"
#include "Testing/WacomEnemySceneRuntimeAutomationTestView.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/DataValidation.h"
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
		FName EnemyId,
		FName PartId)
	{
		UEnemyPartDefinition* PartDefinition = NewObject<UEnemyPartDefinition>(Outer);
		PartDefinition->PartId = PartId;
		PartDefinition->MaxHp = 10;
		UEnemyDefinition* EnemyDefinition = NewObject<UEnemyDefinition>(Outer);
		EnemyDefinition->EnemyId = EnemyId;
		FEnemyPartSlot& Slot = EnemyDefinition->Parts.AddDefaulted_GetRef();
		Slot.PartSlotId = TEXT("Body");
		Slot.PartDef = PartDefinition;
		return EnemyDefinition;
	}

	AWacomBattleEnemyActor* SpawnHost(
		UWorld& World,
		UEnemyDefinition& EnemyDefinition,
		FName EnemySlotId,
		UWacomBattleEnemyPartComponent*& OutPart,
		UWacomBattleEnemyPartFlipbookLayerComponent*& OutVisual)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		AWacomBattleEnemyActor* Host = World.SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(), FTransform::Identity, SpawnParams);
		if (!Host)
		{
			return nullptr;
		}
		Host->EnemyDefinition = &EnemyDefinition;
		Host->EnemySlotId = EnemySlotId;
		OutPart = NewObject<UWacomBattleEnemyPartComponent>(Host, TEXT("Part_Body"), RF_Transient);
		Host->AddInstanceComponent(OutPart);
		OutPart->SetupAttachment(Host->GetRootComponent());
		OutPart->PartSlotId = TEXT("Body");
		OutPart->SetDerivedPartId(EnemyDefinition.Parts[0].PartDef->PartId);
		OutPart->RegisterComponent();

		OutVisual = NewObject<UWacomBattleEnemyPartFlipbookLayerComponent>(
			Host, TEXT("Visual_Body_Main"), RF_Transient);
		Host->AddInstanceComponent(OutVisual);
		OutVisual->SetupAttachment(OutPart);
		OutVisual->LayerId = TEXT("Lifecycle.Body.Main");
		OutVisual->SetFlipbook(NewObject<UPaperFlipbook>(Host, NAME_None, RF_Transient));
		OutVisual->RegisterComponent();
		Host->NotifyEnemySceneComponentTopologyChanged();
		return Host;
	}

	void PrimeRuntimePart(
		AWacomBattleEnemyActor& Host,
		UWacomBattleEnemyPartComponent& Part,
		FName EncounterId,
		FName EnemySlotId)
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
		FWacomEnemySceneRuntimeAutomationTestView::SyncPart(Host, Part, Snapshot);
		FWacomEnemySceneRuntimeAutomationTestView::SetRegisteredAndTargetable(
			Host, Part, true, true);
	}

	bool IssuesContain(const TArray<FText>& Issues, const TCHAR* Expected)
	{
		return Issues.ContainsByPredicate([Expected](const FText& Issue)
		{
			return Issue.ToString().Contains(Expected);
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEncounterSceneLifecycleRetiresMappedHostsAfterBarrierSpec,
	"Wacom.App.BattleTrigger.EncounterSceneLifecycle.RetiresComponentHostsAfterBarrier",
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
	ABattleTriggerActor* Trigger = World->SpawnActor<ABattleTriggerActor>(
		ABattleTriggerActor::StaticClass(), FTransform::Identity, SpawnParams);
	UWacomBattleEnemyPartComponent* Part = nullptr;
	UWacomBattleEnemyPartFlipbookLayerComponent* Visual = nullptr;
	UWacomBattleEnemyPartComponent* ExtraPart = nullptr;
	UWacomBattleEnemyPartFlipbookLayerComponent* ExtraVisual = nullptr;
	AWacomBattleEnemyActor* Host = nullptr;
	AWacomBattleEnemyActor* ExtraHost = nullptr;
	ON_SCOPE_EXIT
	{
		for (AActor* Actor : TArray<AActor*>{ Host, ExtraHost, Trigger })
		{
			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}
	};
	if (!TestNotNull(TEXT("Battle Trigger"), Trigger))
	{
		return false;
	}
	Trigger->PersistentId = TEXT("Trigger.SceneLifecycle");
	UEnemyDefinition* Enemy = MakeEnemyDefinition(
		Trigger, TEXT("Enemy.Lifecycle"), TEXT("Part.Lifecycle.Body"));
	Host = SpawnHost(*World, *Enemy, TEXT("Enemy"), Part, Visual);
	ExtraHost = SpawnHost(*World, *Enemy, TEXT("Extra"), ExtraPart, ExtraVisual);
	if (!TestNotNull(TEXT("Mapped Host"), Host)
		|| !TestNotNull(TEXT("Mapped Part"), Part)
		|| !TestNotNull(TEXT("Mapped Visual"), Visual)
		|| !TestNotNull(TEXT("Extra Host"), ExtraHost))
	{
		return false;
	}
	PrimeRuntimePart(*Host, *Part, Trigger->PersistentId, TEXT("Enemy"));
	const uint32 TopologyRevision = Host->GetEnemySceneComponentTopologyRevision();
	UWacomBattleEnemyPartFlipbookLayerComponent* const OriginalVisual = Visual;

	UEncounterDefinition* Encounter = NewObject<UEncounterDefinition>(Trigger);
	Encounter->EncounterDefinitionId = TEXT("Encounter.SceneLifecycle");
	FEncounterEnemySlot& EncounterSlot = Encounter->EnemySlots.AddDefaulted_GetRef();
	EncounterSlot.EnemySlotId = TEXT("Enemy");
	EncounterSlot.EnemyDefinition = Enemy;
	Trigger->EncounterDefinition = Encounter;
	FWacomBattleSceneEnemyHostSlot HostSlot;
	HostSlot.EnemySlotId = TEXT("Enemy");
	HostSlot.SceneEnemyHost = Host;
	FWacomBattleSceneEnemyHostSlot ExtraSlot;
	ExtraSlot.EnemySlotId = TEXT("Extra");
	ExtraSlot.SceneEnemyHost = ExtraHost;
	Trigger->SceneEnemyHostSlots = { HostSlot, ExtraSlot };

	Trigger->BeginResolvedEncounterSceneRetirement();
	TestTrue(TEXT("Begin marks retirement pending"),
		Trigger->GetBattleTriggerDebugView(nullptr).bResolvedSceneRetirementPending);
	TestFalse(TEXT("Begin disables Trigger collision"), Trigger->GetActorEnableCollision());
	TestFalse(TEXT("Begin keeps mapped Host visible"), Host->IsHidden());
	TestFalse(TEXT("Begin keeps Part target enabled"),
		Part->GetCollisionEnabled() == ECollisionEnabled::NoCollision);

	Trigger->CompleteResolvedEncounterSceneRetirement();
	TestTrue(TEXT("Mapped Host is retired"), Host->IsRuntimeEncounterPresentationRetired());
	TestTrue(TEXT("Mapped Host is hidden"), Host->IsHidden());
	TestFalse(TEXT("Mapped Host collision is disabled"), Host->GetActorEnableCollision());
	TestEqual(TEXT("Part collision is disabled"),
		Part->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	const FWacomBattleEnemyPartRuntimeDebugView RetiredView = Part->GetRuntimeDebugView();
	TestTrue(TEXT("Part runtime is retired"), RetiredView.bRuntimeRetired);
	TestFalse(TEXT("Part binding is cleared"), RetiredView.bBoundToSnapshot);
	TestFalse(TEXT("Extra slot Host is not retired"),
		ExtraHost->IsRuntimeEncounterPresentationRetired());
	TestEqual(TEXT("Visual component is preserved"), Visual, OriginalVisual);
	TestEqual(TEXT("Topology revision is unchanged"),
		Host->GetEnemySceneComponentTopologyRevision(), TopologyRevision);
	Host->RetireRuntimeEncounterPresentation();
	TestTrue(TEXT("Repeated retirement is idempotent"), Host->IsRuntimeEncounterPresentationRetired());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEncounterSceneLifecycleRejectsSharedHostOwnershipSpec,
	"Wacom.App.BattleTrigger.EncounterSceneLifecycle.RejectsSharedHostOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEncounterSceneLifecycleRejectsSharedHostOwnershipSpec::RunTest(
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
	ABattleTriggerActor* First = World->SpawnActor<ABattleTriggerActor>(
		ABattleTriggerActor::StaticClass(), FTransform::Identity, SpawnParams);
	ABattleTriggerActor* Second = World->SpawnActor<ABattleTriggerActor>(
		ABattleTriggerActor::StaticClass(), FTransform::Identity, SpawnParams);
	UWacomBattleEnemyPartComponent* Part = nullptr;
	UWacomBattleEnemyPartFlipbookLayerComponent* Visual = nullptr;
	AWacomBattleEnemyActor* SharedHost = nullptr;
	ON_SCOPE_EXIT
	{
		for (AActor* Actor : TArray<AActor*>{ SharedHost, First, Second })
		{
			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}
	};
	if (!First || !Second)
	{
		return false;
	}
	First->PersistentId = TEXT("Trigger.Shared.First");
	Second->PersistentId = TEXT("Trigger.Shared.Second");
	UEnemyDefinition* Enemy = MakeEnemyDefinition(
		First, TEXT("Enemy.Shared"), TEXT("Part.Shared.Body"));
	SharedHost = SpawnHost(*World, *Enemy, TEXT("Enemy"), Part, Visual);
	if (!TestNotNull(TEXT("Shared Host"), SharedHost))
	{
		return false;
	}
	UEncounterDefinition* Encounter = NewObject<UEncounterDefinition>(First);
	Encounter->EncounterDefinitionId = TEXT("Encounter.Shared");
	FEncounterEnemySlot& Slot = Encounter->EnemySlots.AddDefaulted_GetRef();
	Slot.EnemySlotId = TEXT("Enemy");
	Slot.EnemyDefinition = Enemy;
	First->EncounterDefinition = Encounter;
	Second->EncounterDefinition = Encounter;
	FWacomBattleSceneEnemyHostSlot HostSlot;
	HostSlot.EnemySlotId = TEXT("Enemy");
	HostSlot.SceneEnemyHost = SharedHost;
	First->SceneEnemyHostSlots = { HostSlot };
	Second->SceneEnemyHostSlots = { HostSlot };
	FDataValidationContext Context;
	const EDataValidationResult Result = First->IsDataValid(Context);
	TArray<FText> Warnings;
	TArray<FText> Errors;
	Context.SplitIssues(Warnings, Errors);
	TestEqual(TEXT("Shared Host invalidates Trigger placement"),
		Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Validation reports shared SceneEnemyHost"),
		IssuesContain(Errors, TEXT("共享 SceneEnemyHost")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEncounterSceneLifecycleRetirementPolicySpec,
	"Wacom.App.BattleTrigger.EncounterSceneLifecycle.RetirementPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEncounterSceneLifecycleRetirementPolicySpec::RunTest(
	const FString& /*Parameters*/)
{
	using FPolicy = FWacomEncounterSceneLifecycleAutomationTestView;
	TestTrue(TEXT("Victory retires resolved Encounter"),
		FPolicy::ShouldRetireResolvedEncounterScene(true, EBattleOutcome::Victory, false));
	TestFalse(TEXT("Withdraw preserves Encounter scene"),
		FPolicy::ShouldRetireResolvedEncounterScene(true, EBattleOutcome::Victory, true));
	TestFalse(TEXT("Defeat preserves Encounter scene"),
		FPolicy::ShouldRetireResolvedEncounterScene(true, EBattleOutcome::Defeat, false));
	TestFalse(TEXT("Settlement failure preserves Encounter scene"),
		FPolicy::ShouldRetireResolvedEncounterScene(false, EBattleOutcome::Victory, false));
	return true;
}
