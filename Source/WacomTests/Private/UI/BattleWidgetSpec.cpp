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
#include "UI/Battle/BattlePresentationStackEntryWidget.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Battle/WacomBattleEnemyPartPredictionWidget.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Common/PileCountView.h"
#include "BattleHUDTestHarness.h"
#include "UI/BattleSceneTargetClickTestAccess.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "Events/BattleEvent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ChildActorComponent.h"
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
	UWacomBattleCommandBarTestProbe* CommandBar = Harness->AttachCommandBar();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("CommandBar"), CommandBar))
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
	TestFalse(TEXT("Command bar wait is disabled while pending through coordinator"),
		CommandBar->IsWaitCommandEnabledForTest());
	TestFalse(TEXT("Command bar end turn is disabled while pending through coordinator"),
		CommandBar->IsEndTurnCommandEnabledForTest());
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
	Harness->AttachCommandBar();
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
