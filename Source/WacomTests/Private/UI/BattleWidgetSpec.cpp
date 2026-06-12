// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Actors/BattleTriggerActor.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/BattlePresentationStackEntryWidget.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Battle/WacomBattleEnemyPartPredictionWidget.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"
#include "UI/Common/PileCountView.h"
#include "BattleHUDTestHarness.h"
#include "UI/BattleSceneTargetClickTestAccess.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "Events/BattleEvent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ChildActorComponent.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "InputCoreTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopeExit.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleWidgetSpec
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

	FWacomInteractionTargetHandle MakeWorldTargetHandleForPart(
		const FBattleSnapshot& Snapshot,
		const FGuid& PartInstanceId)
	{
		for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
		{
			for (const FEnemyPartSnapshot& Part : Enemy.Parts)
			{
				if (Part.InstanceId == PartInstanceId)
				{
					return FWacomInteractionTargetHandle::ForWorldTarget(
						Part.InstanceId,
						nullptr,
						FVector::ZeroVector,
						FVector2D::ZeroVector,
						WacomTags::Interaction_Target_Battle_EnemyPart,
						Part.Definition ? Part.Definition->PartId : NAME_None,
						Part.EncounterId,
						Part.EnemySlotId,
						Part.PartSlotId);
				}
			}
		}
		return FWacomInteractionTargetHandle();
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
		const FVector& WorldLocation,
		const FVector2D& ScreenPosition,
		const FGameplayTag& TargetTag,
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

	void SettleBattlePresentationQueue(UWacomBattleHUDDetailTest& HUD, int32 MaxSteps = 32)
	{
		for (int32 Iteration = 0; HUD.IsBattlePresentationBusy() && Iteration < MaxSteps; ++Iteration)
		{
			HUD.AdvanceBattlePresentationQueueForTest();
		}
	}

	void SettleBattlePresentationQueueAndExitStack(UWacomBattleHUDDetailTest& HUD, int32 MaxSteps = 64)
	{
		for (int32 Iteration = 0; HUD.IsBattlePresentationBusy() && Iteration < MaxSteps; ++Iteration)
		{
			bool bFinishedExit = false;
			const TArray<FWacomBattlePresentationStackEntryView> Entries = HUD.GetPresentationStackEntriesForTest();
			for (const FWacomBattlePresentationStackEntryView& Entry : Entries)
			{
				if (Entry.bIsExiting)
				{
					HUD.FinishPresentationStackEntryExitForTest(Entry.EntryId);
					bFinishedExit = true;
					break;
				}
			}

			if (!bFinishedExit)
			{
				HUD.AdvanceBattlePresentationQueueForTest();
			}
		}
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

	FSceneEnemyHostActors SpawnSceneEnemyHostForSlot(
		UWorld& World,
		UEnemyDefinition* EnemyDefinition,
		FName EnemySlotId,
		const TArray<FName>& PartDefinitionIds,
		const TArray<FName>& PartSlotIds)
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
		Result.Host->EnemySlotId = EnemySlotId;
		const int32 PartCount = FMath::Min(PartDefinitionIds.Num(), PartSlotIds.Num());
		for (int32 Index = 0; Index < PartCount; ++Index)
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
			AttachPartActorToHost(Result.Host, PartDefinitionIds[Index], PartSlotIds[Index], PartActor);
		}

		Result.Host->RefreshBattleEnemyPartAuthoringState();
		return Result;
	}

	UEncounterDefinition* MakeEncounterDefinitionForTest(
		UObject* Outer,
		const TArray<TPair<FName, UEnemyDefinition*>>& EnemySlots)
	{
		UEncounterDefinition* Encounter =
			NewObject<UEncounterDefinition>(Outer ? Outer : GetTransientPackage(), NAME_None, RF_Transient);
		Encounter->EncounterDefinitionId = TEXT("Test.Encounter.SceneEnemyHost");
		for (const TPair<FName, UEnemyDefinition*>& EnemySlot : EnemySlots)
		{
			FEncounterEnemySlot Slot;
			Slot.EnemySlotId = EnemySlot.Key;
			Slot.EnemyDefinition = EnemySlot.Value;
			Encounter->EnemySlots.Add(Slot);
		}
		return Encounter;
	}

	void ConfigureTriggerSceneEnemyHostSlotForTest(
		ABattleTriggerActor* Trigger,
		FName EnemySlotId,
		UEnemyDefinition* EnemyDefinition,
		AWacomBattleEnemyActor* Host)
	{
		if (!Trigger)
		{
			return;
		}

		Trigger->EncounterDefinition = MakeEncounterDefinitionForTest(
			Trigger,
			{ TPair<FName, UEnemyDefinition*>(EnemySlotId, EnemyDefinition) });

		FWacomBattleSceneEnemyHostSlot Slot;
		Slot.EnemySlotId = EnemySlotId;
		Slot.SceneEnemyHost = Host;
		Trigger->SceneEnemyHostSlots = { Slot };
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

	FWacomFirstPersonCardDragView MakeCommitDragView(const FGuid& CardInstanceId)
	{
		FWacomFirstPersonCardDragView DragView;
		DragView.CardInstanceId = CardInstanceId;
		DragView.GestureState = EWacomFirstPersonCardGestureState::ArmedForCommit;
		DragView.bCommitArmed = true;
		DragView.PressScreenPosition = FVector2D(500.0f, 600.0f);
		DragView.CurrentScreenPosition = FVector2D(540.0f, 590.0f);
		DragView.PointerViewportPosition = DragView.CurrentScreenPosition;
		DragView.PointerNormalizedViewportPosition = FVector2D(0.65f, 0.42f);
		DragView.bHasPointerViewportPosition = true;
		return DragView;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEventPresentationBuilderChineseTextSpec,
	"Wacom.UI.Battle.EventPresentationChineseText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEventPresentationBuilderChineseTextSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> PoisonFang(NewObject<UCardDefinition>());
	PoisonFang->CardId = TEXT("PoisonFang");
	PoisonFang->DisplayName = FText::FromString(TEXT("毒牙"));

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::CardGained;
		Event.CardDefinition = PoisonFang.Get();
		const FBattleEventPresentationView View = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestEqual(TEXT("CardGained uses display name"),
			View.MessageText.ToString(),
			FString(TEXT("获得卡牌：毒牙")));
		TestTrue(TEXT("CardGained should display"), View.bShouldDisplay);
		TestEqual(TEXT("CardGained tone is positive"), View.VisualTone, EWacomBattleEventVisualTone::Positive);
		TestEqual(TEXT("CardGained icon key"), View.IconKey, FName(TEXT("CardGained")));
		TestEqual(TEXT("FormatEventForPlayer matches view message"),
			UWacomBattleEventPresentationBuilder::FormatEventForPlayer(Event),
			View.MessageText.ToString());
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::StatusApplied;
		Event.Tag = WacomTags::Status_Poison;
		Event.Amount = 1;
		TestEqual(TEXT("StatusApplied localizes poison"),
			UWacomBattleEventPresentationBuilder::FormatEventForPlayer(Event),
			FString(TEXT("施加中毒 1 层")));
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::DamageDealt;
		Event.Tag = WacomTags::Status_Poison;
		Event.Amount = 3;
		TestEqual(TEXT("DamageDealt localizes poison source"),
			UWacomBattleEventPresentationBuilder::FormatEventForPlayer(Event),
			FString(TEXT("中毒造成 3 点伤害")));
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::BattleEnded;
		Event.Count = 1;
		const FBattleEventPresentationView VictoryView = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestEqual(TEXT("BattleEnded victory is Chinese"),
			VictoryView.MessageText.ToString(),
			FString(TEXT("战斗胜利")));
		TestEqual(TEXT("BattleEnded victory is positive"),
			VictoryView.VisualTone,
			EWacomBattleEventVisualTone::Positive);

		Event.Count = 0;
		const FBattleEventPresentationView DefeatView = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestEqual(TEXT("BattleEnded defeat is Chinese"),
			DefeatView.MessageText.ToString(),
			FString(TEXT("战斗失败")));
		TestEqual(TEXT("BattleEnded defeat is danger"),
			DefeatView.VisualTone,
			EWacomBattleEventVisualTone::Danger);
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::HandLimitDiscarded;
		Event.HandLimitDiscardSource = EHandLimitDiscardSource::EffectDraw;
		const FBattleEventPresentationView View = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestEqual(TEXT("HandLimitDiscarded source is Chinese"),
			View.MessageText.ToString(),
			FString(TEXT("因抽牌效果弃置 1 张牌")));
		TestEqual(TEXT("HandLimitDiscarded tone is warning"),
			View.VisualTone,
			EWacomBattleEventVisualTone::Warning);
		TestEqual(TEXT("HandLimitDiscarded icon key"),
			View.IconKey,
			FName(TEXT("HandLimitDiscarded")));
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::HandZoneChanged;
		const FBattleEventPresentationView View = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestTrue(TEXT("HandZoneChanged remains hidden"),
			View.MessageText.IsEmpty());
		TestFalse(TEXT("HandZoneChanged should not display"), View.bShouldDisplay);
		TestEqual(TEXT("Hidden event has no icon"), View.IconKey, NAME_None);
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::CardPlayed;
		Event.Amount = 2;
		const FBattleEventPresentationView View = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestTrue(TEXT("CardPlayed should display"), View.bShouldDisplay);
		TestEqual(TEXT("CardPlayed defaults to neutral tone"),
			View.VisualTone,
			EWacomBattleEventVisualTone::Neutral);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDPileCountDisplaySpec,
	"Wacom.UI.Battle.HUDPileCountDisplaysPlayedPileWithDiscardPile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDPileCountDisplaySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->CreatePileViewsForTest();
	UPileCountView* DrawPileView = HUD->GetDrawPileViewForTest();
	UPileCountView* DiscardPileView = HUD->GetDiscardPileViewForTest();
	UPileCountView* ExhaustPileView = HUD->GetExhaustPileViewForTest();
	if (!TestNotNull(TEXT("Draw pile view"), DrawPileView)
		|| !TestNotNull(TEXT("Discard pile view"), DiscardPileView)
		|| !TestNotNull(TEXT("Exhaust pile view"), ExhaustPileView))
	{
		return false;
	}

	FBattleSnapshot Snapshot;
	Snapshot.Phase = EBattlePhase::PlayerAction;
	Snapshot.PileCounts.DrawCount = 4;
	Snapshot.PileCounts.DiscardCount = 2;
	Snapshot.PileCounts.PlayedCount = 3;
	Snapshot.PileCounts.ExhaustCount = 1;

	HUD->RefreshFromSnapshotForTest(Snapshot);

	TestEqual(TEXT("Draw pile display remains numeric"),
		DrawPileView->GetCountDisplayText().ToString(),
		FString(TEXT("4")));
	TestEqual(TEXT("Discard pile keeps numeric discard count"),
		DiscardPileView->GetCount(),
		2);
	TestEqual(TEXT("Discard pile combines discard and played counts"),
		DiscardPileView->GetCountDisplayText().ToString(),
		FString(TEXT("2+3")));
	TestEqual(TEXT("Exhaust pile display remains numeric"),
		ExhaustPileView->GetCountDisplayText().ToString(),
		FString(TEXT("1")));

	Snapshot.PileCounts.DiscardCount = 5;
	Snapshot.PileCounts.PlayedCount = 0;
	HUD->RefreshFromSnapshotForTest(Snapshot);

	TestEqual(TEXT("Discard pile returns to numeric display without played cards"),
		DiscardPileView->GetCountDisplayText().ToString(),
		FString(TEXT("5")));
	TestEqual(TEXT("Discard pile numeric cache refreshes"),
		DiscardPileView->GetCount(),
		5);

	return true;
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
	FWacomUIBattleCombatLogBuilderPlayCardSpec,
	"Wacom.UI.Battle.CombatLogBuilderBuildsPlayCardBlockWithCardAndTargetNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogBuilderPlayCardSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> PoisonFang(NewObject<UCardDefinition>());
	PoisonFang->CardId = TEXT("PoisonFang");
	PoisonFang->DisplayName = FText::FromString(TEXT("毒牙"));

	TStrongObjectPtr<UEnemyPartDefinition> SnakeHead(NewObject<UEnemyPartDefinition>());
	SnakeHead->PartId = TEXT("SnakeHead");
	SnakeHead->DisplayName = FText::FromString(TEXT("蛇头"));

	const FGuid CardId = FGuid::NewGuid();
	const FGuid TargetPartId = FGuid::NewGuid();

	FBattleSnapshot Snapshot;
	Snapshot.TurnNumber = 1;
	FHandCardSnapshot HandCard;
	HandCard.InstanceId = CardId;
	HandCard.Definition = PoisonFang.Get();
	Snapshot.Hand.Cards.Add(HandCard);
	FEnemyPartSnapshot Part;
	Part.InstanceId = TargetPartId;
	Part.Identity = FBattlePartSlotIdentity(TEXT("Encounter"), TEXT("Enemy"), TEXT("Target"));
	Part.Definition = SnakeHead.Get();
	FEnemySnapshot EnemySnapshot;
	EnemySnapshot.EnemySlotId = TEXT("Enemy");
	EnemySnapshot.Parts.Add(Part);
	Snapshot.Enemies.Add(EnemySnapshot);

	const FWacomBattleCombatLogCommandContext Context =
		UWacomBattleCombatLogBuilder::BuildPlayCardCommandContext(Snapshot, CardId, Part.Identity, FGuid());

	FBattleEvent CardPlayed;
	CardPlayed.Type = EBattleEventType::CardPlayed;
	CardPlayed.Sequence = 10;
	CardPlayed.Amount = 2;

	FBattleEvent Damage;
	Damage.Type = EBattleEventType::DamageDealt;
	Damage.Sequence = 11;
	Damage.Amount = 7;

	const FWacomBattleCombatLogBlockView Block =
		UWacomBattleCombatLogBuilder::BuildCombatLogBlock(Context, { CardPlayed, Damage }, Snapshot, Snapshot);

	TestTrue(TEXT("PlayCard combat block displays"), Block.bShouldDisplay);
	TestEqual(TEXT("PlayCard header names card and target"),
		Block.HeaderText.ToString(),
		FString(TEXT("打出「毒牙」 -> 蛇头，消耗 2 先机")));
	TestEqual(TEXT("CardPlayed is folded into header"), Block.DetailLines.Num(), 1);
	TestEqual(TEXT("Damage appears as detail"), Block.DetailLines[0].MessageText.ToString(), FString(TEXT("造成 7 点伤害")));
	TestEqual(TEXT("Sequence range starts at first event"), Block.FirstEventSequence, 10);
	TestEqual(TEXT("Sequence range ends at last event"), Block.LastEventSequence, 11);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogBuilderWaitEndTurnSystemSpec,
	"Wacom.UI.Battle.CombatLogBuilderBuildsWaitEndTurnAndSystemBlocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogBuilderWaitEndTurnSystemSpec::RunTest(const FString& /*Parameters*/)
{
	FBattleSnapshot Snapshot;
	Snapshot.TurnNumber = 2;

	{
		FBattleEvent WaitEvent;
		WaitEvent.Type = EBattleEventType::WaitPerformed;
		WaitEvent.Amount = 3;
		const FWacomBattleCombatLogBlockView Block =
			UWacomBattleCombatLogBuilder::BuildCombatLogBlock(
				UWacomBattleCombatLogBuilder::BuildWaitCommandContext(Snapshot),
				{ WaitEvent },
				Snapshot,
				Snapshot);
		TestEqual(TEXT("Wait header includes initiative push"), Block.HeaderText.ToString(), FString(TEXT("等待：敌方先机 -3")));
	}

	{
		FBattleEvent TurnEnded;
		TurnEnded.Type = EBattleEventType::TurnEnded;
		TurnEnded.Count = 2;
		FBattleEvent CardsDrawn;
		CardsDrawn.Type = EBattleEventType::CardsDrawn;
		CardsDrawn.Count = 5;
		const FWacomBattleCombatLogBlockView Block =
			UWacomBattleCombatLogBuilder::BuildCombatLogBlock(
				UWacomBattleCombatLogBuilder::BuildEndTurnCommandContext(Snapshot),
				{ TurnEnded, CardsDrawn },
				Snapshot,
				Snapshot);
		TestEqual(TEXT("EndTurn header"), Block.HeaderText.ToString(), FString(TEXT("结束回合")));
		TestEqual(TEXT("EndTurn details include turn end and draw"), Block.DetailLines.Num(), 2);
	}

	{
		FBattleEvent Started;
		Started.Type = EBattleEventType::BattleStarted;
		const FWacomBattleCombatLogBlockView Block =
			UWacomBattleCombatLogBuilder::BuildCombatLogBlock(
				UWacomBattleCombatLogBuilder::BuildSystemCommandContext(Snapshot),
				{ Started },
				Snapshot,
				Snapshot);
		TestEqual(TEXT("System header uses turn"), Block.HeaderText.ToString(), FString(TEXT("战斗记录 · 第 2 回合")));
		TestEqual(TEXT("System detail includes battle start"), Block.DetailLines[0].MessageText.ToString(), FString(TEXT("战斗开始")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogBuilderMoveEventsSpec,
	"Wacom.UI.Battle.CombatLogBuilderShowsDiscardExhaustGainButHidesHandZoneChanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogBuilderMoveEventsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> RewardCard(NewObject<UCardDefinition>());
	RewardCard->CardId = TEXT("Reward");
	RewardCard->DisplayName = FText::FromString(TEXT("毒牙"));

	FBattleSnapshot Snapshot;

	FBattleEvent Hidden;
	Hidden.Type = EBattleEventType::HandZoneChanged;

	FBattleEvent Discarded;
	Discarded.Type = EBattleEventType::CardDiscarded;
	Discarded.HandCardZoneMoveReason = EHandCardZoneMoveReason::Effect;

	FBattleEvent Exhausted;
	Exhausted.Type = EBattleEventType::CardExhausted;
	Exhausted.HandCardZoneMoveReason = EHandCardZoneMoveReason::TurnEnd;

	FBattleEvent Gained;
	Gained.Type = EBattleEventType::CardGained;
	Gained.CardDefinition = RewardCard.Get();

	const FWacomBattleCombatLogBlockView Block =
		UWacomBattleCombatLogBuilder::BuildCombatLogBlock(
			UWacomBattleCombatLogBuilder::BuildSystemCommandContext(Snapshot),
			{ Hidden, Discarded, Exhausted, Gained },
			Snapshot,
			Snapshot);

	TestEqual(TEXT("Hidden HandZoneChanged is omitted"), Block.DetailLines.Num(), 3);
	TestEqual(TEXT("Discarded is combat-log visible"), Block.DetailLines[0].MessageText.ToString(), FString(TEXT("效果弃置 1 张牌")));
	TestEqual(TEXT("Exhausted is combat-log visible"), Block.DetailLines[1].MessageText.ToString(), FString(TEXT("回合结束消耗 1 张牌")));
	TestEqual(TEXT("Card gained remains visible"), Block.DetailLines[2].MessageText.ToString(), FString(TEXT("获得卡牌：毒牙")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogFeedSpec,
	"Wacom.UI.Battle.ScrollableCombatLogFeedMirrorsBlocksAndTrims",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogFeedSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBattleCombatLogFeedWidget> Feed(NewObject<UBattleCombatLogFeedWidget>());
	Feed->MaxVisibleBlocks = 2;
	Feed->TakeWidget();
	TestTrue(TEXT("Feed fallback owns a scroll box"), Feed->HasScrollBoxForTest());

	FWacomBattleCombatLogBlockView Hidden;
	Hidden.bShouldDisplay = false;
	Hidden.HeaderText = FText::FromString(TEXT("隐藏"));

	FWacomBattleCombatLogBlockView First;
	First.bShouldDisplay = true;
	First.HeaderText = FText::FromString(TEXT("打出「毒牙」"));

	FWacomBattleCombatLogBlockView Second;
	Second.bShouldDisplay = true;
	Second.HeaderText = FText::FromString(TEXT("等待"));

	FWacomBattleCombatLogBlockView Third;
	Third.bShouldDisplay = true;
	Third.HeaderText = FText::FromString(TEXT("结束回合"));

	Feed->SetCombatLogBlocks({ Hidden, First, Second, Third });

	TestEqual(TEXT("Feed filters hidden and trims"), Feed->GetVisibleBlockCount(), 2);
	TestEqual(TEXT("Feed keeps recent second block"), Feed->GetCurrentBlocks()[0].HeaderText.ToString(), FString(TEXT("等待")));
	TestEqual(TEXT("Feed keeps latest block"), Feed->GetCurrentBlocks()[1].HeaderText.ToString(), FString(TEXT("结束回合")));

	Feed->ClearCombatLog();
	TestEqual(TEXT("Feed clears blocks"), Feed->GetVisibleBlockCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCombatLogSpec,
	"Wacom.UI.Battle.HUDCombatLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCombatLogSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UBattleCombatLogFeedWidget> Feed(NewObject<UBattleCombatLogFeedWidget>(HUD.Get()));
	HUD->BattleCombatLogMaxBlocks = 2;
	HUD->SetCombatLogFeedForTest(Feed.Get());
	Feed->TakeWidget();

	FWacomBattleCombatLogBlockView Hidden;
	Hidden.bShouldDisplay = false;
	Hidden.HeaderText = FText::FromString(TEXT("隐藏"));

	FWacomBattleCombatLogBlockView First;
	First.bShouldDisplay = true;
	First.HeaderText = FText::FromString(TEXT("战斗开始"));

	FWacomBattleCombatLogBlockView Second;
	Second.bShouldDisplay = true;
	Second.HeaderText = FText::FromString(TEXT("等待"));

	FWacomBattleCombatLogBlockView Third;
	Third.bShouldDisplay = true;
	Third.HeaderText = FText::FromString(TEXT("战斗胜利"));

	HUD->AppendBattleCombatLogBlockForTest(Hidden);
	HUD->AppendBattleCombatLogBlockForTest(First);
	HUD->AppendBattleCombatLogBlockForTest(Second);
	HUD->AppendBattleCombatLogBlockForTest(Third);

	TestEqual(TEXT("HUD combat log history trims to max"), HUD->GetBattleCombatLogBlockCount(), 2);
	TestEqual(TEXT("HUD keeps recent second block"), HUD->GetBattleCombatLogHistoryForTest()[0].HeaderText.ToString(), FString(TEXT("等待")));
	TestEqual(TEXT("HUD keeps latest block"), HUD->GetBattleCombatLogHistoryForTest()[1].HeaderText.ToString(), FString(TEXT("战斗胜利")));
	TestEqual(TEXT("Feed mirrors combat log blocks"), Feed->GetVisibleBlockCount(), 2);
	TestEqual(TEXT("Feed latest text"), Feed->GetCurrentBlocks()[1].HeaderText.ToString(), FString(TEXT("战斗胜利")));

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
	HUD->SetSession(Session.Get());
	HUD->SetSession(nullptr);
	TestEqual(TEXT("Session change clears HUD history"), HUD->GetBattleCombatLogBlockCount(), 0);
	TestEqual(TEXT("Session change clears feed"), Feed->GetVisibleBlockCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDInitialEventsConsumedSpec,
	"Wacom.UI.Battle.HUDInitialEventsConsumedOnSessionSet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDInitialEventsConsumedSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UBattleCombatLogFeedWidget> Feed(NewObject<UBattleCombatLogFeedWidget>(HUD.Get()));
	Feed->TakeWidget();
	HUD->SetCombatLogFeedForTest(Feed.Get());
	HUD->SetSession(Session);

	TestTrue(TEXT("SetSession consumes initial visible battle events immediately"),
		HUD->GetBattleCombatLogBlockCount() > 0);
	TestTrue(TEXT("Combat log feed receives initial visible battle events"),
		Feed->GetVisibleBlockCount() > 0);

	const TArray<FWacomBattleCombatLogBlockView> InitialBlocks = HUD->GetBattleCombatLogHistoryForTest();
	const bool bHasBattleStarted = InitialBlocks.ContainsByPredicate(
		[](const FWacomBattleCombatLogBlockView& Block)
		{
			return Block.DetailLines.ContainsByPredicate(
				[](const FWacomBattleCombatLogLineView& Line)
				{
					return Line.SourceEventType == EBattleEventType::BattleStarted;
				});
		});
	const bool bHasCardsDrawn = InitialBlocks.ContainsByPredicate(
		[](const FWacomBattleCombatLogBlockView& Block)
		{
			return Block.DetailLines.ContainsByPredicate(
				[](const FWacomBattleCombatLogLineView& Line)
				{
					return Line.SourceEventType == EBattleEventType::CardsDrawn;
				});
		});
	TestTrue(TEXT("Initial log includes battle start"), bHasBattleStarted);
	TestTrue(TEXT("Initial log includes opening draw"), bHasCardsDrawn);

	const int32 EntryCountAfterSetSession = HUD->GetBattleCombatLogBlockCount();
	HUD->OnWaitRequested();

	const TArray<FWacomBattleCombatLogBlockView> BlocksAfterWait = HUD->GetBattleCombatLogHistoryForTest();
	const int32 BattleStartedCountAfterWait = BlocksAfterWait.FilterByPredicate(
		[](const FWacomBattleCombatLogBlockView& Block)
		{
			return Block.DetailLines.ContainsByPredicate(
				[](const FWacomBattleCombatLogLineView& Line)
				{
					return Line.SourceEventType == EBattleEventType::BattleStarted;
				});
		}).Num();
	TestEqual(TEXT("Initial battle start is not consumed again after first command"), BattleStartedCountAfterWait, 1);
	TestTrue(TEXT("Wait appends later command events"),
		HUD->GetBattleCombatLogBlockCount() > EntryCountAfterSetSession);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationStackWidgetOrderSpec,
	"Wacom.UI.Battle.BattlePresentationStackWidgetStacksOldestOnTopNewestOnBottom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationStackWidgetOrderSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBattlePresentationStackWidget> Stack(NewObject<UBattlePresentationStackWidget>());
	Stack->MaxVisibleEntries = 3;
	Stack->TakeWidget();

	TArray<FWacomBattlePresentationStackEntryView> Entries;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		FWacomBattlePresentationStackEntryView Entry;
		Entry.EntryId = Index + 1;
		Entry.CardInstanceId = FGuid::NewGuid();
		Entry.CardViewData.Name = FText::FromString(FString::Printf(TEXT("Card%d"), Index + 1));
		Entries.Add(Entry);
	}

	Stack->SetPresentationStackEntries(Entries);
	TestEqual(TEXT("Internal entries preserve oldest-to-newest order"), Stack->GetCurrentEntries()[0].EntryId, 1);
	TestEqual(TEXT("Visible entry count trims to max"), Stack->GetVisibleEntryCount(), 3);
	TestEqual(TEXT("All entries retained internally"), Stack->GetCurrentEntries().Num(), 5);
	TestFalse(TEXT("Oldest visible entry does not need text fields"), Stack->GetCurrentEntries()[0].CardViewData.Name.IsEmpty());

	Stack->ClearPresentationStack();
	TestEqual(TEXT("Clear removes entries"), Stack->GetCurrentEntries().Num(), 0);
	TestEqual(TEXT("Clear removes visible entries"), Stack->GetVisibleEntryCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationStackFallbackSpec,
	"Wacom.UI.Battle.BattlePresentationStackUsesConfigurableMiniCardViewFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationStackFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	UClass* DefaultCardViewClass = LoadClass<UWacomCardView>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_CardView.WBP_CardView_C"));
	UClass* FirstPersonCardViewClass = LoadClass<UWacomFirstPersonCardViewWidget>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_FPCardView.WBP_FPCardView_C"));

	TStrongObjectPtr<UBattlePresentationStackWidget> DefaultStack(NewObject<UBattlePresentationStackWidget>());
	if (TestNotNull(TEXT("WBP_CardView loads for presentation stack default"), DefaultCardViewClass))
	{
		TestEqual(
			TEXT("Presentation stack defaults to WBP_CardView"),
			DefaultStack->MiniCardViewClass.Get(),
			DefaultCardViewClass);
	}
	if (FirstPersonCardViewClass)
	{
		TestNotEqual(
			TEXT("Presentation stack does not default to WBP_FPCardView"),
			DefaultStack->MiniCardViewClass.Get(),
			FirstPersonCardViewClass);
	}

	TStrongObjectPtr<UBattlePresentationStackWidget> Stack(NewObject<UBattlePresentationStackWidget>());
	Stack->MiniCardViewClass = UWacomCardView::StaticClass();
	Stack->TakeWidget();

	FWacomBattlePresentationStackEntryView Entry;
	Entry.EntryId = 1;
	Entry.CardViewData.Name = FText::FromString(TEXT("毒牙"));
	Stack->SetPresentationStackEntries({ Entry });

	TestEqual(TEXT("Entry retained"), Stack->GetCurrentEntries().Num(), 1);
	TestEqual(TEXT("Configured fallback class is preserved"), Stack->MiniCardViewClass.Get(), UWacomCardView::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationStackPureCardEntrySpec,
	"Wacom.UI.Battle.BattlePresentationStackEntryIsPureScaledCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationStackPureCardEntrySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBattlePresentationStackEntryWidget> EntryWidget(NewObject<UBattlePresentationStackEntryWidget>());
	EntryWidget->SetMiniCardViewClass(UWacomCardView::StaticClass());
	EntryWidget->TakeWidget();

	FWacomBattlePresentationStackEntryView Entry;
	Entry.EntryId = 1;
	Entry.CardInstanceId = FGuid::NewGuid();
	Entry.CardViewData.Name = FText::FromString(TEXT("毒牙"));
	EntryWidget->SetPresentationStackEntryData(Entry);

	TestNotNull(TEXT("Entry creates mini card view"), EntryWidget->GetMiniCardView());
	TestTrue(TEXT("Entry uses whole-card scale host"), EntryWidget->HasMiniCardScaleHostForTest());
	TestFalse(TEXT("Entry has no header/target text widgets"), EntryWidget->HasHeaderOrTargetTextWidgetsForTest());
	TestEqual(TEXT("Entry remains hit-test invisible"), EntryWidget->GetVisibility(), ESlateVisibility::HitTestInvisible);

	Entry.bIsExiting = true;
	EntryWidget->SetPresentationStackEntryData(Entry);
	EntryWidget->TickExitForTest(0.08f);
	TestTrue(TEXT("Exit motion fades card"), EntryWidget->GetRenderOpacity() < 1.0f);
	TestTrue(TEXT("Exit motion moves card upward"), EntryWidget->GetRenderTransform().Translation.Y < 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueIgnoresTextOnlyEventsSpec,
	"Wacom.UI.Battle.PresentationQueue.IgnoresTextOnlyEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueIgnoresTextOnlyEventsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);

	FBattleEvent First;
	First.Type = EBattleEventType::BattleStarted;
	First.Sequence = 1;

	FBattleEvent Second;
	Second.Type = EBattleEventType::DamageDealt;
	Second.Sequence = 2;
	Second.Amount = 3;

	HUD->EnqueueBattlePresentationEventsForTest({ First, Second });

	World->GetTimerManager().Tick(0.01f);
	TestFalse(TEXT("Text-only battle events do not create presentation steps"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueNonblockingInputSpec,
	"Wacom.UI.Battle.PresentationQueue.NonblockingInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueNonblockingInputSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCardDefinition* NoTargetCard = Fx.MakeNoopCard(0);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, NoTargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 50, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("PlayerController spawned"), Harness->PlayerController()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UBattleCombatLogFeedWidget* CombatLogFeed = Harness->AttachCombatLogFeed();
	Harness->AttachPresentationStack();
	UWacomActionPanelTestProbe* ActionPanel = Harness->AttachActionPanel();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("CombatLogFeed"), CombatLogFeed)
		|| !TestNotNull(TEXT("ActionPanel"), ActionPanel))
	{
		return false;
	}
	TestFalse(TEXT("Initial session presentation has settled before focused blocking check"),
		HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("HUD returns idle after initial session presentation"), HUD->GetUIState(), EBattleUIState::Idle);
	TestTrue(TEXT("Action panel wait starts enabled"), ActionPanel->IsWaitButtonEnabledForTest());
	TestTrue(TEXT("Action panel end turn starts enabled"), ActionPanel->IsEndTurnButtonEnabledForTest());

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid NoTargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::None);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Target card exists"), TargetCardId.IsValid())
		|| !TestTrue(TEXT("No target card exists"), NoTargetCardId.IsValid())
		|| !TestTrue(TEXT("Target part exists"), TargetPartId.IsValid()))
	{
		return false;
	}

	FBattleEvent Event;
	Event.Type = EBattleEventType::DamageDealt;
	Event.Sequence = 1;
	Event.ActorEnemyPartKey = FWacomBattleFixture::FindPartKeyByInstanceId(InitialSnapshot, TargetPartId);
	Event.Amount = 1;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });
	World->GetTimerManager().Tick(0.01f);

	TestTrue(TEXT("Queue reports busy"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("HUD stays idle while presenting"), HUD->GetUIState(), EBattleUIState::Idle);
	TestTrue(TEXT("Action panel wait stays enabled while presenting"), ActionPanel->IsWaitButtonEnabledForTest());
	TestTrue(TEXT("Action panel end turn stays enabled while presenting"), ActionPanel->IsEndTurnButtonEnabledForTest());

	const int32 CombatLogCountBeforeTargetSelect = HUD->GetBattleCombatLogBlockCount();
	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("Target card can enter target select while presenting"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestEqual(TEXT("Target card becomes pending while presenting"), HUD->GetPendingTargetingCardId(), TargetCardId);
	TestEqual(TEXT("Target select alone does not append combat log"), HUD->GetBattleCombatLogBlockCount(), CombatLogCountBeforeTargetSelect);

	const int32 VersionBeforeTargetSubmit = Session->BuildSnapshot().Version;
	HUD->OnEnemyPartClickedByUser(
		WacomBattleWidgetSpec::MakeWorldTargetHandleForPart(Session->BuildSnapshot(), TargetPartId));
	TestEqual(TEXT("Target submit returns idle while presenting"), HUD->GetUIState(), EBattleUIState::Idle);
	TestTrue(TEXT("Target submit resolves while presenting"),
		Session->BuildSnapshot().Version > VersionBeforeTargetSubmit);
	TestTrue(TEXT("Presentation queue remains busy after appended card events"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Target submit appends presentation stack entry"), HUD->GetPresentationStackEntryCountForTest(), 1);
	TestEqual(TEXT("Oldest stack entry is target card"), HUD->GetPresentationStackEntriesForTest()[0].CardInstanceId, TargetCardId);
	TestEqual(TEXT("Target submit appends one combat log block"),
		HUD->GetBattleCombatLogBlockCount(),
		CombatLogCountBeforeTargetSelect + 1);
	TestTrue(TEXT("Target submit block uses PlayCard header"),
		HUD->GetBattleCombatLogHistoryForTest().Last().HeaderText.ToString().Contains(TEXT("打出")));

	HUD->OnCardClickedByUser(NoTargetCardId);
	TestEqual(TEXT("No target card can submit while presenting"), HUD->GetUIState(), EBattleUIState::Idle);
	TestFalse(TEXT("No target submit clears pending while presenting"), HUD->GetPendingTargetingCardId().IsValid());
	TestTrue(TEXT("Presentation queue still has appended events after no-target card"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("No-target submit appends second presentation stack entry"), HUD->GetPresentationStackEntryCountForTest(), 2);
	TestEqual(TEXT("Second stack entry is newest card"), HUD->GetPresentationStackEntriesForTest()[1].CardInstanceId, NoTargetCardId);
	TestEqual(TEXT("No-target submit appends one more combat log block"),
		HUD->GetBattleCombatLogBlockCount(),
		CombatLogCountBeforeTargetSelect + 2);

	const int32 WaitValueBefore = Session->BuildSnapshot().CurrentWaitValue;
	HUD->OnWaitRequested();
	TestEqual(TEXT("Wait does not resolve while presentation stack has cards"), Session->BuildSnapshot().CurrentWaitValue, WaitValueBefore);
	TestTrue(TEXT("Wait becomes pending"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestFalse(TEXT("Action panel wait disabled while pending"), ActionPanel->IsWaitButtonEnabledForTest());
	TestFalse(TEXT("Action panel end turn disabled while pending"), ActionPanel->IsEndTurnButtonEnabledForTest());

	const int32 VersionBeforeBlockedCard = Session->BuildSnapshot().Version;
	HUD->OnCardClickedByUser(NoTargetCardId);
	TestEqual(TEXT("Pending turn boundary blocks further card submits"), Session->BuildSnapshot().Version, VersionBeforeBlockedCard);
	TestEqual(TEXT("Pending turn boundary does not append another stack entry"), HUD->GetPresentationStackEntryCountForTest(), 2);

	while (HUD->IsBattlePresentationBusy() && !HUD->GetPresentationStackEntriesForTest().IsEmpty()
		&& !HUD->GetPresentationStackEntriesForTest()[0].bIsExiting)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}
	TestTrue(TEXT("Boundary marks oldest stack entry exiting"), HUD->GetPresentationStackEntriesForTest()[0].bIsExiting);
	TestTrue(TEXT("Pending wait remains while exit motion plays"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestEqual(TEXT("Pending wait still does not mutate during exit motion"), Session->BuildSnapshot().CurrentWaitValue, WaitValueBefore);

	HUD->FinishPresentationStackEntryExitForTest(HUD->GetPresentationStackEntriesForTest()[0].EntryId);
	Harness->SettlePresentationQueueAndExitStack();
	TestFalse(TEXT("Pending wait runs after stack drains"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestEqual(TEXT("Wait resolves after stack drains"), Session->BuildSnapshot().CurrentWaitValue, WaitValueBefore + 1);
	TestEqual(TEXT("Wait appends after stack drains"),
		HUD->GetBattleCombatLogBlockCount(),
		CombatLogCountBeforeTargetSelect + 3);
	TestEqual(TEXT("Presentation stack drained"), HUD->GetPresentationStackEntryCountForTest(), 0);

	const int32 VersionBeforeEndTurn = Session->BuildSnapshot().Version;
	HUD->OnEndTurnRequested();
	TestTrue(TEXT("End turn resolves immediately when stack is empty"), Session->BuildSnapshot().Version > VersionBeforeEndTurn);
	TestEqual(TEXT("Scrollable feed mirrors combat log history"),
		CombatLogFeed->GetVisibleBlockCount(),
		HUD->GetBattleCombatLogBlockCount());

	Harness->SettlePresentationQueue();
	TestFalse(TEXT("Queue no longer busy"), HUD->IsBattlePresentationBusy());
	TestTrue(TEXT("HUD remains in a non-battle-end command state after presentation"),
		HUD->GetUIState() == EBattleUIState::Idle || HUD->GetUIState() == EBattleUIState::BattleEnd);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationStackEndTurnBarrierSpec,
	"Wacom.UI.Battle.EndTurnWhilePresentationStackPendingLocksFurtherPlayerCommandsAndRunsAfterDrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationStackEndTurnBarrierSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 50, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("PlayerController spawned"), Harness->PlayerController()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->AttachPresentationStack();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Target card exists"), TargetCardId.IsValid())
		|| !TestTrue(TEXT("Target part exists"), TargetPartId.IsValid()))
	{
		return false;
	}

	HUD->OnCardClickedByUser(TargetCardId);
	HUD->OnEnemyPartClickedByUser(
		WacomBattleWidgetSpec::MakeWorldTargetHandleForPart(Session->BuildSnapshot(), TargetPartId));
	TestEqual(TEXT("PlayCard appends one stack entry"), HUD->GetPresentationStackEntryCountForTest(), 1);
	const int32 VersionBeforeEndTurn = Session->BuildSnapshot().Version;

	HUD->OnEndTurnRequested();
	TestTrue(TEXT("EndTurn becomes pending"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestEqual(TEXT("EndTurn does not mutate while stack pending"), Session->BuildSnapshot().Version, VersionBeforeEndTurn);

	HUD->OnEndTurnRequested();
	TestEqual(TEXT("Repeated EndTurn remains ignored while pending"), Session->BuildSnapshot().Version, VersionBeforeEndTurn);

	while (HUD->IsBattlePresentationBusy() && !HUD->GetPresentationStackEntriesForTest().IsEmpty()
		&& !HUD->GetPresentationStackEntriesForTest()[0].bIsExiting)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}
	TestTrue(TEXT("EndTurn waits while stack entry is exiting"), HUD->HasPendingTurnBoundaryCommandForTest());
	HUD->FinishPresentationStackEntryExitForTest(HUD->GetPresentationStackEntriesForTest()[0].EntryId);
	Harness->SettlePresentationQueueAndExitStack();
	TestFalse(TEXT("Pending EndTurn clears after drain"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestTrue(TEXT("EndTurn runs after stack drains"), Session->BuildSnapshot().Version > VersionBeforeEndTurn);
	TestEqual(TEXT("Presentation stack drained"), HUD->GetPresentationStackEntryCountForTest(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueDamageCueSpec,
	"Wacom.UI.Battle.PresentationQueue.DamageCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueDamageCueSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(Session->BuildSnapshot(), 0);

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(*World, Enemy, { TEXT("Test.Part.Solo") });
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestTrue(TEXT("Scene enemy has one part"), SceneEnemy.Parts.Num() == 1)
		|| !TestNotNull(TEXT("Scene enemy part"), SceneEnemy.Parts.IsValidIndex(0) ? SceneEnemy.Parts[0] : nullptr))
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
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
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		SceneEnemy.Parts[0]->GetWorldTargetBridgeComponent();
	if (!TestNotNull(TEXT("Scene part bridge"), Bridge))
	{
		return false;
	}

	FBattleEvent Event;
	Event.Type = EBattleEventType::DamageDealt;
	Event.Sequence = 1;
	Event.ActorEnemyPartKey = FWacomBattleFixture::FindPartKeyByInstanceId(Session->BuildSnapshot(), TargetPartId);
	Event.Amount = 7;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });

	World->GetTimerManager().Tick(0.01f);
	const FWacomBattleEnemyPartPresentationDebugView View =
		SceneEnemy.Parts[0]->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
	TestEqual(TEXT("Target cue plays for damage event"), View.CuePlayCount, 1);
	TestEqual(TEXT("Target cue kind is damage"), View.LastCueKind, FName(TEXT("DamageDealt")));
	TestEqual(TEXT("Target cue type is damage"), View.LastCueType, EBattleEventType::DamageDealt);
	TestEqual(TEXT("Target cue carries damage amount"), View.LastCueAmount, 7);

	HUD->AdvanceBattlePresentationQueueForTest();
	TestFalse(TEXT("Queue finishes after target cue pacing"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueBlocksPlayerActionOutsidePlayerPhaseSpec,
	"Wacom.UI.Battle.PresentationQueue.BlocksPlayerActionOutsidePlayerPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueBlocksPlayerActionOutsidePlayerPhaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* KillerCard = nullptr;
	UCharacterDefinition* Character = [&Fx, &KillerCard]()
	{
		UCardDefinition* LeftHand = Fx.MakeNoopCard(0);
		UCardDefinition* RightHand = Fx.MakeNoopCard(0);
		KillerCard = Fx.MakeSimpleDamageCard(0, 100);
		TArray<UCardDefinition*> Deck = { KillerCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) };
		return Fx.MakeCharacter(LeftHand, RightHand, Deck);
	}();
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(50, 50, 50, 7, 7, 7);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetSession(Session);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid KillerCardId = FWacomBattleFixture::FindHandInstanceByCardId(InitialSnapshot, KillerCard->CardId);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Killer card exists"), KillerCardId.IsValid())
		|| !TestTrue(TEXT("Target part exists"), TargetPartId.IsValid()))
	{
		return false;
	}

	TestTrue(TEXT("Submit killer card"),
		Session->SubmitCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(
			InitialSnapshot,
			KillerCardId,
			TargetPartId)).IsOk());
	TestEqual(TEXT("Session enters pending knockdown"), Session->BuildSnapshot().Phase, EBattlePhase::PendingKnockdownChoice);
	TestFalse(TEXT("HUD command gate blocks pending knockdown"), HUD->CanSubmitPlayerActionCommand());

	const int32 VersionBeforeWait = Session->BuildSnapshot().Version;
	HUD->OnWaitRequested();
	TestEqual(TEXT("Wait does not resolve during pending knockdown"), Session->BuildSnapshot().Version, VersionBeforeWait);

	FGuid FillerCardId;
	for (const FHandCardSnapshot& Card : Session->BuildSnapshot().Hand.Cards)
	{
		if (Card.Definition && Card.Definition->TargetMode == ECardTargetMode::None)
		{
			FillerCardId = Card.InstanceId;
			break;
		}
	}
	TestTrue(TEXT("Filler card exists"), FillerCardId.IsValid());
	HUD->OnCardClickedByUser(FillerCardId);
	TestEqual(TEXT("Card click does not submit during pending knockdown"), Session->BuildSnapshot().Version, VersionBeforeWait);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueInvalidTargetCueSkippedSpec,
	"Wacom.UI.Battle.PresentationQueue.InvalidTargetCueSkipped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueInvalidTargetCueSkippedSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);

	FBattleEvent Event;
	Event.Type = EBattleEventType::DamageDealt;
	Event.Sequence = 1;
	Event.Amount = 5;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });

	World->GetTimerManager().Tick(0.01f);
	TestFalse(TEXT("Invalid target damage does not create presentation steps"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueClearsOnSessionChangeSpec,
	"Wacom.UI.Battle.PresentationQueue.ClearsOnSessionChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueClearsOnSessionChangeSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetSession(Session);

	FBattleEvent First;
	First.Type = EBattleEventType::DamageDealt;
	First.Sequence = 1;
	First.ActorEnemyPartKey = FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("Enemy"), TEXT("QueuedA"));
	First.Amount = 4;
	FBattleEvent Second;
	Second.Type = EBattleEventType::DamageDealt;
	Second.Sequence = 2;
	Second.ActorEnemyPartKey = FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("Enemy"), TEXT("QueuedB"));
	Second.Amount = 4;
	HUD->EnqueueBattlePresentationEventsForTest({ First, Second });

	World->GetTimerManager().Tick(0.01f);
	TestTrue(TEXT("Queue is busy before session change"), HUD->IsBattlePresentationBusy());

	HUD->SetSession(nullptr);
	TestFalse(TEXT("Session change clears queue"), HUD->IsBattlePresentationBusy());

	World->GetTimerManager().Tick(0.50f);
	TestFalse(TEXT("Cleared queue does not resume queued target cue"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueBattleEndClearsQueueSafelySpec,
	"Wacom.UI.Battle.PresentationQueue.BattleEndClearsQueueSafely",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueBattleEndClearsQueueSafelySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* Killer = Fx.MakeSimpleDamageCard(0, 100);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Killer, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(10, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(InitialSnapshot, Killer->CardId);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	TestTrue(TEXT("Play killer card"),
		Session->SubmitCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(
			InitialSnapshot,
			KillerId,
			TargetPartId)).IsOk());
	TestTrue(TEXT("Submit final Aid"), Session->SubmitCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid)).IsOk());
	TestTrue(TEXT("Session reached BattleEnd"), Session->GetPhase() == EBattlePhase::BattleEnd);
	Session->ConsumeEvents();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetSession(Session);
	HUD->OnBattleEndedNative.AddUObject(
		HUD.Get(),
		&UWacomBattleHUDDetailTest::ClearPresentationQueueOnBattleEndedForTest);

	FBattleEvent VictorySignal;
	VictorySignal.Type = EBattleEventType::BattleEnded;
	VictorySignal.Sequence = 1;
	VictorySignal.Count = 1;

	FBattleEvent ShouldNotPlayAfterClear;
	ShouldNotPlayAfterClear.Type = EBattleEventType::DamageDealt;
	ShouldNotPlayAfterClear.Sequence = 2;
	ShouldNotPlayAfterClear.ActorEnemyPartKey = FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("Enemy"), TEXT("Cleared"));
	ShouldNotPlayAfterClear.Amount = 9;

	HUD->EnqueueBattlePresentationEventsForTest({ VictorySignal, ShouldNotPlayAfterClear });

	World->GetTimerManager().Tick(0.01f);
	TestTrue(TEXT("BattleEnd callback clears queue during presentation"),
		HUD->GetBattleEndedCallbackCountForTest() > 0);
	TestFalse(TEXT("Queue no longer busy after battle end callback clears it"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("HUD is in BattleEnd after battle end step"), HUD->GetUIState(), EBattleUIState::BattleEnd);

	HUD->AdvanceBattlePresentationQueueForTest();
	World->GetTimerManager().Tick(1.0f);
	TestFalse(TEXT("Cleared queue does not play trailing event"), HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationQueueKnockdownDialogDelayedAndGuardedSpec,
	"Wacom.UI.Battle.PresentationQueue.KnockdownDialogDelayedAndGuarded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationQueueKnockdownDialogDelayedAndGuardedSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* Killer = Fx.MakeSimpleDamageCard(0, 100);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Killer, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(InitialSnapshot, Killer->CardId);
	const FGuid HeadId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	TestTrue(TEXT("Play killer card"),
		Session->SubmitCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(
			InitialSnapshot,
			KillerId,
			HeadId)).IsOk());
	TestTrue(TEXT("Session is pending knockdown"), Session->BuildPendingKnockdownChoiceView().bHasPendingChoice);
	Session->ConsumeEvents();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("PlayerController spawned"), PC))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetSession(Session);

	FBattleEvent IntroCue;
	IntroCue.Type = EBattleEventType::DamageDealt;
	IntroCue.Sequence = 1;
	IntroCue.ActorEnemyPartKey = FWacomBattleFixture::FindPartKeyByInstanceId(Session->BuildSnapshot(), HeadId);
	IntroCue.Amount = 100;

	FBattleEvent KnockdownRequest;
	KnockdownRequest.Type = EBattleEventType::KnockdownChoiceRequested;
	KnockdownRequest.Sequence = 2;

	HUD->EnqueueBattlePresentationEventsForTest({ IntroCue, KnockdownRequest });

	World->GetTimerManager().Tick(0.01f);
	TestTrue(TEXT("Target cue delays knockdown modal step"), HUD->IsBattlePresentationBusy());

	HUD->AdvanceBattlePresentationQueueForTest();
	TestFalse(TEXT("Knockdown step is consumed after the pacing delay"), HUD->IsBattlePresentationBusy());
	TestTrue(TEXT("Valid pending choice is still available for the dialog path"),
		Session->BuildPendingKnockdownChoiceView().bHasPendingChoice);

	HUD->ClearBattlePresentationQueueForTest();
	TestTrue(TEXT("Resolve pending knockdown choice"),
		Session->SubmitCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid)).IsOk());
	Session->ConsumeEvents();
	TestFalse(TEXT("No pending choice remains after Aid"),
		Session->BuildPendingKnockdownChoiceView().bHasPendingChoice);

	HUD->EnqueueBattlePresentationEventsForTest({ KnockdownRequest });
	World->GetTimerManager().Tick(0.01f);
	TestFalse(TEXT("Stale knockdown request is guarded and finishes without a modal dependency"),
		HUD->IsBattlePresentationBusy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleKnockdownChoiceDialogViewSpec,
	"Wacom.UI.Battle.KnockdownChoiceDialogUsesViewData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleKnockdownChoiceDialogViewSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleKnockdownChoiceDialogTest> Dialog(
		NewObject<UWacomBattleKnockdownChoiceDialogTest>());

	FKnockdownChoiceView View;
	View.bHasPendingChoice = true;
	View.PartName = FText::FromString(TEXT("蛇尾"));
	View.AidOption.Choice = EKnockdownChoice::Aid;
	View.AidOption.bAvailable = true;
	View.WithdrawOption.Choice = EKnockdownChoice::Withdraw;
	View.WithdrawOption.bAvailable = false;
	View.WithdrawOption.DisabledReason = FName(TEXT("NoLivingEnemyPart"));
	View.DestroyOption.Choice = EKnockdownChoice::Destroy;
	View.DestroyOption.bAvailable = true;

	Dialog->TakeWidget();
	Dialog->SetContext(nullptr, View);

	TestEqual(TEXT("Part name comes from view data"), Dialog->GetPartNameTextForTest(), TEXT("蛇尾"));
	TestTrue(TEXT("Aid button follows view availability"), Dialog->IsAidButtonEnabledForTest());
	TestFalse(TEXT("Withdraw button follows view availability"), Dialog->IsWithdrawButtonEnabledForTest());
	TestTrue(TEXT("Destroy button follows view availability"), Dialog->IsDestroyButtonEnabledForTest());

	View.AidOption.bAvailable = false;
	View.AidOption.DisabledReason = FName(TEXT("LeftHandMissing"));
	View.WithdrawOption.bAvailable = true;
	View.WithdrawOption.DisabledReason = FName(TEXT("None"));
	View.DestroyOption.bAvailable = false;
	View.DestroyOption.DisabledReason = FName(TEXT("RightHandMissing"));

	Dialog->SetContext(nullptr, View);

	TestFalse(TEXT("Aid button refreshes from updated view"), Dialog->IsAidButtonEnabledForTest());
	TestTrue(TEXT("Withdraw button refreshes from updated view"), Dialog->IsWithdrawButtonEnabledForTest());
	TestFalse(TEXT("Destroy button refreshes from updated view"), Dialog->IsDestroyButtonEnabledForTest());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCardDetailDefaultsSpec,
	"Wacom.UI.Battle.BattleHUD.FallbackLayout.CardDetailDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardDetailDefaultsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBattleHUD> HUD(NewObject<UBattleHUD>());

	TestEqual(TEXT("Default card detail width"), HUD->CardDetailPanelEstimatedSize.X, 360.0);
	TestEqual(TEXT("Default card detail height"), HUD->CardDetailPanelEstimatedSize.Y, 420.0);
	TestEqual(TEXT("Default card detail padding"), HUD->CardDetailPanelPadding, 12.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCardDetailPositionSpec,
	"Wacom.UI.Battle.BattleHUD.CardDetail.Position",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardDetailPositionSpec::RunTest(const FString& /*Parameters*/)
{
	const FVector2D PanelSize(360.0f, 420.0f);
	const FVector2D LayerSize(1200.0f, 800.0f);

	const FVector2D LeftSide = UBattleHUD::ComputeCardDetailPanelPositionBeside(
		FVector2D(500.0f, 500.0f),
		FVector2D(120.0f, 160.0f),
		LayerSize,
		PanelSize,
		12.0f);
	TestEqual(TEXT("Detail panel prefers left side when there is room"), LeftSide, FVector2D(128.0f, 370.0f));

	const FVector2D RightSide = UBattleHUD::ComputeCardDetailPanelPositionBeside(
		FVector2D(20.0f, 100.0f),
		FVector2D(120.0f, 160.0f),
		LayerSize,
		PanelSize,
		12.0f);
	TestEqual(TEXT("Detail panel falls back to right side when left side has no room"), RightSide, FVector2D(152.0f, 0.0f));

	const FVector2D ClampRight = UBattleHUD::ComputeCardDetailPanelPositionBeside(
		FVector2D(1120.0f, 700.0f),
		FVector2D(120.0f, 160.0f),
		LayerSize,
		PanelSize,
		12.0f);
	TestEqual(TEXT("Detail panel uses left side near right edge and clamps vertical position"), ClampRight, FVector2D(748.0f, 380.0f));

	const FVector2D ClampBottom = UBattleHUD::ComputeCardDetailPanelPositionBeside(
		FVector2D(500.0f, 780.0f),
		FVector2D(120.0f, 160.0f),
		LayerSize,
		PanelSize,
		12.0f);
	TestEqual(TEXT("Detail panel clamps to bottom edge"), ClampBottom, FVector2D(128.0f, 380.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDTargetSelectionViewSpec,
	"Wacom.UI.Battle.BattleHUD.CommandFlow.TargetSelectionView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDTargetSelectionViewSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* LeftHand = Fx.MakeNoopCard(0);
	UCardDefinition* RightHand = Fx.MakeNoopCard(0);
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(1, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(LeftHand, RightHand, { TargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
	FBattleInitParams Params;
	Params.Character = Character;
	Params.RandomSeed = 1;
	FBattleEnemySlotInit EnemySlot;
	EnemySlot.EnemySlotId = TEXT("Enemy");
	EnemySlot.Enemy = Enemy;
	Params.EnemySlots.Add(EnemySlot);
	FBattlePartSlotIdentity DestroyedPart;
	DestroyedPart.EncounterId = TEXT("Encounter");
	DestroyedPart.EnemySlotId = TEXT("Enemy");
	DestroyedPart.PartSlotId = TEXT("Test.Part.Body");
	Params.PreDestroyedParts.Add(DestroyedPart);
	TestTrue(TEXT("Session initialize"), Session->Initialize(Params).IsOk());

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetSession(Session.Get());
	HUD->TakeWidget();

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FEnemySnapshot* EnemySnapshot = FWacomBattleFixture::GetEnemySnapshot(Snapshot, 0);
	TestNotNull(TEXT("Enemy snapshot exists"), EnemySnapshot);
	TestEqual(TEXT("Enemy part count"), EnemySnapshot ? EnemySnapshot->Parts.Num() : 0, 3);
	if (!EnemySnapshot || EnemySnapshot->Parts.Num() != 3)
	{
		return false;
	}

	const FBattleTargetSelectionView IdleView = HUD->BuildTargetSelectionView();
	TestFalse(TEXT("Idle view is not selecting"), IdleView.bIsTargetSelecting);
	TestEqual(TEXT("Idle view includes all parts"), IdleView.TargetableParts.Num(), 3);
	TestFalse(TEXT("Idle head not targetable"), IdleView.TargetableParts[0].bTargetable);
	TestEqual(TEXT("Idle disabled reason"), IdleView.TargetableParts[0].DisabledReason, FName(TEXT("NotTargetSelecting")));

	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	TestTrue(TEXT("Targeting card is in hand"), TargetCardId.IsValid());
	if (!TargetCardId.IsValid())
	{
		return false;
	}

	HUD->SetTargetSelectionStateForTest(TargetCardId);
	const FBattleTargetSelectionView TargetView = HUD->BuildTargetSelectionView();
	TestTrue(TEXT("Target view is selecting"), TargetView.bIsTargetSelecting);
	TestTrue(TEXT("Target view pending card valid"), TargetView.PendingCardInstanceId.IsValid());
	TestEqual(TEXT("Target view includes all parts"), TargetView.TargetableParts.Num(), 3);
	TestTrue(TEXT("Living head is targetable"), TargetView.TargetableParts[0].bTargetable);
	TestEqual(TEXT("Living head reason none"), TargetView.TargetableParts[0].DisabledReason, NAME_None);
	TestFalse(TEXT("Destroyed body is not targetable"), TargetView.TargetableParts[1].bTargetable);
	TestEqual(TEXT("Destroyed body reason"), TargetView.TargetableParts[1].DisabledReason, FName(TEXT("PartDestroyed")));
	TestTrue(TEXT("Living tail is targetable"), TargetView.TargetableParts[2].bTargetable);

	HUD->ClearTargetSelectionStateForTest();
	const FBattleTargetSelectionView ClearedView = HUD->BuildTargetSelectionView();
	TestFalse(TEXT("Cleared view is not selecting"), ClearedView.bIsTargetSelecting);
	TestFalse(TEXT("Cleared view invalid pending card"), ClearedView.PendingCardInstanceId.IsValid());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCardClickFlowSpec,
	"Wacom.UI.Battle.BattleHUD.CommandFlow.CardClick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardClickFlowSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* LeftHand = Fx.MakeNoopCard(0);
	UCardDefinition* RightHand = Fx.MakeNoopCard(0);
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCardDefinition* NoTargetCard = Fx.MakeNoopCard(0);
	UCharacterDefinition* Character = Fx.MakeCharacter(LeftHand, RightHand, { TargetCard, NoTargetCard });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetSession(Session);
	HUD->TakeWidget();

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid NoTargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::None);
	TestTrue(TEXT("Targeting card is in hand"), TargetCardId.IsValid());
	TestTrue(TEXT("No-target card is in hand"), NoTargetCardId.IsValid());
	if (!TargetCardId.IsValid() || !NoTargetCardId.IsValid())
	{
		return false;
	}

	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("Targeting card enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestEqual(TEXT("Targeting card becomes pending"), HUD->GetPendingTargetingCardId(), TargetCardId);

	const int32 VersionBeforeNoTarget = Session->BuildSnapshot().Version;
	HUD->OnCardClickedByUser(NoTargetCardId);
	TestEqual(TEXT("No-target card returns/remains idle after submit"), HUD->GetUIState(), EBattleUIState::Idle);
	TestFalse(TEXT("No-target submit leaves no pending card"), HUD->GetPendingTargetingCardId().IsValid());
	TestTrue(TEXT("No-target card submit changes battle state"),
		Session->BuildSnapshot().Version > VersionBeforeNoTarget);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDWaitEndTurnCancelTargetSelectSpec,
	"Wacom.UI.Battle.BattleHUD.CommandFlow.WaitEndTurnCancelTargetSelect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDWaitEndTurnCancelTargetSelectSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);

	{
		UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
		TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
		HUD->SetSession(Session);
		HUD->TakeWidget();

		HUD->SetTargetSelectionStateForTest(FGuid::NewGuid());
		TestEqual(TEXT("Wait precondition target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
		const int32 WaitValueBefore = Session->BuildSnapshot().CurrentWaitValue;

		HUD->OnWaitRequested();

		TestEqual(TEXT("Wait cancels target select and returns idle"), HUD->GetUIState(), EBattleUIState::Idle);
		TestFalse(TEXT("Wait clears pending target card"), HUD->GetPendingTargetingCardId().IsValid());
		TestEqual(TEXT("Wait command still resolves"), Session->BuildSnapshot().CurrentWaitValue, WaitValueBefore + 1);
	}

	{
		FWacomBattleFixture SecondFx;
		UCharacterDefinition* SecondCharacter = SecondFx.MakeCharacter(
			SecondFx.MakeNoopCard(0),
			SecondFx.MakeNoopCard(0),
			{ SecondFx.MakeNoopCard(0), SecondFx.MakeNoopCard(0), SecondFx.MakeNoopCard(0) });
		UEnemyDefinition* SecondEnemy = SecondFx.MakeSinglePartEnemy(20, 5, 0);
		UBattleSession* Session = SecondFx.CreateSession(SecondCharacter, SecondEnemy, 1);
		TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
		HUD->SetSession(Session);
		HUD->TakeWidget();

		HUD->SetTargetSelectionStateForTest(FGuid::NewGuid());
		TestEqual(TEXT("EndTurn precondition target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
		const FBattleSnapshot SnapshotBefore = Session->BuildSnapshot();

		HUD->OnEndTurnRequested();

		TestEqual(TEXT("EndTurn cancels target select and returns idle"), HUD->GetUIState(), EBattleUIState::Idle);
		TestFalse(TEXT("EndTurn clears pending target card"), HUD->GetPendingTargetingCardId().IsValid());
		TestTrue(TEXT("EndTurn command still resolves"),
			Session->BuildSnapshot().Version > SnapshotBefore.Version);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartWorldTargetBridgeBindsRuntimeTargetSpec,
	"Wacom.UI.Battle.InteractionTarget.EnemyPartWorldBridge.BindsRuntimeTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartWorldTargetBridgeBindsRuntimeTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	FWacomUIBattleSceneEnemyPartPredictionWidgetFacadeSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartPredictionWidgetFacadeIsReadOnlyScreenSpace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionWidgetFacadeSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	FWacomUIBattleSceneEnemyPartPredictionHoverInitiativeSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverShowsCurrentInitiativePredictionBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionHoverInitiativeSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	PartActor->GetPresentationComponent()->SetHoverProbeState(
		WacomBattleWidgetSpec::MakeBattlePartHoverHandle(PartActor),
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	FWacomUIBattleSceneEnemyPartPredictionInvalidTargetSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartPredictionRejectedTargetDoesNotShowDelta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionInvalidTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	FWacomUIBattleSceneEnemyPartHoverScalePrioritySpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverScaleDoesNotOverrideDragOrTargetableState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverScalePrioritySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	FWacomInteractionTargetHandle HoverHandle = WacomBattleWidgetSpec::MakeBattleEnemyPartHandle(
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
	TestEqual(TEXT("Clearing hover restores base scale"), Primitive->GetRelativeScale3D(), BaseScale);
	TestEqual(TEXT("Hover clear reason recorded"),
		Presentation->GetBattleEnemyPartPresentationDebugView().HoverReason,
		FName(TEXT("NoTarget")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartBridgeRuntimeFactsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartBridgeReportsRuntimeInitiativeFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartBridgeRuntimeFactsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	FWacomUIBattleEnemyPartWorldTargetBridgeClearsDestroyedPartSpec,
	"Wacom.UI.Battle.InteractionTarget.EnemyPartWorldBridge.ClearsDestroyedPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartWorldTargetBridgeClearsDestroyedPartSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	FWacomUIBattleSceneEnemyPartActorRefreshesFacadeSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartActorRefreshesFacadeAndBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartActorRefreshesFacadeSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	PartActor->TargetConfirmPulseScale = 1.21f;
	PartActor->DamagePulseScale = 1.31f;
	PartActor->DestroyedPulseScale = 1.41f;
	PartActor->TargetableAffordanceScale = 1.07f;
	PartActor->DragTargetPreviewScale = 1.09f;
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
	TestEqual(TEXT("Presentation target confirm scale"), Presentation ? Presentation->TargetConfirmPulseScale : 0.0f, 1.21f);
	TestEqual(TEXT("Presentation damage scale"), Presentation ? Presentation->DamagePulseScale : 0.0f, 1.31f);
	TestEqual(TEXT("Presentation destroyed scale"), Presentation ? Presentation->DestroyedPulseScale : 0.0f, 1.41f);
	TestEqual(TEXT("Presentation targetable scale"), Presentation ? Presentation->TargetableAffordanceScale : 0.0f, 1.07f);
	TestEqual(TEXT("Presentation drag preview scale"), Presentation ? Presentation->DragTargetPreviewScale : 0.0f, 1.09f);
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
	FWacomUIBattleSceneEnemyHostVisualMakesChildPartsHitOnlySpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostVisualMakesChildPartActorsHitOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostVisualMakesChildPartsHitOnlySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	FWacomUIBattleSceneEnemyHostVisualBeginPlaySpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostVisualInitializesAtBeginPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostVisualBeginPlaySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	TestEqual(TEXT("Host details generated component count after BeginPlay"),
		SceneEnemy.Host->AuthoringGeneratedHostVisualComponentCount,
		1);
	TestEqual(TEXT("Host details registered component count after BeginPlay"),
		SceneEnemy.Host->AuthoringRegisteredHostVisualComponentCount,
		1);
	TestEqual(TEXT("Host details visible component count after BeginPlay"),
		SceneEnemy.Host->AuthoringVisibleHostVisualComponentCount,
		1);

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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostVisualPartFeedbackScalesPartVisualLayersRoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostVisualPartFeedbackStaysPerPartSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor =
		SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	SceneEnemy.Host->HostSprite = NewObject<UPaperSprite>(SceneEnemy.Host);
	FWacomBattleEnemyPartVisualLayer Layer;
	Layer.LayerId = TEXT("Head.Main");
	Layer.Sprite = NewObject<UPaperSprite>(PartActor);
	PartActor->VisualLayers = { Layer };
	PartActor->TargetConfirmPulseScale = 1.22f;
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

	TestEqual(TEXT("Part feedback scales visual layers root"),
		PartActor->GetVisualLayersRoot()->GetRelativeScale3D(),
		PartBaseScale * PartActor->TargetConfirmPulseScale);
	TestEqual(TEXT("Part feedback does not scale host visual"),
		HostVisual->GetRelativeScale3D(),
		HostBaseScale);

	PartActor->GetPresentationComponent()->ClearDragTargetPreviewState();
	PartActor->GetPresentationComponent()->ClearHoverProbeState(TEXT("Test"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostVisualDoesNotCreateTargetProviderSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostVisualDoesNotAlterHitBoundsTargetRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostVisualDoesNotCreateTargetProviderSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());

	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, SceneEnemy.Host, HostVisual);
	FWacomInteractionTargetHandle HostVisualHandle;
	TestFalse(TEXT("Host visual component is not a battle target provider"),
		FWacomBattleSceneTargetClickTestAccess::ProbeTarget(PC, HostVisualHandle));
	TestFalse(TEXT("Host visual probe does not synthesize a target"), HostVisualHandle.IsValid());

	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("HUD enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, PartActor, PartActor->GetHitBounds());
	TestTrue(TEXT("Part hit bounds still routes with host visual present"),
		FWacomBattleSceneTargetClickTestAccess::RouteClick(PC));
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	TestEqual(TEXT("HUD returns idle after part target"), HUD->GetUIState(), EBattleUIState::Idle);
	TestGreaterThan(TEXT("Target card submission advances snapshot"),
		Session->BuildSnapshot().Version,
		Snapshot.Version);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartVisualLayersRefreshSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartVisualLayersRefreshRemovesStaleComponentsAndReturnsToNoVisualMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartVisualLayersRefreshSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	FlipbookLayer.Flipbook = WacomBattleWidgetSpec::MakeOneFrameFlipbookForTest(PartActor);
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
		WacomBattleWidgetSpec::ValidateObjectForTest(PartActor, Warnings, Errors);
	TestEqual(TEXT("Empty visual layer id invalidates part"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Empty layer id error mentions LayerId"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Errors, TEXT("LayerId")));

	FWacomBattleEnemyPartVisualLayer DuplicateLayerA;
	DuplicateLayerA.LayerId = TEXT("Silhouette");
	DuplicateLayerA.Sprite = ValidSprite;
	FWacomBattleEnemyPartVisualLayer DuplicateLayerB = DuplicateLayerA;
	PartActor->VisualLayers = { DuplicateLayerA, DuplicateLayerB };
	Result = WacomBattleWidgetSpec::ValidateObjectForTest(PartActor, Warnings, Errors);
	TestEqual(TEXT("Duplicate visual layer id invalidates part"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Duplicate layer id error mentions id"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Errors, TEXT("Silhouette")));
	TestTrue(TEXT("Debug reports duplicate visual layer id"),
		PartActor->GetBattleSceneEnemyPartDebugView().DuplicateVisualLayerIds.Contains(TEXT("Silhouette")));

	FWacomBattleEnemyPartVisualLayer ZeroScaleLayer;
	ZeroScaleLayer.LayerId = TEXT("ZeroScale");
	ZeroScaleLayer.Sprite = ValidSprite;
	ZeroScaleLayer.RelativeScale3D = FVector(1.0f, 0.0f, 1.0f);
	PartActor->VisualLayers = { ZeroScaleLayer };
	Result = WacomBattleWidgetSpec::ValidateObjectForTest(PartActor, Warnings, Errors);
	TestEqual(TEXT("Zero visual layer scale invalidates part"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Zero scale error mentions RelativeScale3D"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Errors, TEXT("RelativeScale3D")));

	FWacomBattleEnemyPartVisualLayer MissingSpriteLayer;
	MissingSpriteLayer.LayerId = TEXT("MissingSprite");
	PartActor->VisualLayers = { MissingSpriteLayer };
	Result = WacomBattleWidgetSpec::ValidateObjectForTest(PartActor, Warnings, Errors);
	TestEqual(TEXT("Missing visual layer sprite is warning only"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Missing sprite has no errors"), Errors.Num(), 0);
	TestTrue(TEXT("Missing sprite warning mentions Sprite"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("StaticSprite")));
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
	Result = WacomBattleWidgetSpec::ValidateObjectForTest(PartActor, Warnings, Errors);
	TestEqual(TEXT("Missing visual layer flipbook is warning only"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Missing flipbook has no errors"), Errors.Num(), 0);
	TestTrue(TEXT("Missing flipbook warning mentions Flipbook"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("Flipbook")));
	TestEqual(TEXT("Debug counts missing flipbook"),
		PartActor->GetBattleSceneEnemyPartDebugView().MissingVisualLayerFlipbookCount,
		1);

	PartActor->VisualLayers.Reset();
	Result = WacomBattleWidgetSpec::ValidateObjectForTest(PartActor, Warnings, Errors);
	TestEqual(TEXT("Missing all visual resources is warning only"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Missing all visual resources has no errors"), Errors.Num(), 0);
	TestTrue(TEXT("Missing visual resource warning mentions VisualLayers"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("VisualLayers")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostVisualValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostVisualAndHitOnlyPartsValidateAsLegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostVisualValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	const WacomBattleWidgetSpec::FSlotIdentityEnemyDefinitionFixture EnemyFixture =
		WacomBattleWidgetSpec::MakeSlotIdentityEnemyDefinition();
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
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Snake.Head"), TEXT("Head"), Head);
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Snake.Body"), TEXT("Body"), Body);
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Snake.Tail"), TEXT("Tail"), Tail);
	for (AWacomBattleEnemyPartActor* PartActor : { Head, Body, Tail })
	{
		PartActor->VisualLayers.Reset();
	}

	TArray<FText> Warnings;
	TArray<FText> Errors;
	EDataValidationResult Result =
		WacomBattleWidgetSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("Host visual plus hit-only parts is valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Legal hit-only host has no errors"), Errors.Num(), 0);
	TestFalse(TEXT("Legal hit-only host does not warn about no-art enemy"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("只有命中体")));
	TestEqual(TEXT("Head is hit-only"), Head->VisualAuthoringMode, FName(TEXT("HitOnly")));
	TestEqual(TEXT("Host authoring ready"), Host->AuthoringState, FName(TEXT("Ready")));

	Result = WacomBattleWidgetSpec::ValidateObjectForTest(Head, Warnings, Errors);
	TestEqual(TEXT("Hit-only part validation is valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Hit-only part has no errors"), Errors.Num(), 0);
	TestFalse(TEXT("Hit-only part does not warn about missing independent visual"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("VisualLayers")));

	Host->HostSprite = nullptr;
	Host->RefreshBattleEnemyPartAuthoringState();
	Result = WacomBattleWidgetSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("No-art enemy is warning only"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("No-art enemy has no errors"), Errors.Num(), 0);
	TestTrue(TEXT("No-art enemy warning mentions hit-only debug-only state"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("只有命中体")));

	Host->HostSprite = NewObject<UPaperSprite>(Host);
	Head->HitBoundsExtent = FVector(0.0f, 42.0f, 42.0f);
	Host->RefreshBattleEnemyPartAuthoringState();
	Result = WacomBattleWidgetSpec::ValidateObjectForTest(Head, Warnings, Errors);
	TestEqual(TEXT("Host visual does not hide invalid hit bounds"),
		Result,
		EDataValidationResult::Invalid);
	TestTrue(TEXT("Invalid hit bounds error remains visible"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Errors, TEXT("HitBoundsExtent")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartActorWorldTargetHandleSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartActorBuildsWorldTargetHandleForPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartActorWorldTargetHandleSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	FWacomUIBattleSceneEnemyPartDebugPredictionFactsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartDebugSummaryReportsPredictionReadinessFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartDebugPredictionFactsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
		WacomBattleWidgetSpec::MakeBattleEnemyPartHandle(
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
	FWacomUIBattleSceneEnemyPartActorCueAndDragPreviewSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartActorRoutesCueAndDragPreviewThroughPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartActorCueAndDragPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	PartActor->TargetConfirmPulseScale = 1.25f;
	PartActor->DragTargetPreviewScale = 1.15f;
	PartActor->RefreshAuthoringState();

	UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge =
		PartActor->GetWorldTargetBridgeComponent();
	const FVector BaseScale = PartActor->GetVisualLayersRoot()->GetRelativeScale3D();

	FWacomBattlePresentationTargetCue Cue;
	Cue.CueKind = EWacomBattlePresentationTargetCueKind::TargetConfirmed;
	Cue.SourceEventType = EBattleEventType::None;
	PartActor->GetPresentationComponent()->PlayBattlePresentationCue(Cue);

	FWacomBattleEnemyPartPresentationDebugView View =
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
	TestEqual(TEXT("Target confirm cue count"), View.CuePlayCount, 1);
	TestEqual(TEXT("Target confirm scales visual layer root"),
		PartActor->GetVisualLayersRoot()->GetRelativeScale3D(),
		BaseScale * 1.25f);

	PartActor->GetPresentationComponent()->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	View = PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
	TestTrue(TEXT("Drag preview active"), View.bDragPreviewActive);
	TestEqual(TEXT("Drag preview scales visual layer root"),
		PartActor->GetVisualLayersRoot()->GetRelativeScale3D(),
		BaseScale * 1.15f);

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
	FWacomUIBattleSceneEnemyHostReportsChildPartActorFactsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostReportsChildPartActorFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostReportsChildPartActorFactsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Head"), Head);
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Body"), Body);

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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	WacomBattleWidgetSpec::AttachPartActorToHost(
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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

	const WacomBattleWidgetSpec::FSlotIdentityEnemyDefinitionFixture Definition =
		WacomBattleWidgetSpec::MakeSlotIdentityEnemyDefinition();
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
		WacomBattleWidgetSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("Duplicate child part slot id invalidates host"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Duplicate child part slot id error mentions slot"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Errors, TEXT("Claw")));
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Head"), Head);
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Body"), Body);
	Host->RefreshBattleEnemyPartAuthoringState();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	Head->GetPresentationComponent()->SetHoverProbeState(
		WacomBattleWidgetSpec::MakeBattleEnemyPartHandle(
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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

	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Head"), Head);
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Body"), Body);
	Host->RefreshBattleEnemyPartAuthoringState();

	Head->GetPresentationComponent()->SetHoverProbeState(
		WacomBattleWidgetSpec::MakeBattleEnemyPartHandle(
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Head"), Head);
	Host->RefreshBattleEnemyPartAuthoringState();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	Head->GetPresentationComponent()->SetHoverProbeState(
		WacomBattleWidgetSpec::MakeBattleEnemyPartHandle(
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Unknown"), Part);

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleWidgetSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("Unknown part id warning keeps host valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("No host validation errors"), Errors.Num(), 0);
	TestTrue(TEXT("Warning mentions unknown part"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("Test.Part.Unknown")));
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Test.Part.Head"), Head);

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleWidgetSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("Missing definition part warning keeps host valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("No missing definition validation errors"), Errors.Num(), 0);
	TestTrue(TEXT("Warning mentions missing body"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("Test.Part.Body")));
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	WacomBattleWidgetSpec::FSlotIdentityEnemyDefinitionFixture EnemyFixture =
		WacomBattleWidgetSpec::MakeSlotIdentityEnemyDefinition();

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
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Snake.Head"), TEXT("Head"), Head);
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Snake.Body"), TEXT("Body"), Body);
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Snake.Tail"), TEXT("Tail"), Tail);

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleWidgetSpec::ValidateObjectForTest(Host, Warnings, Errors);
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	WacomBattleWidgetSpec::FSlotIdentityEnemyDefinitionFixture EnemyFixture =
		WacomBattleWidgetSpec::MakeSlotIdentityEnemyDefinition();

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
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Snake.Head"), TEXT("Snake.Head"), Head);
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Snake.Body"), TEXT("Snake.Body"), Body);
	WacomBattleWidgetSpec::AttachPartActorToHost(Host, TEXT("Snake.Tail"), TEXT("Snake.Tail"), Tail);

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleWidgetSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("Part slot mismatch stays warning-level"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("No part slot mismatch validation errors"), Errors.Num(), 0);
	TestTrue(TEXT("Warning mentions unknown authored slot"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("Snake.Head")));
	TestTrue(TEXT("Warning mentions missing definition slot"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("Head")));
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
		WacomBattleWidgetSpec::ValidateObjectForTest(First, Warnings, Errors);
	TestEqual(TEXT("Duplicate world part id keeps actor valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("No duplicate validation errors"), Errors.Num(), 0);
	TestFalse(TEXT("No duplicate warning at PartActor level"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("重复")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostDuplicateDefaultPartSlotIdValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostDuplicateDefaultPartSlotIdInvalidates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostDuplicateDefaultPartSlotIdValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	WacomBattleWidgetSpec::AttachPartActorToHost(
		Host,
		TEXT("Test.Part.Head"),
		TEXT("Test.Part.Head"),
		First);
	WacomBattleWidgetSpec::AttachPartActorToHost(
		Host,
		TEXT("Test.Part.Head"),
		TEXT("Test.Part.Head"),
		Second);
	Host->RefreshBattleEnemyPartAuthoringState();

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleWidgetSpec::ValidateObjectForTest(Host, Warnings, Errors);
	TestEqual(TEXT("Duplicate explicit part slot id invalidates host"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Duplicate explicit part slot id error"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Errors, TEXT("Test.Part.Head")));
	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestTrue(TEXT("Debug duplicate explicit part slot id"),
		View.DuplicatePartSlotIds.Contains(TEXT("Test.Part.Head")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerSceneEnemyHostRegistrySpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.BattleTriggerSceneEnemyHostDrivesHUDTargetRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerSceneEnemyHostRegistrySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body") });
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);

	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	WacomBattleWidgetSpec::ConfigureTriggerSceneEnemyHostSlotForTest(
		Trigger.Get(),
		TEXT("Enemy"),
		Enemy,
		SceneEnemy.Host);
	const FWacomBattleTriggerDebugView TriggerView = Trigger->GetBattleTriggerDebugView(nullptr);
	TestEqual(TEXT("Trigger debug reports host part count"), TriggerView.SceneEnemyHostPartCount, 2);
	TestTrue(TEXT("Trigger debug reports matching definition"), TriggerView.bSceneEnemyHostDefinitionMatches);

	TArray<AWacomBattleEnemyActor*> SceneHosts;
	Trigger->BuildBattleSceneEnemyHosts(SceneHosts);
	HUD->SetBattleSceneEnemyHostsForTest(SceneHosts);
	TestEqual(TEXT("HUD registry uses trigger host parts"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(),
		2);
	TestEqual(TEXT("Host debug reports active HUD usage"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugViewForHUD(HUD.Get()).bUsedByBattleHUD,
		true);
	TestTrue(TEXT("Host HUD summary reports usage"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugSummaryForHUD(HUD.Get()).Contains(TEXT("UsedByBattleHUD=true")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerSceneEnemyHostSlotsRegistrySpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.BattleTriggerSceneEnemyHostSlotsDriveHUDTargetRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerSceneEnemyHostSlotsRegistrySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* DamageCard = nullptr;
	UBattleSession* Session = nullptr;
	UEnemyDefinition* LeftEnemy = nullptr;
	UEnemyDefinition* RightEnemy = nullptr;
	UCharacterDefinition* Character = nullptr;
	{
		DamageCard = Fx.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/3);
		Character = Fx.MakeCharacter(
			Fx.MakeNoopCard(0),
			DamageCard,
			{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
		LeftEnemy = Fx.MakeSinglePartEnemy(/*Hp*/20, /*Initiative*/5, /*IntentResist*/0);
		RightEnemy = Fx.MakeSinglePartEnemy(/*Hp*/20, /*Initiative*/5, /*IntentResist*/0);

		FBattleInitParams Params;
		Params.Character = Character;
		Params.EncounterId = TEXT("Encounter.SceneHostSlots");
		Params.RandomSeed = 1;
		FBattleEnemySlotInit LeftSlot;
		LeftSlot.EnemySlotId = TEXT("LeftEnemy");
		LeftSlot.Enemy = LeftEnemy;
		Params.EnemySlots.Add(LeftSlot);
		FBattleEnemySlotInit RightSlot;
		RightSlot.EnemySlotId = TEXT("RightEnemy");
		RightSlot.Enemy = RightEnemy;
		Params.EnemySlots.Add(RightSlot);

		Session = NewObject<UBattleSession>(GetTransientPackage(), NAME_None, RF_Transient);
		const FWacomStatus Status = Session->Initialize(Params);
		if (!TestTrue(TEXT("Session initializes"), Status.IsOk()))
		{
			return false;
		}
	}

	WacomBattleWidgetSpec::FSceneEnemyHostActors LeftHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHostForSlot(
			*World,
			LeftEnemy,
			TEXT("LeftEnemy"),
			{ TEXT("Test.Part.Solo") },
			{ TEXT("Test.Part.Solo") });
	WacomBattleWidgetSpec::FSceneEnemyHostActors RightHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHostForSlot(
			*World,
			RightEnemy,
			TEXT("RightEnemy"),
			{ TEXT("Test.Part.Solo") },
			{ TEXT("Test.Part.Solo") });
	if (!TestNotNull(TEXT("Left host"), LeftHost.Host)
		|| !TestNotNull(TEXT("Right host"), RightHost.Host)
		|| !TestEqual(TEXT("Left host part count"), LeftHost.Parts.Num(), 1)
		|| !TestEqual(TEXT("Right host part count"), RightHost.Parts.Num(), 1))
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(RightHost);
		WacomBattleWidgetSpec::DestroySceneEnemyHost(LeftHost);
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(RightHost);
		WacomBattleWidgetSpec::DestroySceneEnemyHost(LeftHost);
	};

	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Test.Battle.SceneHostSlots");
	Trigger->EncounterDefinition = WacomBattleWidgetSpec::MakeEncounterDefinitionForTest(
		Trigger.Get(),
		{
			TPair<FName, UEnemyDefinition*>(TEXT("LeftEnemy"), LeftEnemy),
			TPair<FName, UEnemyDefinition*>(TEXT("RightEnemy"), RightEnemy),
		});
	FWacomBattleSceneEnemyHostSlot LeftSceneSlot;
	LeftSceneSlot.EnemySlotId = TEXT("LeftEnemy");
	LeftSceneSlot.SceneEnemyHost = LeftHost.Host;
	FWacomBattleSceneEnemyHostSlot RightSceneSlot;
	RightSceneSlot.EnemySlotId = TEXT("RightEnemy");
	RightSceneSlot.SceneEnemyHost = RightHost.Host;
	Trigger->SceneEnemyHostSlots = { LeftSceneSlot, RightSceneSlot };

	TArray<AWacomBattleEnemyActor*> SceneHosts;
	Trigger->BuildBattleSceneEnemyHosts(SceneHosts);
	TestEqual(TEXT("Trigger exports two scene hosts"), SceneHosts.Num(), 2);
	TestEqual(TEXT("Left host slot id synced"), LeftHost.Host->GetEffectiveEnemySlotId(), FName(TEXT("LeftEnemy")));
	TestEqual(TEXT("Right host slot id synced"), RightHost.Host->GetEffectiveEnemySlotId(), FName(TEXT("RightEnemy")));

	const FWacomBattleTriggerDebugView TriggerView = Trigger->GetBattleTriggerDebugView(nullptr);
	TestEqual(TEXT("Trigger debug reports host slot count"), TriggerView.SceneEnemyHostSlotCount, 2);
	TestEqual(TEXT("Trigger debug reports host count"), TriggerView.SceneEnemyHostCount, 2);
	TestEqual(TEXT("Trigger debug reports total part count"), TriggerView.SceneEnemyHostPartCount, 2);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest(SceneHosts);
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	TestEqual(TEXT("HUD registry uses both scene host slots"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(),
		2);
	TestTrue(TEXT("Left host is in current registry"),
		HUD->IsBattleSceneEnemyHostInCurrentRegistry(LeftHost.Host));
	TestTrue(TEXT("Right host is in current registry"),
		HUD->IsBattleSceneEnemyHostInCurrentRegistry(RightHost.Host));
	TestTrue(TEXT("Left part binds to left enemy slot"),
		LeftHost.Parts[0]->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bBoundToSnapshot);
	TestTrue(TEXT("Right part binds to right enemy slot"),
		RightHost.Parts[0]->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bBoundToSnapshot);
	TestEqual(TEXT("Left bridge enemy slot"),
		LeftHost.Parts[0]->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().EnemySlotId,
		FName(TEXT("LeftEnemy")));
	TestEqual(TEXT("Right bridge enemy slot"),
		RightHost.Parts[0]->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().EnemySlotId,
		FName(TEXT("RightEnemy")));
	TestEqual(TEXT("Left host debug reports active HUD usage"),
		LeftHost.Host->GetBattleSceneEnemyDebugViewForHUD(HUD.Get()).bUsedByBattleHUD,
		true);
	TestEqual(TEXT("Right host debug reports active HUD usage"),
		RightHost.Host->GetBattleSceneEnemyDebugViewForHUD(HUD.Get()).bUsedByBattleHUD,
		true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDSyncsOnlyCurrentHostSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.HUDSyncsOnlyCurrentHostChildPartActors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDSyncsOnlyCurrentHostSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	WacomBattleWidgetSpec::FSceneEnemyHostActors CurrentHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body") });
	WacomBattleWidgetSpec::FSceneEnemyHostActors OtherHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	if (!TestNotNull(TEXT("Current host"), CurrentHost.Host)
		|| !TestNotNull(TEXT("Other host"), OtherHost.Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(OtherHost);
		WacomBattleWidgetSpec::DestroySceneEnemyHost(CurrentHost);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ CurrentHost.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	TestEqual(TEXT("Only current host bridges are registered"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(),
		2);
	TestTrue(TEXT("Current head binds"),
		CurrentHost.Parts[0]->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
	TestTrue(TEXT("Current body binds"),
		CurrentHost.Parts[1]->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
	TestFalse(TEXT("Unrelated host does not bind"),
		OtherHost.Parts[0]->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
	TestEqual(TEXT("Other host debug stays unused"),
		OtherHost.Host->GetBattleSceneEnemyDebugViewForHUD(HUD.Get()).bUsedByBattleHUD,
		false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDIgnoresUnrelatedSceneEnemyPartsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.HUDDoesNotBindOrPreviewUnrelatedSceneEnemyParts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDIgnoresUnrelatedSceneEnemyPartsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	const FGuid CardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleWidgetSpec::FSceneEnemyHostActors CurrentHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	WacomBattleWidgetSpec::FSceneEnemyHostActors OtherHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Body") });
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Current host"), CurrentHost.Host)
		|| !TestNotNull(TEXT("Other host"), OtherHost.Host)
		|| !TestTrue(TEXT("Target card exists"), CardId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(OtherHost);
		WacomBattleWidgetSpec::DestroySceneEnemyHost(CurrentHost);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ CurrentHost.Host });
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());
	OtherHost.Parts[0]->GetInteractionTargetComponent()->SetTargetId(FWacomBattleFixture::FindPartInstanceId(Snapshot, 1));
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, OtherHost.Parts[0], OtherHost.Parts[0]->GetHitBounds());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = CardId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.bHasPointerViewportPosition = true;
	DragView.PointerViewportPosition = FVector2D(540.0f, 590.0f);
	HUD->HandleFirstPersonCardDragUpdatedForTest(CardId, DragView);

	TestFalse(TEXT("Unrelated part does not preview"),
		OtherHost.Parts[0]->GetPresentationComponent()
			->GetBattleEnemyPartPresentationDebugView()
			.bDragPreviewActive);
	TestFalse(TEXT("Unrelated part does not hover"),
		OtherHost.Parts[0]->GetPresentationComponent()
			->GetBattleEnemyPartPresentationDebugView()
			.bHoverActive);
	const FWacomBattleCardDropResolveResult DropResult =
		HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Unrelated world target rejected"),
		DropResult.RejectReason,
		EWacomBattleCardDropRejectReason::InvalidWorldTarget);

	HUD->SetTargetSelectionStateForTest(CardId);
	const int32 VersionBeforeClick = Session->BuildSnapshot().Version;
	TestFalse(TEXT("Unrelated part click is not routed"),
		FWacomBattleSceneTargetClickTestAccess::RouteClick(PC));
	TestEqual(TEXT("Unrelated part click does not submit"),
		Session->BuildSnapshot().Version,
		VersionBeforeClick);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerEncounterMissingSceneEnemyHostInvalidSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.EncounterMissingSceneEnemyHostInvalidatesTrigger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerEncounterMissingSceneEnemyHostInvalidSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);

	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Test.Battle.EncounterMissingHost");
	Trigger->EncounterDefinition = WacomBattleWidgetSpec::MakeEncounterDefinitionForTest(
		Trigger.Get(),
		{ TPair<FName, UEnemyDefinition*>(TEXT("Enemy"), Enemy) });
	Trigger->SceneEnemyHostSlots.Reset();

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleWidgetSpec::ValidateObjectForTest(Trigger.Get(), Warnings, Errors);
	TestEqual(TEXT("Encounter missing scene host invalidates trigger"),
		Result,
		EDataValidationResult::Invalid);
	TestTrue(TEXT("Missing encounter host error mentions SceneEnemyHost"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Errors, TEXT("SceneEnemyHost")));
	TestFalse(TEXT("Debug reports host missing"),
		Trigger->GetBattleTriggerDebugView(nullptr).bSceneEnemyHostConfigured);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerSceneEnemyHostDefinitionMismatchWarningSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.SceneEnemyHostDefinitionMismatchReportsWarning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerSceneEnemyHostDefinitionMismatchWarningSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* TriggerEnemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UEnemyDefinition* HostEnemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			HostEnemy,
			{ TEXT("Test.Part.Head") });
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Test.Battle.MismatchedHost");
	WacomBattleWidgetSpec::ConfigureTriggerSceneEnemyHostSlotForTest(
		Trigger.Get(),
		TEXT("Enemy"),
		TriggerEnemy,
		SceneEnemy.Host);

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleWidgetSpec::ValidateObjectForTest(Trigger.Get(), Warnings, Errors);
	TestEqual(TEXT("Definition mismatch keeps trigger valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Definition mismatch has no errors"), Errors.Num(), 0);
	TestTrue(TEXT("Mismatch warning mentions SceneEnemyHost"),
		WacomBattleWidgetSpec::ValidationIssuesContain(Warnings, TEXT("SceneEnemyHost")));
	TestFalse(TEXT("Debug reports definition mismatch"),
		Trigger->GetBattleTriggerDebugView(nullptr).bSceneEnemyHostDefinitionMatches);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCurrentHostRegistryRoutesFeedbackSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.TargetCueHoverPredictionAndDragPreviewUseCurrentHostRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCurrentHostRegistryRoutesFeedbackSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	const FGuid CardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleWidgetSpec::FSceneEnemyHostActors CurrentHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	WacomBattleWidgetSpec::FSceneEnemyHostActors OtherHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Body") });
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Current host"), CurrentHost.Host)
		|| !TestNotNull(TEXT("Other host"), OtherHost.Host)
		|| !TestTrue(TEXT("Target card exists"), CardId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(OtherHost);
		WacomBattleWidgetSpec::DestroySceneEnemyHost(CurrentHost);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	AWacomBattleEnemyPartActor* CurrentPart = CurrentHost.Parts[0];
	AWacomBattleEnemyPartActor* OtherPart = OtherHost.Parts[0];
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ CurrentHost.Host });
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());

	HUD->PlayTargetConfirmedCueForTest(Snapshot.Enemies[0].Parts[0].Identity);
	TestEqual(TEXT("Current host cue routed"),
		CurrentPart->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().CuePlayCount,
		1);
	TestEqual(TEXT("Other host cue ignored"),
		OtherPart->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().CuePlayCount,
		0);

	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, CurrentPart, CurrentPart->GetHitBounds());
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestTrue(TEXT("Current host hover activates"),
		CurrentPart->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().bHoverActive);
	TestFalse(TEXT("Current host hover prediction handled by enemy panel"),
		CurrentPart->GetPresentationComponent()
			->GetBattleEnemyPartPresentationDebugView()
			.PredictionView.bVisible);

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = CardId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.bHasPointerViewportPosition = true;
	DragView.PointerViewportPosition = FVector2D(540.0f, 590.0f);
	HUD->HandleFirstPersonCardDragStartedForTest(CardId, DragView);
	HUD->HandleFirstPersonCardDragUpdatedForTest(CardId, DragView);
	TestTrue(TEXT("Current host drag preview activates"),
		CurrentPart->GetPresentationComponent()
			->GetBattleEnemyPartPresentationDebugView()
			.bDragPreviewActive);
	TestFalse(TEXT("Other host drag preview stays inactive"),
		OtherPart->GetPresentationComponent()
			->GetBattleEnemyPartPresentationDebugView()
			.bDragPreviewActive);

	const FWacomBattleCardDropResolveResult DropResult =
		HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Current host drop target resolves"),
		DropResult.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardWorldTarget);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartPredictionBadgeOffsetSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartPredictionBadgeAppliesConfiguredOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartPredictionBadgeOffsetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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

	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(*World, Enemy, { TEXT("Test.Part.Head") });
	AWacomBattleEnemyPartActor* PartActor = SceneEnemy.Parts.Num() > 0 ? SceneEnemy.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
	};

	PartActor->PredictionRelativeLocation = FVector(0.0f, 0.0f, 80.0f);
	PartActor->PredictionBadgeZOffsetWhenVisible = 36.0f;
	PartActor->RefreshAuthoringState();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);

	const FVector PredictionBaseLocation = PartActor->GetPredictionWidgetComponent()->GetRelativeLocation();
	PartActor->GetPresentationComponent()->SetHoverProbeState(
		WacomBattleWidgetSpec::MakeBattleEnemyPartHandle(
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
		WacomBattleWidgetSpec::ValidateObjectForTest(PartCDO, Warnings, Errors),
		EDataValidationResult::Valid);
	TestEqual(TEXT("Part CDO no warnings"), Warnings.Num(), 0);
	TestEqual(TEXT("Part CDO no errors"), Errors.Num(), 0);

	TestEqual(TEXT("Host CDO remains valid"),
		WacomBattleWidgetSpec::ValidateObjectForTest(HostCDO, Warnings, Errors),
		EDataValidationResult::Valid);
	TestEqual(TEXT("Host CDO no warnings"), Warnings.Num(), 0);
	TestEqual(TEXT("Host CDO no errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneClickRoutesTaggedInteractionTargetSpec,
	"Wacom.UI.Battle.InteractionTarget.SceneClickRoutesTaggedEnemyPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneClickRoutesTaggedInteractionTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
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
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, SceneEnemy.Parts[0], SceneEnemy.Parts[0]->GetHitBounds());

	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("HUD enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);

	TestTrue(TEXT("Tagged world target routes"), FWacomBattleSceneTargetClickTestAccess::RouteClick(PC));
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
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
	HUD->SetSession(Session);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, Owner, Primitive);

	HUD->OnCardClickedByUser(TargetCardId);
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
	UWorld* World = nullptr;
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (UWorld* Candidate = Context.World())
			{
				World = Candidate;
				break;
			}
		}
	}
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

	Owner->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartHoverProbeSetsBridgeStateSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartHoverProbeSetsBridgeHoverState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartHoverProbeSetsBridgeStateSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	const FGuid CardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::None);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
	const FGuid CardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::None);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleWidgetSpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
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
		WacomBattleWidgetSpec::DestroySceneEnemyHost(SceneEnemy);
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
	WacomBattleWidgetSpec::SettleBattlePresentationQueue(*HUD);
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
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
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
		WacomBattleWidgetSpec::MakeBattleEnemyPartHandle(
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetRegistryUnknownTargetNoopsSpec,
	"Wacom.UI.Battle.PresentationTargetRegistry.UnknownTargetNoops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetRegistryUnknownTargetNoopsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, FBattlePartSlotIdentity(), 3);
	HUD->PlayBattlePresentationCueForTest(
		EBattleEventType::DamageDealt,
		FBattlePartSlotIdentity(TEXT("Encounter"), TEXT("UnknownEnemy"), TEXT("UnknownPart")),
		3);

	TestEqual(TEXT("Unknown cues do not create registry entries"), HUD->GetBattlePresentationTargetCountForTest(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDPrivateCoordinatorSurfaceSpec,
	"Wacom.UI.Battle.BattleHUDPrivateCoordinatorsPreservePublicHUDContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDPrivateCoordinatorSurfaceSpec::RunTest(const FString& /*Parameters*/)
{
	static const TCHAR* PrivateHelperHeaders[] = {
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDSceneEnemyTargetCoordinator.h"),
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDPresentationCoordinator.h"),
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDCombatLogController.h"),
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDFirstPersonHandBridge.h"),
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDCardDetailController.h"),
	};

	for (const TCHAR* RelativeHeaderPath : PrivateHelperHeaders)
	{
		const FString HeaderPath = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(FPaths::ProjectDir(), RelativeHeaderPath));
		FString HeaderText;
		if (!TestTrue(
				FString::Printf(TEXT("Private helper header exists: %s"), RelativeHeaderPath),
				FPaths::FileExists(HeaderPath)))
		{
			continue;
		}
		if (!TestTrue(
				FString::Printf(TEXT("Private helper header can be read: %s"), RelativeHeaderPath),
				FFileHelper::LoadFileToString(HeaderText, *HeaderPath)))
		{
			continue;
		}

		TestFalse(
			FString::Printf(TEXT("%s does not declare a reflected UCLASS"), RelativeHeaderPath),
			HeaderText.Contains(TEXT("UCLASS(")));
		TestFalse(
			FString::Printf(TEXT("%s does not declare a reflected USTRUCT"), RelativeHeaderPath),
			HeaderText.Contains(TEXT("USTRUCT(")));
		TestFalse(
			FString::Printf(TEXT("%s does not use GENERATED_BODY"), RelativeHeaderPath),
			HeaderText.Contains(TEXT("GENERATED_BODY")));
		TestFalse(
			FString::Printf(TEXT("%s does not export a WacomApp public symbol"), RelativeHeaderPath),
			HeaderText.Contains(TEXT("WACOMAPP_API FWacomBattleHUD")));
	}

	const FString PresentationCoordinatorSourcePath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDPresentationCoordinator.cpp")));
	FString PresentationCoordinatorSource;
	if (TestTrue(
			TEXT("Presentation coordinator source exists"),
			FPaths::FileExists(PresentationCoordinatorSourcePath))
		&& TestTrue(
			TEXT("Presentation coordinator source can be read"),
			FFileHelper::LoadFileToString(PresentationCoordinatorSource, *PresentationCoordinatorSourcePath)))
	{
		const FString DestructorSignature =
			TEXT("FWacomBattleHUDPresentationCoordinator::~FWacomBattleHUDPresentationCoordinator()");
		const int32 DestructorStart = PresentationCoordinatorSource.Find(DestructorSignature);
		if (TestTrue(TEXT("Presentation coordinator destructor exists"), DestructorStart != INDEX_NONE))
		{
			const int32 NextMethodStart = PresentationCoordinatorSource.Find(
				TEXT("\nvoid FWacomBattleHUDPresentationCoordinator::"),
				ESearchCase::CaseSensitive,
				ESearchDir::FromStart,
				DestructorStart + DestructorSignature.Len());
			const FString DestructorBody = NextMethodStart != INDEX_NONE
				? PresentationCoordinatorSource.Mid(DestructorStart, NextMethodStart - DestructorStart)
				: PresentationCoordinatorSource.Mid(DestructorStart);
			TestFalse(
				TEXT("Presentation coordinator destructor does not call HUD/World teardown"),
				DestructorBody.Contains(TEXT("GetWorld(")));
			TestFalse(
				TEXT("Presentation coordinator destructor does not clear timer manager"),
				DestructorBody.Contains(TEXT("GetTimerManager")));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDSceneEnemyCoordinatorLifecycleSpec,
	"Wacom.UI.Battle.BattleHUDSceneEnemyCoordinatorLifecycleCleansHostState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDSceneEnemyCoordinatorLifecycleSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("HUD"), Harness->HUD()))
	{
		return false;
	}

	FWacomBattleHUDTestSceneEnemyHost& CurrentHost =
		Harness->AttachSceneEnemyHost(
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body"), TEXT("Test.Part.Tail") });
	WacomBattleWidgetSpec::FSceneEnemyHostActors OtherHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Other.Part.Head") });
	if (!TestNotNull(TEXT("Current scene enemy host"), CurrentHost.Host)
		|| !TestNotNull(TEXT("Other scene enemy host"), OtherHost.Host)
		|| !TestEqual(TEXT("Current host part count"), CurrentHost.Parts.Num(), 3)
		|| !TestEqual(TEXT("Other host part count"), OtherHost.Parts.Num(), 1))
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(OtherHost);
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(OtherHost);
	};

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ CurrentHost.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	Harness->SettlePresentationQueue();

	TestTrue(TEXT("HUD registry contains configured scene enemy host"),
		HUD->IsBattleSceneEnemyHostInCurrentRegistry(CurrentHost.Host));
	TestFalse(TEXT("HUD registry rejects unconfigured scene enemy host"),
		HUD->IsBattleSceneEnemyHostInCurrentRegistry(OtherHost.Host));
	TestEqual(TEXT("Coordinator exposes only current host bridges through HUD"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(),
		3);

	AWacomBattleEnemyPartActor* CurrentPart = CurrentHost.Parts[0];
	AWacomBattleEnemyPartActor* OtherPart = OtherHost.Parts[0];
	const FWacomInteractionTargetHandle CurrentHandle = WacomBattleWidgetSpec::MakeBattleEnemyPartHandle(
		CurrentPart->GetInteractionTargetComponent(),
		CurrentPart->GetWorldTargetBridgeComponent()->GetPartInstanceId(),
		CurrentPart->GetEffectivePartDefinitionId(),
		CurrentPart->GetWorldTargetBridgeComponent()->GetBoundEncounterId(),
		CurrentPart->GetWorldTargetBridgeComponent()->GetBoundEnemySlotId(),
		CurrentPart->GetWorldTargetBridgeComponent()->GetBoundPartSlotId(),
		FVector2D(640.0f, 360.0f));
	const FWacomInteractionTargetHandle OtherHandle = WacomBattleWidgetSpec::MakeBattleEnemyPartHandle(
		OtherPart->GetInteractionTargetComponent(),
		OtherPart->GetWorldTargetBridgeComponent()->GetPartInstanceId(),
		OtherPart->GetEffectivePartDefinitionId(),
		OtherPart->GetWorldTargetBridgeComponent()->GetBoundEncounterId(),
		OtherPart->GetWorldTargetBridgeComponent()->GetBoundEnemySlotId(),
		OtherPart->GetWorldTargetBridgeComponent()->GetBoundPartSlotId(),
		FVector2D(720.0f, 360.0f));

	TestTrue(TEXT("Current host part is accepted by HUD registry"),
		HUD->IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(CurrentHandle));
	TestFalse(TEXT("Other host part is filtered by HUD registry"),
		HUD->IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(OtherHandle));
	TestTrue(TEXT("Current bridge is bound to snapshot"),
		CurrentPart->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bBoundToSnapshot);
	TestFalse(TEXT("Other bridge remains unbound"),
		OtherPart->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bBoundToSnapshot);

	HUD->SetSession(nullptr);
	TestFalse(TEXT("Session clear removes scene enemy host"),
		HUD->IsBattleSceneEnemyHostInCurrentRegistry(CurrentHost.Host));
	TestEqual(TEXT("Session clear removes bridge registry"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(),
		0);
	TestFalse(TEXT("Session clear unbinds current bridge"),
		CurrentPart->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bBoundToSnapshot);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDPresentationCoordinatorContractSpec,
	"Wacom.UI.Battle.BattleHUDPresentationCoordinatorPendingBarrierLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDPresentationCoordinatorContractSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 50, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->AttachPresentationStack();
	UWacomActionPanelTestProbe* ActionPanel = Harness->AttachActionPanel();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("ActionPanel"), ActionPanel))
	{
		return false;
	}

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Target card exists"), TargetCardId.IsValid())
		|| !TestTrue(TEXT("Target part exists"), TargetPartId.IsValid()))
	{
		return false;
	}

	FBattleEvent PresentationCueEvent;
	PresentationCueEvent.Type = EBattleEventType::DamageDealt;
	PresentationCueEvent.Sequence = 1;
	PresentationCueEvent.ActorEnemyPartKey = FWacomBattleFixture::FindPartKeyByInstanceId(InitialSnapshot, TargetPartId);
	PresentationCueEvent.Amount = 1;
	HUD->EnqueueBattlePresentationEventsForTest({ PresentationCueEvent });
	if (World)
	{
		World->GetTimerManager().Tick(0.01f);
	}
	TestTrue(TEXT("Seed cue makes presentation coordinator busy through HUD"), HUD->IsBattlePresentationBusy());

	HUD->OnCardClickedByUser(TargetCardId);
	HUD->OnEnemyPartClickedByUser(
		WacomBattleWidgetSpec::MakeWorldTargetHandleForPart(Session->BuildSnapshot(), TargetPartId));
	TestTrue(TEXT("PlayCard creates presentation stack busy state"), HUD->IsBattlePresentationBusy());
	TestEqual(TEXT("Presentation stack contains played card"), HUD->GetPresentationStackEntryCountForTest(), 1);

	const int32 VersionBeforeWait = Session->BuildSnapshot().Version;
	HUD->OnWaitRequested();
	TestTrue(TEXT("Wait is queued through HUD while stack is non-empty"),
		HUD->HasPendingTurnBoundaryCommandForTest());
	TestTrue(TEXT("Pending command text remains player readable"),
		HUD->GetPendingTurnBoundaryCommandText().ToString().Contains(TEXT("等待")));
	TestFalse(TEXT("Action panel wait is disabled while pending through coordinator"),
		ActionPanel->IsWaitButtonEnabledForTest());
	TestFalse(TEXT("Action panel end turn is disabled while pending through coordinator"),
		ActionPanel->IsEndTurnButtonEnabledForTest());
	TestEqual(TEXT("Pending wait does not mutate immediately"),
		Session->BuildSnapshot().Version,
		VersionBeforeWait);

	while (HUD->IsBattlePresentationBusy()
		&& !HUD->GetPresentationStackEntriesForTest().IsEmpty()
		&& !HUD->GetPresentationStackEntriesForTest()[0].bIsExiting)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}

	const TArray<FWacomBattlePresentationStackEntryView> EntriesAtBoundary =
		HUD->GetPresentationStackEntriesForTest();
	if (!EntriesAtBoundary.IsEmpty())
	{
		TestTrue(TEXT("Boundary marks stack entry exiting through HUD"),
			EntriesAtBoundary[0].bIsExiting);
		TestTrue(TEXT("Pending command survives stack exit motion"),
			HUD->HasPendingTurnBoundaryCommandForTest());
		HUD->FinishPresentationStackEntryExitForTest(EntriesAtBoundary[0].EntryId);
	}
	Harness->SettlePresentationQueueAndExitStack();
	TestFalse(TEXT("Pending command clears after stack drains"),
		HUD->HasPendingTurnBoundaryCommandForTest());
	TestEqual(TEXT("Stack drains through HUD"), HUD->GetPresentationStackEntryCountForTest(), 0);
	TestTrue(TEXT("Pending wait executes after stack drain"),
		Session->BuildSnapshot().Version > VersionBeforeWait);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDPresentationCoordinatorTeardownSpec,
	"Wacom.UI.Battle.BattleHUDPresentationCoordinatorTeardownDoesNotTouchDestroyedHUD",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDPresentationCoordinatorTeardownSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 50, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->AttachPresentationStack();
	Harness->AttachActionPanel();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid TargetPartId = FWacomBattleFixture::FindPartInstanceId(InitialSnapshot, 0);
	if (!TestTrue(TEXT("Target card exists"), TargetCardId.IsValid())
		|| !TestTrue(TEXT("Target part exists"), TargetPartId.IsValid()))
	{
		return false;
	}

	FBattleEvent PresentationCueEvent;
	PresentationCueEvent.Type = EBattleEventType::DamageDealt;
	PresentationCueEvent.Sequence = 1;
	PresentationCueEvent.ActorEnemyPartKey = FWacomBattleFixture::FindPartKeyByInstanceId(InitialSnapshot, TargetPartId);
	PresentationCueEvent.Amount = 1;
	HUD->EnqueueBattlePresentationEventsForTest({ PresentationCueEvent });
	TestTrue(TEXT("Presentation queue is busy before teardown"), HUD->IsBattlePresentationBusy());

	HUD->OnCardClickedByUser(TargetCardId);
	HUD->OnEnemyPartClickedByUser(
		WacomBattleWidgetSpec::MakeWorldTargetHandleForPart(Session->BuildSnapshot(), TargetPartId));
	HUD->OnWaitRequested();
	TestTrue(TEXT("Stack or queue is busy before teardown"), HUD->IsBattlePresentationBusy());
	TestTrue(TEXT("Pending turn boundary exists before teardown"), HUD->HasPendingTurnBoundaryCommandForTest());

	HUD->NativeDestructForTest();

	TestFalse(TEXT("NativeDestruct clears presentation busy state"), HUD->IsBattlePresentationBusy());
	TestFalse(TEXT("NativeDestruct clears pending turn boundary"), HUD->HasPendingTurnBoundaryCommandForTest());
	TestEqual(TEXT("NativeDestruct clears stack entries"), HUD->GetPresentationStackEntryCountForTest(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCombatLogControllerContractSpec,
	"Wacom.UI.Battle.BattleHUDCombatLogControllerClearsAndTrimsThroughHUD",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCombatLogControllerContractSpec::RunTest(const FString& /*Parameters*/)
{
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(WacomBattleWidgetSpec::FindAutomationWorld());
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UBattleCombatLogFeedWidget* Feed = Harness->AttachCombatLogFeed();
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("CombatLogFeed"), Feed))
	{
		return false;
	}
	HUD->BattleCombatLogMaxBlocks = 2;

	FWacomBattleCombatLogBlockView First;
	First.bShouldDisplay = true;
	First.HeaderText = FText::FromString(TEXT("第一块"));
	FWacomBattleCombatLogBlockView Second;
	Second.bShouldDisplay = true;
	Second.HeaderText = FText::FromString(TEXT("第二块"));
	FWacomBattleCombatLogBlockView Third;
	Third.bShouldDisplay = true;
	Third.HeaderText = FText::FromString(TEXT("第三块"));

	HUD->AppendBattleCombatLogBlockForTest(First);
	HUD->AppendBattleCombatLogBlockForTest(Second);
	HUD->AppendBattleCombatLogBlockForTest(Third);
	TestEqual(TEXT("HUD exposes trimmed combat log block count"), HUD->GetBattleCombatLogBlockCount(), 2);
	TestEqual(TEXT("HUD history keeps recent block"),
		HUD->GetBattleCombatLogHistoryForTest()[0].HeaderText.ToString(),
		FString(TEXT("第二块")));
	TestEqual(TEXT("Feed mirrors controller history through HUD"),
		Feed->GetVisibleBlockCount(),
		HUD->GetBattleCombatLogBlockCount());

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(Character, Fx.MakeSinglePartEnemy(20, 5, 0), 1);
	Harness->SetSession(Session);
	TestTrue(TEXT("SetSession appends initial system-visible combat log"),
		HUD->GetBattleCombatLogBlockCount() > 0);
	Harness->SetSession(nullptr);
	TestEqual(TEXT("Session clear clears combat log through HUD"), HUD->GetBattleCombatLogBlockCount(), 0);
	TestEqual(TEXT("Session clear syncs feed through HUD"), Feed->GetVisibleBlockCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDFirstPersonHandBridgeContractSpec,
	"Wacom.UI.Battle.BattleHUD.HandPresentation.FirstPersonBridgeCleansRuntimeStateOnClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDFirstPersonHandBridgeContractSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 5, 0), 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("PlayerController"), Harness->PlayerController()))
	{
		return false;
	}
	AWacomPlayerCharacter* Character = Harness->AttachFirstPersonCharacter();
	if (!TestNotNull(TEXT("Character"), Character))
	{
		return false;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor = Harness->FirstPersonAnchor();
	UWacomBattleCameraLookComponent* BattleCamera = Harness->BattleCameraLook();
	if (!TestNotNull(TEXT("First-person card anchor"), Anchor)
		|| !TestNotNull(TEXT("Battle camera look"), BattleCamera))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomBattleWidgetSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	if (!TestTrue(TEXT("Target card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	TestTrue(TEXT("HUD bridge writes runtime hand to anchor"), Anchor->HasRuntimeCardLayerData());
	TestTrue(TEXT("HUD bridge enables first-person hand interaction"),
		Anchor->IsBattleHandInteractionEnabled());

	TestTrue(TEXT("Battle camera activates for drag override"), BattleCamera->ActivateBattleCameraLook());
	FWacomFirstPersonCardPointerView PointerView;
	PointerView.CardInstanceId = CardId;
	PointerView.bHasPointerViewportPosition = true;
	PointerView.PointerNormalizedViewportPosition = FVector2D(0.35f, -0.45f);
	HUD->HandleFirstPersonCardPointerMovedForTest(PointerView);
	TestTrue(TEXT("Hover pointer writes camera look override"), BattleCamera->HasCursorLookOverrideForTest());
	TestEqual(
		TEXT("Hover pointer override stores normalized pointer"),
		BattleCamera->GetCursorLookOverrideNormalizedForTest(),
		FVector2D(0.35f, -0.45f));
	HUD->HandleFirstPersonCardPointerLeftForTest();
	TestFalse(TEXT("Hover pointer leave clears camera look override"), BattleCamera->HasCursorLookOverrideForTest());

	FWacomFirstPersonCardDragView DragView = WacomBattleWidgetSpec::MakeCommitDragView(CardId);
	HUD->HandleFirstPersonCardDragStartedForTest(CardId, DragView);
	HUD->HandleFirstPersonCardDragUpdatedForTest(CardId, DragView);
	TestTrue(TEXT("Drag update writes camera look override"), BattleCamera->HasCursorLookOverrideForTest());

	HUD->ClearFirstPersonBattleHandLayerForTest();
	TestFalse(TEXT("HUD bridge clear removes runtime hand"), Anchor->HasRuntimeCardLayerData());
	TestFalse(TEXT("HUD bridge clear disables interaction"), Anchor->IsBattleHandInteractionEnabled());
	TestFalse(TEXT("HUD bridge clear removes camera look override"), BattleCamera->HasCursorLookOverrideForTest());
	TestFalse(TEXT("HUD bridge clear hides first-person detail"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	const int32 VersionBeforeStaleDelegate = Session->BuildSnapshot().Version;
	FWacomFirstPersonCardDragView StaleDragView = WacomBattleWidgetSpec::MakeCommitDragView(CardId);
	Anchor->OnFirstPersonCardLayerDragReleased.Broadcast(CardId, StaleDragView);
	TestEqual(TEXT("Cleared bridge unbinds anchor drag delegate"),
		Session->BuildSnapshot().Version,
		VersionBeforeStaleDelegate);
	TestEqual(TEXT("Cleared bridge does not set target select"),
		HUD->GetUIState(),
		EBattleUIState::Idle);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCardDetailControllerContractSpec,
	"Wacom.UI.Battle.BattleHUD.CardDetail.ControllerUsesFirstPersonViewportOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardDetailControllerContractSpec::RunTest(const FString& /*Parameters*/)
{
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(WacomBattleWidgetSpec::FindAutomationWorld());
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("HUD"), Harness->HUD()))
	{
		return false;
	}
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	TStrongObjectPtr<UCardDefinition> FirstPersonCard(NewObject<UCardDefinition>());
	FirstPersonCard->CardId = TEXT("Contract.FirstPerson.Detail");
	FirstPersonCard->DisplayName = FText::FromString(TEXT("第一人称详情合同卡"));
	FHandCardSnapshot FirstPersonSnap;
	FirstPersonSnap.InstanceId = FGuid::NewGuid();
	FirstPersonSnap.Definition = FirstPersonCard.Get();
	FirstPersonSnap.RuntimeCost = 1;
	FirstPersonSnap.bIsPlayable = true;

	FBattleSnapshot Snapshot;
	Snapshot.Phase = EBattlePhase::PlayerAction;
	Snapshot.Hand.Cards.Add(FirstPersonSnap);
	Snapshot.Hand.NormalCardCount = 1;
	HUD->RefreshFromSnapshotForTest(Snapshot);

	FWacomFirstPersonCardLayerSlotView SlotView;
	SlotView.Entry.CardInstanceId = FirstPersonSnap.InstanceId;
	SlotView.ScreenPosition = FVector2D(700.0f, 520.0f);
	SlotView.RenderScale = 1.0f;
	SlotView.RenderOpacity = 1.0f;
	SlotView.bProjected = true;
	HUD->HandleFirstPersonCardHoveredForTest(FirstPersonSnap.InstanceId, SlotView);
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("First-person detail is visible through HUD wrapper"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestEqual(TEXT("First-person detail name is exposed through HUD wrapper"),
		HUD->GetFirstPersonCardDetailPanelNameTextForTest().ToString(),
		FString(TEXT("第一人称详情合同卡")));

	HUD->HideCardDetailForTest();
	TestFalse(TEXT("HUD hide clears first-person detail host"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestFalse(TEXT("HUD hide reports no visible detail"),
		HUD->IsCardDetailPanelVisible());

	HUD->HandleFirstPersonCardHoveredForTest(FirstPersonSnap.InstanceId, SlotView);
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("First-person detail can show again before BattleEnd"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	FBattleSnapshot BattleEndSnapshot = Snapshot;
	BattleEndSnapshot.Phase = EBattlePhase::BattleEnd;
	HUD->RefreshFromSnapshotForTest(BattleEndSnapshot);
	TestFalse(TEXT("BattleEnd refresh clears first-person detail"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestFalse(TEXT("BattleEnd refresh reports no visible detail"),
		HUD->IsCardDetailPanelVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCardDetailReadabilityMotionSpec,
	"Wacom.UI.Battle.BattleHUD.CardDetail.FirstPersonReadabilityMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardDetailReadabilityMotionSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	Card->CardId = TEXT("BattleDetailMotionCard");
	Card->DisplayName = FText::FromString(TEXT("详情动效卡"));

	FHandCardSnapshot Snap;
	Snap.InstanceId = FGuid::NewGuid();
	Snap.Definition = Card.Get();
	Snap.RuntimeCost = 1;
	Snap.bIsPlayable = true;

	FBattleSnapshot BattleSnapshot;
	BattleSnapshot.Phase = EBattlePhase::PlayerAction;
	BattleSnapshot.Hand.Cards.Add(Snap);
	BattleSnapshot.Hand.NormalCardCount = 1;

	FWacomFirstPersonCardLayerSlotView SlotView;
	SlotView.Entry.CardInstanceId = Snap.InstanceId;
	SlotView.ScreenPosition = FVector2D(700.0f, 520.0f);
	SlotView.RenderScale = 1.0f;
	SlotView.RenderOpacity = 1.0f;
	SlotView.bProjected = true;

	HUD->TakeWidget();
	HUD->RefreshFromSnapshotForTest(BattleSnapshot);

	HUD->HandleFirstPersonCardHoveredForTest(Snap.InstanceId, SlotView);
	TestFalse(TEXT("Initial hover waits for delay"), HUD->IsCardDetailPanelVisible());
	HUD->TickCardDetailMotionForTest(0.05f);
	TestFalse(TEXT("Detail is still hidden before delay finishes"), HUD->IsCardDetailPanelVisible());
	HUD->HandleFirstPersonCardUnhoveredForTest(Snap.InstanceId, SlotView);
	HUD->TickCardDetailMotionForTest(0.20f);
	TestFalse(TEXT("Hover leave before delay cancels detail"), HUD->IsCardDetailPanelVisible());

	HUD->SetCardDetailReadabilityPolishForTest(false);
	HUD->HandleFirstPersonCardHoveredForTest(Snap.InstanceId, SlotView);
	TestTrue(TEXT("Motion disabled shows immediately"), HUD->IsCardDetailPanelVisible());
	HUD->HandleFirstPersonCardUnhoveredForTest(Snap.InstanceId, SlotView);
	TestFalse(TEXT("Motion disabled hides immediately"), HUD->IsCardDetailPanelVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDFirstPersonInspectDetailUnhoverGuardSpec,
	"Wacom.UI.Battle.BattleHUD.CardDetail.FirstPersonInspectUnhoverGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDFirstPersonInspectDetailUnhoverGuardSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("FirstPersonInspectDetailCard");
	Card->DisplayName = FText::FromString(TEXT("读牌详情卡"));

	FHandCardSnapshot Snap;
	Snap.InstanceId = FGuid::NewGuid();
	Snap.Definition = Card.Get();
	Snap.RuntimeCost = 1;
	Snap.bIsPlayable = true;

	FBattleSnapshot BattleSnapshot;
	BattleSnapshot.Phase = EBattlePhase::PlayerAction;
	BattleSnapshot.Hand.Cards.Add(Snap);
	BattleSnapshot.Hand.NormalCardCount = 1;

	HUD->TakeWidget();
	HUD->RefreshFromSnapshotForTest(BattleSnapshot);

	FWacomFirstPersonCardLayerSlotView HoverSlot;
	HoverSlot.Entry.CardInstanceId = Snap.InstanceId;
	HoverSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	HoverSlot.RenderScale = 1.0f;
	HoverSlot.RenderOpacity = 1.0f;
	HoverSlot.bProjected = true;
	HUD->HandleFirstPersonCardHoveredForTest(Snap.InstanceId, HoverSlot);
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("First-person hover detail is visible"), HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	FWacomFirstPersonCardLayerSlotView InspectSlot = HoverSlot;
	InspectSlot.ScreenPosition = FVector2D(960.0f, 496.0f);
	InspectSlot.RenderScale = 1.18f;
	InspectSlot.GestureState = EWacomFirstPersonCardGestureState::Inspecting;
	HUD->HandleFirstPersonCardLayoutUpdatedForTest(Snap.InstanceId, InspectSlot);
	HUD->TickCardDetailMotionForTest(0.02f);

	HUD->HandleFirstPersonCardUnhoveredForTest(Snap.InstanceId, HoverSlot);
	HUD->TickCardDetailMotionForTest(0.5f);
	TestTrue(TEXT("Inspect detail survives same-card hover loss"), HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestEqual(TEXT("Inspect detail keeps card data"),
		HUD->GetFirstPersonCardDetailPanelNameTextForTest().ToString(),
		TEXT("读牌详情卡"));

	return true;
}
