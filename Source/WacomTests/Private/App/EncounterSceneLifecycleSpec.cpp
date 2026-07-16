// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/BattleTriggerActor.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyHostAnimationStyle.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Snapshots/BattleSnapshot.h"
#include "Testing/WacomEncounterSceneLifecycleAutomationTestView.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/WidgetComponent.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopeExit.h"

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

	UPaperFlipbook* MakeFlipbook(UObject* Outer, const FName Name)
	{
		UPaperSprite* FrameSprite = NewObject<UPaperSprite>(Outer);
		UPaperFlipbook* Flipbook = NewObject<UPaperFlipbook>(Outer, Name);
		FScopedFlipbookMutator Mutator(Flipbook);
		Mutator.FramesPerSecond = 10.0f;
		FPaperFlipbookKeyFrame KeyFrame;
		KeyFrame.Sprite = FrameSprite;
		KeyFrame.FrameRun = 2;
		Mutator.KeyFrames.Add(KeyFrame);
		return Flipbook;
	}

	UEnemyDefinition* MakeEnemyDefinition(
		UObject* Outer,
		const FName EnemyId,
		const FName PartId)
	{
		UEnemyPartDefinition* PartDefinition = NewObject<UEnemyPartDefinition>(Outer);
		PartDefinition->PartId = PartId;
		PartDefinition->DisplayName = FText::FromName(PartId);
		PartDefinition->MaxHp = 10;

		UEnemyDefinition* EnemyDefinition = NewObject<UEnemyDefinition>(Outer);
		EnemyDefinition->EnemyId = EnemyId;
		EnemyDefinition->DisplayName = FText::FromName(EnemyId);
		FEnemyPartSlot Slot;
		Slot.PartSlotId = TEXT("Body");
		Slot.PartDef = PartDefinition;
		EnemyDefinition->Parts = { Slot };
		return EnemyDefinition;
	}

	AWacomBattleEnemyActor* SpawnHost(
		UWorld& World,
		UEnemyDefinition& EnemyDefinition,
		const FName EnemySlotId)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		AWacomBattleEnemyActor* Host = World.SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (Host)
		{
			Host->EnemyDefinition = &EnemyDefinition;
			Host->EnemySlotId = EnemySlotId;
		}
		return Host;
	}

	AWacomBattleEnemyPartActor* SpawnPart(
		UWorld& World,
		AWacomBattleEnemyActor& Host,
		const UEnemyDefinition& EnemyDefinition,
		const bool bAddVisualLayer)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		AWacomBattleEnemyPartActor* Part = World.SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (!Part || EnemyDefinition.Parts.IsEmpty())
		{
			return Part;
		}

		Part->PartSlotId = EnemyDefinition.Parts[0].PartSlotId;
		Part->PartId = EnemyDefinition.Parts[0].PartDef
			? EnemyDefinition.Parts[0].PartDef->PartId
			: NAME_None;
		if (bAddVisualLayer)
		{
			FWacomBattleEnemyPartVisualLayer Layer;
			Layer.LayerId = TEXT("LifecycleLayer");
			Layer.Sprite = NewObject<UPaperSprite>(Part);
			Part->VisualLayers = { Layer };
		}
		Part->AttachToActor(&Host, FAttachmentTransformRules::KeepWorldTransform);
		Part->RefreshAuthoringState();
		Host.RefreshBattleEnemyPartAuthoringState();
		return Part;
	}

	UPaperSpriteComponent* FindGeneratedPartSprite(
		const AWacomBattleEnemyPartActor& Part)
	{
		TInlineComponentArray<UPaperSpriteComponent*> Components;
		Part.GetComponents(Components);
		for (UPaperSpriteComponent* Component : Components)
		{
			if (Component
				&& Component->GetAttachParent() == Part.GetVisualLayersRoot()
				&& Component->GetName().StartsWith(TEXT("VisualLayer_")))
			{
				return Component;
			}
		}
		return nullptr;
	}

	void ConfigureTerminalHost(AWacomBattleEnemyActor& Host)
	{
		UPaperFlipbook* Idle = MakeFlipbook(&Host, TEXT("Lifecycle_Idle"));
		UPaperFlipbook* Destroyed = MakeFlipbook(&Host, TEXT("Lifecycle_Destroyed"));
		UWacomBattleEnemyHostAnimationStyle* Style =
			NewObject<UWacomBattleEnemyHostAnimationStyle>(&Host);
		Style->DestroyedClip.Flipbook = Destroyed;
		Style->DestroyedClip.PlayRate = 1.0f;

		Host.HostAuthoringMode = EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual;
		Host.HostVisualMode = EWacomBattleEnemyHostVisualMode::Flipbook;
		Host.HostFlipbook = Idle;
		Host.HostFlipbookPlayRate = 1.0f;
		Host.bLoopHostFlipbook = true;
		Host.bAutoPlayHostFlipbook = true;
		Host.bHostVisualVisible = true;
		Host.HostAnimationStyle = Style;
		Host.RefreshBattleEnemyPartAuthoringState();

		Host.PlayRuntimeHostDestroyedAnimation([]() {});
		if (UPaperFlipbookComponent* Visual = Host.GetGeneratedHostFlipbookVisualComponent())
		{
			Visual->OnFinishedPlaying.Broadcast();
		}
	}

	void PrimeRuntimePartState(
		AWacomBattleEnemyPartActor& Part,
		const FName EncounterId,
		const FName EnemySlotId)
	{
		FEnemyPartSnapshot PartSnapshot;
		PartSnapshot.InstanceId = FGuid::NewGuid();
		PartSnapshot.Definition = nullptr;
		PartSnapshot.EncounterId = EncounterId;
		PartSnapshot.EnemySlotId = EnemySlotId;
		PartSnapshot.PartSlotId = Part.PartSlotId;
		PartSnapshot.CurrentInitiative = 5;
		PartSnapshot.CurrentIntent.IntentId = TEXT("Intent.Lifecycle");

		FEnemySnapshot EnemySnapshot;
		EnemySnapshot.EncounterId = EncounterId;
		EnemySnapshot.EnemySlotId = EnemySlotId;
		EnemySnapshot.Parts = { PartSnapshot };
		FBattleSnapshot Snapshot;
		Snapshot.EncounterId = EncounterId;
		Snapshot.Enemies = { EnemySnapshot };

		UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
			Part.GetWorldTargetBridgeComponent();
		Bridge->SetPartId(Part.PartId);
		Bridge->SetBattlePartSlotIdentity(EncounterId, EnemySlotId, Part.PartSlotId);
		FEnemyPartSnapshot MatchedPart;
		Bridge->SyncFromBattleSnapshot(Snapshot, &MatchedPart);
		Bridge->SetBattleHUDSceneRegistryState(true);
		Bridge->SetBattleTargetableState(true);

		UWacomBattleEnemyPartPresentationComponent* Presentation =
			Part.GetPresentationComponent();
		Presentation->CacheRuntimePartFacts(Part.PartId, MatchedPart);
		Presentation->SetTargetableAffordance(true);
		Presentation->SetDragTargetPreviewState(
			EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
		if (UWidgetComponent* PredictionWidget = Part.GetPredictionWidgetComponent())
		{
			PredictionWidget->SetVisibility(true, true);
		}
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
	"Wacom.App.BattleTrigger.EncounterSceneLifecycle.RetiresMappedHostsAfterBarrier",
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
	if (!TestNotNull(TEXT("Battle Trigger"), Trigger))
	{
		return false;
	}

	AWacomBattleEnemyActor* TerminalHost = nullptr;
	AWacomBattleEnemyActor* LayeredHost = nullptr;
	AWacomBattleEnemyActor* ExtraHost = nullptr;
	AWacomBattleEnemyPartActor* TerminalPart = nullptr;
	AWacomBattleEnemyPartActor* LayeredPart = nullptr;
	ON_SCOPE_EXIT
	{
		for (AActor* Actor : TArray<AActor*>{
			TerminalPart, LayeredPart, TerminalHost, LayeredHost, ExtraHost, Trigger })
		{
			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}
	};

	Trigger->PersistentId = TEXT("Trigger.SceneLifecycle");
	UEnemyDefinition* TerminalEnemy = MakeEnemyDefinition(
		Trigger, TEXT("Enemy.Lifecycle.Terminal"), TEXT("Part.Lifecycle.Terminal"));
	UEnemyDefinition* LayeredEnemy = MakeEnemyDefinition(
		Trigger, TEXT("Enemy.Lifecycle.Layered"), TEXT("Part.Lifecycle.Layered"));
	TerminalHost = SpawnHost(*World, *TerminalEnemy, TEXT("Terminal"));
	LayeredHost = SpawnHost(*World, *LayeredEnemy, TEXT("Layered"));
	ExtraHost = SpawnHost(*World, *TerminalEnemy, TEXT("Extra"));
	if (!TestNotNull(TEXT("Terminal Host"), TerminalHost)
		|| !TestNotNull(TEXT("Layered Host"), LayeredHost)
		|| !TestNotNull(TEXT("Extra Host"), ExtraHost))
	{
		return false;
	}

	TerminalPart = SpawnPart(*World, *TerminalHost, *TerminalEnemy, false);
	LayeredPart = SpawnPart(*World, *LayeredHost, *LayeredEnemy, true);
	if (!TestNotNull(TEXT("Terminal Part"), TerminalPart)
		|| !TestNotNull(TEXT("Layered Part"), LayeredPart))
	{
		return false;
	}

	ConfigureTerminalHost(*TerminalHost);
	LayeredHost->HostAuthoringMode = EWacomBattleEnemyHostAuthoringMode::MultiPartVisualLayers;
	LayeredHost->RefreshBattleEnemyPartAuthoringState();
	PrimeRuntimePartState(*TerminalPart, Trigger->PersistentId, TEXT("Terminal"));
	PrimeRuntimePartState(*LayeredPart, Trigger->PersistentId, TEXT("Layered"));

	UEncounterDefinition* Encounter = NewObject<UEncounterDefinition>(Trigger);
	Encounter->EncounterDefinitionId = TEXT("Encounter.SceneLifecycle");
	FEncounterEnemySlot TerminalEncounterSlot;
	TerminalEncounterSlot.EnemySlotId = TEXT("Terminal");
	TerminalEncounterSlot.EnemyDefinition = TerminalEnemy;
	FEncounterEnemySlot LayeredEncounterSlot;
	LayeredEncounterSlot.EnemySlotId = TEXT("Layered");
	LayeredEncounterSlot.EnemyDefinition = LayeredEnemy;
	Encounter->EnemySlots = { TerminalEncounterSlot, LayeredEncounterSlot };
	Trigger->EncounterDefinition = Encounter;

	FWacomBattleSceneEnemyHostSlot TerminalHostSlot;
	TerminalHostSlot.EnemySlotId = TEXT("Terminal");
	TerminalHostSlot.SceneEnemyHost = TerminalHost;
	FWacomBattleSceneEnemyHostSlot MissingHostSlot;
	MissingHostSlot.EnemySlotId = TEXT("Missing");
	FWacomBattleSceneEnemyHostSlot LayeredHostSlot;
	LayeredHostSlot.EnemySlotId = TEXT("Layered");
	LayeredHostSlot.SceneEnemyHost = LayeredHost;
	FWacomBattleSceneEnemyHostSlot ExtraHostSlot;
	ExtraHostSlot.EnemySlotId = TEXT("Extra");
	ExtraHostSlot.SceneEnemyHost = ExtraHost;
	Trigger->SceneEnemyHostSlots = {
		TerminalHostSlot, MissingHostSlot, LayeredHostSlot, ExtraHostSlot };

	UPaperFlipbookComponent* TerminalVisual =
		TerminalHost->GetGeneratedHostFlipbookVisualComponent();
	UPaperSpriteComponent* LayerVisual = FindGeneratedPartSprite(*LayeredPart);
	const uint32 TerminalTopologyRevision = TerminalHost->GetRuntimePartTopologyRevision();
	const uint32 LayeredTopologyRevision = LayeredHost->GetRuntimePartTopologyRevision();
	if (!TestNotNull(TEXT("Terminal Flipbook component"), TerminalVisual)
		|| !TestNotNull(TEXT("Layered Part Sprite component"), LayerVisual))
	{
		return false;
	}
	const UPaperFlipbook* TerminalClip = TerminalVisual->GetFlipbook();
	TestTrue(TEXT("Destroyed animation reached terminal state"),
		TerminalHost->GetBattleSceneEnemyDebugView().bHostAnimationTerminalState);
	TestTrue(TEXT("Runtime bridge is primed before retirement"),
		TerminalPart->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
	TestTrue(TEXT("Runtime presentation is primed before retirement"),
		TerminalPart->GetPresentationComponent()
			->GetBattleEnemyPartPresentationDebugView().bDragPreviewActive);

	Trigger->BeginResolvedEncounterSceneRetirement();
	const FWacomBattleTriggerDebugView PendingView = Trigger->GetBattleTriggerDebugView(nullptr);
	TestTrue(TEXT("Begin marks retirement pending"), PendingView.bResolvedSceneRetirementPending);
	TestFalse(TEXT("Begin disables Trigger collision"), Trigger->GetActorEnableCollision());
	TestFalse(TEXT("Begin keeps terminal Host visible"), TerminalHost->IsHidden());
	TestFalse(TEXT("Begin keeps terminal Part visible"), TerminalPart->IsHidden());
	TestEqual(TEXT("Begin preserves terminal Flipbook component"),
		TerminalHost->GetGeneratedHostFlipbookVisualComponent(), TerminalVisual);
	TestTrue(TEXT("Begin preserves terminal clip"), TerminalVisual->GetFlipbook() == TerminalClip);

	Trigger->CompleteResolvedEncounterSceneRetirement();
	TestTrue(TEXT("Terminal Host is retired"),
		TerminalHost->IsRuntimeEncounterPresentationRetired());
	TestTrue(TEXT("Layered Host is retired"),
		LayeredHost->IsRuntimeEncounterPresentationRetired());
	TestTrue(TEXT("All mapped Parts are retired"),
		TerminalPart->IsRuntimeEncounterPresentationRetired()
			&& LayeredPart->IsRuntimeEncounterPresentationRetired());
	TestTrue(TEXT("Mapped Hosts are hidden"), TerminalHost->IsHidden() && LayeredHost->IsHidden());
	TestTrue(TEXT("Mapped Parts are hidden"), TerminalPart->IsHidden() && LayeredPart->IsHidden());
	TestFalse(TEXT("Mapped Host collision is disabled"), TerminalHost->GetActorEnableCollision());
	TestFalse(TEXT("Mapped Part collision is disabled"), TerminalPart->GetActorEnableCollision());
	TestFalse(TEXT("Extra slot Host is not retired"),
		ExtraHost->IsRuntimeEncounterPresentationRetired());
	TestFalse(TEXT("Extra slot Host remains visible"), ExtraHost->IsHidden());
	TestFalse(TEXT("Bridge binding is cleared"),
		TerminalPart->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
	const FWacomBattleEnemyPartPresentationDebugView RetiredPresentation =
		TerminalPart->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
	TestFalse(TEXT("Presentation runtime facts are cleared"),
		RetiredPresentation.bHasRuntimePartFacts);
	TestFalse(TEXT("Drag preview is cleared"), RetiredPresentation.bDragPreviewActive);
	TestFalse(TEXT("Prediction widget is hidden"),
		TerminalPart->GetPredictionWidgetComponent()->IsVisible());
	TestEqual(TEXT("Terminal Flipbook component is preserved"),
		TerminalHost->GetGeneratedHostFlipbookVisualComponent(), TerminalVisual);
	TestTrue(TEXT("Terminal clip is preserved"), TerminalVisual->GetFlipbook() == TerminalClip);
	TestEqual(TEXT("VisualLayer component is preserved"),
		FindGeneratedPartSprite(*LayeredPart), LayerVisual);
	TestEqual(TEXT("Terminal topology revision is unchanged"),
		TerminalHost->GetRuntimePartTopologyRevision(), TerminalTopologyRevision);
	TestEqual(TEXT("Layered topology revision is unchanged"),
		LayeredHost->GetRuntimePartTopologyRevision(), LayeredTopologyRevision);

	// Repeated retirement calls are safe even after Trigger destruction has begun.
	TerminalHost->RetireRuntimeEncounterPresentation();
	LayeredHost->RetireRuntimeEncounterPresentation();
	Trigger->CompleteResolvedEncounterSceneRetirement();
	TestTrue(TEXT("Trigger completion is idempotent"),
		Trigger->GetBattleTriggerDebugView(nullptr).bResolvedSceneRetirementCompleted);
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
	if (!TestNotNull(TEXT("First Trigger"), First)
		|| !TestNotNull(TEXT("Second Trigger"), Second))
	{
		return false;
	}

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

	First->PersistentId = TEXT("Trigger.SceneLifecycle.Shared.First");
	Second->PersistentId = TEXT("Trigger.SceneLifecycle.Shared.Second");
	UEnemyDefinition* Enemy = MakeEnemyDefinition(
		First, TEXT("Enemy.Lifecycle.Shared"), TEXT("Part.Lifecycle.Shared"));
	SharedHost = SpawnHost(*World, *Enemy, TEXT("Enemy"));
	if (!TestNotNull(TEXT("Shared Host"), SharedHost))
	{
		return false;
	}

	UEncounterDefinition* Encounter = NewObject<UEncounterDefinition>(First);
	Encounter->EncounterDefinitionId = TEXT("Encounter.SceneLifecycle.Shared");
	FEncounterEnemySlot EncounterSlot;
	EncounterSlot.EnemySlotId = TEXT("Enemy");
	EncounterSlot.EnemyDefinition = Enemy;
	Encounter->EnemySlots = { EncounterSlot };
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
	TestTrue(TEXT("Aid victory retires resolved Encounter"),
		FPolicy::ShouldRetireResolvedEncounterScene(
			true, EBattleOutcome::Victory, false));
	TestTrue(TEXT("Destroy victory retires resolved Encounter"),
		FPolicy::ShouldRetireResolvedEncounterScene(
			true, EBattleOutcome::Victory, false));
	TestFalse(TEXT("Withdraw preserves Encounter scene"),
		FPolicy::ShouldRetireResolvedEncounterScene(
			true, EBattleOutcome::Victory, true));
	TestFalse(TEXT("Defeat preserves Encounter scene"),
		FPolicy::ShouldRetireResolvedEncounterScene(
			true, EBattleOutcome::Defeat, false));
	TestFalse(TEXT("Undetermined preserves Encounter scene"),
		FPolicy::ShouldRetireResolvedEncounterScene(
			true, EBattleOutcome::Undetermined, false));
	TestFalse(TEXT("Settlement failure preserves Encounter scene"),
		FPolicy::ShouldRetireResolvedEncounterScene(
			false, EBattleOutcome::Victory, false));
	return true;
}
