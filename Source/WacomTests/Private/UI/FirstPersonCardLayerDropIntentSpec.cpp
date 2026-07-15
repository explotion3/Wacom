// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Cards/CardDefinition.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "TimerManager.h"
#include "UI/BattleSceneTargetClickTestAccess.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomFirstPersonCardLayerSourceIds.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/FirstPersonCardLayerSpecReceiver.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

namespace WacomFirstPersonCardLayerDropIntentSpec
{
	static UWorld* FindAutomationWorld()
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

	static UWacomFirstPersonCardAnchorSpecProbeComponent* AddProbe(AWacomPlayerCharacter* Character)
	{
		UWacomFirstPersonCardAnchorSpecProbeComponent* Probe =
			NewObject<UWacomFirstPersonCardAnchorSpecProbeComponent>(Character);
		if (Probe)
		{
			Probe->RegisterComponent();
			Probe->FollowInterpSpeed = 0.0f;
		}
		return Probe;
	}

	static void PrimeBattleHUDWithCharacter(
		UWacomBattleHUDDetailTest* HUD,
		APlayerController* PC,
		AWacomPlayerCharacter* Character,
		UWorld* World)
	{
		if (PC && Character)
		{
			PC->Possess(Character);
		}
		if (HUD)
		{
			HUD->SetOwningPlayerForTest(PC);
			HUD->SetWorldForTest(World);
		}
	}

	static void SettleBattlePresentationQueue(UWacomBattleHUDDetailTest& HUD, int32 MaxSteps = 32)
	{
		for (int32 Iteration = 0; HUD.IsBattlePresentationBusy() && Iteration < MaxSteps; ++Iteration)
		{
			HUD.AdvanceBattlePresentationQueueForTest();
		}
	}

	static FWacomCardViewData BuildBattleCardViewDataForTest(const FHandCardSnapshot& CardSnapshot)
	{
		FWacomCardPresentationRuntimeContext RuntimeContext;
		RuntimeContext.bHasRuntimeCost = true;
		RuntimeContext.RuntimeCost = CardSnapshot.RuntimeCost;
		RuntimeContext.bHasPlayableState = true;
		RuntimeContext.bIsPlayable = CardSnapshot.bIsPlayable;
		return UWacomCardPresentationBuilder::BuildCardViewData(CardSnapshot.Definition, RuntimeContext);
	}

	static FGuid FindFirstHandCardByTargetMode(const FBattleSnapshot& Snapshot, ECardTargetMode TargetMode)
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

	static FGuid FindFirstHandAnchor(const FBattleSnapshot& Snapshot)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.bIsHandAnchor)
			{
				return Card.InstanceId;
			}
		}
		return FGuid();
	}

	static FWacomFirstPersonCardDragView MakeDropDragView(
		const FGuid& CardInstanceId,
		EWacomFirstPersonCardGestureState GestureState,
		bool bCommitArmed = false)
	{
		FWacomFirstPersonCardDragView DragView;
		DragView.CardInstanceId = CardInstanceId;
		DragView.GestureState = GestureState;
		DragView.bCommitArmed = bCommitArmed;
		DragView.PressScreenPosition = FVector2D(500.0f, 600.0f);
		DragView.CurrentScreenPosition = FVector2D(540.0f, 590.0f);
		DragView.PointerViewportPosition = DragView.CurrentScreenPosition;
		DragView.PointerNormalizedViewportPosition = FVector2D::ZeroVector;
		DragView.bHasPointerViewportPosition = true;
		return DragView;
	}

	static EWacomFirstPersonCardInteractionIntent BattleInteractionIntentForTargetModeProjection(
		ECardTargetMode TargetMode)
	{
		switch (TargetMode)
		{
		case ECardTargetMode::SingleEnemyPart:
			return EWacomFirstPersonCardInteractionIntent::AimWorldTarget;
		case ECardTargetMode::HandCard:
			return EWacomFirstPersonCardInteractionIntent::AimCardTarget;
		case ECardTargetMode::None:
		case ECardTargetMode::Self:
		case ECardTargetMode::AllEnemyParts:
		default:
			return EWacomFirstPersonCardInteractionIntent::DragToDropTarget;
		}
	}

	static void SetEntryInteractionIntent(
		FWacomFirstPersonCardLayerEntry& Entry,
		EWacomFirstPersonCardInteractionIntent InteractionIntent)
	{
		Entry.InteractionIntent = InteractionIntent;
	}

	static void SetEntryBattleTargetModeProjection(
		FWacomFirstPersonCardLayerEntry& Entry,
		ECardTargetMode TargetMode)
	{
		SetEntryInteractionIntent(
			Entry,
			BattleInteractionIntentForTargetModeProjection(TargetMode));
	}

	struct FSceneEnemyHostActors
	{
		AWacomBattleEnemyActor* Host = nullptr;
		TArray<AWacomBattleEnemyPartActor*> Parts;
	};

	static FSceneEnemyHostActors SpawnSceneEnemyHost(
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
			PartActor->PartId = PartIds[Index];
			PartActor->PartSlotId = PartIds[Index];
			PartActor->AttachToActor(Result.Host, FAttachmentTransformRules::KeepWorldTransform);
		}

		Result.Host->RefreshBattleEnemyPartAuthoringState();
		return Result;
	}

	static void DestroySceneEnemyHost(FSceneEnemyHostActors& Actors)
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
	FWacomFirstPersonDropIntentNoTargetArmedTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.NoTargetArmedResolvesPlayCardNoTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentNoTargetArmedTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* NoTargetCard = Fx.MakeNoopCard(0);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { NoTargetCard });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::None);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("No-target card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);

	const FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::ArmedForCommit,
		true);
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Armed no-target resolves to no-target play"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardNoTarget);
	TestTrue(TEXT("Armed no-target can submit"), Result.bCanSubmit);
	TestEqual(TEXT("No reject reason"), Result.RejectReason, EWacomBattleCardDropRejectReason::None);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentNoTargetNotArmedTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.NoTargetNotArmedRejectsWithoutCommitReady",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentNoTargetNotArmedTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::None);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("No-target card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);

	const FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::DraggingNoTargetCard,
		false);
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Unarmed no-target rejects"), Result.IntentKind, EWacomBattleCardDropIntentKind::Reject);
	TestFalse(TEXT("Unarmed no-target cannot submit"), Result.bCanSubmit);
	TestEqual(TEXT("Unarmed reject reason"), Result.RejectReason, EWacomBattleCardDropRejectReason::NotArmed);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentCardTargetProbeTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.CardTargetUnsupportedSourceRemainsProbeOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentCardTargetProbeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeSimpleDamageCard(0, 1);
	UCardDefinition* TargetCard = Fx.MakeNoopCard(0);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { SourceCard, TargetCard });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::SingleEnemyPart);
	const FGuid TargetCardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::None);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		SourceCardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(
		TargetCardId,
		HUD,
		FVector2D(650.0f, 600.0f));
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, DragView);
	TestEqual(TEXT("Card target resolves to probe only"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::ProbeCardTarget);
	TestEqual(TEXT("Card target preserves target id"), Result.TargetHandle.CardInstanceId, TargetCardId);
	TestFalse(TEXT("Card target does not submit"), Result.bCanSubmit);
	TestEqual(TEXT("Card target records unsupported reason"),
		Result.RejectReason,
		EWacomBattleCardDropRejectReason::UnsupportedCardTarget);
	TestEqual(TEXT("Card target records validation reason"),
		Result.TargetValidationRejectReason,
		EWacomBattleTargetRejectReason::UnsupportedCardTarget);
	TestTrue(TEXT("Card target debug includes validation"),
		Result.ToDebugString().Contains(TEXT("ValidationReject=UnsupportedCardTarget")));
	TestTrue(TEXT("Card target exposes feedback position"), Result.bHasFeedbackTargetScreenPosition);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentValidHandCardTargetTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.CardTargetValidHandCardResolvesPlayCardCardTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentValidHandCardTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/2, /*bReduceCost*/false);
	UCardDefinition* TargetCard = Fx.MakeNoopCard(3);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid TargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		SourceCardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(
		TargetCardId,
		HUD,
		FVector2D(650.0f, 600.0f));
	const FWacomBattleCardDropResolveResult Result =
		HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, DragView);
	TestEqual(TEXT("Valid hand-card target resolves to card play"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardCardTarget);
	TestEqual(TEXT("Card target preserves target id"), Result.TargetHandle.CardInstanceId, TargetCardId);
	TestTrue(TEXT("Card target can submit"), Result.bCanSubmit);
	TestEqual(TEXT("No reject reason"), Result.RejectReason, EWacomBattleCardDropRejectReason::None);
	TestEqual(TEXT("No validation reject reason"),
		Result.TargetValidationRejectReason,
		EWacomBattleTargetRejectReason::None);
	TestTrue(TEXT("Valid card target debug includes validation"),
		Result.ToDebugString().Contains(TEXT("ValidationReject=None")));
	TestTrue(TEXT("Card target exposes feedback position"), Result.bHasFeedbackTargetScreenPosition);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentSelectedZoneMoveCardTargetTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.SelectedZoneMoveCardTargetResolvesPlayCardCardTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentSelectedZoneMoveCardTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, /*bExhaust*/false);
	UCardDefinition* TargetCard = Fx.MakeNoopCard(3);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid TargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		SourceCardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(
		TargetCardId,
		HUD,
		FVector2D(650.0f, 600.0f));
	const FWacomBattleCardDropResolveResult Result =
		HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, DragView);
	TestEqual(TEXT("Selected zone move normal card target resolves to card play"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardCardTarget);
	TestTrue(TEXT("Selected zone move normal card target can submit"), Result.bCanSubmit);
	TestEqual(TEXT("No reject reason"), Result.RejectReason, EWacomBattleCardDropRejectReason::None);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentSelectedZoneMoveHandAnchorRejectTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.SelectedZoneMoveHandAnchorRejects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentSelectedZoneMoveHandAnchorRejectTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, /*bExhaust*/true);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid AnchorCardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandAnchor(Snapshot);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Anchor card exists"), AnchorCardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		SourceCardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(
		AnchorCardId,
		HUD,
		FVector2D(650.0f, 600.0f));
	const FWacomBattleCardDropResolveResult Result =
		HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, DragView);
	TestEqual(TEXT("Selected zone move anchor target rejects"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::Reject);
	TestFalse(TEXT("Selected zone move anchor target cannot submit"), Result.bCanSubmit);
	TestEqual(TEXT("Anchor target records unsupported reason"),
		Result.RejectReason,
		EWacomBattleCardDropRejectReason::UnsupportedCardTarget);
	TestEqual(TEXT("Anchor target records validation reason"),
		Result.TargetValidationRejectReason,
		EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentFilterRejectedCardTargetTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.FilterRejectedCardTargetShowsInvalidFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentFilterRejectedCardTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/2, /*bReduceCost*/false);
	SourceCard->HandCardTargetFilter.bUseExplicitHandCardTargetFilter = true;
	SourceCard->HandCardTargetFilter.bAllowNormalHandCards = false;
	SourceCard->HandCardTargetFilter.bAllowHandAnchors = true;
	UCardDefinition* TargetCard = Fx.MakeNoopCard(3);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid TargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		PC->Destroy();
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		SourceCardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(
		TargetCardId,
		HUD,
		FVector2D(650.0f, 600.0f));
	const FWacomBattleCardDropResolveResult Result =
		HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, DragView);
	TestEqual(TEXT("Filter-rejected card target rejects"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::Reject);
	TestFalse(TEXT("Filter-rejected card target cannot submit"), Result.bCanSubmit);
	TestEqual(TEXT("Filter-rejected card target maps to unsupported card target"),
		Result.RejectReason,
		EWacomBattleCardDropRejectReason::UnsupportedCardTarget);
	TestEqual(TEXT("Filter rejection carries validation reason"),
		Result.TargetValidationRejectReason,
		EWacomBattleTargetRejectReason::UnsupportedNormalHandCardTarget);
	TestTrue(TEXT("Debug includes filter validation reason"),
		Result.ToDebugString().Contains(TEXT("ValidationReject=UnsupportedNormalHandCardTarget")));

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentKeywordRejectDebugTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.DropResolverDebugIncludesKeywordRejectReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentKeywordRejectDebugTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FGameplayTagContainer RequiredKeywords;
	RequiredKeywords.AddTag(WacomTags::Card_Keyword_Companion);
	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeHandCardCostModifierCardWithTargetKeywordFilter(
		/*Cost*/0,
		/*Magnitude*/2,
		/*bReduceCost*/false,
		RequiredKeywords,
		FGameplayTagContainer());
	UCardDefinition* TargetCard = Fx.MakeNoopCard(3);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid TargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		PC->Destroy();
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		SourceCardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(
		TargetCardId,
		HUD,
		FVector2D(650.0f, 600.0f));
	const FWacomBattleCardDropResolveResult Result =
		HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, DragView);

	TestEqual(TEXT("Keyword-rejected card target rejects"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("Keyword rejection maps to unsupported card target"),
		Result.RejectReason,
		EWacomBattleCardDropRejectReason::UnsupportedCardTarget);
	TestEqual(TEXT("Keyword rejection carries validation reason"),
		Result.TargetValidationRejectReason,
		EWacomBattleTargetRejectReason::MissingRequiredTargetKeyword);
	TestTrue(TEXT("Debug includes keyword validation reason"),
		Result.ToDebugString().Contains(TEXT("ValidationReject=MissingRequiredTargetKeyword")));

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonLayerDraggingHandCardBuildsAffordanceTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.DraggingHandCardSourceBuildsFullHandCardAffordance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonLayerDraggingHandCardBuildsAffordanceTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, /*bExhaust*/true);
	UCardDefinition* NormalTargetCard = Fx.MakeNoopCard(3);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, NormalTargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid NormalTargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, NormalTargetCard->CardId);
	const FGuid AnchorCardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandAnchor(Snapshot);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
			AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Normal target exists"), NormalTargetCardId.IsValid())
		|| !TestTrue(TEXT("Anchor target exists"), AnchorCardId.IsValid()))
	{
		return false;
	}

	WacomFirstPersonCardLayerDropIntentSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);

	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerDropIntentSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::SetFirstPersonCardLayerInteractionEnabled(*Anchor, true);
	TArray<FWacomFirstPersonCardLayerEntry> CardEntries;
	for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = CardSnapshot.InstanceId;
		Entry.CardViewData = WacomFirstPersonCardLayerDropIntentSpec::BuildBattleCardViewDataForTest(CardSnapshot);
		Entry.bIsPlayable = CardSnapshot.bIsPlayable;
		WacomFirstPersonCardLayerDropIntentSpec::SetEntryBattleTargetModeProjection(
			Entry,
			CardSnapshot.Definition
				? CardSnapshot.Definition->TargetMode
				: ECardTargetMode::None);
		Entry.Zone = CardSnapshot.Zone;
		Entry.bIsHandAnchor = CardSnapshot.bIsHandAnchor;
		CardEntries.Add(MoveTemp(Entry));
	}
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), CardEntries);
	HUD->SetFirstPersonCardAnchorForTest(Anchor);
	Anchor->RefreshAnchor(0.0f);
	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());
	UWacomFirstPersonCardLayerWidget* Layer = FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
	if (!TestNotNull(TEXT("First-person layer"), Layer))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = nullptr;
	UWacomFirstPersonCardLayerSlotWidget* NormalTargetWidget = nullptr;
	UWacomFirstPersonCardLayerSlotWidget* AnchorTargetWidget = nullptr;
	for (int32 Index = 0; Index < Layer->GetCardViewCount(); ++Index)
	{
		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(Index);
		if (!SlotWidget)
		{
			continue;
		}
		const FGuid SlotCardId = SlotWidget->GetSlotView().Entry.CardInstanceId;
		if (SlotCardId == SourceCardId)
		{
			SourceWidget = SlotWidget;
		}
		else if (SlotCardId == NormalTargetCardId)
		{
			NormalTargetWidget = SlotWidget;
		}
		else if (SlotCardId == AnchorCardId)
		{
			AnchorTargetWidget = SlotWidget;
		}
	}
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Normal target slot"), NormalTargetWidget)
		|| !TestNotNull(TEXT("Anchor target slot"), AnchorTargetWidget))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	const FVector2D SourcePosition = SourceWidget->GetVisualSlotView().ScreenPosition;
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourcePosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, SourcePosition + FVector2D(80.0f, -20.0f));
	TestEqual(TEXT("Aiming drag records selected-source gesture state"),
		SourceWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	Anchor->OnFirstPersonCardLayerDragUpdated.Broadcast(SourceCardId, FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView);

	TestEqual(TEXT("Normal hand card shows valid affordance"),
		FWacomFirstPersonCardLayerTestAccess::View(*NormalTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestEqual(TEXT("Normal hand card records affordance state"),
		FWacomFirstPersonCardLayerTestAccess::View(*NormalTargetWidget).CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestFalse(TEXT("Normal hand card affordance is not pointer focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*NormalTargetWidget).bCardDragTargetFocusActive);
	TestEqual(TEXT("Hand anchor shows invalid affordance for selected zone move"),
		FWacomFirstPersonCardLayerTestAccess::View(*AnchorTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget);
	TestEqual(TEXT("Hand anchor records affordance state"),
		FWacomFirstPersonCardLayerTestAccess::View(*AnchorTargetWidget).CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget);
	TestFalse(TEXT("Hand anchor affordance is not pointer focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*AnchorTargetWidget).bCardDragTargetFocusActive);
	TestTrue(TEXT("Debug reports affordance counts"), Layer->GetDragTargetDebugSummary().Contains(TEXT("AffordanceValid=")));

	Layer->CancelCardDragGesture(true);
	TestEqual(TEXT("Normal affordance clears on cancel"),
		FWacomFirstPersonCardLayerTestAccess::View(*NormalTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::None);

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonLayerWorldTargetSourceDoesNotBuildHandCardAffordanceTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.WorldTargetSourceDoesNotBuildFullHandCardAffordance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonLayerWorldTargetSourceDoesNotBuildHandCardAffordanceTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/1);
	UCardDefinition* NormalTargetCard = Fx.MakeNoopCard(0);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, NormalTargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId =
		WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::SingleEnemyPart);
	const FGuid NormalTargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, NormalTargetCard->CardId);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
			AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Normal target exists"), NormalTargetCardId.IsValid()))
	{
		if (Character)
		{
			Character->Destroy();
		}
		if (PC)
		{
			PC->Destroy();
		}
		return false;
	}

	WacomFirstPersonCardLayerDropIntentSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);

	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerDropIntentSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::SetFirstPersonCardLayerInteractionEnabled(*Anchor, true);
	TArray<FWacomFirstPersonCardLayerEntry> CardEntries;
	for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = CardSnapshot.InstanceId;
		Entry.CardViewData = WacomFirstPersonCardLayerDropIntentSpec::BuildBattleCardViewDataForTest(CardSnapshot);
		Entry.bIsPlayable = CardSnapshot.bIsPlayable;
		WacomFirstPersonCardLayerDropIntentSpec::SetEntryBattleTargetModeProjection(
			Entry,
			CardSnapshot.Definition
				? CardSnapshot.Definition->TargetMode
				: ECardTargetMode::None);
		Entry.Zone = CardSnapshot.Zone;
		Entry.bIsHandAnchor = CardSnapshot.bIsHandAnchor;
		CardEntries.Add(MoveTemp(Entry));
	}
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), CardEntries);
	HUD->SetFirstPersonCardAnchorForTest(Anchor);
	Anchor->RefreshAnchor(0.0f);
	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());
	UWacomFirstPersonCardLayerWidget* Layer = FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
	if (!TestNotNull(TEXT("First-person layer"), Layer))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = nullptr;
	UWacomFirstPersonCardLayerSlotWidget* NormalTargetWidget = nullptr;
	for (int32 Index = 0; Index < Layer->GetCardViewCount(); ++Index)
	{
		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(Index);
		if (!SlotWidget)
		{
			continue;
		}
		const FGuid SlotCardId = SlotWidget->GetSlotView().Entry.CardInstanceId;
		if (SlotCardId == SourceCardId)
		{
			SourceWidget = SlotWidget;
		}
		else if (SlotCardId == NormalTargetCardId)
		{
			NormalTargetWidget = SlotWidget;
		}
	}
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Normal target slot"), NormalTargetWidget))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	const FVector2D SourcePosition = SourceWidget->GetVisualSlotView().ScreenPosition;
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourcePosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, SourcePosition + FVector2D(80.0f, -20.0f));
	TestEqual(TEXT("World-target card keeps aiming gesture"),
		SourceWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	Anchor->OnFirstPersonCardLayerDragUpdated.Broadcast(SourceCardId, FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView);

	const FWacomFirstPersonCardSlotAutomationTestView TargetView =
		FWacomFirstPersonCardLayerTestAccess::View(*NormalTargetWidget);
	TestFalse(TEXT("World-target source does not build hand-card affordance"), TargetView.bCardDragTargetAffordanceFeedback);
	TestEqual(TEXT("World-target source leaves hand-card affordance state empty"),
		TargetView.CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::None);
	TestTrue(TEXT("Debug reports no valid affordances"),
		Layer->GetDragTargetDebugSummary().Contains(TEXT("AffordanceValid=0")));
	TestTrue(TEXT("Debug reports no invalid affordances"),
		Layer->GetDragTargetDebugSummary().Contains(TEXT("AffordanceInvalid=0")));

	Layer->CancelCardDragGesture(true);
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonLayerKeywordFilterAffordanceTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.FullHandAffordanceUsesKeywordFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonLayerKeywordFilterAffordanceTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FGameplayTagContainer RequiredKeywords;
	RequiredKeywords.AddTag(WacomTags::Card_Keyword_Companion);
	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeHandCardCostModifierCardWithTargetKeywordFilter(
		/*Cost*/0,
		/*Magnitude*/1,
		/*bReduceCost*/false,
		RequiredKeywords,
		FGameplayTagContainer());
	UCardDefinition* CompanionTargetCard = Fx.MakeDamageCardWithKeywords(
		/*Cost*/3,
		/*Damage*/1,
		{ WacomTags::Card_Keyword_Companion });
	UCardDefinition* PlainTargetCard = Fx.MakeNoopCard(3);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, CompanionTargetCard, PlainTargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid CompanionTargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, CompanionTargetCard->CardId);
	const FGuid PlainTargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PlainTargetCard->CardId);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
			AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Companion target exists"), CompanionTargetCardId.IsValid())
		|| !TestTrue(TEXT("Plain target exists"), PlainTargetCardId.IsValid()))
	{
		return false;
	}

	WacomFirstPersonCardLayerDropIntentSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);

	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerDropIntentSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::SetFirstPersonCardLayerInteractionEnabled(*Anchor, true);
	TArray<FWacomFirstPersonCardLayerEntry> CardEntries;
	for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = CardSnapshot.InstanceId;
		Entry.CardViewData = WacomFirstPersonCardLayerDropIntentSpec::BuildBattleCardViewDataForTest(CardSnapshot);
		Entry.bIsPlayable = CardSnapshot.bIsPlayable;
		WacomFirstPersonCardLayerDropIntentSpec::SetEntryBattleTargetModeProjection(
			Entry,
			CardSnapshot.Definition
				? CardSnapshot.Definition->TargetMode
				: ECardTargetMode::None);
		Entry.Zone = CardSnapshot.Zone;
		Entry.bIsHandAnchor = CardSnapshot.bIsHandAnchor;
		CardEntries.Add(MoveTemp(Entry));
	}
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), CardEntries);
	HUD->SetFirstPersonCardAnchorForTest(Anchor);
	Anchor->RefreshAnchor(0.0f);
	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());
	UWacomFirstPersonCardLayerWidget* Layer = FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
	if (!TestNotNull(TEXT("First-person layer"), Layer))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = nullptr;
	UWacomFirstPersonCardLayerSlotWidget* CompanionTargetWidget = nullptr;
	UWacomFirstPersonCardLayerSlotWidget* PlainTargetWidget = nullptr;
	for (int32 Index = 0; Index < Layer->GetCardViewCount(); ++Index)
	{
		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(Index);
		if (!SlotWidget)
		{
			continue;
		}
		const FGuid SlotCardId = SlotWidget->GetSlotView().Entry.CardInstanceId;
		if (SlotCardId == SourceCardId)
		{
			SourceWidget = SlotWidget;
		}
		else if (SlotCardId == CompanionTargetCardId)
		{
			CompanionTargetWidget = SlotWidget;
		}
		else if (SlotCardId == PlainTargetCardId)
		{
			PlainTargetWidget = SlotWidget;
		}
	}
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Companion target slot"), CompanionTargetWidget)
		|| !TestNotNull(TEXT("Plain target slot"), PlainTargetWidget))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	const FVector2D SourcePosition = SourceWidget->GetVisualSlotView().ScreenPosition;
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourcePosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, SourcePosition + FVector2D(80.0f, -20.0f));
	Anchor->OnFirstPersonCardLayerDragUpdated.Broadcast(SourceCardId, FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView);

	TestEqual(TEXT("Keyword-allowed card shows valid card feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*CompanionTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestEqual(TEXT("Keyword-rejected card shows invalid card feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*PlainTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget);

	Layer->CancelCardDragGesture(true);
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentWrongKindCardTargetProbeTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.WrongKindCardTargetRemainsProbe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentWrongKindCardTargetProbeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeSimpleDamageCard(0, 1) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::SingleEnemyPart);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(CardId, HUD, FVector2D(500.0f, 600.0f));
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(
		TEXT("Wrong card target kind stays a probe"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::ProbeCardTarget);
	TestEqual(
		TEXT("Wrong target kind wins over self identity"),
		Result.RejectReason,
		EWacomBattleCardDropRejectReason::UnsupportedCardTarget);
	TestEqual(
		TEXT("Target validation preserves the canonical wrong-kind reason"),
		Result.TargetValidationRejectReason,
		EWacomBattleTargetRejectReason::UnsupportedCardTarget);
	TestFalse(TEXT("Wrong-kind card target cannot submit"), Result.bCanSubmit);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentHandCardSelfTargetRejectTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.HandCardSelfTargetRejects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentHandCardSelfTargetRejectTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* HandTargetCard = Fx.MakeHandCardCostModifierCard(
		/*Cost*/0,
		/*Magnitude*/1,
		/*bReduceCost*/false);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { HandTargetCard });
	UBattleSession* Session = Fx.CreateSession(
		CharacterDefinition,
		Fx.MakeSinglePartEnemy(20, 0, 0),
		1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::HandCard);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
			AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("HandCard source exists"), CardId.IsValid()))
	{
		if (IsValid(PC))
		{
			PC->Destroy();
		}
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(
		CardId,
		HUD,
		FVector2D(500.0f, 600.0f));
	const FWacomBattleCardDropResolveResult Result =
		HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(
		TEXT("Same-kind self target rejects"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::Reject);
	TestEqual(
		TEXT("HandCard self target keeps SelfTarget reason"),
		Result.RejectReason,
		EWacomBattleCardDropRejectReason::SelfTarget);
	TestEqual(
		TEXT("Target validation reports SelfTarget"),
		Result.TargetValidationRejectReason,
		EWacomBattleTargetRejectReason::SelfTarget);
	TestFalse(TEXT("HandCard self target cannot submit"), Result.bCanSubmit);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentZoneRejectTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.ZoneTargetRejectsAsUnsupported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentZoneRejectTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeSimpleDamageCard(0, 1) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::SingleEnemyPart);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForZoneTarget(TEXT("TestZone"), HUD, FVector2D(700.0f, 500.0f));
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Zone target rejects"), Result.IntentKind, EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("Zone target reject reason"),
		Result.RejectReason,
		EWacomBattleCardDropRejectReason::UnsupportedZoneTarget);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentUIBlockedTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.PhaseBlockedOrMissingSessionRejects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentUIBlockedTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::None);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	const FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::ArmedForCommit,
		true);
	FWacomBattleCardDropResolveResult MissingSession = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Missing session rejects"), MissingSession.IntentKind, EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("Missing session reason"), MissingSession.RejectReason, EWacomBattleCardDropRejectReason::MissingSession);

	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);
	HUD->SetUIStateForTest(EBattleUIState::BattleEnd);
	FWacomBattleCardDropResolveResult UIBlocked = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("BattleEnd UI rejects"), UIBlocked.IntentKind, EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("BattleEnd reject reason"), UIBlocked.RejectReason, EWacomBattleCardDropRejectReason::UIBlocked);

	HUD->SetUIStateForTest(EBattleUIState::Idle);
	FBattleEvent Event;
	Event.Type = EBattleEventType::DamageDealt;
	Event.Sequence = 1;
	Event.ActorEnemyPartKey = FWacomBattleFixture::FindPartKey(Snapshot, 0);
	Event.Amount = 1;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });
	World->GetTimerManager().Tick(0.01f);
	TestTrue(TEXT("Presentation queue is busy"), HUD->IsBattlePresentationBusy());
	FWacomBattleCardDropResolveResult PresentationBusy = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Presentation busy no longer blocks drop intent"),
		PresentationBusy.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardNoTarget);
	TestTrue(TEXT("Presentation busy drop can submit"), PresentationBusy.bCanSubmit);

	HUD->HandleFirstPersonCardDragReleasedForTest(CardId, DragView);
	TestTrue(TEXT("PlayCard creates presentation stack"), HUD->GetPresentationStackEntryCountForTest() > 0);
	HUD->OnWaitRequested();
	TestTrue(TEXT("Wait while stack pending creates turn-boundary barrier"), HUD->HasPendingTurnBoundaryCommandForTest());
	FWacomBattleCardDropResolveResult PendingBarrier = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Pending turn boundary blocks first-person drop"),
		PendingBarrier.IntentKind,
		EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("Pending turn boundary reject reason"),
		PendingBarrier.RejectReason,
		EWacomBattleCardDropRejectReason::UIBlocked);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentWorldTargetTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.TargetedCardValidWorldResolvesPlayCardWorldTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentWorldTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 0, 0);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeSimpleDamageCard(0, 1) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::SingleEnemyPart);
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);

	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	WacomFirstPersonCardLayerDropIntentSpec::FSceneEnemyHostActors SceneEnemy =
		WacomFirstPersonCardLayerDropIntentSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Solo") });
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestTrue(TEXT("Scene enemy part exists"), SceneEnemy.Parts.Num() > 0)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid())
		|| !TestTrue(TEXT("Part exists"), PartId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomFirstPersonCardLayerDropIntentSpec::DestroySceneEnemyHost(SceneEnemy);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, SceneEnemy.Parts[0], SceneEnemy.Parts[0]->GetHitBounds());

	const FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Valid world target resolves to world play"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardWorldTarget);
	TestTrue(TEXT("Valid world target can submit"), Result.bCanSubmit);
	TestEqual(TEXT("World target id preserved"), Result.TargetHandle.WorldTargetId, PartId);
	TestTrue(TEXT("World target exposes feedback position"), Result.bHasFeedbackTargetScreenPosition);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentInvalidWorldTargetTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.TargetedCardInvalidWorldRejectsWithoutSubmit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentInvalidWorldTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 0, 0);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeSimpleDamageCard(0, 1) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::SingleEnemyPart);
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);

	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	WacomFirstPersonCardLayerDropIntentSpec::FSceneEnemyHostActors CurrentHost =
		WacomFirstPersonCardLayerDropIntentSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Solo") });
	WacomFirstPersonCardLayerDropIntentSpec::FSceneEnemyHostActors OtherHost =
		WacomFirstPersonCardLayerDropIntentSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Solo") });
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Current host"), CurrentHost.Host)
		|| !TestTrue(TEXT("Current host part exists"), CurrentHost.Parts.Num() > 0)
		|| !TestNotNull(TEXT("Other host"), OtherHost.Host)
		|| !TestTrue(TEXT("Other host part exists"), OtherHost.Parts.Num() > 0)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid())
		|| !TestTrue(TEXT("Part exists"), PartId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomFirstPersonCardLayerDropIntentSpec::DestroySceneEnemyHost(OtherHost);
		WacomFirstPersonCardLayerDropIntentSpec::DestroySceneEnemyHost(CurrentHost);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ CurrentHost.Host });
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD);
	OtherHost.Parts[0]->GetInteractionTargetComponent()->SetTargetId(PartId);
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, OtherHost.Parts[0], OtherHost.Parts[0]->GetHitBounds());

	const FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Invalid world target rejects"), Result.IntentKind, EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("Invalid world reason"), Result.RejectReason, EWacomBattleCardDropRejectReason::InvalidWorldTarget);
	TestFalse(TEXT("Invalid world target cannot submit"), Result.bCanSubmit);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentPreviewReleaseConsistencyTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.PreviewAndReleaseUseSameResolvedIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentPreviewReleaseConsistencyTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::None);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);
	const int32 VersionBeforeRelease = Session->BuildSnapshot().Version;

	const FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::ArmedForCommit,
		true);
	const FWacomBattleCardDropResolveResult PreviewResult = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	HUD->HandleFirstPersonCardDragUpdatedForTest(CardId, DragView);
	HUD->HandleFirstPersonCardDragReleasedForTest(CardId, DragView);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);

	TestEqual(TEXT("Preview used submit intent"),
		PreviewResult.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardNoTarget);
	TestTrue(TEXT("Release submitted same intent path"),
		Session->BuildSnapshot().Version > VersionBeforeRelease);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentReleaseOnCardTargetSubmitTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.ReleaseOnValidCardTargetSubmitsHandCardCommand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentReleaseOnCardTargetSubmitTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/2, /*bReduceCost*/false);
	UCardDefinition* TargetCard = Fx.MakeNoopCard(3);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid TargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);
	const int32 VersionBeforeRelease = Session->BuildSnapshot().Version;

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerDropIntentSpec::MakeDropDragView(
		SourceCardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(
		TargetCardId,
		HUD,
		FVector2D(650.0f, 600.0f));
	const FWacomBattleCardDropResolveResult PreviewResult =
		HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, DragView);
	HUD->HandleFirstPersonCardDragReleasedForTest(SourceCardId, DragView);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);
	Snapshot = Session->BuildSnapshot();

	TestEqual(TEXT("Preview used card target submit intent"),
		PreviewResult.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardCardTarget);
	TestTrue(TEXT("Release submitted card target path"), Snapshot.Version > VersionBeforeRelease);
	int32 TargetRuntimeCost = INDEX_NONE;
	for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
	{
		if (Card.InstanceId == TargetCardId)
		{
			TargetRuntimeCost = Card.RuntimeCost;
			break;
		}
	}
	TestEqual(TEXT("Target card cost updated"), TargetRuntimeCost, 5);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentLayerGestureCardTargetSubmitTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.LayerGestureReleaseOnCardTargetSubmitsHandCardCommand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentLayerGestureCardTargetSubmitTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerDropIntentSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/2, /*bReduceCost*/false);
	UCardDefinition* TargetCard = Fx.MakeNoopCard(3);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerDropIntentSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid TargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
			AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		return false;
	}

	WacomFirstPersonCardLayerDropIntentSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	UWacomFirstPersonCardAnchorComponent* DefaultAnchor = Character->GetFirstPersonCardAnchorComponent();
	UWacomFirstPersonCardLayerWidget* RuntimeLayer = NewObject<UWacomFirstPersonCardLayerWidget>(HUD);
	if (!TestNotNull(TEXT("Default first-person anchor"), DefaultAnchor)
		|| !TestNotNull(TEXT("Runtime first-person layer"), RuntimeLayer))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}
	RuntimeLayer->TakeWidget();
	RuntimeLayer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetCardLayer(*DefaultAnchor, RuntimeLayer);
	HUD->TakeWidget();
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);

	TArray<FWacomFirstPersonCardLayerEntry> CardEntries;
	CardEntries.Reserve(Snapshot.Hand.Cards.Num());
	for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = CardSnapshot.InstanceId;
		Entry.CardViewData = WacomFirstPersonCardLayerDropIntentSpec::BuildBattleCardViewDataForTest(CardSnapshot);
		Entry.bIsPlayable = CardSnapshot.bIsPlayable;
		WacomFirstPersonCardLayerDropIntentSpec::SetEntryBattleTargetModeProjection(
			Entry,
			CardSnapshot.Definition
				? CardSnapshot.Definition->TargetMode
				: ECardTargetMode::None);
		Entry.Zone = CardSnapshot.Zone;
		Entry.bIsHandAnchor = CardSnapshot.bIsHandAnchor;
		CardEntries.Add(MoveTemp(Entry));
	}

	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor =
		WacomFirstPersonCardLayerDropIntentSpec::AddProbe(Character);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(HUD);
	if (!TestNotNull(TEXT("Anchor probe"), Anchor)
		|| !TestNotNull(TEXT("Gesture first-person layer"), Layer))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}
	Layer->TakeWidget();
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetCardLayer(*Anchor, Layer);
	FWacomFirstPersonCardLayerTestAccess::SetFirstPersonCardLayerInteractionEnabled(*Anchor, true);
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), CardEntries);
	HUD->SetFirstPersonCardAnchorForTest(Anchor);
	Anchor->RefreshAnchor(0.0f);
	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());
	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = nullptr;
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = nullptr;
	for (int32 Index = 0; Index < Snapshot.Hand.Cards.Num(); ++Index)
	{
		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(Index);
		if (!SlotWidget)
		{
			continue;
		}
		if (SlotWidget->GetSlotView().Entry.CardInstanceId == SourceCardId)
		{
			SourceWidget = SlotWidget;
		}
		else if (SlotWidget->GetSlotView().Entry.CardInstanceId == TargetCardId)
		{
			TargetWidget = SlotWidget;
		}
	}
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	const FVector2D SourcePosition = SourceWidget->GetVisualSlotView().ScreenPosition;
	const FVector2D TargetPosition = TargetWidget->GetVisualSlotView().ScreenPosition;
	const int32 VersionBeforeRelease = Session->BuildSnapshot().Version;

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourcePosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, TargetPosition);
	TestEqual(TEXT("Layer gesture resolves pointer card target"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.CardInstanceId,
		TargetCardId);
	Anchor->OnFirstPersonCardLayerDragUpdated.Broadcast(SourceCardId, FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView);
	TestEqual(TEXT("HUD marks card target as valid before release"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, TargetPosition);
	WacomFirstPersonCardLayerDropIntentSpec::SettleBattlePresentationQueue(*HUD);
	Snapshot = Session->BuildSnapshot();

	TestTrue(TEXT("Layer gesture release submitted card target path"), Snapshot.Version > VersionBeforeRelease);
	int32 TargetRuntimeCost = INDEX_NONE;
	for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
	{
		if (Card.InstanceId == TargetCardId)
		{
			TargetRuntimeCost = Card.RuntimeCost;
			break;
		}
	}
	TestEqual(TEXT("Target card cost updated through layer gesture"), TargetRuntimeCost, 5);
	TestTrue(TEXT("Source release used confirm feedback"), FWacomFirstPersonCardLayerTestAccess::View(*SourceWidget).bConfirmFeedbackActive);
	TestFalse(TEXT("Source release did not play deny feedback"), FWacomFirstPersonCardLayerTestAccess::View(*SourceWidget).bDenyFeedbackActive);

	Character->Destroy();
	PC->Destroy();
	return true;
}
