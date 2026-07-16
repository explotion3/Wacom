// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/ChildActorComponent.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Battle/WacomBattleEnemyPartDragPredictionTypes.h"
#include "UI/Battle/WacomBattleEnemyPartPredictionWidget.h"
#include "UI/Battle/WacomBattleEnemyPartVisualLayerTypes.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/BattleSceneTargetClickTestAccess.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Events/BattleEvent.h"
#include "GameFramework/Actor.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopeExit.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleSceneEnemyActorSpec
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

	UPaperFlipbook* MakeOneFrameFlipbookForTest(UObject* Outer)
	{
		UPaperSprite* FrameSprite = NewObject<UPaperSprite>(Outer);
		UPaperFlipbook* Flipbook = NewObject<UPaperFlipbook>(Outer);
		FScopedFlipbookMutator Mutator(Flipbook);
		Mutator.FramesPerSecond = 12.0f;
		FPaperFlipbookKeyFrame KeyFrame;
		KeyFrame.Sprite = FrameSprite;
		KeyFrame.FrameRun = 1;
		Mutator.KeyFrames.Add(KeyFrame);
		return Flipbook;
	}

	EDataValidationResult ValidateObjectForTest(
		const UObject* Object,
		TArray<FText>& OutWarnings,
		TArray<FText>& OutErrors)
	{
		OutWarnings.Reset();
		OutErrors.Reset();
		if (!Object)
		{
			OutErrors.Add(FText::FromString(TEXT("Missing object")));
			return EDataValidationResult::Invalid;
		}

		FDataValidationContext Context;
		const EDataValidationResult Result = Object->IsDataValid(Context);
		Context.SplitIssues(OutWarnings, OutErrors);
		return Result;
	}

	bool ValidationIssuesContain(const TArray<FText>& Issues, const TCHAR* ExpectedText)
	{
		for (const FText& Issue : Issues)
		{
			if (Issue.ToString().Contains(ExpectedText))
			{
				return true;
			}
		}
		return false;
	}

	struct FSlotIdentityEnemyDefinitionFixture
	{
		TStrongObjectPtr<UEnemyDefinition> Enemy;
		TArray<TStrongObjectPtr<UEnemyPartDefinition>> Parts;
	};

	FSlotIdentityEnemyDefinitionFixture MakeSlotIdentityEnemyDefinition()
	{
		FSlotIdentityEnemyDefinitionFixture Result;
		Result.Enemy.Reset(NewObject<UEnemyDefinition>(GetTransientPackage(), NAME_None, RF_Transient));
		Result.Enemy->EnemyId = TEXT("Test.Enemy.SlotIdentity");

		const TArray<TPair<FName, FName>> PartSpecs = {
			{ TEXT("Snake.Head"), TEXT("Head") },
			{ TEXT("Snake.Body"), TEXT("Body") },
			{ TEXT("Snake.Tail"), TEXT("Tail") }
		};
		for (const TPair<FName, FName>& PartSpec : PartSpecs)
		{
			UEnemyPartDefinition* Part =
				NewObject<UEnemyPartDefinition>(GetTransientPackage(), NAME_None, RF_Transient);
			Part->PartId = PartSpec.Key;
			Part->MaxHp = 20;
			Result.Parts.Add(TStrongObjectPtr<UEnemyPartDefinition>(Part));

			FEnemyPartSlot Slot;
			Slot.PartSlotId = PartSpec.Value;
			Slot.PartDef = Part;
			Result.Enemy->Parts.Add(Slot);
		}
		return Result;
	}

	struct FSceneEnemyHostActors
	{
		AWacomBattleEnemyActor* Host = nullptr;
		TArray<AWacomBattleEnemyPartActor*> Parts;
	};

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

	void AttachPartActorToHost(
		AWacomBattleEnemyActor* Host,
		FName PartId,
		FName PartSlotId,
		AWacomBattleEnemyPartActor* PartActor)
	{
		if (!Host || !PartActor)
		{
			return;
		}

		PartActor->PartId = PartId;
		PartActor->PartSlotId = PartSlotId;
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

	FWacomInteractionTargetHandle MakeBattlePartHoverHandle(AWacomBattleEnemyPartActor* PartActor)
	{
		if (!PartActor || !PartActor->GetWorldTargetBridgeComponent() || !PartActor->GetInteractionTargetComponent())
		{
			return FWacomInteractionTargetHandle();
		}

		const UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge = PartActor->GetWorldTargetBridgeComponent();
		return MakeBattleEnemyPartHandle(
			PartActor->GetInteractionTargetComponent(),
			Bridge->GetPartInstanceId(),
			PartActor->GetEffectivePartDefinitionId(),
			Bridge->GetBoundEncounterId(),
			Bridge->GetBoundEnemySlotId(),
			Bridge->GetBoundPartSlotId());
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

	FWacomFirstPersonCardLayerSlotView MakeProjectedCardSlot(
		const FGuid& CardInstanceId,
		const FVector2D& ScreenPosition = FVector2D(500.0f, 600.0f))
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.bIsPlayable = true;
		Slot.ScreenPosition = ScreenPosition;
		Slot.WidgetPosition = ScreenPosition;
		Slot.InputHitCenter = ScreenPosition;
		Slot.RenderScale = 0.55f;
		Slot.RenderOpacity = 1.0f;
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardDragView MakeTargetedCardDragView(
		const FGuid& CardInstanceId,
		const FWacomInteractionTargetHandle& TargetHandle)
	{
		FWacomFirstPersonCardDragView DragView;
		DragView.CardInstanceId = CardInstanceId;
		DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
		DragView.SourceSlotView = MakeProjectedCardSlot(CardInstanceId);
		DragView.PressScreenPosition = FVector2D(500.0f, 600.0f);
		DragView.CurrentScreenPosition = FVector2D(540.0f, 590.0f);
		DragView.PointerViewportPosition = DragView.CurrentScreenPosition;
		DragView.PointerNormalizedViewportPosition = FVector2D::ZeroVector;
		DragView.bHasPointerViewportPosition = true;
		DragView.CurrentTarget = TargetHandle;
		return DragView;
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
	FWacomUIBattleHandSnapshotReportsSwiftSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleHandSnapshotReportsSwiftForPrediction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHandSnapshotReportsSwiftSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SwiftCard = Fx.MakeSimpleDamageCard(1, 1);
	SwiftCard->Keywords.AddTag(WacomTags::Card_Keyword_Swift);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SwiftCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();

	bool bFoundSwift = false;
	for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
	{
		if (Card.Definition == SwiftCard)
		{
			bFoundSwift = true;
			TestTrue(TEXT("Snapshot reports swift keyword"), Card.bIsSwift);
			TestEqual(TEXT("Snapshot keeps runtime cost"), Card.RuntimeCost, 1);
		}
	}
	TestTrue(TEXT("Swift card appears in hand snapshot"), bFoundSwift);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionWidgetFacadeSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartPredictionWidgetFacadeIsReadOnlyScreenSpace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionWidgetFacadeSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
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

	PartActor->PredictionRelativeLocation = FVector(0.f, 12.f, 140.f);
	PartActor->PredictionDrawSize = FVector2D(222.f, 88.f);
	PartActor->RefreshAuthoringState();

	UWidgetComponent* PredictionComponent = PartActor->GetPredictionWidgetComponent();
	TestNotNull(TEXT("Prediction widget component"), PredictionComponent);
	TestEqual(TEXT("Prediction widget relative location"),
		PredictionComponent->GetRelativeLocation(),
		FVector(0.f, 12.f, 140.f));
	TestEqual(TEXT("Prediction widget draw size"), PredictionComponent->GetDrawSize(), FVector2D(222.f, 88.f));
	TestEqual(TEXT("Prediction widget screen-space"), PredictionComponent->GetWidgetSpace(), EWidgetSpace::Screen);
	TestEqual(TEXT("Prediction widget has no collision"),
		PredictionComponent->GetCollisionEnabled(),
		ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Prediction widget does not generate overlap"),
		PredictionComponent->GetGenerateOverlapEvents());
	TestFalse(TEXT("Presentation prediction starts hidden"),
		PartActor->GetPresentationComponent()
			->GetBattleEnemyPartPresentationDebugView()
			.PredictionView.bVisible);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartActorRefreshesFacadeSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartActorRefreshesFacadeAndBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartActorRefreshesFacadeSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
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

	PartActor->PartId = TEXT("Test.Part.Head");
	PartActor->PartSlotId = TEXT("Test.Part.Head");
	PartActor->HitBoundsExtent = FVector(71.f, 53.f, 41.f);
	PartActor->ImpactAnchorRelativeLocation = FVector(7.f, -3.f, 11.f);
	PartActor->TargetableAffordanceScale = 1.07f;
	PartActor->CueHoldSeconds = 0.22f;
	PartActor->RefreshAuthoringState();

	TestEqual(TEXT("Hit bounds extent sync"),
		PartActor->GetHitBounds()->GetUnscaledBoxExtent(),
		FVector(71.f, 53.f, 41.f));
	TestEqual(TEXT("Hit bounds query only"),
		PartActor->GetHitBounds()->GetCollisionEnabled(),
		ECollisionEnabled::QueryOnly);
	TestEqual(TEXT("Hit bounds blocks visibility"),
		PartActor->GetHitBounds()->GetCollisionResponseToChannel(ECC_Visibility),
		ECR_Block);
	TestNotNull(TEXT("Visual layers root exists"), PartActor->GetVisualLayersRoot());
	TestEqual(TEXT("No part visual resources reports none"),
		PartActor->GetBattleSceneEnemyPartDebugView().VisualAuthoringMode,
		FName(TEXT("None")));
	TestEqual(TEXT("Interaction stable id"),
		PartActor->GetInteractionTargetComponent()->GetStableTargetId(),
		FName(TEXT("Test.Part.Head")));
	TestTrue(TEXT("Interaction battle tag"),
		PartActor->GetInteractionTargetComponent()->GetInteractionTargetTag().MatchesTagExact(
			WacomTags::Interaction_Target_Battle_EnemyPart));

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		PartActor->GetWorldTargetBridgeComponent();
	TestNotNull(TEXT("Bridge"), Bridge);
	TestEqual(TEXT("Bridge part id"),
		Bridge->GetBattleWorldTargetDebugView().PartId,
		FName(TEXT("Test.Part.Head")));
	const UWacomBattleEnemyPartPresentationComponent* Presentation = PartActor->GetPresentationComponent();
	TestNotNull(TEXT("Presentation"), Presentation);
	TestTrue(TEXT("Presentation feedback target"),
		Presentation && Presentation->FeedbackTargetComponent == PartActor->GetVisualLayersRoot());
	TestNull(TEXT("Presentation primitive target no longer uses legacy visual"),
		Presentation ? Presentation->VisualTargetComponent.Get() : nullptr);
	TestNotNull(TEXT("Impact anchor exists"), PartActor->GetImpactAnchorComponent());
	TestEqual(TEXT("Impact anchor facade location"),
		PartActor->GetImpactAnchorComponent()->GetRelativeLocation(),
		FVector(7.f, -3.f, 11.f));
	TestEqual(TEXT("Presentation targetable scale"), Presentation ? Presentation->TargetableAffordanceScale : 0.0f, 1.07f);
	TestEqual(TEXT("Presentation hover scale"), Presentation ? Presentation->HoverProbeScale : 0.0f, 1.04f);
	TestEqual(TEXT("Presentation hold seconds"), Presentation ? Presentation->CueHoldSeconds : 0.0f, 0.22f);
	TestTrue(TEXT("Prediction widget component exists"),
		PartActor->GetPredictionWidgetComponent() != nullptr);
	TestEqual(TEXT("Prediction widget relative location"),
		PartActor->GetPredictionWidgetComponent()->GetRelativeLocation(),
		PartActor->PredictionRelativeLocation);
	TestEqual(TEXT("Prediction widget draw size"),
		PartActor->GetPredictionWidgetComponent()->GetDrawSize(),
		PartActor->PredictionDrawSize);
	TestEqual(TEXT("Prediction widget screen space"),
		PartActor->GetPredictionWidgetComponent()->GetWidgetSpace(),
		EWidgetSpace::Screen);
	TestEqual(TEXT("Prediction widget no collision"),
		PartActor->GetPredictionWidgetComponent()->GetCollisionEnabled(),
		ECollisionEnabled::NoCollision);

	const FWacomBattleSceneEnemyPartDebugView DebugView =
		PartActor->GetBattleSceneEnemyPartDebugView();
	TestEqual(TEXT("Debug part id"), DebugView.PartId, FName(TEXT("Test.Part.Head")));
	TestTrue(TEXT("Debug interaction configured"), DebugView.bInteractionTargetConfigured);
	TestTrue(TEXT("Debug impact anchor ready"), DebugView.bImpactAnchorReady);
	TestEqual(TEXT("Debug impact anchor name"), DebugView.ImpactAnchorName, FName(TEXT("ImpactAnchor")));
	TestEqual(TEXT("Debug impact anchor relative location"),
		DebugView.ImpactAnchorRelativeLocation,
		FVector(7.f, -3.f, 11.f));
	TestEqual(TEXT("Details authoring state mirrors debug view"),
		PartActor->AuthoringState,
		DebugView.AuthoringState);
	TestEqual(TEXT("Details authoring ready mirrors debug view"),
		PartActor->bAuthoringReady,
		DebugView.bAuthoringReady);
	TestEqual(TEXT("Details stable target mirrors debug view"),
		PartActor->AuthoringStableSceneTargetId,
		DebugView.StableSceneTargetId);
	TestTrue(TEXT("Debug summary reports part id"),
		PartActor->GetBattleSceneEnemyPartDebugSummary().Contains(TEXT("PartId=Test.Part.Head")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartBridgeRuntimeFactsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartBridgeReportsRuntimeInitiativeFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartBridgeRuntimeFactsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 7, 5, 3);
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

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		NewObject<UWacomBattleEnemyPartWorldTargetBridgeComponent>(Owner);
	Owner->AddInstanceComponent(Bridge);
	Bridge->RegisterComponent();
	UWacomBattleEnemyPartPresentationComponent* Presentation =
		NewObject<UWacomBattleEnemyPartPresentationComponent>(Owner);
	Owner->AddInstanceComponent(Presentation);
	Presentation->RegisterComponent();
	Bridge->SetPartId(TEXT("Test.Part.Head"));
	Bridge->SetBattlePartSlotIdentity(Session->BuildSnapshot().EncounterId, TEXT("Enemy"), TEXT("Test.Part.Head"));

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	FEnemyPartSnapshot MatchedPart;
	Bridge->SyncFromBattleSnapshot(Snapshot, &MatchedPart);
	Presentation->CacheRuntimePartFacts(Bridge->PartId, MatchedPart);

	const FWacomBattleEnemyPartWorldTargetDebugView BridgeView = Bridge->GetBattleWorldTargetDebugView();
	const FWacomBattleEnemyPartPresentationDebugView PresentationView =
		Presentation->GetBattleEnemyPartPresentationDebugView();
	TestTrue(TEXT("Bridge binds to runtime snapshot"), BridgeView.bBoundToSnapshot);
	TestTrue(TEXT("Bridge records runtime part id"), BridgeView.bHasRuntimePartFacts);
	TestTrue(TEXT("Presentation reports runtime facts"), PresentationView.bHasRuntimePartFacts);
	TestEqual(TEXT("Presentation reports current initiative"), PresentationView.CurrentInitiative, 7);
	TestEqual(TEXT("Presentation reports intent id"), PresentationView.CurrentIntentId, FName(TEXT("Test.Part.Head")));
	TestFalse(TEXT("Presentation reports part not destroyed"), PresentationView.bRuntimePartDestroyed);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartActorWorldTargetHandleSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartActorBuildsWorldTargetHandleForPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartActorWorldTargetHandleSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
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
	const FGuid HeadInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);

	WacomBattleSceneEnemyActorSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyActorSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	HUD->RefreshFromSnapshotForTest(Snapshot);

	TestTrue(TEXT("Actor bridge binds to snapshot"),
		PartActor->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
	TestEqual(TEXT("Bridge runtime id matches"),
		PartActor->GetWorldTargetBridgeComponent()->GetPartInstanceId(),
		HeadInstanceId);
	TestTrue(TEXT("Bridge reports runtime facts"),
		PartActor->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bHasRuntimePartFacts);
	TestEqual(TEXT("Presentation reports runtime initiative"),
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().CurrentInitiative,
		5);
	TestEqual(TEXT("Interaction target runtime id"),
		PartActor->GetInteractionTargetComponent()->GetTargetId(),
		HeadInstanceId);

	const FWacomInteractionTargetHandle Handle =
		PartActor->GetInteractionTargetComponent()->BuildWorldTargetHandle();
	TestEqual(TEXT("Handle world target id"), Handle.WorldTargetId, HeadInstanceId);
	TestEqual(TEXT("Handle stable id"), Handle.StableTargetId, FName(TEXT("Test.Part.Head")));
	TestTrue(TEXT("Handle battle enemy tag"),
		Handle.TargetTag.MatchesTagExact(WacomTags::Interaction_Target_Battle_EnemyPart));
	TestTrue(TEXT("Handle source object"),
		Handle.SourceObject.Get() == PartActor->GetInteractionTargetComponent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartActorCueAndDragPreviewSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartActorRoutesCueAndDragPreviewThroughPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartActorCueAndDragPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
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
	PartActor->RefreshAuthoringState();

	const FVector BaseScale = PartActor->GetVisualLayersRoot()->GetRelativeScale3D();

	FWacomBattlePresentationTargetCue Cue;
	Cue.CueKind = EWacomBattlePresentationTargetCueKind::TargetConfirmed;
	Cue.SourceEventType = EBattleEventType::None;
	PartActor->GetPresentationComponent()->PlayBattlePresentationCue(Cue);

	FWacomBattleEnemyPartPresentationDebugView View =
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
	TestEqual(TEXT("Target confirm cue count"), View.CuePlayCount, 1);
	TestTrue(TEXT("Target confirm playback active"), View.bCuePlaybackActive);
	TestEqual(TEXT("Target confirm playback kind"), View.ActiveCueKind, FName(TEXT("TargetConfirmed")));
	TestEqual(TEXT("Target confirm does not scale visual layer root"),
		PartActor->GetVisualLayersRoot()->GetRelativeScale3D(),
		BaseScale);

	PartActor->GetPresentationComponent()->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	View = PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
	TestTrue(TEXT("Drag preview active"), View.bDragPreviewActive);
	TestEqual(TEXT("Drag preview keeps visual layer root authored scale"),
		PartActor->GetVisualLayersRoot()->GetRelativeScale3D(),
		BaseScale);

	PartActor->GetPresentationComponent()->ClearDragTargetPreviewState();
	TestEqual(TEXT("Drag preview restores visual layer root"),
		PartActor->GetVisualLayersRoot()->GetRelativeScale3D(),
		BaseScale);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartActorHiddenComponentsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartActorInternalComponentsRemainHiddenAndNonEditable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartActorHiddenComponentsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomBattleEnemyPartActor> PartActor(NewObject<AWacomBattleEnemyPartActor>());
	TestFalse(TEXT("Hit bounds not editable when inherited"),
		PartActor->GetHitBounds()->IsEditableWhenInherited());
	TestFalse(TEXT("Visual layers root not editable when inherited"),
		PartActor->GetVisualLayersRoot()->IsEditableWhenInherited());
	TestFalse(TEXT("Interaction target not editable when inherited"),
		PartActor->GetInteractionTargetComponent()->IsEditableWhenInherited());
	TestFalse(TEXT("Bridge not editable when inherited"),
		PartActor->GetWorldTargetBridgeComponent()->IsEditableWhenInherited());
	TestFalse(TEXT("Prediction widget not editable when inherited"),
		PartActor->GetPredictionWidgetComponent()->IsEditableWhenInherited());
	TestTrue(TEXT("Hit bounds hides collision category"),
		PartActor->GetHitBounds()->GetClass()->IsFunctionHidden(TEXT("SetCollisionEnabled"))
		|| PartActor->GetHitBounds()->GetClass()->HasMetaData(TEXT("HideCategories")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionHoverInitiativeSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverShowsCurrentInitiativePredictionBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionHoverInitiativeSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 7, 5, 3);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	WacomBattleSceneEnemyActorSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyActorSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	PartActor->GetPresentationComponent()->SetHoverProbeState(
		WacomBattleSceneEnemyActorSpec::MakeBattlePartHoverHandle(PartActor),
		TEXT("Hovered"));

	const FWacomBattleEnemyPartPresentationDebugView DebugView =
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
	TestFalse(TEXT("Hover prediction is handled by enemy panel"), DebugView.PredictionView.bVisible);
	TestEqual(TEXT("Hover clears part prediction badge"),
		DebugView.PredictionView.RejectReason,
		FName(TEXT("EnemyPanelHover")));
	TestEqual(TEXT("Prediction widget component visible"),
		PartActor->GetPredictionWidgetComponent()->IsVisible(),
		false);
	if (const UWacomBattleEnemyPartPredictionWidget* PredictionWidget =
		Cast<UWacomBattleEnemyPartPredictionWidget>(
			PartActor->GetPredictionWidgetComponent()->GetUserWidgetObject()))
	{
		TestFalse(TEXT("Widget hides hover prediction badge"),
			PredictionWidget->GetPredictionView().bVisible);
	}
	else
	{
		TestNull(TEXT("Hidden prediction badge may remain uninitialized"),
			PartActor->GetPredictionWidgetComponent()->GetUserWidgetObject());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionDragValidSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartDragPredictionShowsInitiativeDelta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionDragValidSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(2, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	WacomBattleSceneEnemyActorSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyActorSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	PredictionInput.bHasSourceCard = true;
	PredictionInput.SourceCardInstanceId = FGuid::NewGuid();
	PredictionInput.SourceCardRuntimeCost = 2;
	PredictionInput.bPreviewCanSubmit = true;
	PartActor->GetPresentationComponent()->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		PredictionInput);

	const FWacomBattleEnemyPartPredictionView PredictionView =
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().PredictionView;
	TestTrue(TEXT("Prediction visible"), PredictionView.bVisible);
	TestEqual(TEXT("Prediction mode card"),
		PredictionView.Mode,
		EWacomBattleEnemyPartPredictionMode::CardPrediction);
	TestEqual(TEXT("Predicted initiative"), PredictionView.PredictedInitiative, 3);
	TestFalse(TEXT("Not perfect candidate"), PredictionView.bPerfectReleaseCandidate);
	TestFalse(TEXT("No action risk"), PredictionView.bActionRisk);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionPerfectAndRiskSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartPredictionMarksPerfectAndActionRisk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionPerfectAndRiskSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(5, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	WacomBattleSceneEnemyActorSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyActorSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	PredictionInput.bHasSourceCard = true;
	PredictionInput.SourceCardRuntimeCost = 5;
	PredictionInput.bPreviewCanSubmit = true;
	PartActor->GetPresentationComponent()->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		PredictionInput);

	const FWacomBattleEnemyPartPredictionView PredictionView =
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().PredictionView;
	TestEqual(TEXT("Predicted initiative reaches zero"), PredictionView.PredictedInitiative, 0);
	TestTrue(TEXT("Perfect candidate marked"), PredictionView.bPerfectReleaseCandidate);
	TestTrue(TEXT("Action risk marked"), PredictionView.bActionRisk);
	TestTrue(TEXT("Summary reports perfect candidate"),
		PartActor->GetBattleSceneEnemyPartDebugSummary().Contains(TEXT("PerfectCandidate=true")));
	TestTrue(TEXT("Summary reports action risk"),
		PartActor->GetBattleSceneEnemyPartDebugSummary().Contains(TEXT("ActionRisk=true")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionSwiftSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartPredictionSwiftShowsNoInitiativeDelta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionSwiftSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(5, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	WacomBattleSceneEnemyActorSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyActorSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	PredictionInput.bHasSourceCard = true;
	PredictionInput.SourceCardRuntimeCost = 5;
	PredictionInput.bSourceCardSwift = true;
	PredictionInput.bPreviewCanSubmit = true;
	PartActor->GetPresentationComponent()->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		PredictionInput);

	const FWacomBattleEnemyPartPredictionView PredictionView =
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().PredictionView;
	TestEqual(TEXT("Swift predicted initiative remains current"), PredictionView.PredictedInitiative, 5);
	TestFalse(TEXT("Swift does not mark perfect candidate"), PredictionView.bPerfectReleaseCandidate);
	TestFalse(TEXT("Swift does not mark action risk"), PredictionView.bActionRisk);
	TestTrue(TEXT("Swift detail mentions swift"), PredictionView.DetailText.ToString().Contains(TEXT("迅捷")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartActionPreviewAllActingPartsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartActionPreviewAppliesToAllActingParts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartActionPreviewAllActingPartsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 3) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 1, 1, 1);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId =
		WacomBattleSceneEnemyActorSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::SingleEnemyPart);

	WacomBattleSceneEnemyActorSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyActorSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body"), TEXT("Test.Part.Tail") });
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestEqual(TEXT("Three scene parts"), SceneEnemy.Parts.Num(), 3)
		|| !TestNotNull(TEXT("PlayerController"), PC)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid()))
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
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
	HUD->SetBattleInputReadyForTest(true);

	AWacomBattleEnemyPartActor* TargetPart = SceneEnemy.Parts[0];
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, TargetPart, TargetPart->GetHitBounds());
	FWacomFirstPersonCardDragView DragView =
		WacomBattleSceneEnemyActorSpec::MakeTargetedCardDragView(
			SourceCardId,
			WacomBattleSceneEnemyActorSpec::MakeBattlePartHoverHandle(TargetPart));
	DragView.CurrentTarget = FWacomInteractionTargetHandle();
	DragView.bHasPointerViewportPosition = false;
	const FWacomBattleCardDropResolveResult DropResult =
		HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, DragView);
	if (!DropResult.bCanSubmit)
	{
		AddError(FString::Printf(
			TEXT("Drag target reject debug: %s"),
			*DropResult.ToDebugString()));
	}
	TestEqual(TEXT("Drag target resolves play intent"),
		DropResult.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardWorldTarget);
	TestTrue(TEXT("Drag target can submit"), DropResult.bCanSubmit);

	HUD->HandleFirstPersonCardDragUpdatedForTest(SourceCardId, DragView);

	for (int32 PartIndex = 0; PartIndex < SceneEnemy.Parts.Num(); ++PartIndex)
	{
		AWacomBattleEnemyPartActor* PartActor = SceneEnemy.Parts[PartIndex];
		if (!TestNotNull(FString::Printf(TEXT("Scene part %d"), PartIndex), PartActor)
			|| !TestNotNull(
				FString::Printf(TEXT("Scene part %d presentation"), PartIndex),
				PartActor->GetPresentationComponent()))
		{
			return false;
		}

		const FWacomBattleEnemyPartPresentationDebugView DebugView =
			PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
		TestTrue(FString::Printf(TEXT("Part %d action preview active"), PartIndex),
			DebugView.bActionPreviewPartActive);
		TestTrue(FString::Printf(TEXT("Part %d prediction visible"), PartIndex),
			DebugView.PredictionView.bVisible);
		TestEqual(FString::Printf(TEXT("Part %d prediction mode"), PartIndex),
			DebugView.PredictionView.Mode,
			EWacomBattleEnemyPartPredictionMode::CardPrediction);
		TestEqual(FString::Printf(TEXT("Part %d predicted initiative"), PartIndex),
			DebugView.PredictionView.PredictedInitiative,
			0);
		TestTrue(FString::Printf(TEXT("Part %d action risk"), PartIndex),
			DebugView.PredictionView.bActionRisk);
	}

	HUD->HandleFirstPersonCardDragCancelledForTest(SourceCardId, DragView);
	for (int32 PartIndex = 0; PartIndex < SceneEnemy.Parts.Num(); ++PartIndex)
	{
		const FWacomBattleEnemyPartPresentationDebugView DebugView =
			SceneEnemy.Parts[PartIndex]->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
		TestFalse(FString::Printf(TEXT("Part %d action preview clears"), PartIndex),
			DebugView.bActionPreviewPartActive);
		TestFalse(FString::Printf(TEXT("Part %d prediction hides"), PartIndex),
			DebugView.PredictionView.bVisible);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionInvalidTargetSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartPredictionRejectedTargetDoesNotShowDelta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionInvalidTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(2, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	WacomBattleSceneEnemyActorSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyActorSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	PredictionInput.bHasSourceCard = true;
	PredictionInput.SourceCardRuntimeCost = 2;
	PredictionInput.bPreviewCanSubmit = false;
	PredictionInput.PreviewRejectReason = TEXT("InvalidWorldTarget");
	PartActor->GetPresentationComponent()->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::Invalid,
		PredictionInput);

	const FWacomBattleEnemyPartPredictionView PredictionView =
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().PredictionView;
	TestEqual(TEXT("Rejected prediction mode"),
		PredictionView.Mode,
		EWacomBattleEnemyPartPredictionMode::Rejected);
	TestEqual(TEXT("Rejected prediction keeps current initiative"), PredictionView.PredictedInitiative, 5);
	TestEqual(TEXT("Rejected prediction reason"),
		PredictionView.RejectReason,
		FName(TEXT("InvalidWorldTarget")));
	TestFalse(TEXT("Rejected prediction no perfect marker"), PredictionView.bPerfectReleaseCandidate);
	TestFalse(TEXT("Rejected prediction no action risk marker"), PredictionView.bActionRisk);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartDebugPredictionFactsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartDebugSummaryReportsPredictionReadinessFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartDebugPredictionFactsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
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

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		PartActor->GetWorldTargetBridgeComponent();
	FWacomBattleEnemyPartDragPredictionDebugInput PredictionInput;
	PredictionInput.bHasSourceCard = true;
	PredictionInput.SourceCardInstanceId = FGuid::NewGuid();
	PredictionInput.SourceCardRuntimeCost = 4;
	PredictionInput.bSourceCardSwift = false;
	PredictionInput.bPreviewCanSubmit = false;
	PredictionInput.PreviewRejectReason = TEXT("InvalidWorldTarget");
	PartActor->GetPresentationComponent()->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::Invalid,
		PredictionInput);
	const FGuid HoverWorldTargetId = FGuid::NewGuid();
	PartActor->GetPresentationComponent()->SetHoverProbeState(
		WacomBattleSceneEnemyActorSpec::MakeBattleEnemyPartHandle(
			HoverWorldTargetId,
			Bridge,
			FVector::ZeroVector,
			FVector2D(210.0f, 130.0f),
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.Head")),
		TEXT("Hovered"));

	const FString Summary = PartActor->GetBattleSceneEnemyPartDebugSummary();
	TestTrue(TEXT("Part summary reports drag cost"), Summary.Contains(TEXT("DragCost=4")));
	TestTrue(TEXT("Part summary reports swift flag"), Summary.Contains(TEXT("DragSwift=false")));
	TestTrue(TEXT("Part summary reports submit flag"), Summary.Contains(TEXT("DragCanSubmit=false")));
	TestTrue(TEXT("Part summary reports reject reason"), Summary.Contains(TEXT("DragReject=InvalidWorldTarget")));
	TestTrue(TEXT("Part summary reports hover active"), Summary.Contains(TEXT("HoverActive=true")));
	TestTrue(TEXT("Part summary reports hover stable id"), Summary.Contains(TEXT("HoverStableId=Test.Part.Head")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionBadgeOffsetSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartPredictionBadgeAppliesConfiguredOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionBadgeOffsetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 7, 5, 3);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();

	WacomBattleSceneEnemyActorSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyActorSpec::SpawnSceneEnemyHost(*World, Enemy, { TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor = SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	PartActor->PredictionRelativeLocation = FVector(0.0f, 0.0f, 80.0f);
	PartActor->PredictionBadgeZOffsetWhenVisible = 36.0f;
	PartActor->RefreshAuthoringState();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	HUD->RefreshFromSnapshotForTest(Snapshot);

	const FVector PredictionBaseLocation = PartActor->GetPredictionWidgetComponent()->GetRelativeLocation();
	PartActor->GetPresentationComponent()->SetHoverProbeState(
		WacomBattleSceneEnemyActorSpec::MakeBattleEnemyPartHandle(
			PartActor->GetWorldTargetBridgeComponent()->GetPartInstanceId(),
			PartActor->GetInteractionTargetComponent(),
			FVector::ZeroVector,
			FVector2D(220.0f, 120.0f),
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.Head")),
		TEXT("Hovered"));

	const FWacomBattleEnemyPartPresentationDebugView DebugView =
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
	TestFalse(TEXT("Hover prediction is handled by enemy panel"), DebugView.PredictionView.bVisible);
	TestFalse(TEXT("Prediction offset stays inactive for hover"), DebugView.bPredictionBadgeOffsetActive);
	TestEqual(TEXT("Prediction offset is not applied for hover"),
		PartActor->GetPredictionWidgetComponent()->GetRelativeLocation().Z,
		PredictionBaseLocation.Z);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostBadgeStaggerSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostAppliesStableBadgeStaggerToChildPartActors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostBadgeStaggerSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	WacomBattleSceneEnemyActorSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyActorSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body"), TEXT("Test.Part.Tail") });
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestEqual(TEXT("Three parts spawned"), SceneEnemy.Parts.Num(), 3))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	SceneEnemy.Host->BadgeStaggerHorizontalStep = 18.0f;
	SceneEnemy.Host->BadgeStaggerVerticalStep = 12.0f;
	SceneEnemy.Host->RefreshAttachedPartBadgeLayout();

	TestEqual(TEXT("First stagger index"), SceneEnemy.Parts[0]->GetBadgeLayoutStaggerIndex(), 0);
	TestEqual(TEXT("Middle stagger index"), SceneEnemy.Parts[1]->GetBadgeLayoutStaggerIndex(), 1);
	TestEqual(TEXT("Last stagger index"), SceneEnemy.Parts[2]->GetBadgeLayoutStaggerIndex(), 2);
	TestEqual(TEXT("First stagger offset"),
		SceneEnemy.Parts[0]->GetBadgeLayoutStaggerOffset(),
		FVector(0.0f, -18.0f, 12.0f));
	TestEqual(TEXT("Middle stagger offset"),
		SceneEnemy.Parts[1]->GetBadgeLayoutStaggerOffset(),
		FVector::ZeroVector);
	TestEqual(TEXT("Last stagger offset"),
		SceneEnemy.Parts[2]->GetBadgeLayoutStaggerOffset(),
		FVector(0.0f, 18.0f, 12.0f));
	TestTrue(TEXT("Host debug reports staggered parts"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("BadgeLayoutAppliedParts=3")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartBadgeLayoutDebugSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartBadgeLayoutDebugReportsReadableFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartBadgeLayoutDebugSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
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

	PartActor->SetBadgeLayoutStagger(2, FVector(0.0f, 20.0f, 10.0f));
	PartActor->PredictionBadgeScale = 0.77f;
	PartActor->PredictionBadgeZOffsetWhenVisible = 33.0f;
	PartActor->RefreshAuthoringState();

	const FWacomBattleSceneEnemyPartDebugView View =
		PartActor->GetBattleSceneEnemyPartDebugView();
	TestEqual(TEXT("Debug view reports stagger index"), View.BadgeLayoutStaggerIndex, 2);
	TestEqual(TEXT("Debug view reports stagger offset"),
		View.BadgeLayoutStaggerOffset,
		FVector(0.0f, 20.0f, 10.0f));
	TestEqual(TEXT("Debug view reports prediction scale"), View.PredictionBadgeScale, 0.77f);
	const FString Summary = PartActor->GetBattleSceneEnemyPartDebugSummary();
	TestTrue(TEXT("Summary reports prediction draw size"),
		Summary.Contains(TEXT("PredictionBadgeDrawSize=")));
	TestTrue(TEXT("Summary reports stagger index"),
		Summary.Contains(TEXT("BadgeStaggerIndex=2")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostVisualMakesChildPartsHitOnlySpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostVisualMakesChildPartActorsHitOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostVisualMakesChildPartsHitOnlySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	WacomBattleSceneEnemyActorSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyActorSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body"), TEXT("Test.Part.Tail") });
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestEqual(TEXT("Scene enemy part count"), SceneEnemy.Parts.Num(), 3))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	SceneEnemy.Host->HostSprite = NewObject<UPaperSprite>(SceneEnemy.Host);
	SceneEnemy.Host->HostVisualMode = EWacomBattleEnemyHostVisualMode::StaticSprite;
	SceneEnemy.Host->RefreshBattleEnemyPartAuthoringState();

	TestTrue(TEXT("Host visual active"), SceneEnemy.Host->IsHostVisualActive());
	TestEqual(TEXT("Host details visual mode"),
		SceneEnemy.Host->AuthoringHostVisualMode,
		FName(TEXT("StaticSprite")));
	TestTrue(TEXT("Host details using visual"), SceneEnemy.Host->bAuthoringUsingHostVisual);

	for (AWacomBattleEnemyPartActor* PartActor : SceneEnemy.Parts)
	{
		if (!TestNotNull(TEXT("Part actor"), PartActor))
		{
			return false;
		}

		const FWacomBattleSceneEnemyPartDebugView PartView =
			PartActor->GetBattleSceneEnemyPartDebugView();
		TestEqual(TEXT("Part visual mode becomes hit-only"),
			PartView.VisualAuthoringMode,
			FName(TEXT("HitOnly")));
		TestEqual(TEXT("Part authoring state becomes hit-only"),
			PartView.AuthoringState,
			FName(TEXT("HitOnly")));
		TestTrue(TEXT("Part reports host visual context"), PartView.bUsingHostVisual);
		TestTrue(TEXT("Part reports hit-only visual"), PartView.bHitOnlyVisual);
		TestTrue(TEXT("Part remains target-authoring ready"), PartView.bAuthoringReady);
		TestEqual(TEXT("Details visual mode mirrors hit-only"),
			PartActor->VisualAuthoringMode,
			FName(TEXT("HitOnly")));
		TestTrue(TEXT("Details using host visual"), PartActor->bAuthoringUsingHostVisual);
		TestTrue(TEXT("Details hit-only visual"), PartActor->bAuthoringHitOnlyVisual);
		TestEqual(TEXT("Hit bounds remains query-only"),
			PartActor->GetHitBounds()->GetCollisionEnabled(),
			ECollisionEnabled::QueryOnly);
		TestEqual(TEXT("Hit bounds still blocks visibility"),
			PartActor->GetHitBounds()->GetCollisionResponseToChannel(ECC_Visibility),
			ECR_Block);
		TestTrue(TEXT("Interaction target remains battle enemy part"),
			PartActor->GetInteractionTargetComponent()->GetInteractionTargetTag().MatchesTagExact(
				WacomTags::Interaction_Target_Battle_EnemyPart));
		TestTrue(TEXT("Presentation feedback stays on part visual root"),
			PartActor->GetPresentationComponent()->FeedbackTargetComponent == PartActor->GetVisualLayersRoot());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostVisualDoesNotCreateTargetProviderSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostVisualDoesNotAlterHitBoundsTargetRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostVisualDoesNotCreateTargetProviderSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
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
	const FGuid TargetCardId = WacomBattleSceneEnemyActorSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleSceneEnemyActorSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyActorSpec::SpawnSceneEnemyHost(
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
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	SceneEnemy.Host->HostSprite = NewObject<UPaperSprite>(SceneEnemy.Host);
	SceneEnemy.Host->RefreshBattleEnemyPartAuthoringState();
	UPaperSpriteComponent* HostVisual = SceneEnemy.Host->GetGeneratedHostSpriteVisualComponent();
	if (!TestNotNull(TEXT("Host visual component"), HostVisual))
	{
		return false;
	}

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleSceneEnemyActorSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());

	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, SceneEnemy.Host, HostVisual);
	FWacomInteractionTargetHandle HostVisualHandle;
	TestFalse(TEXT("Host visual component is not a battle target provider"),
		FWacomBattleSceneTargetClickTestAccess::ProbeTarget(PC, HostVisualHandle));
	TestFalse(TEXT("Host visual probe does not synthesize a target"), HostVisualHandle.IsValid());

	HUD->SetTargetSelectionStateForTest(TargetCardId);
	TestEqual(TEXT("Target select test state is active"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, PartActor, PartActor->GetHitBounds());
	TestTrue(TEXT("Part hit bounds still routes with host visual present"),
		FWacomBattleSceneTargetClickTestAccess::RouteClick(PC));
	WacomBattleSceneEnemyActorSpec::SettleBattlePresentationQueue(*HUD);
	TestEqual(TEXT("HUD returns idle after part target"), HUD->GetUIState(), EBattleUIState::Idle);
	TestGreaterThan(TEXT("Target card submission advances snapshot"),
		Session->BuildSnapshot().Version,
		Snapshot.Version);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostVisualBeginPlaySpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostVisualInitializesAtBeginPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostVisualBeginPlaySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	WacomBattleSceneEnemyActorSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyActorSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body"), TEXT("Test.Part.Tail") });
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestEqual(TEXT("Scene enemy part count"), SceneEnemy.Parts.Num(), 3))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	UPaperSprite* HostSprite = NewObject<UPaperSprite>(SceneEnemy.Host);
	SceneEnemy.Host->HostVisualMode = EWacomBattleEnemyHostVisualMode::StaticSprite;
	SceneEnemy.Host->HostSprite = HostSprite;

	for (AWacomBattleEnemyPartActor* PartActor : SceneEnemy.Parts)
	{
		if (PartActor)
		{
			PartActor->SetHostVisualContext(false);
		}
	}
	TestNull(TEXT("Fixture starts without generated host sprite after explicit reset"),
		SceneEnemy.Host->GetGeneratedHostSpriteVisualComponent());

	SceneEnemy.Host->DispatchBeginPlay();

	UPaperSpriteComponent* RuntimeHostVisual =
		SceneEnemy.Host->GetGeneratedHostSpriteVisualComponent();
	if (!TestNotNull(TEXT("BeginPlay regenerates host sprite visual"), RuntimeHostVisual))
	{
		return false;
	}
	TestEqual(TEXT("Runtime host sprite asset"), RuntimeHostVisual->GetSprite(), HostSprite);
	TestTrue(TEXT("Runtime host sprite registered"), RuntimeHostVisual->IsRegistered());
	TestTrue(TEXT("Runtime host sprite visible"), RuntimeHostVisual->IsVisible());
	TestEqual(TEXT("Runtime host sprite no collision"),
		RuntimeHostVisual->GetCollisionEnabled(),
		ECollisionEnabled::NoCollision);
	TestEqual(TEXT("BeginPlay does not refresh authoring generated component count"),
		SceneEnemy.Host->AuthoringGeneratedHostVisualComponentCount,
		0);
	TestEqual(TEXT("BeginPlay does not refresh authoring registered component count"),
		SceneEnemy.Host->AuthoringRegisteredHostVisualComponentCount,
		0);
	TestEqual(TEXT("BeginPlay does not refresh authoring visible component count"),
		SceneEnemy.Host->AuthoringVisibleHostVisualComponentCount,
		0);

	for (AWacomBattleEnemyPartActor* PartActor : SceneEnemy.Parts)
	{
		if (!TestNotNull(TEXT("Part actor"), PartActor))
		{
			return false;
		}

		const FWacomBattleSceneEnemyPartDebugView PartView =
			PartActor->GetBattleSceneEnemyPartDebugView();
		TestEqual(TEXT("BeginPlay switches child part to hit-only"),
			PartView.VisualAuthoringMode,
			FName(TEXT("HitOnly")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostVisualKeepsPartVisualLayersSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostVisualDoesNotOverridePartVisualLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostVisualKeepsPartVisualLayersSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	WacomBattleSceneEnemyActorSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyActorSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	SceneEnemy.Host->HostSprite = NewObject<UPaperSprite>(SceneEnemy.Host);
	FWacomBattleEnemyPartVisualLayer Layer;
	Layer.LayerId = TEXT("Head.Main");
	Layer.Sprite = NewObject<UPaperSprite>(PartActor);
	Layer.RelativeScale3D = FVector(1.5f, 1.25f, 1.0f);
	Layer.SortOrder = 6;
	PartActor->VisualLayers = { Layer };
	SceneEnemy.Host->RefreshBattleEnemyPartAuthoringState();

	const FWacomBattleSceneEnemyPartDebugView PartView =
		PartActor->GetBattleSceneEnemyPartDebugView();
	TestTrue(TEXT("Host visual active"), SceneEnemy.Host->IsHostVisualActive());
	TestEqual(TEXT("VisualLayers override hit-only mode"),
		PartView.VisualAuthoringMode,
		FName(TEXT("VisualLayers")));
	TestEqual(TEXT("Part authoring state remains visual layers"),
		PartView.AuthoringState,
		FName(TEXT("UsingVisualLayers")));
	TestTrue(TEXT("Part still knows host visual context exists"), PartView.bUsingHostVisual);
	TestFalse(TEXT("Part is not hit-only when visual layers exist"), PartView.bHitOnlyVisual);
	TestEqual(TEXT("Generated layer count"), PartView.GeneratedVisualLayerComponentCount, 1);
	TestTrue(TEXT("Presentation feedback stays on part visual layers root"),
		PartActor->GetPresentationComponent()->FeedbackTargetComponent == PartActor->GetVisualLayersRoot());
	TestTrue(TEXT("Host summary still reports host visual"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("UsingHostVisual=true")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostVisualPartFeedbackStaysPerPartSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostVisualPartCueDoesNotScaleHostOrPartVisual",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostVisualPartFeedbackStaysPerPartSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	WacomBattleSceneEnemyActorSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyActorSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyActorSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	SceneEnemy.Host->HostSprite = NewObject<UPaperSprite>(SceneEnemy.Host);
	FWacomBattleEnemyPartVisualLayer Layer;
	Layer.LayerId = TEXT("Head.Main");
	Layer.Sprite = NewObject<UPaperSprite>(PartActor);
	PartActor->VisualLayers = { Layer };
	SceneEnemy.Host->RefreshBattleEnemyPartAuthoringState();

	UPaperSpriteComponent* HostVisual = SceneEnemy.Host->GetGeneratedHostSpriteVisualComponent();
	if (!TestNotNull(TEXT("Host visual component"), HostVisual))
	{
		return false;
	}

	const FVector HostBaseScale = HostVisual->GetRelativeScale3D();
	const FVector PartBaseScale = PartActor->GetVisualLayersRoot()->GetRelativeScale3D();
	FWacomBattlePresentationTargetCue Cue;
	Cue.CueKind = EWacomBattlePresentationTargetCueKind::TargetConfirmed;
	Cue.SourceEventType = EBattleEventType::None;
	PartActor->GetPresentationComponent()->PlayBattlePresentationCue(Cue);

	TestEqual(TEXT("Semantic cue does not scale part visual layers root"),
		PartActor->GetVisualLayersRoot()->GetRelativeScale3D(),
		PartBaseScale);
	TestEqual(TEXT("Semantic cue does not scale host visual"),
		HostVisual->GetRelativeScale3D(),
		HostBaseScale);
	TestTrue(TEXT("Semantic cue playback remains active for future effect consumers"),
		PartActor->GetPresentationComponent()
			->GetBattleEnemyPartPresentationDebugView().bCuePlaybackActive);

	PartActor->GetPresentationComponent()->ClearDragTargetPreviewState();
	PartActor->GetPresentationComponent()->ClearHoverProbeState(TEXT("Test"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartVisualLayersRefreshSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartVisualLayersRefreshRemovesStaleComponentsAndReturnsToNoVisualMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartVisualLayersRefreshSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
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

	FWacomBattleEnemyPartVisualLayer Layer;
	Layer.LayerId = TEXT("Only");
	Layer.Sprite = NewObject<UPaperSprite>(PartActor);
	PartActor->VisualLayers = { Layer };
	PartActor->RefreshAuthoringState();
	TArray<UPaperSpriteComponent*> SpriteComponents;
	PartActor->GetComponents<UPaperSpriteComponent>(SpriteComponents);
	TestEqual(TEXT("One visual layer component generated"), SpriteComponents.Num(), 1);

	PartActor->RefreshAuthoringState();
	SpriteComponents.Reset();
	PartActor->GetComponents<UPaperSpriteComponent>(SpriteComponents);
	TestEqual(TEXT("Repeated refresh does not keep stale components"), SpriteComponents.Num(), 1);

	FWacomBattleEnemyPartVisualLayer FlipbookLayer;
	FlipbookLayer.LayerId = TEXT("FlipbookOnly");
	FlipbookLayer.LayerMode = EWacomBattleEnemyPartVisualLayerMode::Flipbook;
	FlipbookLayer.Flipbook = WacomBattleSceneEnemyActorSpec::MakeOneFrameFlipbookForTest(PartActor);
	PartActor->VisualLayers = { FlipbookLayer };
	PartActor->RefreshAuthoringState();
	SpriteComponents.Reset();
	PartActor->GetComponents<UPaperSpriteComponent>(SpriteComponents);
	TArray<UPaperFlipbookComponent*> FlipbookComponents;
	PartActor->GetComponents<UPaperFlipbookComponent>(FlipbookComponents);
	TestEqual(TEXT("Switching to flipbook removes stale sprite components"), SpriteComponents.Num(), 0);
	TestEqual(TEXT("One flipbook layer component generated"), FlipbookComponents.Num(), 1);

	PartActor->RefreshAuthoringState();
	FlipbookComponents.Reset();
	PartActor->GetComponents<UPaperFlipbookComponent>(FlipbookComponents);
	TestEqual(TEXT("Repeated refresh does not keep stale flipbook components"), FlipbookComponents.Num(), 1);

	PartActor->VisualLayers.Reset();
	PartActor->RefreshAuthoringState();
	SpriteComponents.Reset();
	PartActor->GetComponents<UPaperSpriteComponent>(SpriteComponents);
	FlipbookComponents.Reset();
	PartActor->GetComponents<UPaperFlipbookComponent>(FlipbookComponents);
	TestEqual(TEXT("Clearing layers removes generated components"), SpriteComponents.Num(), 0);
	TestEqual(TEXT("Clearing layers removes generated flipbook components"), FlipbookComponents.Num(), 0);
	TestEqual(TEXT("Clearing layers returns to no visual resource mode"),
		PartActor->GetBattleSceneEnemyPartDebugView().VisualAuthoringMode,
		FName(TEXT("None")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartVisualLayerValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartVisualLayerValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartVisualLayerValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
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

	PartActor->PartId = TEXT("Test.Part.Head");
	PartActor->PartSlotId = TEXT("Test.Part.Head");
	UPaperSprite* ValidSprite = NewObject<UPaperSprite>(PartActor);
	TArray<FText> Warnings;
	TArray<FText> Errors;

	FWacomBattleEnemyPartVisualLayer MissingIdLayer;
	MissingIdLayer.Sprite = ValidSprite;
	PartActor->VisualLayers = { MissingIdLayer };
	EDataValidationResult Result =
		WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(PartActor, Warnings, Errors);
	TestEqual(TEXT("Empty visual layer id invalidates part"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Empty layer id error mentions LayerId"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Errors, TEXT("LayerId")));

	FWacomBattleEnemyPartVisualLayer DuplicateLayerA;
	DuplicateLayerA.LayerId = TEXT("Silhouette");
	DuplicateLayerA.Sprite = ValidSprite;
	FWacomBattleEnemyPartVisualLayer DuplicateLayerB = DuplicateLayerA;
	PartActor->VisualLayers = { DuplicateLayerA, DuplicateLayerB };
	Result = WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(PartActor, Warnings, Errors);
	TestEqual(TEXT("Duplicate visual layer id invalidates part"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Duplicate layer id error mentions id"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Errors, TEXT("Silhouette")));
	TestTrue(TEXT("Debug reports duplicate visual layer id"),
		PartActor->GetBattleSceneEnemyPartDebugView().DuplicateVisualLayerIds.Contains(TEXT("Silhouette")));

	FWacomBattleEnemyPartVisualLayer ZeroScaleLayer;
	ZeroScaleLayer.LayerId = TEXT("ZeroScale");
	ZeroScaleLayer.Sprite = ValidSprite;
	ZeroScaleLayer.RelativeScale3D = FVector(1.0f, 0.0f, 1.0f);
	PartActor->VisualLayers = { ZeroScaleLayer };
	Result = WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(PartActor, Warnings, Errors);
	TestEqual(TEXT("Zero visual layer scale invalidates part"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Zero scale error mentions RelativeScale3D"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Errors, TEXT("RelativeScale3D")));

	FWacomBattleEnemyPartVisualLayer MissingSpriteLayer;
	MissingSpriteLayer.LayerId = TEXT("MissingSprite");
	PartActor->VisualLayers = { MissingSpriteLayer };
	Result = WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(PartActor, Warnings, Errors);
	TestEqual(TEXT("Missing visual layer sprite is warning only"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Missing sprite has no errors"), Errors.Num(), 0);
	TestTrue(TEXT("Missing sprite warning mentions Sprite"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Warnings, TEXT("StaticSprite")));
	TestEqual(TEXT("Debug counts missing visual asset"),
		PartActor->GetBattleSceneEnemyPartDebugView().MissingVisualLayerAssetCount,
		1);
	TestEqual(TEXT("Debug counts missing sprite"),
		PartActor->GetBattleSceneEnemyPartDebugView().MissingVisualLayerSpriteCount,
		1);

	FWacomBattleEnemyPartVisualLayer MissingFlipbookLayer;
	MissingFlipbookLayer.LayerId = TEXT("MissingFlipbook");
	MissingFlipbookLayer.LayerMode = EWacomBattleEnemyPartVisualLayerMode::Flipbook;
	PartActor->VisualLayers = { MissingFlipbookLayer };
	Result = WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(PartActor, Warnings, Errors);
	TestEqual(TEXT("Missing visual layer flipbook is warning only"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Missing flipbook has no errors"), Errors.Num(), 0);
	TestTrue(TEXT("Missing flipbook warning mentions Flipbook"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Warnings, TEXT("Flipbook")));
	TestEqual(TEXT("Debug counts missing flipbook"),
		PartActor->GetBattleSceneEnemyPartDebugView().MissingVisualLayerFlipbookCount,
		1);

	PartActor->VisualLayers.Reset();
	Result = WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(PartActor, Warnings, Errors);
	TestEqual(TEXT("Missing all visual resources is warning only"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Missing all visual resources has no errors"), Errors.Num(), 0);
	TestTrue(TEXT("Missing visual resource warning mentions VisualLayers"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Warnings, TEXT("VisualLayers")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostVisualValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostVisualAndHitOnlyPartsValidateAsLegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostVisualValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	const WacomBattleSceneEnemyActorSpec::FSlotIdentityEnemyDefinitionFixture EnemyFixture =
		WacomBattleSceneEnemyActorSpec::MakeSlotIdentityEnemyDefinition();
	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Head =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* Body =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(200.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* Tail =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(300.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host)
		|| !TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Body"), Body)
		|| !TestNotNull(TEXT("Tail"), Tail))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Tail))
		{
			Tail->Destroy();
		}
		if (IsValid(Body))
		{
			Body->Destroy();
		}
		if (IsValid(Head))
		{
			Head->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Host->EnemyDefinition = EnemyFixture.Enemy.Get();
	Host->HostSprite = NewObject<UPaperSprite>(Host);
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Snake.Head"), TEXT("Head"), Head);
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Snake.Body"), TEXT("Body"), Body);
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Snake.Tail"), TEXT("Tail"), Tail);
	for (AWacomBattleEnemyPartActor* PartActor : { Head, Body, Tail })
	{
		PartActor->VisualLayers.Reset();
	}

	TArray<FText> Warnings;
	TArray<FText> Errors;
	EDataValidationResult Result =
		WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("Host visual plus hit-only parts is valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Legal hit-only host has no errors"), Errors.Num(), 0);
	TestFalse(TEXT("Legal hit-only host does not warn about no-art enemy"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Warnings, TEXT("只有命中体")));
	TestEqual(TEXT("Head is hit-only"), Head->VisualAuthoringMode, FName(TEXT("HitOnly")));
	TestEqual(TEXT("Host authoring ready"), Host->AuthoringState, FName(TEXT("Ready")));

	Result = WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(Head, Warnings, Errors);
	TestEqual(TEXT("Hit-only part validation is valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Hit-only part has no errors"), Errors.Num(), 0);
	TestFalse(TEXT("Hit-only part does not warn about missing independent visual"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Warnings, TEXT("VisualLayers")));

	Host->HostSprite = nullptr;
	Host->RefreshBattleEnemyPartAuthoringState();
	Result = WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("No-art enemy is warning only"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("No-art enemy has no errors"), Errors.Num(), 0);
	TestTrue(TEXT("No-art enemy warning mentions hit-only debug-only state"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Warnings, TEXT("只有命中体")));

	Host->HostSprite = NewObject<UPaperSprite>(Host);
	Head->HitBoundsExtent = FVector(0.0f, 42.0f, 42.0f);
	Host->RefreshBattleEnemyPartAuthoringState();
	Result = WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(Head, Warnings, Errors);
	TestEqual(TEXT("Host visual does not hide invalid hit bounds"),
		Result,
		EDataValidationResult::Invalid);
	TestTrue(TEXT("Invalid hit bounds error remains visible"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Errors, TEXT("HitBoundsExtent")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyBlueprintDefaultsValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyBlueprintDefaultsRemainValidForAuthoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyBlueprintDefaultsValidationSpec::RunTest(const FString& /*Parameters*/)
{
	const AWacomBattleEnemyPartActor* PartCDO =
		GetDefault<AWacomBattleEnemyPartActor>();
	const AWacomBattleEnemyActor* HostCDO =
		GetDefault<AWacomBattleEnemyActor>();
	TArray<FText> Warnings;
	TArray<FText> Errors;

	TestEqual(TEXT("Part CDO remains valid"),
		WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(PartCDO, Warnings, Errors),
		EDataValidationResult::Valid);
	TestEqual(TEXT("Part CDO no warnings"), Warnings.Num(), 0);
	TestEqual(TEXT("Part CDO no errors"), Errors.Num(), 0);

	TestEqual(TEXT("Host CDO remains valid"),
		WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(HostCDO, Warnings, Errors),
		EDataValidationResult::Valid);
	TestEqual(TEXT("Host CDO no warnings"), Warnings.Num(), 0);
	TestEqual(TEXT("Host CDO no errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostReportsChildPartActorFactsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostReportsChildPartActorFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostReportsChildPartActorFactsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 7, 5, 3);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Head =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* Body =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(200.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host)
		|| !TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Body"), Body))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Head))
		{
			Head->Destroy();
		}
		if (IsValid(Body))
		{
			Body->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Host->EnemyDefinition = Enemy;
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Head"), Head);
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Body"), Body);

	Host->RefreshBattleEnemyPartAuthoringState();
	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Part actor count"), View.AttachedPartActorCount, 2);
	TestTrue(TEXT("Head part id included"), View.AttachedPartIds.Contains(TEXT("Test.Part.Head")));
	TestTrue(TEXT("Body part id included"), View.AttachedPartIds.Contains(TEXT("Test.Part.Body")));
	TestTrue(TEXT("Head part slot is explicitly resolved from enemy definition"),
		View.AttachedPartSlotIds.Contains(TEXT("Test.Part.Head")));
	TestTrue(TEXT("Default stable scene target includes host slot"),
		View.StableSceneTargetIds.Contains(TEXT("Enemy.Test.Part.Head")));
	TestEqual(TEXT("Details host authoring state mirrors debug view"),
		Host->AuthoringState,
		View.AuthoringState);
	TestEqual(TEXT("Details host authoring ready mirrors debug view"),
		Host->bAuthoringReady,
		View.bAuthoringReady);
	TestEqual(TEXT("Details host part count mirrors debug view"),
		Host->AuthoringPartActorCount,
		View.AttachedPartActorCount);
	TestTrue(TEXT("Details host part ids include head"),
		Host->AuthoringPartIds.Contains(TEXT("Test.Part.Head")));
	TestTrue(TEXT("Details host stable target includes head"),
		Host->AuthoringStableSceneTargetIds.Contains(TEXT("Enemy.Test.Part.Head")));
	TestTrue(TEXT("Details host summary reports count"),
		Host->AuthoringDebugSummary.Contains(TEXT("PartCount=2")));
	TestTrue(TEXT("Host summary reports count"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("PartCount=2")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostChildPartActorPrefabPathSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostUsesOnlyChildPartActors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostChildPartActorPrefabPathSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Head =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* DetachedPartActor =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(200.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host)
		|| !TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Detached part actor"), DetachedPartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Head))
		{
			Head->Destroy();
		}
		if (IsValid(DetachedPartActor))
		{
			DetachedPartActor->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Host->EnemySlotId = TEXT("SnakeA");
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(
		Host,
		TEXT("Test.Part.Head"),
		TEXT("Head"),
		Head);
	DetachedPartActor->PartId = TEXT("Unattached.BeforeRefresh");

	Host->RefreshBattleEnemyPartAuthoringState();
	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Only attached child part is used"), View.AttachedPartActorCount, 1);
	TestTrue(TEXT("Child part id included"), View.AttachedPartIds.Contains(TEXT("Test.Part.Head")));
	TestFalse(TEXT("Unattached part actor ignored"),
		View.AttachedPartIds.Contains(TEXT("Unattached.BeforeRefresh")));
	TestTrue(TEXT("Child part slot id included"), View.AttachedPartSlotIds.Contains(TEXT("Head")));
	TestTrue(TEXT("Stable scene target combines enemy and part slots"),
		View.StableSceneTargetIds.Contains(TEXT("SnakeA.Head")));
	TestEqual(TEXT("Host injects enemy slot into child part"), Head->GetBattleSceneEnemyPartDebugView().EnemySlotId,
		FName(TEXT("SnakeA")));
	TestEqual(TEXT("Detached part actor is not rewritten"), DetachedPartActor->PartId,
		FName(TEXT("Unattached.BeforeRefresh")));
	TestTrue(TEXT("Host summary reports stable target"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("StableSceneTargets=[SnakeA.Head]")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostChildActorComponentPrefabPathSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostScansChildActorComponents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostChildActorComponentPrefabPathSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
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

	Host->EnemySlotId = TEXT("SnakeB");

	UChildActorComponent* ChildPartComponent =
		NewObject<UChildActorComponent>(Host, TEXT("PrefabHeadPart"));
	if (!TestNotNull(TEXT("Child part component"), ChildPartComponent))
	{
		return false;
	}

	ChildPartComponent->SetupAttachment(Host->GetRootComponent());
	Host->AddInstanceComponent(ChildPartComponent);
	ChildPartComponent->RegisterComponent();
	ChildPartComponent->SetChildActorClass(AWacomBattleEnemyPartActor::StaticClass());

	AWacomBattleEnemyPartActor* Head =
		Cast<AWacomBattleEnemyPartActor>(ChildPartComponent->GetChildActor());
	if (!TestNotNull(TEXT("Child actor part"), Head))
	{
		return false;
	}

	Head->PartId = TEXT("Test.Part.Head");
	Head->PartSlotId = TEXT("Head");

	Host->RefreshBattleEnemyPartAuthoringState();
	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Child actor component part count"), View.AttachedPartActorCount, 1);
	TestTrue(TEXT("Child actor component part id included"),
		View.AttachedPartIds.Contains(TEXT("Test.Part.Head")));
	TestTrue(TEXT("Child actor component part slot id included"),
		View.AttachedPartSlotIds.Contains(TEXT("Head")));
	TestTrue(TEXT("Child actor component stable scene target included"),
		View.StableSceneTargetIds.Contains(TEXT("SnakeB.Head")));
	TestEqual(TEXT("Host injects enemy slot into child actor component part"),
		Head->GetBattleSceneEnemyPartDebugView().EnemySlotId,
		FName(TEXT("SnakeB")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostChildActorComponentInstanceDoesNotDoubleCountTemplateSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostChildActorComponentsPreferInstancesOverTemplates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostChildActorComponentInstanceDoesNotDoubleCountTemplateSpec::RunTest(
	const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
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

	Host->EnemySlotId = TEXT("Enemy");

	UChildActorComponent* HeadComponent = NewObject<UChildActorComponent>(Host, TEXT("SnakeHeadPart"));
	UChildActorComponent* BodyComponent = NewObject<UChildActorComponent>(Host, TEXT("SnakeBodyPart"));
	UChildActorComponent* TailComponent = NewObject<UChildActorComponent>(Host, TEXT("SnakeTailPart"));
	if (!TestNotNull(TEXT("Head child component"), HeadComponent)
		|| !TestNotNull(TEXT("Body child component"), BodyComponent)
		|| !TestNotNull(TEXT("Tail child component"), TailComponent))
	{
		return false;
	}

	for (UChildActorComponent* ChildComponent : { HeadComponent, BodyComponent, TailComponent })
	{
		ChildComponent->SetupAttachment(Host->GetRootComponent());
		Host->AddInstanceComponent(ChildComponent);
		ChildComponent->RegisterComponent();
		ChildComponent->SetChildActorClass(AWacomBattleEnemyPartActor::StaticClass());
	}

	AWacomBattleEnemyPartActor* Head = Cast<AWacomBattleEnemyPartActor>(HeadComponent->GetChildActor());
	AWacomBattleEnemyPartActor* Body = Cast<AWacomBattleEnemyPartActor>(BodyComponent->GetChildActor());
	AWacomBattleEnemyPartActor* Tail = Cast<AWacomBattleEnemyPartActor>(TailComponent->GetChildActor());
	if (!TestNotNull(TEXT("Head child actor"), Head)
		|| !TestNotNull(TEXT("Body child actor"), Body)
		|| !TestNotNull(TEXT("Tail child actor"), Tail))
	{
		return false;
	}

	Head->PartId = TEXT("Snake.Head");
	Head->PartSlotId = TEXT("Head");
	Body->PartId = TEXT("Snake.Body");
	Body->PartSlotId = TEXT("Body");
	Tail->PartId = TEXT("Snake.Tail");
	Tail->PartSlotId = TEXT("Tail");

	Host->RefreshBattleEnemyPartAuthoringState();
	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Only live child actor instances are counted"), View.AttachedPartActorCount, 3);
	TestEqual(TEXT("No duplicate slot ids from child actor templates"), View.DuplicatePartSlotIds.Num(), 0);
	TestTrue(TEXT("Head slot included once"), View.AttachedPartSlotIds.Contains(TEXT("Head")));
	TestTrue(TEXT("Body slot included once"), View.AttachedPartSlotIds.Contains(TEXT("Body")));
	TestTrue(TEXT("Tail slot included once"), View.AttachedPartSlotIds.Contains(TEXT("Tail")));
	TestTrue(TEXT("Stable scene targets use live child actors"),
		View.StableSceneTargetIds.Contains(TEXT("Enemy.Head"))
		&& View.StableSceneTargetIds.Contains(TEXT("Enemy.Body"))
		&& View.StableSceneTargetIds.Contains(TEXT("Enemy.Tail")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostRefreshDoesNotFillBlankChildActorIdentitySpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostRefreshDoesNotFillBlankChildActorIdentityFromDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostRefreshDoesNotFillBlankChildActorIdentitySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
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

	const WacomBattleSceneEnemyActorSpec::FSlotIdentityEnemyDefinitionFixture Definition =
		WacomBattleSceneEnemyActorSpec::MakeSlotIdentityEnemyDefinition();
	Host->EnemyDefinition = Definition.Enemy.Get();

	UChildActorComponent* HeadComponent = NewObject<UChildActorComponent>(Host, TEXT("SnakeHeadPart"));
	UChildActorComponent* BodyComponent = NewObject<UChildActorComponent>(Host, TEXT("SnakeBodyPart"));
	UChildActorComponent* TailComponent = NewObject<UChildActorComponent>(Host, TEXT("SnakeTailPart"));
	if (!TestNotNull(TEXT("Head child component"), HeadComponent)
		|| !TestNotNull(TEXT("Body child component"), BodyComponent)
		|| !TestNotNull(TEXT("Tail child component"), TailComponent))
	{
		return false;
	}

	for (UChildActorComponent* ChildComponent : { HeadComponent, BodyComponent, TailComponent })
	{
		ChildComponent->SetupAttachment(Host->GetRootComponent());
		Host->AddInstanceComponent(ChildComponent);
		ChildComponent->RegisterComponent();
		ChildComponent->SetChildActorClass(AWacomBattleEnemyPartActor::StaticClass());
	}

	AWacomBattleEnemyPartActor* Head = Cast<AWacomBattleEnemyPartActor>(HeadComponent->GetChildActor());
	AWacomBattleEnemyPartActor* Body = Cast<AWacomBattleEnemyPartActor>(BodyComponent->GetChildActor());
	AWacomBattleEnemyPartActor* Tail = Cast<AWacomBattleEnemyPartActor>(TailComponent->GetChildActor());
	if (!TestNotNull(TEXT("Head part"), Head)
		|| !TestNotNull(TEXT("Body part"), Body)
		|| !TestNotNull(TEXT("Tail part"), Tail))
	{
		return false;
	}

	TestTrue(TEXT("Blank head starts without identity"), Head->PartId.IsNone() && Head->PartSlotId.IsNone());
	TestTrue(TEXT("Blank body starts without identity"), Body->PartId.IsNone() && Body->PartSlotId.IsNone());
	TestTrue(TEXT("Blank tail starts without identity"), Tail->PartId.IsNone() && Tail->PartSlotId.IsNone());

	Host->RefreshBattleEnemyPartAuthoringState();

	TestTrue(TEXT("Head identity is not auto-filled"), Head->PartId.IsNone() && Head->PartSlotId.IsNone());
	TestTrue(TEXT("Body identity is not auto-filled"), Body->PartId.IsNone() && Body->PartSlotId.IsNone());
	TestTrue(TEXT("Tail identity is not auto-filled"), Tail->PartId.IsNone() && Tail->PartSlotId.IsNone());
	if (AWacomBattleEnemyPartActor* HeadTemplate =
		Cast<AWacomBattleEnemyPartActor>(HeadComponent->GetChildActorTemplate()))
	{
		TestTrue(TEXT("Head template identity is not auto-filled"),
			HeadTemplate->PartId.IsNone() && HeadTemplate->PartSlotId.IsNone());
	}

	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Host remains not ready without explicit identities"), View.AuthoringState, FName(TEXT("PartSlotMismatch")));
	TestFalse(TEXT("Details host stays not ready without explicit identities"), Host->bAuthoringReady);
	TestFalse(TEXT("Details stable scene target does not invent head"),
		Host->AuthoringStableSceneTargetIds.Contains(TEXT("Enemy.Head")));
	TestEqual(TEXT("Head details report missing identity"),
		Head->AuthoringState,
		FName(TEXT("MissingIdentity")));
	TestFalse(TEXT("Head details not ready"), Head->bAuthoringReady);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostConfigureDebugSnakeSampleSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostConfigureDebugSnakeSampleUsesExistingHeadBodyTailParts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostConfigureDebugSnakeSampleSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
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

	UChildActorComponent* HeadComponent = NewObject<UChildActorComponent>(Host, TEXT("SnakeHeadPart"));
	UChildActorComponent* BodyComponent = NewObject<UChildActorComponent>(Host, TEXT("SnakeBodyPart"));
	UChildActorComponent* TailComponent = NewObject<UChildActorComponent>(Host, TEXT("SnakeTailPart"));
	if (!TestNotNull(TEXT("Head child component"), HeadComponent)
		|| !TestNotNull(TEXT("Body child component"), BodyComponent)
		|| !TestNotNull(TEXT("Tail child component"), TailComponent))
	{
		return false;
	}

	for (UChildActorComponent* ChildComponent : { HeadComponent, BodyComponent, TailComponent })
	{
		ChildComponent->SetupAttachment(Host->GetRootComponent());
		Host->AddInstanceComponent(ChildComponent);
		ChildComponent->RegisterComponent();
		ChildComponent->SetChildActorClass(AWacomBattleEnemyPartActor::StaticClass());
	}

	AWacomBattleEnemyPartActor* Head = Cast<AWacomBattleEnemyPartActor>(HeadComponent->GetChildActor());
	AWacomBattleEnemyPartActor* Body = Cast<AWacomBattleEnemyPartActor>(BodyComponent->GetChildActor());
	AWacomBattleEnemyPartActor* Tail = Cast<AWacomBattleEnemyPartActor>(TailComponent->GetChildActor());
	if (!TestNotNull(TEXT("Head part"), Head)
		|| !TestNotNull(TEXT("Body part"), Body)
		|| !TestNotNull(TEXT("Tail part"), Tail))
	{
		return false;
	}

	Host->ConfigureDebugSnakeHostSample();

	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Snake sample enemy slot"), View.EnemySlotId, FName(TEXT("Enemy")));
	TestEqual(TEXT("Snake sample part count"), View.AttachedPartActorCount, 3);
	TestTrue(TEXT("Snake sample head part id"), View.AttachedPartIds.Contains(TEXT("Snake.Head")));
	TestTrue(TEXT("Snake sample body part id"), View.AttachedPartIds.Contains(TEXT("Snake.Body")));
	TestTrue(TEXT("Snake sample tail part id"), View.AttachedPartIds.Contains(TEXT("Snake.Tail")));
	TestTrue(TEXT("Snake sample head part slot"), View.AttachedPartSlotIds.Contains(TEXT("Head")));
	TestTrue(TEXT("Snake sample body part slot"), View.AttachedPartSlotIds.Contains(TEXT("Body")));
	TestTrue(TEXT("Snake sample tail part slot"), View.AttachedPartSlotIds.Contains(TEXT("Tail")));
	TestTrue(TEXT("Snake sample head stable target"), View.StableSceneTargetIds.Contains(TEXT("Enemy.Head")));
	TestTrue(TEXT("Snake sample body stable target"), View.StableSceneTargetIds.Contains(TEXT("Enemy.Body")));
	TestTrue(TEXT("Snake sample tail stable target"), View.StableSceneTargetIds.Contains(TEXT("Enemy.Tail")));
	TestEqual(TEXT("Snake head part id"), Head->PartId, FName(TEXT("Snake.Head")));
	TestEqual(TEXT("Snake head part slot"), Head->PartSlotId, FName(TEXT("Head")));
	TestEqual(TEXT("Snake body part id"), Body->PartId, FName(TEXT("Snake.Body")));
	TestEqual(TEXT("Snake body part slot"), Body->PartSlotId, FName(TEXT("Body")));
	TestEqual(TEXT("Snake tail part id"), Tail->PartId, FName(TEXT("Snake.Tail")));
	TestEqual(TEXT("Snake tail part slot"), Tail->PartSlotId, FName(TEXT("Tail")));
	TestEqual(TEXT("Snake head local position"), HeadComponent->GetRelativeLocation(), FVector(96.0f, -6.0f, 16.0f));
	TestEqual(TEXT("Snake body local position"), BodyComponent->GetRelativeLocation(), FVector::ZeroVector);
	TestEqual(TEXT("Snake tail local position"), TailComponent->GetRelativeLocation(), FVector(-92.0f, 16.0f, -8.0f));
	TestTrue(TEXT("Snake head badge stagger applied"), Head->GetBadgeLayoutStaggerIndex() != INDEX_NONE);
	TestTrue(TEXT("Snake body badge stagger applied"), Body->GetBadgeLayoutStaggerIndex() != INDEX_NONE);
	TestTrue(TEXT("Snake tail badge stagger applied"), Tail->GetBadgeLayoutStaggerIndex() != INDEX_NONE);
	TestEqual(TEXT("Snake sample missing definition parts"), View.MissingDefinitionPartIds.Num(), 0);
	TestTrue(TEXT("Snake sample summary reports stable target"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("StableSceneTargets=[Enemy.Head,Enemy.Body,Enemy.Tail]")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostConfigureDebugSnakeTemplateSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostConfigureDebugSnakeSampleUpdatesChildActorTemplates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostConfigureDebugSnakeTemplateSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomBattleEnemyActor> Host(NewObject<AWacomBattleEnemyActor>(
		GetTransientPackage(),
		TEXT("BP_SnakeHost_Debug_CDO"),
		RF_ArchetypeObject | RF_Transactional));
	UChildActorComponent* HeadComponent = NewObject<UChildActorComponent>(
		Host.Get(),
		TEXT("SnakeHeadPart"),
		RF_ArchetypeObject | RF_Transactional);
	UChildActorComponent* BodyComponent = NewObject<UChildActorComponent>(
		Host.Get(),
		TEXT("SnakeBodyPart"),
		RF_ArchetypeObject | RF_Transactional);
	UChildActorComponent* TailComponent = NewObject<UChildActorComponent>(
		Host.Get(),
		TEXT("SnakeTailPart"),
		RF_ArchetypeObject | RF_Transactional);
	if (!TestNotNull(TEXT("Head template component"), HeadComponent)
		|| !TestNotNull(TEXT("Body template component"), BodyComponent)
		|| !TestNotNull(TEXT("Tail template component"), TailComponent))
	{
		return false;
	}

	for (UChildActorComponent* ChildComponent : { HeadComponent, BodyComponent, TailComponent })
	{
		ChildComponent->SetupAttachment(Host->GetRootComponent());
		Host->AddInstanceComponent(ChildComponent);
		ChildComponent->SetChildActorClass(AWacomBattleEnemyPartActor::StaticClass());
	}

	AWacomBattleEnemyPartActor* HeadTemplate =
		Cast<AWacomBattleEnemyPartActor>(HeadComponent->GetChildActorTemplate());
	AWacomBattleEnemyPartActor* BodyTemplate =
		Cast<AWacomBattleEnemyPartActor>(BodyComponent->GetChildActorTemplate());
	AWacomBattleEnemyPartActor* TailTemplate =
		Cast<AWacomBattleEnemyPartActor>(TailComponent->GetChildActorTemplate());
	if (!TestNotNull(TEXT("Head child actor template"), HeadTemplate)
		|| !TestNotNull(TEXT("Body child actor template"), BodyTemplate)
		|| !TestNotNull(TEXT("Tail child actor template"), TailTemplate))
	{
		return false;
	}

	Host->ConfigureDebugSnakeHostSample();

	TestEqual(TEXT("Template head part id"), HeadTemplate->PartId, FName(TEXT("Snake.Head")));
	TestEqual(TEXT("Template head part slot id"), HeadTemplate->PartSlotId, FName(TEXT("Head")));
	TestEqual(TEXT("Template body part id"), BodyTemplate->PartId, FName(TEXT("Snake.Body")));
	TestEqual(TEXT("Template body part slot id"), BodyTemplate->PartSlotId, FName(TEXT("Body")));
	TestEqual(TEXT("Template tail part id"), TailTemplate->PartId, FName(TEXT("Snake.Tail")));
	TestEqual(TEXT("Template tail part slot id"), TailTemplate->PartSlotId, FName(TEXT("Tail")));
	TestEqual(TEXT("Template head component location"), HeadComponent->GetRelativeLocation(), FVector(96.0f, -6.0f, 16.0f));
	TestEqual(TEXT("Template body component location"), BodyComponent->GetRelativeLocation(), FVector::ZeroVector);
	TestEqual(TEXT("Template tail component location"), TailComponent->GetRelativeLocation(), FVector(-92.0f, 16.0f, -8.0f));

	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Template snake sample part count"), View.AttachedPartActorCount, 3);
	TestTrue(TEXT("Template snake sample stable targets"),
		View.StableSceneTargetIds.Contains(TEXT("Enemy.Head"))
		&& View.StableSceneTargetIds.Contains(TEXT("Enemy.Body"))
		&& View.StableSceneTargetIds.Contains(TEXT("Enemy.Tail")));
	TestTrue(TEXT("Template snake sample summary includes targets"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("StableSceneTargets=[Enemy.Head,Enemy.Body,Enemy.Tail]")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostDuplicateChildPartSlotValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostDuplicateChildPartSlotIdInvalidates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostDuplicateChildPartSlotValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Left =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* Right =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(200.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host)
		|| !TestNotNull(TEXT("Left"), Left)
		|| !TestNotNull(TEXT("Right"), Right))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Right))
		{
			Right->Destroy();
		}
		if (IsValid(Left))
		{
			Left->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Left->PartId = TEXT("Test.Part.LeftClaw");
	Left->PartSlotId = TEXT("Claw");
	Left->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);
	Right->PartId = TEXT("Test.Part.RightClaw");
	Right->PartSlotId = TEXT("Claw");
	Right->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("Duplicate child part slot id invalidates host"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Duplicate child part slot id error mentions slot"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Errors, TEXT("Claw")));
	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestTrue(TEXT("Debug duplicate child part slot id"), View.DuplicatePartSlotIds.Contains(TEXT("Claw")));
	TestTrue(TEXT("Details duplicate child part slot id"), Host->AuthoringDuplicatePartSlotIds.Contains(TEXT("Claw")));
	TestEqual(TEXT("Details duplicate state"), Host->AuthoringState, FName(TEXT("MissingEnemyDefinition")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostRuntimeFactsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostAggregatesRuntimePartFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostRuntimeFactsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 7, 5, 3);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Head =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* Body =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(200.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host)
		|| !TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Body"), Body))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Head))
		{
			Head->Destroy();
		}
		if (IsValid(Body))
		{
			Body->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Host->EnemyDefinition = Enemy;
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Head"), Head);
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Body"), Body);
	Host->RefreshBattleEnemyPartAuthoringState();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	Head->GetPresentationComponent()->SetHoverProbeState(
		WacomBattleSceneEnemyActorSpec::MakeBattleEnemyPartHandle(
			Head->GetWorldTargetBridgeComponent()->GetPartInstanceId(),
			Head->GetInteractionTargetComponent(),
			FVector::ZeroVector,
			FVector2D(240.0f, 120.0f),
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.Head")),
		TEXT("Hovered"));

	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Host aggregates bound parts"), View.BoundPartActorCount, 2);
	TestEqual(TEXT("Host aggregates runtime facts"), View.RuntimeFactsPartActorCount, 2);
	TestEqual(TEXT("Host sums runtime initiative"), View.RuntimeInitiativeTotal, 12);
	TestEqual(TEXT("Host aggregates hovered parts"), View.HoveredPartActorCount, 1);
	TestTrue(TEXT("Host summary reports runtime facts"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("RuntimeFacts=2")));
	TestTrue(TEXT("Host summary reports initiative total"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("RuntimeInitiativeTotal=12")));
	TestTrue(TEXT("Host summary reports hovered count"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("HoveredParts=1")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostHoveredPartCountSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostReportsHoveredPartCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostHoveredPartCountSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Head =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* Body =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(200.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host)
		|| !TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Body"), Body))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Head))
		{
			Head->Destroy();
		}
		if (IsValid(Body))
		{
			Body->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Head"), Head);
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Body"), Body);
	Host->RefreshBattleEnemyPartAuthoringState();

	Head->GetPresentationComponent()->SetHoverProbeState(
		WacomBattleSceneEnemyActorSpec::MakeBattleEnemyPartHandle(
			FGuid::NewGuid(),
			Head->GetInteractionTargetComponent(),
			FVector::ZeroVector,
			FVector2D(240.0f, 120.0f),
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.Head")),
		TEXT("Hovered"));

	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Host aggregates hovered parts"), View.HoveredPartActorCount, 1);
	TestTrue(TEXT("Host summary reports hovered count"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("HoveredParts=1")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostPredictionVisibleCountSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostReportsPredictionVisiblePartCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostPredictionVisibleCountSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 7, 5, 3);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Head =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host)
		|| !TestNotNull(TEXT("Head"), Head))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Head))
		{
			Head->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Host->EnemyDefinition = Enemy;
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Head"), Head);
	Host->RefreshBattleEnemyPartAuthoringState();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	Head->GetPresentationComponent()->SetHoverProbeState(
		WacomBattleSceneEnemyActorSpec::MakeBattleEnemyPartHandle(
			Head->GetWorldTargetBridgeComponent()->GetPartInstanceId(),
			Head->GetInteractionTargetComponent(),
			FVector::ZeroVector,
			FVector2D(240.0f, 120.0f),
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.Head")),
		TEXT("Hovered"));

	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Host hover no longer aggregates part prediction badges"),
		View.PredictionVisiblePartActorCount,
		0);
	TestTrue(TEXT("Host summary reports hidden prediction count"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("PredictionVisibleParts=0")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostDefinitionUnknownPartValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostDefinitionWarnsOnUnknownPartId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostDefinitionUnknownPartValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Part =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host) || !TestNotNull(TEXT("Part"), Part))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Part))
		{
			Part->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Host->EnemyDefinition = Enemy;
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Unknown"), Part);

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("Unknown part id warning keeps host valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("No host validation errors"), Errors.Num(), 0);
	TestTrue(TEXT("Warning mentions unknown part"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Warnings, TEXT("Test.Part.Unknown")));
	TestTrue(TEXT("Debug unknown part id"),
		Host->GetBattleSceneEnemyDebugView().UnknownPartIds.Contains(TEXT("Test.Part.Unknown")));
	TestTrue(TEXT("Debug missing definition part id"),
		Host->GetBattleSceneEnemyDebugView().MissingDefinitionPartIds.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostMissingDefinitionPartValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostWarnsOnMissingDefinitionPartId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostMissingDefinitionPartValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Head =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host) || !TestNotNull(TEXT("Head"), Head))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Head))
		{
			Head->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Host->EnemyDefinition = Enemy;
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Head"), Head);

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("Missing definition part warning keeps host valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("No missing definition validation errors"), Errors.Num(), 0);
	TestTrue(TEXT("Warning mentions missing body"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Warnings, TEXT("Test.Part.Body")));
	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestTrue(TEXT("Debug missing body"), View.MissingDefinitionPartIds.Contains(TEXT("Test.Part.Body")));
	TestTrue(TEXT("Debug missing tail"), View.MissingDefinitionPartIds.Contains(TEXT("Test.Part.Tail")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostPartSlotIdentityValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostValidatesPartSlotIdentityContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostPartSlotIdentityValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	WacomBattleSceneEnemyActorSpec::FSlotIdentityEnemyDefinitionFixture EnemyFixture =
		WacomBattleSceneEnemyActorSpec::MakeSlotIdentityEnemyDefinition();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Head =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* Body =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(200.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* Tail =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(300.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host)
		|| !TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Body"), Body)
		|| !TestNotNull(TEXT("Tail"), Tail))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Tail))
		{
			Tail->Destroy();
		}
		if (IsValid(Body))
		{
			Body->Destroy();
		}
		if (IsValid(Head))
		{
			Head->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Host->EnemyDefinition = EnemyFixture.Enemy.Get();
	Host->HostSprite = NewObject<UPaperSprite>(Host);
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Snake.Head"), TEXT("Head"), Head);
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Snake.Body"), TEXT("Body"), Body);
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Snake.Tail"), TEXT("Tail"), Tail);

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("Slot identity host validates cleanly"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("No slot identity validation errors"), Errors.Num(), 0);
	TestEqual(TEXT("No slot identity validation warnings"), Warnings.Num(), 0);
	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("No unknown part slot ids"), View.UnknownPartSlotIds.Num(), 0);
	TestEqual(TEXT("No missing definition part slot ids"), View.MissingDefinitionPartSlotIds.Num(), 0);
	TestTrue(TEXT("Summary reports authored part slots"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("PartSlotIds=[Head,Body,Tail]")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostPartSlotMismatchValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostWarnsWhenPartIdsMatchButPartSlotIdsMismatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostPartSlotMismatchValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	WacomBattleSceneEnemyActorSpec::FSlotIdentityEnemyDefinitionFixture EnemyFixture =
		WacomBattleSceneEnemyActorSpec::MakeSlotIdentityEnemyDefinition();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Head =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* Body =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(200.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* Tail =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(300.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host)
		|| !TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Body"), Body)
		|| !TestNotNull(TEXT("Tail"), Tail))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Tail))
		{
			Tail->Destroy();
		}
		if (IsValid(Body))
		{
			Body->Destroy();
		}
		if (IsValid(Head))
		{
			Head->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Host->EnemyDefinition = EnemyFixture.Enemy.Get();
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Snake.Head"), TEXT("Snake.Head"), Head);
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Snake.Body"), TEXT("Snake.Body"), Body);
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(Host, TEXT("Snake.Tail"), TEXT("Snake.Tail"), Tail);

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("Part slot mismatch stays warning-level"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("No part slot mismatch validation errors"), Errors.Num(), 0);
	TestTrue(TEXT("Warning mentions unknown authored slot"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Warnings, TEXT("Snake.Head")));
	TestTrue(TEXT("Warning mentions missing definition slot"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Warnings, TEXT("Head")));
	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Part ids still match definition"), View.UnknownPartIds.Num(), 0);
	TestEqual(TEXT("No definition part id missing"), View.MissingDefinitionPartIds.Num(), 0);
	TestTrue(TEXT("Debug unknown authored part slot"), View.UnknownPartSlotIds.Contains(TEXT("Snake.Head")));
	TestTrue(TEXT("Debug missing definition part slot"), View.MissingDefinitionPartSlotIds.Contains(TEXT("Head")));
	TestTrue(TEXT("Summary reports missing slot ids"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("MissingDefinitionPartSlotIds=[Head,Body,Tail]")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartDuplicatePartIdAcrossHostsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartDuplicatePartIdAcrossHostsIsValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartDuplicatePartIdAcrossHostsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyPartActor* First =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* Second =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("First"), First) || !TestNotNull(TEXT("Second"), Second))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Second))
		{
			Second->Destroy();
		}
		if (IsValid(First))
		{
			First->Destroy();
		}
	};

	First->PartId = TEXT("Test.Part.Head");
	First->PartSlotId = TEXT("Head");
	Second->PartId = TEXT("Test.Part.Head");
	Second->PartSlotId = TEXT("Head");

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(First, Warnings, Errors);
	TestEqual(TEXT("Duplicate world part id keeps actor valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("No duplicate validation errors"), Errors.Num(), 0);
	TestFalse(TEXT("No duplicate warning at PartActor level"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Warnings, TEXT("重复")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostDuplicateDefaultPartSlotIdValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostDuplicateDefaultPartSlotIdInvalidates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostDuplicateDefaultPartSlotIdValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomBattleEnemyPartActor* First =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(100.f, 0.f, 0.f)),
			SpawnParams);
	AWacomBattleEnemyPartActor* Second =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform(FVector(200.f, 0.f, 0.f)),
			SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host)
		|| !TestNotNull(TEXT("First"), First)
		|| !TestNotNull(TEXT("Second"), Second))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Second))
		{
			Second->Destroy();
		}
		if (IsValid(First))
		{
			First->Destroy();
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	Host->EnemyDefinition = Enemy;
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(
		Host,
		TEXT("Test.Part.Head"),
		TEXT("Test.Part.Head"),
		First);
	WacomBattleSceneEnemyActorSpec::AttachPartActorToHost(
		Host,
		TEXT("Test.Part.Head"),
		TEXT("Test.Part.Head"),
		Second);
	Host->RefreshBattleEnemyPartAuthoringState();

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleSceneEnemyActorSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("Duplicate explicit part slot id invalidates host"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Duplicate explicit part slot id error"),
		WacomBattleSceneEnemyActorSpec::ValidationIssuesContain(Errors, TEXT("Test.Part.Head")));
	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestTrue(TEXT("Debug duplicate explicit part slot id"),
		View.DuplicatePartSlotIds.Contains(TEXT("Test.Part.Head")));
	return true;
}
