// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleEnemyPartDragPredictionTypes.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/BattleSceneTargetClickTestAccess.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleInteractionTargetSpec
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

	FGuid FindFirstHandCardByTargetMode(const FBattleSnapshot& Snapshot, ECardTargetMode TargetMode)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.Definition && Card.Definition->TargetMode == TargetMode)
			{
				return Card.InstanceId;
			}
		}
		return FGuid();
	}

	struct FSceneEnemyHostActors
	{
		AWacomBattleEnemyActor* Host = nullptr;
		TArray<AWacomBattleEnemyPartActor*> Parts;
	};

	FName ResolvePartSlotIdForDefinitionPart(
		const UEnemyDefinition* EnemyDefinition,
		FName PartId)
	{
		if (!EnemyDefinition || PartId.IsNone())
		{
			return NAME_None;
		}

		for (const FEnemyPartSlot& Slot : EnemyDefinition->Parts)
		{
			if (Slot.PartDef && Slot.PartDef->PartId == PartId)
			{
				return Slot.PartSlotId;
			}
		}
		return NAME_None;
	}

	void AttachPartActorToHost(
		AWacomBattleEnemyActor* Host,
		FName PartId,
		AWacomBattleEnemyPartActor* PartActor)
	{
		if (!Host || !PartActor)
		{
			return;
		}

		PartActor->PartId = PartId;
		PartActor->PartSlotId = ResolvePartSlotIdForDefinitionPart(Host->EnemyDefinition, PartId);
		PartActor->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);
	}

	FSceneEnemyHostActors SpawnSceneEnemyHost(
		UWorld& World,
		UEnemyDefinition* EnemyDefinition,
		const TArray<FName>& PartIds)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;

		FSceneEnemyHostActors Result;
		Result.Host = World.SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (!Result.Host)
		{
			return Result;
		}

		Result.Host->EnemyDefinition = EnemyDefinition;
		for (int32 Index = 0; Index < PartIds.Num(); ++Index)
		{
			AWacomBattleEnemyPartActor* PartActor =
				World.SpawnActor<AWacomBattleEnemyPartActor>(
					AWacomBattleEnemyPartActor::StaticClass(),
					FTransform(FVector(100.f * static_cast<float>(Index + 1), 0.f, 0.f)),
					SpawnParams);
			if (!PartActor)
			{
				continue;
			}

			Result.Parts.Add(PartActor);
			AttachPartActorToHost(Result.Host, PartIds[Index], PartActor);
		}

		Result.Host->RefreshBattleEnemyPartAuthoringState();
		return Result;
	}

	void DestroySceneEnemyHost(FSceneEnemyHostActors& Actors)
	{
		for (AWacomBattleEnemyPartActor* PartActor : Actors.Parts)
		{
			if (IsValid(PartActor))
			{
				PartActor->Destroy();
			}
		}
		Actors.Parts.Reset();

		if (IsValid(Actors.Host))
		{
			Actors.Host->Destroy();
		}
		Actors.Host = nullptr;
	}

	void SettleBattlePresentationQueue(UWacomBattleHUDDetailTest& HUD, int32 MaxSteps = 32)
	{
		for (int32 Iteration = 0; HUD.IsBattlePresentationBusy() && Iteration < MaxSteps; ++Iteration)
		{
			HUD.AdvanceBattlePresentationQueueForTest();
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartWorldTargetBridgeBindsRuntimeTargetSpec,
	"Wacom.UI.Battle.InteractionTarget.EnemyPartWorldBridge.BindsRuntimeTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartWorldTargetBridgeBindsRuntimeTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleInteractionTargetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene owner"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	USceneComponent* Root = NewObject<USceneComponent>(Owner);
	Owner->SetRootComponent(Root);
	Root->RegisterComponent();

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Primitive->SetupAttachment(Root);
	Primitive->RegisterComponent();

	UWacomInteractionTargetComponent* InteractionTarget = NewObject<UWacomInteractionTargetComponent>(Owner);
	Owner->AddInstanceComponent(InteractionTarget);
	InteractionTarget->RegisterComponent();

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		NewObject<UWacomBattleEnemyPartWorldTargetBridgeComponent>(Owner);
	Owner->AddInstanceComponent(Bridge);
	Bridge->RegisterComponent();
	UWacomBattleEnemyPartPresentationComponent* Presentation =
		NewObject<UWacomBattleEnemyPartPresentationComponent>(Owner);
	Owner->AddInstanceComponent(Presentation);
	Presentation->RegisterComponent();
	Bridge->SetPartId(TEXT("Test.Part.Head"));
	Bridge->SetBattlePartSlotIdentity(Snapshot.EncounterId, TEXT("Enemy"), TEXT("Test.Part.Head"));

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	Bridge->SyncFromBattleSnapshot(Snapshot);

	TestTrue(TEXT("Bridge binds to current part"), Bridge->IsBoundToBattlePart());
	TestEqual(TEXT("Bridge runtime id matches snapshot"), Bridge->GetPartInstanceId(), HeadInstanceId);
	TestEqual(TEXT("Interaction target gets runtime id"), InteractionTarget->GetTargetId(), HeadInstanceId);
	TestEqual(TEXT("Interaction target gets stable part id"), InteractionTarget->GetStableTargetId(), FName(TEXT("Test.Part.Head")));
	TestTrue(TEXT("Interaction target gets battle enemy part tag"),
		InteractionTarget->GetInteractionTargetTag().MatchesTagExact(WacomTags::Interaction_Target_Battle_EnemyPart));
	TestEqual(TEXT("Bridge binding alone does not register cue target with HUD"),
		HUD->GetBattlePresentationTargetCountForTest(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartWorldTargetBridgeRejectsPartIdOnlyBindingSpec,
	"Wacom.UI.Battle.InteractionTarget.EnemyPartWorldBridge.RejectsPartIdOnlyBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartWorldTargetBridgeRejectsPartIdOnlyBindingSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleInteractionTargetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene owner"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		NewObject<UWacomBattleEnemyPartWorldTargetBridgeComponent>(Owner);
	Owner->AddInstanceComponent(Bridge);
	Bridge->RegisterComponent();
	Bridge->SetPartId(TEXT("Test.Part.Head"));

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);

	Bridge->SyncFromBattleSnapshot(Snapshot);

	TestFalse(TEXT("PartId-only bridge no longer binds by fallback"), Bridge->IsBoundToBattlePart());
	TestEqual(TEXT("Bridge reports missing formal identity"),
		Bridge->GetBattleWorldTargetDebugView().LastBindResult,
		FName(TEXT("MissingBattlePartSlotIdentity")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartWorldTargetBridgeRoutesCueSpec,
	"Wacom.UI.Battle.InteractionTarget.EnemyPartWorldBridge.RoutesTargetCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartWorldTargetBridgeRoutesCueSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleInteractionTargetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene owner"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Primitive->RegisterComponent();
	Primitive->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f));

	UWacomInteractionTargetComponent* InteractionTarget = NewObject<UWacomInteractionTargetComponent>(Owner);
	Owner->AddInstanceComponent(InteractionTarget);
	InteractionTarget->RegisterComponent();

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		NewObject<UWacomBattleEnemyPartWorldTargetBridgeComponent>(Owner);
	Owner->AddInstanceComponent(Bridge);
	Bridge->RegisterComponent();
	UWacomBattleEnemyPartPresentationComponent* Presentation =
		NewObject<UWacomBattleEnemyPartPresentationComponent>(Owner);
	Owner->AddInstanceComponent(Presentation);
	Presentation->RegisterComponent();
	Bridge->SetPartId(TEXT("Test.Part.Body"));
	Presentation->VisualTargetComponent = Primitive;
	Presentation->TargetConfirmPulseScale = 1.25f;

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	Bridge->SetBattlePartSlotIdentity(Snapshot.EncounterId, TEXT("Enemy"), TEXT("Test.Part.Body"));
	Bridge->SyncFromBattleSnapshot(Snapshot);

	const FVector BaseScale = Primitive->GetRelativeScale3D();
	FWacomBattlePresentationTargetCue Cue;
	Cue.CueKind = EWacomBattlePresentationTargetCueKind::TargetConfirmed;
	Presentation->PlayBattlePresentationCue(Cue);
	const FWacomBattleEnemyPartPresentationDebugView View =
		Presentation->GetBattleEnemyPartPresentationDebugView();
	TestEqual(TEXT("Presentation receives target confirm cue"), View.CuePlayCount, 1);
	TestEqual(TEXT("Presentation records target confirm kind"), View.LastCueKind, FName(TEXT("TargetConfirmed")));
	TestEqual(TEXT("Presentation does not mark target confirm as damage"), View.LastCueType, EBattleEventType::None);
	TestEqual(TEXT("Target confirm scales primitive"), Primitive->GetRelativeScale3D(), BaseScale * 1.25f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartWorldTargetBridgeDragPreviewSpec,
	"Wacom.UI.Battle.InteractionTarget.EnemyPartWorldBridge.TracksDragPreviewState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartWorldTargetBridgeDragPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleInteractionTargetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene owner"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Primitive->RegisterComponent();
	Primitive->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f));

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		NewObject<UWacomBattleEnemyPartWorldTargetBridgeComponent>(Owner);
	Owner->AddInstanceComponent(Bridge);
	Bridge->RegisterComponent();
	UWacomBattleEnemyPartPresentationComponent* Presentation =
		NewObject<UWacomBattleEnemyPartPresentationComponent>(Owner);
	Owner->AddInstanceComponent(Presentation);
	Presentation->RegisterComponent();
	Presentation->VisualTargetComponent = Primitive;
	Presentation->DragTargetPreviewScale = 1.15f;

	const FVector BaseScale = Primitive->GetRelativeScale3D();
	Presentation->SetDragTargetPreviewState(EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	FWacomBattleEnemyPartPresentationDebugView View =
		Presentation->GetBattleEnemyPartPresentationDebugView();
	TestTrue(TEXT("Drag preview active"), View.bDragPreviewActive);
	TestEqual(TEXT("Drag preview state recorded"),
		View.DragPreviewState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	TestEqual(TEXT("Drag preview scales primitive"), Primitive->GetRelativeScale3D(), BaseScale * 1.15f);
	TestEqual(TEXT("Drag preview does not count as battle cue"), View.CuePlayCount, 0);

	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	PredictionInput.bHasSourceCard = true;
	PredictionInput.SourceCardInstanceId = FGuid::NewGuid();
	PredictionInput.SourceCardRuntimeCost = 2;
	PredictionInput.bSourceCardSwift = true;
	PredictionInput.bPreviewCanSubmit = true;
	PredictionInput.PreviewRejectReason = TEXT("None");
	Presentation->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		PredictionInput);
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestTrue(TEXT("Prediction input records source card"),
		View.LastDragPredictionDebugInput.SourceCardInstanceId == PredictionInput.SourceCardInstanceId);
	TestEqual(TEXT("Prediction input records runtime cost"),
		View.LastDragPredictionDebugInput.SourceCardRuntimeCost,
		2);
	TestTrue(TEXT("Prediction input records swift flag"),
		View.LastDragPredictionDebugInput.bSourceCardSwift);
	TestTrue(TEXT("Prediction input records submit flag"),
		View.LastDragPredictionDebugInput.bPreviewCanSubmit);
	TestEqual(TEXT("Prediction input records reject reason"),
		View.LastDragPredictionDebugInput.PreviewRejectReason,
		FName(TEXT("None")));
	Presentation->ClearDragTargetPreviewState();
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestFalse(TEXT("Drag preview clears"), View.bDragPreviewActive);
	TestEqual(TEXT("Drag preview state clears"),
		View.DragPreviewState,
		EWacomFirstPersonCardDragTargetFeedbackState::None);
	TestFalse(TEXT("Prediction input clears"), View.LastDragPredictionDebugInput.bHasSourceCard);
	TestEqual(TEXT("Drag preview restores base scale"), Primitive->GetRelativeScale3D(), BaseScale);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartWorldTargetBridgeClearsDestroyedPartSpec,
	"Wacom.UI.Battle.InteractionTarget.EnemyPartWorldBridge.ClearsDestroyedPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartWorldTargetBridgeClearsDestroyedPartSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleInteractionTargetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
	FBattleInitParams Params;
	Params.Character = Character;
	Params.RandomSeed = 1;
	FBattleEnemySlotInit EnemySlot;
	EnemySlot.EnemySlotId = TEXT("Enemy");
	EnemySlot.Enemy = Enemy;
	Params.EnemySlots.Add(EnemySlot);
	Params.PreDestroyedParts.Add(FBattlePartSlotIdentity::Make(
		Params.EncounterId,
		EnemySlot.EnemySlotId,
		TEXT("Test.Part.Body")));
	TestTrue(TEXT("Session initialize"), Session->Initialize(Params).IsOk());

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene owner"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UWacomInteractionTargetComponent* InteractionTarget = NewObject<UWacomInteractionTargetComponent>(Owner);
	Owner->AddInstanceComponent(InteractionTarget);
	InteractionTarget->RegisterComponent();
	InteractionTarget->SetTargetId(FGuid::NewGuid());

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		NewObject<UWacomBattleEnemyPartWorldTargetBridgeComponent>(Owner);
	Owner->AddInstanceComponent(Bridge);
	Bridge->RegisterComponent();
	Bridge->SetPartId(TEXT("Test.Part.Body"));

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session.Get());
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	Bridge->SetBattlePartSlotIdentity(Snapshot.EncounterId, TEXT("Enemy"), TEXT("Test.Part.Body"));
	Bridge->SyncFromBattleSnapshot(Snapshot);

	TestFalse(TEXT("Destroyed part does not bind"), Bridge->IsBoundToBattlePart());
	TestFalse(TEXT("Interaction target runtime id is cleared"), InteractionTarget->GetTargetId().IsValid());
	TestEqual(TEXT("Bridge reports destroyed bind result"),
		Bridge->GetBattleWorldTargetDebugView().LastBindResult, FName(TEXT("PartDestroyed")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneClickRoutesTaggedInteractionTargetSpec,
	"Wacom.UI.Battle.InteractionTarget.SceneClickRoutesTaggedEnemyPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneClickRoutesTaggedInteractionTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleInteractionTargetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { TargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleInteractionTargetSpec::FindFirstHandCardByTargetMode(
		Snapshot, ECardTargetMode::SingleEnemyPart);
	TestTrue(TEXT("Fixture draws target card"), TargetCardId.IsValid());

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleInteractionTargetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleInteractionTargetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestTrue(TEXT("Scene enemy part exists"), SceneEnemy.Parts.Num() > 0))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleInteractionTargetSpec::DestroySceneEnemyHost(SceneEnemy);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleInteractionTargetSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, SceneEnemy.Parts[0], SceneEnemy.Parts[0]->GetHitBounds());

	HUD->SetTargetSelectionStateForTest(TargetCardId);
	TestEqual(TEXT("HUD enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);

	TestTrue(TEXT("Tagged world target routes"), FWacomBattleSceneTargetClickTestAccess::RouteClick(PC));
	WacomBattleInteractionTargetSpec::SettleBattlePresentationQueue(*HUD);
	TestEqual(TEXT("HUD returns idle after routed target"), HUD->GetUIState(), EBattleUIState::Idle);
	TestGreaterThan(TEXT("Playing target card advances battle snapshot"),
		Session->BuildSnapshot().Version,
		Snapshot.Version);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneClickIgnoresUntaggedWorldTargetSpec,
	"Wacom.UI.Battle.InteractionTarget.SceneClickIgnoresUntaggedWorldTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneClickIgnoresUntaggedWorldTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleInteractionTargetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { TargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleInteractionTargetSpec::FindFirstHandCardByTargetMode(
		Snapshot, ECardTargetMode::SingleEnemyPart);
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	TestTrue(TEXT("Fixture draws target card"), TargetCardId.IsValid());

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene owner"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Primitive->RegisterComponent();

	UWacomInteractionTargetComponent* InteractionTarget = NewObject<UWacomInteractionTargetComponent>(Owner);
	Owner->AddInstanceComponent(InteractionTarget);
	InteractionTarget->RegisterComponent();
	InteractionTarget->SetTargetId(HeadInstanceId);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleInteractionTargetSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, Owner, Primitive);

	HUD->SetTargetSelectionStateForTest(TargetCardId);
	TestEqual(TEXT("HUD enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestFalse(TEXT("Untagged world target does not route as battle enemy part"),
		FWacomBattleSceneTargetClickTestAccess::RouteClick(PC));
	TestEqual(TEXT("HUD remains target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestEqual(TEXT("Snapshot version unchanged"), Session->BuildSnapshot().Version, Snapshot.Version);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneProbeUsesOnlyWorldInteractionTargetSpec,
	"Wacom.UI.Battle.InteractionTarget.SceneProbeUsesOnlyWorldInteractionTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneProbeUsesOnlyWorldInteractionTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleInteractionTargetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD);
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("Hit actor"), Owner))
	{
		PC->Destroy();
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, Owner);
	FWacomInteractionTargetHandle MissingProviderHandle;
	TestFalse(TEXT("Actor without world provider is not a drag world target"),
		FWacomBattleSceneTargetClickTestAccess::ProbeTarget(PC, MissingProviderHandle));
	TestFalse(TEXT("No UI fallback target is synthesized"), MissingProviderHandle.IsValid());

	UWacomInteractionTargetComponent* InteractionTarget = NewObject<UWacomInteractionTargetComponent>(Owner);
	Owner->AddInstanceComponent(InteractionTarget);
	InteractionTarget->RegisterComponent();
	const FGuid PartId = FGuid::NewGuid();
	InteractionTarget->SetTargetId(PartId);
	InteractionTarget->SetStableTargetId(TEXT("Test.Part.Head"));
	InteractionTarget->SetInteractionTargetTag(WacomTags::Interaction_Target_Battle_EnemyPart);

	FWacomInteractionTargetHandle Handle;
	TestTrue(TEXT("Actor with provider can be probed"),
		FWacomBattleSceneTargetClickTestAccess::ProbeTarget(PC, Handle));
	TestEqual(TEXT("Probe returns world target"), Handle.TargetKind, EWacomInteractionTargetKind::World);
	TestEqual(TEXT("Probe preserves provider target id"), Handle.WorldTargetId, PartId);
	TestTrue(TEXT("Probe source is interaction target component"), Handle.SourceObject.Get() == InteractionTarget);

	return true;
}
