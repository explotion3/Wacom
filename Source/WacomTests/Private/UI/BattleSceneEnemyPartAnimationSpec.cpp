// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Actors/WacomBattleEnemyPartAnimationStyle.h"
#include "Components/WacomBattleEnemyPartVisualLayerComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "UI/Battle/WacomBattleEnemyActionPlaybackTypes.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "BattleHUDTestHarness.h"
#include "UI/BattleWidgetSpecReceiver.h"

#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "Engine/World.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopeExit.h"

namespace WacomBattleSceneEnemyPartAnimationSpec
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

	UPaperFlipbook* MakeFlipbook(UObject* Outer, FName Name, int32 FrameRun = 2)
	{
		UPaperSprite* FrameSprite = NewObject<UPaperSprite>(Outer);
		UPaperFlipbook* Flipbook = NewObject<UPaperFlipbook>(Outer, Name);
		FScopedFlipbookMutator Mutator(Flipbook);
		Mutator.FramesPerSecond = 10.0f;
		FPaperFlipbookKeyFrame KeyFrame;
		KeyFrame.Sprite = FrameSprite;
		KeyFrame.FrameRun = FrameRun;
		Mutator.KeyFrames.Add(KeyFrame);
		return Flipbook;
	}

	template <typename TComponent>
	TComponent* FindGeneratedVisualLayer(AActor& Actor, FName LayerId)
	{
		TInlineComponentArray<TComponent*> Components;
		Actor.GetComponents(Components);
		for (TComponent* Component : Components)
		{
			if (Component
				&& Component->GetName().StartsWith(TEXT("VisualLayer_"))
				&& Component->GetName().Contains(LayerId.ToString()))
			{
				return Component;
			}
		}
		return nullptr;
	}

	TArray<FName> BuildDefinitionPartIds(const UEnemyDefinition& Enemy)
	{
		TArray<FName> PartIds;
		for (const FEnemyPartSlot& PartSlot : Enemy.Parts)
		{
			if (PartSlot.PartDef)
			{
				PartIds.Add(PartSlot.PartDef->PartId);
			}
		}
		return PartIds;
	}

	FBattleEvent MakeActionEvent(
		const FBattleEnemyPartKey& PartKey,
		int32 Sequence,
		FName IntentId,
		int32 Count = 1)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::EnemyPartActed;
		Event.Sequence = Sequence;
		Event.ActorEnemyPartKey = PartKey;
		Event.IntentId = IntentId;
		Event.Count = Count;
		return Event;
	}

	struct FConfiguredPartAnimation
	{
		FName LayerId = NAME_None;
		UPaperFlipbook* Idle = nullptr;
		UPaperFlipbook* Action = nullptr;
		UPaperFlipbook* ExplicitAction = nullptr;
		UPaperFlipbook* Destroyed = nullptr;
		UWacomBattleEnemyPartAnimationStyle* Style = nullptr;
	};

	FConfiguredPartAnimation ConfigurePartAnimation(
		AWacomBattleEnemyPartActor& Part,
		const FString& Prefix)
	{
		FConfiguredPartAnimation Result;
		Result.LayerId = FName(*(Prefix + TEXT(".Main")));
		Result.Idle = MakeFlipbook(&Part, FName(*(Prefix + TEXT("_Idle"))), 4);
		Result.Action = MakeFlipbook(&Part, FName(*(Prefix + TEXT("_Action"))), 2);
		Result.ExplicitAction = MakeFlipbook(
			&Part,
			FName(*(Prefix + TEXT("_ExplicitAction"))),
			3);
		Result.Destroyed = MakeFlipbook(&Part, FName(*(Prefix + TEXT("_Destroyed"))), 1);

		Result.Style = NewObject<UWacomBattleEnemyPartAnimationStyle>(&Part);
		Result.Style->TargetVisualLayerId = Result.LayerId;
		Result.Style->DefaultActionClip.Flipbook = Result.Action;
		Result.Style->DefaultActionClip.PlayRate = 1.0f;
		FWacomBattleEnemyPartAnimationClip ExplicitClip;
		ExplicitClip.Flipbook = Result.ExplicitAction;
		ExplicitClip.PlayRate = 1.25f;
		Result.Style->ActionClipsByIntentId.Add(TEXT("Intent.Explicit"), ExplicitClip);
		Part.PartAnimationStyle = Result.Style;

		FWacomBattleEnemyPartVisualLayer Layer;
		Layer.LayerId = Result.LayerId;
		Layer.LayerMode = EWacomBattleEnemyPartVisualLayerMode::Flipbook;
		Layer.Flipbook = Result.Idle;
		Layer.DestroyedFlipbook = Result.Destroyed;
		Layer.FlipbookPlayRate = 0.75f;
		Layer.bLoopFlipbook = true;
		Layer.FlipbookStartTimeSeconds = 0.04f;
		Layer.bAutoPlayFlipbook = true;
		Part.VisualLayers = { Layer };
		return Result;
	}

	AWacomBattleEnemyPartActor* FindPartBySlot(
		const FWacomBattleHUDTestSceneEnemyHost& SceneEnemy,
		FName PartSlotId)
	{
		for (AWacomBattleEnemyPartActor* Part : SceneEnemy.Parts)
		{
			if (Part && Part->PartSlotId == PartSlotId)
			{
				return Part;
			}
		}
		return nullptr;
	}

	const FEnemyPartSnapshot* FindSnapshotPart(
		const FBattleSnapshot& Snapshot,
		FName PartSlotId)
	{
		if (Snapshot.Enemies.IsEmpty())
		{
			return nullptr;
		}
		return Snapshot.Enemies[0].Parts.FindByPredicate(
			[PartSlotId](const FEnemyPartSnapshot& Part)
			{
				return Part.PartSlotId == PartSlotId;
			});
	}

	bool IssuesContain(const TArray<FText>& Issues, const TCHAR* ExpectedText)
	{
		return Issues.ContainsByPredicate([ExpectedText](const FText& Issue)
		{
			return Issue.ToString().Contains(ExpectedText);
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartAnimationStyleSpec,
	"Wacom.UI.Battle.BattleSceneEnemyPartAnimation.StyleResolutionAndValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartAnimationStyleSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyPartAnimationSpec;
	TStrongObjectPtr<UWacomBattleEnemyPartAnimationStyle> Style(
		NewObject<UWacomBattleEnemyPartAnimationStyle>());
	Style->TargetVisualLayerId = TEXT("Head.Main");
	UPaperFlipbook* DefaultAction = MakeFlipbook(Style.Get(), TEXT("DefaultAction"));
	UPaperFlipbook* ExplicitAction = MakeFlipbook(Style.Get(), TEXT("ExplicitAction"));
	Style->DefaultActionClip.Flipbook = DefaultAction;
	FWacomBattleEnemyPartAnimationClip ExplicitClip;
	ExplicitClip.Flipbook = ExplicitAction;
	Style->ActionClipsByIntentId.Add(TEXT("Intent.Bite"), ExplicitClip);

	const FWacomBattleEnemyPartAnimationClip* ResolvedExplicit =
		Style->ResolveActionClip(TEXT("Intent.Bite"));
	const FWacomBattleEnemyPartAnimationClip* ResolvedFallback =
		Style->ResolveActionClip(TEXT("Intent.Unknown"));
	TestTrue(TEXT("Explicit Intent clip wins"),
		ResolvedExplicit && ResolvedExplicit->Flipbook == ExplicitAction);
	TestTrue(TEXT("Missing Intent uses Default Action"),
		ResolvedFallback && ResolvedFallback->Flipbook == DefaultAction);

	Style->DefaultActionClip.Flipbook = nullptr;
	Style->ActionClipsByIntentId.Reset();
	TestNull(TEXT("Flipbook-like Intent names are never inferred"),
		Style->ResolveActionClip(FName(*ExplicitAction->GetName())));

	Style->TargetVisualLayerId = NAME_None;
	FDataValidationContext MissingLayerContext;
	TestEqual(TEXT("Empty target layer is invalid"),
		Style->IsDataValid(MissingLayerContext), EDataValidationResult::Invalid);

	Style->TargetVisualLayerId = TEXT("Head.Main");
	FWacomBattleEnemyPartAnimationClip InvalidClip;
	InvalidClip.Flipbook = ExplicitAction;
	InvalidClip.PlayRate = 0.0f;
	Style->ActionClipsByIntentId.Add(TEXT("Intent.Invalid"), InvalidClip);
	FDataValidationContext InvalidClipContext;
	TestEqual(TEXT("Non-positive explicit clip rate is invalid"),
		Style->IsDataValid(InvalidClipContext), EDataValidationResult::Invalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartAnimationComponentSpec,
	"Wacom.UI.Battle.BattleSceneEnemyPartAnimation.ComponentLifecycleDestroyedAndWatchdog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartAnimationComponentSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyPartAnimationSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host = World->SpawnActor<AWacomBattleEnemyActor>(
		AWacomBattleEnemyActor::StaticClass(), FTransform::Identity, SpawnParams);
	AWacomBattleEnemyPartActor* Part = World->SpawnActor<AWacomBattleEnemyPartActor>(
		AWacomBattleEnemyPartActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene enemy Host"), Host)
		|| !TestNotNull(TEXT("Scene enemy Part"), Part))
	{
		if (IsValid(Part)) { Part->Destroy(); }
		if (IsValid(Host)) { Host->Destroy(); }
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Part)) { Part->Destroy(); }
		if (IsValid(Host)) { Host->Destroy(); }
	};

	Host->HostAuthoringMode = EWacomBattleEnemyHostAuthoringMode::MultiPartVisualLayers;
	Part->PartId = TEXT("Test.Head");
	Part->PartSlotId = TEXT("Head");
	Part->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);
	const FConfiguredPartAnimation Animation = ConfigurePartAnimation(*Part, TEXT("Head"));
	Host->RefreshBattleEnemyPartAuthoringState();

	UPaperFlipbookComponent* Visual =
		FindGeneratedVisualLayer<UPaperFlipbookComponent>(*Part, Animation.LayerId);
	if (!TestNotNull(TEXT("Generated Part Flipbook component"), Visual))
	{
		return false;
	}
	const int32 ComponentCountBefore = Part->GetBattleSceneEnemyPartDebugView()
		.GeneratedVisualLayerComponentCount;
	const uint32 TopologyRevisionBefore = Host->GetRuntimePartTopologyRevision();

	const TArray<FWacomBattleEnemyPartVisualLayer> ValidVisualLayers = Part->VisualLayers;
	const FWacomBattleEnemyPartVisualLayer DuplicateLayer = Part->VisualLayers[0];
	Part->VisualLayers.Add(DuplicateLayer);
	FDataValidationContext DuplicateLayerContext;
	TestEqual(TEXT("Duplicate target Layer is invalid"),
		Part->IsDataValid(DuplicateLayerContext), EDataValidationResult::Invalid);
	Part->VisualLayers = ValidVisualLayers;

	Animation.Style->TargetVisualLayerId = TEXT("Missing.Main");
	FDataValidationContext MissingTargetLayerContext;
	TestEqual(TEXT("Missing target Layer is invalid"),
		Part->IsDataValid(MissingTargetLayerContext), EDataValidationResult::Invalid);
	Animation.Style->TargetVisualLayerId = Animation.LayerId;

	Part->VisualLayers[0].LayerMode =
		EWacomBattleEnemyPartVisualLayerMode::StaticSprite;
	Part->VisualLayers[0].Sprite = NewObject<UPaperSprite>(Part);
	Part->VisualLayers[0].Flipbook = nullptr;
	FDataValidationContext WrongLayerModeContext;
	TestEqual(TEXT("Non-Flipbook target Layer is invalid"),
		Part->IsDataValid(WrongLayerModeContext), EDataValidationResult::Invalid);
	Part->VisualLayers = ValidVisualLayers;

	Host->HostAuthoringMode = EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual;
	FDataValidationContext WrongHostModeContext;
	Part->IsDataValid(WrongHostModeContext);
	TArray<FText> WrongModeWarnings;
	TArray<FText> WrongModeErrors;
	WrongHostModeContext.SplitIssues(WrongModeWarnings, WrongModeErrors);
	TestTrue(TEXT("Part Style outside MultiPart Host is diagnosed"),
		IssuesContain(WrongModeWarnings, TEXT("MultiPartVisualLayers")));
	Host->HostAuthoringMode = EWacomBattleEnemyHostAuthoringMode::MultiPartVisualLayers;

	bool bActionCompleted = false;
	FWacomBattleEnemyActionPlaybackCallbacks ActionCallbacks;
	ActionCallbacks.OnCompleted = [&bActionCompleted]()
	{
		bActionCompleted = true;
	};
	Part->PlayRuntimePartActionAnimation(TEXT("Intent.Explicit"), MoveTemp(ActionCallbacks));
	TestEqual(TEXT("Explicit action switches the existing component"),
		Visual->GetFlipbook(), Animation.ExplicitAction);
	TestTrue(TEXT("Part action is a real barrier"),
		Part->GetBattleSceneEnemyPartDebugView().bPartAnimationPlaybackActive
		&& !bActionCompleted);
	Visual->OnFinishedPlaying.Broadcast();
	TestTrue(TEXT("Real completion fires callback"), bActionCompleted);
	TestEqual(TEXT("Completion restores authored Idle"), Visual->GetFlipbook(), Animation.Idle);
	TestTrue(TEXT("Completion restores authored looping"), Visual->IsLooping());
	TestTrue(TEXT("Completion restores authored play rate"),
		FMath::IsNearlyEqual(Visual->GetPlayRate(), 0.75f));
	TestTrue(TEXT("Completion restores authored start time"),
		FMath::IsNearlyEqual(Visual->GetPlaybackPosition(), 0.04f));

	bool bWatchdogCompleted = false;
	const int32 WatchdogBefore = Part->GetBattleSceneEnemyPartDebugView()
		.PartAnimationWatchdogCompletionCount;
	FWacomBattleEnemyActionPlaybackCallbacks WatchdogCallbacks;
	WatchdogCallbacks.OnCompleted = [&bWatchdogCompleted]()
	{
		bWatchdogCompleted = true;
	};
	Part->PlayRuntimePartActionAnimation(TEXT("Intent.Unknown"), MoveTemp(WatchdogCallbacks));
	++GFrameCounter;
	World->GetTimerManager().Tick(0.50f);
	++GFrameCounter;
	World->GetTimerManager().Tick(0.50f);
	TestTrue(TEXT("Watchdog completes stalled action"), bWatchdogCompleted);
	TestEqual(TEXT("Watchdog completion is diagnosed"),
		Part->GetBattleSceneEnemyPartDebugView().PartAnimationWatchdogCompletionCount,
		WatchdogBefore + 1);

	bool bCancelledByDestroyed = false;
	bool bDestroyedCancellationImpactFired = false;
	FWacomBattleEnemyActionPlaybackCallbacks DestroyedCallbacks;
	DestroyedCallbacks.OnImpact = [&bDestroyedCancellationImpactFired]()
	{
		bDestroyedCancellationImpactFired = true;
	};
	DestroyedCallbacks.OnCompleted = [&bCancelledByDestroyed]()
	{
		bCancelledByDestroyed = true;
	};
	Part->PlayRuntimePartActionAnimation(TEXT("Intent.Unknown"), MoveTemp(DestroyedCallbacks));
	Part->ApplyRuntimeDestroyedVisualState();
	TestTrue(TEXT("Destroyed safely completes residual action barrier"), bCancelledByDestroyed);
	TestFalse(TEXT("Destroyed cancellation discards the stale action Impact"),
		bDestroyedCancellationImpactFired);
	TestEqual(TEXT("Destroyed owns terminal visual"), Visual->GetFlipbook(), Animation.Destroyed);
	TestFalse(TEXT("Destroyed leaves no active action"),
		Part->GetBattleSceneEnemyPartDebugView().bPartAnimationPlaybackActive);
	++GFrameCounter;
	World->GetTimerManager().Tick(0.50f);
	TestFalse(TEXT("Destroyed cancellation invalidates old action timers"),
		bDestroyedCancellationImpactFired);

	Part->ResetRuntimeDestroyedVisualState();
	TestEqual(TEXT("New battle reset restores authored Idle"), Visual->GetFlipbook(), Animation.Idle);
	TestTrue(TEXT("All playback paths reuse the same component"),
		FindGeneratedVisualLayer<UPaperFlipbookComponent>(*Part, Animation.LayerId) == Visual);
	TestEqual(TEXT("No visual layer component is added"),
		Part->GetBattleSceneEnemyPartDebugView().GeneratedVisualLayerComponentCount,
		ComponentCountBefore);
	TestEqual(TEXT("Playback does not change Host topology"),
		Host->GetRuntimePartTopologyRevision(), TopologyRevisionBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartAnimationRoutingSpec,
	"Wacom.UI.Battle.BattleSceneEnemyPartAnimation.FullPartKeyRoutesSerially",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartAnimationRoutingSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyPartAnimationSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{ Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fixture.MakeThreePartEnemy(16, 22, 10, 3, 4, 5);
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, 1);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	FWacomBattleHUDTestSceneEnemyHost& SceneEnemy =
		Harness->AttachSceneEnemyHost(Enemy, BuildDefinitionPartIds(*Enemy));
	SceneEnemy.Host->HostAuthoringMode =
		EWacomBattleEnemyHostAuthoringMode::MultiPartVisualLayers;
	TMap<FName, FConfiguredPartAnimation> Animations;
	for (AWacomBattleEnemyPartActor* Part : SceneEnemy.Parts)
	{
		Animations.Add(
			Part->PartSlotId,
			ConfigurePartAnimation(*Part, Part->PartSlotId.ToString()));
	}
	SceneEnemy.Host->RefreshBattleEnemyPartAuthoringState();
	Harness->SetSession(Session);
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FEnemyPartSnapshot* HeadSnapshot = FindSnapshotPart(Snapshot, TEXT("Test.Part.Head"));
	const FEnemyPartSnapshot* BodySnapshot = FindSnapshotPart(Snapshot, TEXT("Test.Part.Body"));
	AWacomBattleEnemyPartActor* Head = FindPartBySlot(SceneEnemy, TEXT("Test.Part.Head"));
	AWacomBattleEnemyPartActor* Body = FindPartBySlot(SceneEnemy, TEXT("Test.Part.Body"));
	AWacomBattleEnemyPartActor* Tail = FindPartBySlot(SceneEnemy, TEXT("Test.Part.Tail"));
	if (!TestNotNull(TEXT("Head snapshot"), HeadSnapshot)
		|| !TestNotNull(TEXT("Body snapshot"), BodySnapshot)
		|| !TestNotNull(TEXT("Head Part"), Head)
		|| !TestNotNull(TEXT("Body Part"), Body)
		|| !TestNotNull(TEXT("Tail Part"), Tail))
	{
		return false;
	}

	UPaperFlipbookComponent* HeadVisual = FindGeneratedVisualLayer<UPaperFlipbookComponent>(
		*Head, Animations[Head->PartSlotId].LayerId);
	UPaperFlipbookComponent* BodyVisual = FindGeneratedVisualLayer<UPaperFlipbookComponent>(
		*Body, Animations[Body->PartSlotId].LayerId);
	UPaperFlipbookComponent* TailVisual = FindGeneratedVisualLayer<UPaperFlipbookComponent>(
		*Tail, Animations[Tail->PartSlotId].LayerId);
	if (!TestNotNull(TEXT("Head visual"), HeadVisual)
		|| !TestNotNull(TEXT("Body visual"), BodyVisual)
		|| !TestNotNull(TEXT("Tail visual"), TailVisual))
	{
		return false;
	}

	HUD->EnqueueBattlePresentationEventsForTest({
		MakeActionEvent(HeadSnapshot->PartKey, 1, TEXT("Intent.Unknown"), 0) });
	TestFalse(TEXT("Count zero action does not block"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Count zero action does not change Head"),
		HeadVisual->GetFlipbook(), Animations[Head->PartSlotId].Idle);

	FBattleEnemyPartKey MissingPartKey = HeadSnapshot->PartKey;
	MissingPartKey.PartSlotId = TEXT("MissingPart");
	HUD->EnqueueBattlePresentationEventsForTest({
		MakeActionEvent(MissingPartKey, 2, TEXT("Intent.Unknown")) });
	TestFalse(TEXT("Unknown full Part key completes immediately"), HUD->IsBattlePresentationBusy());

	const int32 RegistryRevisionBefore =
		HUD->GetBattleSceneEnemyTargetRegistryRevisionForTest();
	HUD->EnqueueBattlePresentationEventsForTest({
		MakeActionEvent(HeadSnapshot->PartKey, 3, TEXT("Intent.Unknown")),
		MakeActionEvent(BodySnapshot->PartKey, 4, TEXT("Intent.Explicit")) });
	TestTrue(TEXT("First Part action holds queue"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Head receives first action"),
		HeadVisual->GetFlipbook(), Animations[Head->PartSlotId].Action);
	TestEqual(TEXT("Body remains authored Idle"),
		BodyVisual->GetFlipbook(), Animations[Body->PartSlotId].Idle);
	TestEqual(TEXT("Tail remains authored Idle"),
		TailVisual->GetFlipbook(), Animations[Tail->PartSlotId].Idle);

	HeadVisual->SetPlaybackPosition(0.05f, false);
	const float HeadProgressBeforeRefresh = HeadVisual->GetPlaybackPosition();
	HUD->RefreshFromSnapshotForTest(Snapshot);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	TestEqual(TEXT("Repeated Snapshot/Host set keeps registry revision"),
		HUD->GetBattleSceneEnemyTargetRegistryRevisionForTest(), RegistryRevisionBefore);
	TestTrue(TEXT("Snapshot keeps active Part progress"),
		FMath::IsNearlyEqual(HeadVisual->GetPlaybackPosition(), HeadProgressBeforeRefresh));

	SceneEnemy.Host->InvalidateRuntimePartTopology();
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestTrue(TEXT("Topology rebuild reuses Head component"),
		FindGeneratedVisualLayer<UPaperFlipbookComponent>(
			*Head, Animations[Head->PartSlotId].LayerId) == HeadVisual);
	TestEqual(TEXT("Topology rebuild keeps active Head action"),
		HeadVisual->GetFlipbook(), Animations[Head->PartSlotId].Action);

	HeadVisual->OnFinishedPlaying.Broadcast();
	TestTrue(TEXT("Body starts only after Head completes"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Body uses explicit Intent clip"),
		BodyVisual->GetFlipbook(), Animations[Body->PartSlotId].ExplicitAction);
	TestEqual(TEXT("Head restored Idle before Body action"),
		HeadVisual->GetFlipbook(), Animations[Head->PartSlotId].Idle);
	BodyVisual->OnFinishedPlaying.Broadcast();
	TestFalse(TEXT("Queue drains after second Part completes"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Body restores authored Idle"),
		BodyVisual->GetFlipbook(), Animations[Body->PartSlotId].Idle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartAnimationRetiringSpec,
	"Wacom.UI.Battle.BattleSceneEnemyPartAnimation.BattleEndRetainsQueuedPartAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartAnimationRetiringSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyPartAnimationSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{ Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fixture.MakeThreePartEnemy(16, 22, 10, 3, 4, 5);
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, 1);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	FWacomBattleHUDTestSceneEnemyHost& SceneEnemy =
		Harness->AttachSceneEnemyHost(Enemy, BuildDefinitionPartIds(*Enemy));
	SceneEnemy.Host->HostAuthoringMode =
		EWacomBattleEnemyHostAuthoringMode::MultiPartVisualLayers;
	AWacomBattleEnemyPartActor* Head = FindPartBySlot(SceneEnemy, TEXT("Test.Part.Head"));
	if (!TestNotNull(TEXT("Head Part"), Head))
	{
		return false;
	}
	const FConfiguredPartAnimation HeadAnimation = ConfigurePartAnimation(*Head, TEXT("RetiringHead"));
	SceneEnemy.Host->RefreshBattleEnemyPartAuthoringState();
	Harness->SetSession(Session);
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FEnemyPartSnapshot* HeadSnapshot =
		FindSnapshotPart(InitialSnapshot, TEXT("Test.Part.Head"));
	UPaperFlipbookComponent* HeadVisual =
		FindGeneratedVisualLayer<UPaperFlipbookComponent>(*Head, HeadAnimation.LayerId);
	if (!TestNotNull(TEXT("Head snapshot"), HeadSnapshot)
		|| !TestNotNull(TEXT("Head visual"), HeadVisual))
	{
		return false;
	}

	FBattleSnapshot BattleEndSnapshot = InitialSnapshot;
	BattleEndSnapshot.Phase = EBattlePhase::BattleEnd;
	BattleEndSnapshot.Enemies[0].bAllPartsDestroyed = true;
	for (FEnemyPartSnapshot& Part : BattleEndSnapshot.Enemies[0].Parts)
	{
		Part.bDestroyed = true;
		Part.CurrentHp = 0;
	}
	HUD->RefreshFromSnapshotForTest(BattleEndSnapshot);
	TestEqual(TEXT("BattleEnd immediately clears target registry"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(), 0);

	HUD->EnqueueBattlePresentationEventsForTest({
		MakeActionEvent(HeadSnapshot->PartKey, 10, TEXT("Intent.Unknown")) });
	TestTrue(TEXT("Retiring Part still starts queued action"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Retiring action uses exact Head component"),
		HeadVisual->GetFlipbook(), HeadAnimation.Action);
	HeadVisual->OnFinishedPlaying.Broadcast();
	TestFalse(TEXT("Retiring action completion drains queue"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Retiring action restores authored Idle"),
		HeadVisual->GetFlipbook(), HeadAnimation.Idle);
	return true;
}
