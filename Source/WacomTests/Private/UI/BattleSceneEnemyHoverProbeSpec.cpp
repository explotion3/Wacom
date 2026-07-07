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
#include "UI/BattleSceneTargetClickTestAccess.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleSceneEnemyHoverProbeSpec
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

	FWacomInteractionTargetHandle MakeBattleEnemyPartHandle(
		UObject* SourceObject,
		const FGuid& WorldTargetId,
		FName StableTargetId,
		FName EncounterId,
		FName EnemySlotId,
		FName PartSlotId,
		const FVector2D& ScreenPosition = FVector2D(240.0f, 120.0f))
	{
		const FName EffectiveStableTargetId = StableTargetId.IsNone() ? FName(TEXT("Test.Part.Head")) : StableTargetId;
		const FName EffectiveEncounterId = EncounterId.IsNone() ? FName(TEXT("Encounter.Test")) : EncounterId;
		const FName EffectiveEnemySlotId = EnemySlotId.IsNone() ? FName(TEXT("Enemy")) : EnemySlotId;
		const FName EffectivePartSlotId = PartSlotId.IsNone() ? FName(TEXT("Head")) : PartSlotId;
		return FWacomInteractionTargetHandle::ForWorldTarget(
			WorldTargetId,
			SourceObject,
			FVector::ZeroVector,
			ScreenPosition,
			WacomTags::Interaction_Target_Battle_EnemyPart,
			EffectiveStableTargetId,
			EffectiveEncounterId,
			EffectiveEnemySlotId,
			EffectivePartSlotId);
	}

	FWacomInteractionTargetHandle MakeBattleEnemyPartHandle(
		const FGuid& WorldTargetId,
		UObject* SourceObject,
		const FVector& /*WorldLocation*/,
		const FVector2D& ScreenPosition,
		const FGameplayTag& /*TargetTag*/,
		FName StableTargetId = NAME_None,
		FName EncounterId = NAME_None,
		FName EnemySlotId = NAME_None,
		FName PartSlotId = NAME_None)
	{
		return MakeBattleEnemyPartHandle(
			SourceObject,
			WorldTargetId,
			StableTargetId,
			EncounterId,
			EnemySlotId,
			PartSlotId,
			ScreenPosition);
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

	void SettleBattlePresentationQueue(UWacomBattleHUDDetailTest& HUD, int32 MaxSteps = 32)
	{
		for (int32 Iteration = 0; HUD.IsBattlePresentationBusy() && Iteration < MaxSteps; ++Iteration)
		{
			HUD.AdvanceBattlePresentationQueueForTest();
		}
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverScalePrioritySpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverScaleDoesNotOverrideDragOrTargetableState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverScalePrioritySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyHoverProbeSpec::FindAutomationWorld();
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
	Presentation->HoverProbeScale = 1.04f;
	Presentation->TargetableAffordanceScale = 1.10f;
	Presentation->DragTargetPreviewScale = 1.20f;

	const FVector BaseScale = Primitive->GetRelativeScale3D();
	const FGuid WorldTargetId = FGuid::NewGuid();
	FWacomInteractionTargetHandle HoverHandle = WacomBattleSceneEnemyHoverProbeSpec::MakeBattleEnemyPartHandle(
		WorldTargetId,
		Bridge,
		FVector::ZeroVector,
		FVector2D(120.0f, 80.0f),
		WacomTags::Interaction_Target_Battle_EnemyPart,
		TEXT("Test.Part.Head"));

	Presentation->SetHoverProbeState(HoverHandle, TEXT("Hovered"));
	TestTrue(TEXT("Presentation reports hover visual state through bridge debug view"),
		Presentation->GetBattleEnemyPartPresentationDebugView().bHoverActive);
	TestEqual(TEXT("Hover scales primitive"), Primitive->GetRelativeScale3D(), BaseScale * 1.04f);

	Presentation->SetTargetableAffordance(true);
	TestEqual(TEXT("Targetable overrides hover scale"), Primitive->GetRelativeScale3D(), BaseScale * 1.10f);

	Presentation->SetDragTargetPreviewState(EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	TestEqual(TEXT("Drag preview overrides targetable and hover"), Primitive->GetRelativeScale3D(), BaseScale * 1.20f);

	Presentation->ClearDragTargetPreviewState();
	TestEqual(TEXT("Clearing drag restores targetable scale"), Primitive->GetRelativeScale3D(), BaseScale * 1.10f);

	Presentation->SetTargetableAffordance(false);
	TestEqual(TEXT("Clearing targetable restores hover scale"), Primitive->GetRelativeScale3D(), BaseScale * 1.04f);

	Presentation->ClearHoverProbeState(TEXT("NoTarget"));
	TestEqual(TEXT("Hover clear reason recorded"),
		Presentation->GetBattleEnemyPartPresentationDebugView().HoverReason,
		FName(TEXT("NoTarget")));
	TestEqual(TEXT("Clearing hover restores base scale"), Primitive->GetRelativeScale3D(), BaseScale);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverProbeSetsBridgeStateSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverProbeSetsBridgeHoverState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverProbeSetsBridgeStateSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyHoverProbeSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(0, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleSceneEnemyHoverProbeSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyHoverProbeSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyHoverProbeSpec::DestroySceneEnemyHost(SceneEnemy);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	PartActor->HoverProbeScale = 1.04f;
	PartActor->RefreshAuthoringState();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleSceneEnemyHoverProbeSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, PartActor, PartActor->GetHitBounds());

	const FVector BaseScale = PartActor->GetVisualLayersRoot()->GetRelativeScale3D();
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();

	const FWacomBattleEnemyPartPresentationDebugView View =
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
	TestTrue(TEXT("Hover becomes active"), View.bHoverActive);
	TestEqual(TEXT("Hover world target id"),
		View.HoverWorldTargetId,
		PartActor->GetWorldTargetBridgeComponent()->GetPartInstanceId());
	TestEqual(TEXT("Hover stable id"), View.HoverStableId, FName(TEXT("Test.Part.Head")));
	TestEqual(TEXT("Hover reason"), View.HoverReason, FName(TEXT("Hovered")));
	TestEqual(TEXT("Hover scales visual layer root"),
		PartActor->GetVisualLayersRoot()->GetRelativeScale3D(),
		BaseScale * PartActor->HoverProbeScale);
	TestFalse(TEXT("Hover prediction is handled by enemy panel"),
		View.PredictionView.bVisible);
	TestEqual(TEXT("Hover clears part prediction badge"),
		View.PredictionView.RejectReason,
		FName(TEXT("EnemyPanelHover")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverProbeTargetSelectPredictionSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartTargetSelectHoverUsesPendingCardPrediction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverProbeTargetSelectPredictionSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyHoverProbeSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(2, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleSceneEnemyHoverProbeSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleSceneEnemyHoverProbeSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyHoverProbeSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor)
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyHoverProbeSpec::DestroySceneEnemyHost(SceneEnemy);
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
	WacomBattleSceneEnemyHoverProbeSpec::SettleBattlePresentationQueue(*HUD);
	HUD->SetTargetSelectionStateForTest(TargetCardId);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, PartActor, PartActor->GetHitBounds());

	HUD->TickBattleSceneEnemyPartHoverProbeForTest();

	const FWacomBattleEnemyPartPresentationDebugView View =
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
	TestTrue(TEXT("TargetSelect hover still activates part focus"), View.bHoverActive);
	TestFalse(TEXT("TargetSelect hover prediction is handled by enemy panel"), View.PredictionView.bVisible);
	TestEqual(TEXT("TargetSelect hover clears part prediction badge"),
		View.PredictionView.RejectReason,
		FName(TEXT("EnemyPanelHover")));
	TestTrue(TEXT("TargetSelect prediction input records source card"),
		View.LastHoverPredictionInput.bHasSourceCard);
	TestEqual(TEXT("TargetSelect prediction input source cost"),
		View.LastHoverPredictionInput.SourceCardRuntimeCost,
		2);
	TestTrue(TEXT("TargetSelect prediction input can submit"),
		View.LastHoverPredictionInput.bPreviewCanSubmit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverProbeTargetSelectInvalidPredictionSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartTargetSelectInvalidHoverShowsRejectPrediction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverProbeTargetSelectInvalidPredictionSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyHoverProbeSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* NoTargetCard = Fx.MakeNoopCard(2);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ NoTargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomBattleSceneEnemyHoverProbeSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::None);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleSceneEnemyHoverProbeSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyHoverProbeSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyHoverProbeSpec::DestroySceneEnemyHost(SceneEnemy);
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
	WacomBattleSceneEnemyHoverProbeSpec::SettleBattlePresentationQueue(*HUD);
	HUD->SetTargetSelectionStateForTest(CardId);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, PartActor, PartActor->GetHitBounds());

	HUD->TickBattleSceneEnemyPartHoverProbeForTest();

	const FWacomBattleEnemyPartPresentationDebugView View =
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
	TestTrue(TEXT("Invalid TargetSelect hover still activates part focus"), View.bHoverActive);
	TestFalse(TEXT("Invalid TargetSelect hover prediction is handled by enemy panel"), View.PredictionView.bVisible);
	TestEqual(TEXT("Invalid TargetSelect hover clears part prediction badge"),
		View.PredictionView.RejectReason,
		FName(TEXT("EnemyPanelHover")));
	TestTrue(TEXT("Invalid TargetSelect prediction input records source card"),
		View.LastHoverPredictionInput.bHasSourceCard);
	TestFalse(TEXT("Invalid TargetSelect prediction input cannot submit"),
		View.LastHoverPredictionInput.bPreviewCanSubmit);
	TestFalse(TEXT("Invalid TargetSelect reject reason present"),
		View.LastHoverPredictionInput.PreviewRejectReason.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverProbeClearsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverProbeClearsWhenTargetChangesOrInvalid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverProbeClearsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyHoverProbeSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(0, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleSceneEnemyHoverProbeSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyHoverProbeSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body") });
	AWacomBattleEnemyPartActor* Head =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	AWacomBattleEnemyPartActor* Body =
		SceneEnemy.Parts.Num() > 1 ? SceneEnemy.Parts[1] : nullptr;
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Body"), Body))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyHoverProbeSpec::DestroySceneEnemyHost(SceneEnemy);
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
	WacomBattleSceneEnemyHoverProbeSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());

	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, Head, Head->GetHitBounds());
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestTrue(TEXT("Head hover active"),
		Head->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().bHoverActive);

	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, Body, Body->GetHitBounds());
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestFalse(TEXT("Head hover clears when target changes"),
		Head->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().bHoverActive);
	TestTrue(TEXT("Body hover active"),
		Body->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().bHoverActive);

	FWacomBattleSceneTargetClickTestAccess::ClearHit(PC);
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	const FWacomBattleEnemyPartPresentationDebugView BodyView =
		Body->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
	TestFalse(TEXT("Body hover clears when target invalid"), BodyView.bHoverActive);
	TestEqual(TEXT("Invalid probe reason recorded"), BodyView.HoverReason, FName(TEXT("NoTarget")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverProbeGatedSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverProbeIsGatedByBattlePhasePendingBarrierAndDrag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverProbeGatedSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyHoverProbeSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomBattleSceneEnemyHoverProbeSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::None);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleSceneEnemyHoverProbeSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyHoverProbeSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyHoverProbeSpec::DestroySceneEnemyHost(SceneEnemy);
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
	WacomBattleSceneEnemyHoverProbeSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, PartActor, PartActor->GetHitBounds());

	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestTrue(TEXT("Hover active before gates"),
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().bHoverActive);

	FBattleEvent Event;
	Event.Type = EBattleEventType::DamageDealt;
	Event.Sequence = 1;
	Event.ActorEnemyPartKey = FBattleEnemyPartKey::Make(
		PartActor->GetWorldTargetBridgeComponent()->GetBoundEncounterId(),
		PartActor->GetWorldTargetBridgeComponent()->GetBoundEnemySlotId(),
		PartActor->GetWorldTargetBridgeComponent()->GetBoundPartSlotId());
	Event.Amount = 1;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });
	HUD->QueuePendingTurnBoundaryWaitForTest();
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestFalse(TEXT("Pending turn boundary clears hover"),
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().bHoverActive);
	TestEqual(TEXT("Pending reason recorded"),
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().HoverReason,
		FName(TEXT("PendingTurnBoundary")));
	HUD->ClearBattlePresentationQueueForTest();
	HUD->ClearPendingTurnBoundaryCommandForTest();

	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestTrue(TEXT("Hover can resume after pending clears"),
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().bHoverActive);

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = CardId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.bHasPointerViewportPosition = true;
	DragView.PointerViewportPosition = FVector2D(540.0f, 590.0f);
	HUD->HandleFirstPersonCardDragStartedForTest(CardId, DragView);
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestFalse(TEXT("First-person drag clears hover"),
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().bHoverActive);
	TestEqual(TEXT("Drag gate reason recorded"),
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().HoverReason,
		FName(TEXT("FirstPersonDrag")));
	HUD->HandleFirstPersonCardDragCancelledForTest(CardId, DragView);

	HUD->SetUIStateForTest(EBattleUIState::BattleEnd);
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestFalse(TEXT("BattleEnd keeps hover cleared"),
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().bHoverActive);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverDebugSummarySpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverDebugSummaryReportsStableTargetState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverDebugSummarySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyHoverProbeSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyPartActor* PartActor =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	if (!TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PartActor))
		{
			PartActor->Destroy();
		}
	};

	const FGuid HoverWorldTargetId = FGuid::NewGuid();
	PartActor->GetPresentationComponent()->SetHoverProbeState(
		WacomBattleSceneEnemyHoverProbeSpec::MakeBattleEnemyPartHandle(
			HoverWorldTargetId,
			PartActor->GetInteractionTargetComponent(),
			FVector::ZeroVector,
			FVector2D(330.0f, 220.0f),
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.Head")),
		TEXT("Hovered"));

	const FWacomBattleSceneEnemyPartDebugView View = PartActor->GetBattleSceneEnemyPartDebugView();
	TestTrue(TEXT("Debug view reports hover active"), View.PresentationDebugView.bHoverActive);
	TestEqual(TEXT("Debug view reports hover id"), View.PresentationDebugView.HoverWorldTargetId, HoverWorldTargetId);
	TestEqual(TEXT("Debug view reports hover stable id"),
		View.PresentationDebugView.HoverStableId,
		FName(TEXT("Test.Part.Head")));
	const FString Summary = PartActor->GetBattleSceneEnemyPartDebugSummary();
	TestTrue(TEXT("Summary reports hover active"), Summary.Contains(TEXT("HoverActive=true")));
	TestTrue(TEXT("Summary reports hover stable id"), Summary.Contains(TEXT("HoverStableId=Test.Part.Head")));
	TestTrue(TEXT("Summary reports hover screen"), Summary.Contains(TEXT("HoverScreen=")));

	return true;
}
