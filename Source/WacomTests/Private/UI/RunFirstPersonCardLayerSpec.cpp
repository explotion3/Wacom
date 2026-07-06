// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "RunSession.h"
#include "RunState.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomFirstPersonCardLayerSourceIds.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "UI/PlayerControllerRunInteractionTestAccess.h"
#include "UI/RunMenuDropTargetWidgetTestAccess.h"
#include "UI/RunFirstPersonCardLayerSpecReceiver.h"
#include "UI/WacomShopRunEventTestProbes.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomRunFirstPersonCardLayerSpec
{
	UCardDefinition* MakeTypeAContainerCard(FWacomBattleFixture& Fx, int32 Capacity)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->CardId = TEXT("Test.TypeAContainer");
		Card->DisplayName = FText::FromString(TEXT("TypeA Container"));
		Card->Physique.Capacity = Capacity;
		return Card;
	}

	UCardDefinition* MakeTypeBContainerCard(FWacomBattleFixture& Fx, int32 Capacity)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->CardId = TEXT("Test.TypeBContainer");
		Card->DisplayName = FText::FromString(TEXT("TypeB Container"));
		Card->Physique.Capacity = Capacity;
		Card->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;
		return Card;
	}

	UCardDefinition* MakeNamedNoopCard(
		FWacomBattleFixture& Fx,
		FName CardId,
		const FString& DisplayName,
		int32 Cost)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(Cost);
		Card->CardId = CardId;
		Card->DisplayName = FText::FromString(DisplayName);
		return Card;
	}

	UCharacterDefinition* MakePaymentTestCharacter(
		FWacomBattleFixture& Fx,
		UCardDefinition* PaidCard)
	{
		UCardDefinition* Pack = MakeTypeAContainerCard(Fx, 2);
		return Fx.MakeCharacter(nullptr, nullptr, { PaidCard, Pack });
	}

	void AttachFirstPersonPawnForTest(AWacomPlayerControllerProbe* PC)
	{
		if (PC)
		{
			PC->SetPawn(NewObject<AWacomPlayerCharacter>(PC));
		}
	}

	UWacomFirstPersonCardAnchorComponent* GetFirstPersonAnchorForTest(
		AWacomPlayerControllerProbe* PC)
	{
		const AWacomPlayerCharacter* Character =
			PC ? Cast<AWacomPlayerCharacter>(PC->GetPawn()) : nullptr;
		return Character ? Character->GetFirstPersonCardAnchorComponent() : nullptr;
	}

	FWacomFirstPersonCardLayerSlotView MakeProjectedSlotForRunDetail(
		const FGuid& CardInstanceId,
		const FVector2D& ScreenPosition)
	{
		FWacomFirstPersonCardLayerSlotView SlotView;
		SlotView.Entry.CardInstanceId = CardInstanceId;
		SlotView.ScreenPosition = ScreenPosition;
		SlotView.RenderScale = 1.0f;
		SlotView.bProjected = true;
		return SlotView;
	}

	FWacomFirstPersonCardDragView MakeRunDetailDragView(
		const FGuid& CardInstanceId,
		EWacomFirstPersonCardGestureState GestureState,
		const FWacomFirstPersonCardLayerSlotView& SourceSlotView)
	{
		FWacomFirstPersonCardDragView DragView;
		DragView.CardInstanceId = CardInstanceId;
		DragView.GestureState = GestureState;
		DragView.SourceSlotView = SourceSlotView;
		return DragView;
	}

	FWacomRunMenuCardLeaseRequest MakeLeaseRequest(FName LeaseId = TEXT("ProviderLease"))
	{
		FWacomRunMenuCardLeaseRequest Request;
		Request.LeaseId = LeaseId;
		Request.SourceId = FName(*FString::Printf(TEXT("%sSource"), *LeaseId.ToString()));
		return Request;
	}

	bool SetDefinitionLease(
		UWacomRunFirstPersonCardSourceComponent& Source,
		FName LeaseId,
		UCardDefinition* Definition,
		FWacomRunMenuCardLeaseResult& OutResult)
	{
		FWacomRunMenuCardLeaseRequest Request = MakeLeaseRequest(LeaseId);
		Request.AllowedCardDefinitions.Add(Definition);
		return Source.SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, OutResult);
	}

	bool SetDefinitionLease(
		AWacomPlayerControllerProbe& PC,
		FName LeaseId,
		UCardDefinition* Definition,
		FWacomRunMenuCardLeaseResult& OutResult)
	{
		FWacomRunMenuCardLeaseRequest Request = MakeLeaseRequest(LeaseId);
		Request.AllowedCardDefinitions.Add(Definition);
		return PC.SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, OutResult);
	}

	FCardInstance MakeRunCardInstance(UCardDefinition* Definition)
	{
		FCardInstance Instance;
		Instance.InstanceId = FGuid::NewGuid();
		Instance.Definition = Definition;
		return Instance;
	}

	FGuid FindOwnedInstanceIdByDefinition(const FRunState& State, const UCardDefinition* Definition)
	{
		if (!Definition)
		{
			return FGuid();
		}

		auto FindIn = [Definition](const TArray<FCardInstance>& Instances)
		{
			for (const FCardInstance& Instance : Instances)
			{
				if (Instance.Definition == Definition)
				{
					return Instance.InstanceId;
				}
			}
			return FGuid();
		};

		if (const FGuid FoundId = FindIn(State.Backpack); FoundId.IsValid())
		{
			return FoundId;
		}
		if (const FGuid FoundId = FindIn(State.BattleDeck); FoundId.IsValid())
		{
			return FoundId;
		}
		if (const FGuid FoundId = FindIn(State.BurdenZone); FoundId.IsValid())
		{
			return FoundId;
		}
		for (const FSpecialZone& SpecialZone : State.SpecialZones)
		{
			if (const FGuid FoundId = FindIn(SpecialZone.Cards); FoundId.IsValid())
			{
				return FoundId;
			}
		}
		return FGuid();
	}

	void ResetRunOwnedZones(FRunState& State)
	{
		State.Backpack.Reset();
		State.BattleDeck.Reset();
		State.BurdenZone.Reset();
		State.SpecialZones.Reset();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonBuildsEntriesFromBattleDeckSpec,
	"Wacom.UI.RunFirstPersonCardLayer.BuildsEntriesFromBattleDeckPhysicalCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonBuildsEntriesFromBattleDeckSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* First = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.RunCard.A"), TEXT("Run Card A"), 1);
	UCardDefinition* Second = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.RunCard.B"), TEXT("Run Card B"), 2);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 3);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { First, Second, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());

	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	TestTrue(TEXT("Build returns entries"), Source->BuildRunFirstPersonCardEntries(*Run, Entries));
	TestEqual(TEXT("BattleDeck cards become first-person entries"), Entries.Num(), 2);
	TestEqual(TEXT("Entry preserves first instance id"),
		Entries[0].CardInstanceId,
		Run->GetBattleDeck()[0].InstanceId);
	TestEqual(TEXT("Entry preserves second instance id"),
		Entries[1].CardInstanceId,
		Run->GetBattleDeck()[1].InstanceId);
	TestEqual(TEXT("Card view data uses presentation display name"),
		Entries[0].CardViewData.Name.ToString(),
		FString(TEXT("Run Card A")));
	TestFalse(TEXT("Run first-person cards are not visually disabled"),
		Entries[0].CardViewData.bDisabled);
	TestTrue(TEXT("Run first-person entries stay visually playable"), Entries[0].bIsPlayable);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonIncludesProjectedBattleDeckCardsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.IncludesProjectedBattleDeckCardsWhenEnabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonIncludesProjectedBattleDeckCardsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* TypeB = WacomRunFirstPersonCardLayerSpec::MakeTypeBContainerCard(Fx, 3);
	UCardDefinition* Stored = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.Stored"), TEXT("Projected Stored Card"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { TypeB, Stored });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	TestEqual(TEXT("Fixture starts with one TypeB special zone"), State.SpecialZones.Num(), 1);
	TestEqual(TEXT("TypeB starts in Backpack"), State.Backpack.Num(), 1);
	TestEqual(TEXT("Stored card starts in BattleDeck"), State.BattleDeck.Num(), 1);

	const FCardInstance TypeBInstance = State.Backpack[0];
	FCardInstance StoredInstance = State.BattleDeck[0];
	State.Backpack.Reset();
	State.BattleDeck.Reset();
	State.BattleDeck.Add(TypeBInstance);
	State.SpecialZones[0].Cards.Add(StoredInstance);
	State.SpecialZones[0].Cards.Last().bBattleEnabledInSpecialZone = true;

	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());

	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	Source->bIncludeProjectedRunBattleDeckCards = true;
	Source->BuildRunFirstPersonCardEntries(*Run, Entries);
	TestEqual(TEXT("Physical + projected card are included"), Entries.Num(), 2);
	TestEqual(TEXT("Projected card keeps its instance id"),
		Entries[1].CardInstanceId,
		StoredInstance.InstanceId);

	Source->bIncludeProjectedRunBattleDeckCards = false;
	Source->BuildRunFirstPersonCardEntries(*Run, Entries);
	TestEqual(TEXT("Projected cards can be excluded"), Entries.Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRefreshWritesAnchorRuntimeSourceSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ExplorationFlowFeedsAnchorRuntimeEntries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRefreshWritesAnchorRuntimeSourceSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.Refresh"), TEXT("Refresh Card"), 3);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());

	Source->SetRunFirstPersonCardLayerActive(true);
	TestEqual(TEXT("Refresh writes once when activated"), Source->WriteCount, 1);
	TestEqual(TEXT("Default source writes through presentation frame"),
		Source->PresentationFrameWriteCount,
		1);
	TestEqual(TEXT("Refresh writes BattleDeck entry"), Source->LastWrittenEntries.Num(), 1);
	TestEqual(TEXT("Initial default source animates visible Run card"),
		Source->LastWrittenTransitionHints.Num(),
		1);
	if (Source->LastWrittenTransitionHints.Num() == 1)
	{
		TestEqual(TEXT("Initial default source uses Run hand enter transition"),
			Source->LastWrittenTransitionHints[0].TransitionKind,
			EWacomFirstPersonCardSlotTransitionKind::RunHandEntered);
		TestEqual(TEXT("Initial Run hand enter targets BattleDeck card"),
			Source->LastWrittenTransitionHints[0].CardInstanceId,
			Run->GetBattleDeck()[0].InstanceId);
	}
	TestEqual(TEXT("Debug tracks written entry count"),
		Source->GetRunFirstPersonCardSourceDebugView().EntryCount,
		1);

	Source->ClearRunFirstPersonCardLayer();
	TestEqual(TEXT("Clear goes through runtime source cleanup"), Source->ClearCount, 1);
	TestEqual(TEXT("Clear drops cached test entries"), Source->LastWrittenEntries.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonPawnReadyRefreshesInitialMissingAnchorSpec,
	"Wacom.UI.RunFirstPersonCardLayer.PawnReadyRefreshesInitialMissingAnchorSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonPawnReadyRefreshesInitialMissingAnchorSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.PawnReady"), TEXT("Pawn Ready Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	PC->SetRunFirstPersonCardLayerActive(true);

	UWacomRunFirstPersonCardSourceComponent* Source = PC->GetRunFirstPersonCardSourceComponent();
	TestNotNull(TEXT("PC owns Run first-person source"), Source);
	TestEqual(TEXT("Initial active refresh reports missing anchor"),
		Source ? Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult : FName(),
		FName(TEXT("MissingAnchor")));
	TestTrue(TEXT("Missing anchor keeps default source reconcile pending"),
		Source && Source->GetRunFirstPersonCardSourceDebugView().bHasPendingDefaultSourceReconcile);
	TestEqual(TEXT("Missing anchor is the pending block reason"),
		Source ? Source->GetRunFirstPersonCardSourceDebugView().PendingDefaultSourceBlockReason : FName(),
		FName(TEXT("MissingAnchor")));

	TStrongObjectPtr<AWacomPlayerCharacter> CharacterPawn(NewObject<AWacomPlayerCharacter>());
	PC->SetPawn(CharacterPawn.Get());
	UWacomFirstPersonCardAnchorComponent* Anchor =
		WacomRunFirstPersonCardLayerSpec::GetFirstPersonAnchorForTest(PC.Get());
	TestNotNull(TEXT("Pawn provides first-person anchor"), Anchor);
	if (!Source || !Anchor)
	{
		return false;
	}

	TestEqual(TEXT("Pawn-ready refresh writes Run source to anchor"),
		Anchor->GetRuntimeCardLayerSourceId(),
		Source->RunFirstPersonCardLayerSourceId);
	TestEqual(TEXT("Pawn-ready refresh writes BattleDeck card"),
		Anchor->GetRuntimeCardLayerCardCount(),
		1);
	TestEqual(TEXT("Source reports refreshed after pawn arrives"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("Refreshed")));
	TestFalse(TEXT("Pawn-ready refresh clears pending reconcile"),
		Source->GetRunFirstPersonCardSourceDebugView().bHasPendingDefaultSourceReconcile);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonSessionReadyRefreshesInitialMissingSessionSpec,
	"Wacom.UI.RunFirstPersonCardLayer.SessionReadyRefreshesInitialMissingSessionSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonSessionReadyRefreshesInitialMissingSessionSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.SessionReady"), TEXT("Session Ready Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->SetRunFirstPersonCardLayerActive(true);
	TestEqual(TEXT("Initial active refresh reports missing session"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("MissingRunSession")));
	TestTrue(TEXT("Missing session keeps default source reconcile pending"),
		Source->GetRunFirstPersonCardSourceDebugView().bHasPendingDefaultSourceReconcile);
	TestEqual(TEXT("Missing session is the pending block reason"),
		Source->GetRunFirstPersonCardSourceDebugView().PendingDefaultSourceBlockReason,
		FName(TEXT("MissingRunSession")));

	Source->BindRunSession(Run.Get());
	TestEqual(TEXT("Session-ready reconcile writes Run source to anchor"),
		Anchor->GetRuntimeCardLayerSourceId(),
		Source->RunFirstPersonCardLayerSourceId);
	TestEqual(TEXT("Session-ready reconcile writes BattleDeck card"),
		Anchor->GetRuntimeCardLayerCardCount(),
		1);
	TestEqual(TEXT("Source reports refreshed after session arrives"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("Refreshed")));
	TestFalse(TEXT("Session-ready refresh clears pending reconcile"),
		Source->GetRunFirstPersonCardSourceDebugView().bHasPendingDefaultSourceReconcile);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonHoverShowsDefaultBattleDeckCardDetailSpec,
	"Wacom.UI.RunFirstPersonCardLayer.Detail.HoverShowsDefaultBattleDeckCardDetail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonHoverShowsDefaultBattleDeckCardDetailSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.RunDetail.Default"), TEXT("Run Detail Default"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	TestTrue(TEXT("Run has BattleDeck card"), Run->GetBattleDeck().IsValidIndex(0));
	const FGuid CardInstanceId = Run->GetBattleDeck()[0].InstanceId;

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TestTrue(TEXT("Run detail panel is prewarmed after source activation"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelPrewarmed(PC.Get()));
	const UWacomRunFirstPersonCardSourceComponent* Source =
		PC->GetRunFirstPersonCardSourceComponent();
	TestNotNull(TEXT("Run first-person source exists"), Source);
	TestTrue(TEXT("Default Run source owns detail without menu lease"),
		Source && Source->CanHandleRunFirstPersonCardLayerSource(Source->RunFirstPersonCardLayerSourceId));
	TestFalse(TEXT("Run source ownership rejects BattleHand"),
		Source && Source->CanHandleRunFirstPersonCardLayerSource(WacomFirstPersonCardLayerSourceIds::BattleHand()));
	TestFalse(TEXT("Run source ownership rejects suppressed source"),
		Source && Source->CanHandleRunFirstPersonCardLayerSource(WacomFirstPersonCardLayerSourceIds::RunMenuSuppressed()));

	const FWacomFirstPersonCardLayerSlotView SlotView =
		WacomRunFirstPersonCardLayerSpec::MakeProjectedSlotForRunDetail(
			CardInstanceId,
			FVector2D(700.0f, 720.0f));
	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerCardHovered(
		PC.Get(),
		CardInstanceId,
		SlotView);

	TestFalse(TEXT("Hover starts detail motion without immediate pop-in"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));
	TestTrue(TEXT("Hover detail waits in pending show"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailMotionPending(PC.Get()));
	TestEqual(TEXT("Hover detail starts transparent"),
		FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailPanelOpacity(PC.Get()),
		0.0f);
	FWacomPlayerControllerRunInteractionTestAccess::TickRunFirstPersonCardDetail(PC.Get(), 0.2f);
	TestTrue(TEXT("Hover shows Run first-person card detail after motion tick"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));
	TestEqual(TEXT("Detail uses Run card definition display name"),
		FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailPanelNameText(PC.Get()).ToString(),
		FString(TEXT("Run Detail Default")));

	const FVector2D InitialPosition =
		FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailPanelPosition(PC.Get());
	FWacomFirstPersonCardLayerSlotView MovedSlot = SlotView;
	MovedSlot.ScreenPosition = FVector2D(1100.0f, 640.0f);
	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerHoveredCardLayoutUpdated(
		PC.Get(),
		CardInstanceId,
		MovedSlot);
	FWacomPlayerControllerRunInteractionTestAccess::TickRunFirstPersonCardDetail(PC.Get(), 0.05f);
	TestNotEqual(TEXT("Layout update repositions visible detail"),
		FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailPanelPosition(PC.Get()),
		InitialPosition);

	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerCardUnhovered(
		PC.Get(),
		CardInstanceId,
		MovedSlot);
	FWacomPlayerControllerRunInteractionTestAccess::TickRunFirstPersonCardDetail(PC.Get(), 0.2f);
	TestFalse(TEXT("Unhover hides detail"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonHoverShowsMenuLeaseCardDetailSpec,
	"Wacom.UI.RunFirstPersonCardLayer.Detail.HoverShowsMenuLeaseCardDetail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonHoverShowsMenuLeaseCardDetailSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.RunDetail.Lease"), TEXT("Run Detail Lease"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	const FGuid CardInstanceId = Run->GetBattleDeck().IsValidIndex(0)
		? Run->GetBattleDeck()[0].InstanceId
		: FGuid();
	TestTrue(TEXT("Lease card instance is valid"), CardInstanceId.IsValid());

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	PC->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("DetailLease"));
	Request.ExplicitCardInstanceIds.Add(CardInstanceId);
	FWacomRunMenuCardLeaseResult Result;
	TestTrue(TEXT("Menu lease is set"),
		PC->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestTrue(TEXT("Menu lease result is set"), Result.bLeaseSet);
	const UWacomRunFirstPersonCardSourceComponent* Source =
		PC->GetRunFirstPersonCardSourceComponent();
	TestNotNull(TEXT("Run first-person source exists"), Source);
	TestFalse(TEXT("Default Run source is paused while menu lease is active"),
		Source && Source->CanHandleRunFirstPersonCardLayerSource(Source->RunFirstPersonCardLayerSourceId));
	TestTrue(TEXT("Active menu lease source owns detail"),
		Source && Source->CanHandleRunFirstPersonCardLayerSource(Result.SourceId));

	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerCardHovered(
		PC.Get(),
		CardInstanceId,
		WacomRunFirstPersonCardLayerSpec::MakeProjectedSlotForRunDetail(
			CardInstanceId,
			FVector2D(760.0f, 710.0f)));

	FWacomPlayerControllerRunInteractionTestAccess::TickRunFirstPersonCardDetail(PC.Get(), 0.2f);
	TestTrue(TEXT("Menu lease hover shows detail"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));
	TestEqual(TEXT("Menu lease detail uses owned Run card definition"),
		FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailPanelNameText(PC.Get()).ToString(),
		FString(TEXT("Run Detail Lease")));

	PC->ClearRunFirstPersonCardLayerMenuLease(Request.LeaseId);
	TestFalse(TEXT("Clearing lease hides detail"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonDetailRejectsInvalidSourcesAndHidesOnDragSpec,
	"Wacom.UI.RunFirstPersonCardLayer.Detail.RejectsInvalidSourcesAndHidesOnDrag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonDetailRejectsInvalidSourcesAndHidesOnDragSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.RunDetail.Invalid"), TEXT("Run Detail Invalid"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	const FGuid CardInstanceId = Run->GetBattleDeck().IsValidIndex(0)
		? Run->GetBattleDeck()[0].InstanceId
		: FGuid();
	TestTrue(TEXT("Detail card instance is valid"), CardInstanceId.IsValid());

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	PC->SetRunFirstPersonCardLayerActive(true);

	UWacomFirstPersonCardAnchorComponent* Anchor =
		WacomRunFirstPersonCardLayerSpec::GetFirstPersonAnchorForTest(PC.Get());
	TestNotNull(TEXT("Probe pawn has first-person anchor"), Anchor);
	if (!Anchor)
	{
		return false;
	}

	const FWacomFirstPersonCardLayerSlotView SlotView =
		WacomRunFirstPersonCardLayerSpec::MakeProjectedSlotForRunDetail(
			CardInstanceId,
			FVector2D(740.0f, 700.0f));
	TArray<FWacomFirstPersonCardLayerEntry> BattleHandEntries;
	BattleHandEntries.Add(SlotView.Entry);
	Anchor->SetRuntimeCardLayerEntries(WacomFirstPersonCardLayerSourceIds::BattleHand(), BattleHandEntries);
	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerCardHovered(
		PC.Get(),
		CardInstanceId,
		SlotView);
	TestFalse(TEXT("Run detail ignores BattleHand source"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));
	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerDragStarted(
		PC.Get(),
		CardInstanceId,
		WacomRunFirstPersonCardLayerSpec::MakeRunDetailDragView(
			CardInstanceId,
			EWacomFirstPersonCardGestureState::Inspecting,
			SlotView));
	TestFalse(TEXT("Run inspect detail ignores BattleHand source"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));

	PC->RefreshRunFirstPersonCardLayer();
	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerCardHovered(
		PC.Get(),
		CardInstanceId,
		SlotView);
	FWacomPlayerControllerRunInteractionTestAccess::TickRunFirstPersonCardDetail(PC.Get(), 0.2f);
	TestTrue(TEXT("Default Run source shows detail again"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = CardInstanceId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::DraggingNoTargetCard;
	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerDragStarted(
		PC.Get(),
		CardInstanceId,
		DragView);
	FWacomPlayerControllerRunInteractionTestAccess::TickRunFirstPersonCardDetail(PC.Get(), 0.2f);
	TestFalse(TEXT("Starting drag hides Run detail"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));

	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerCardHovered(
		PC.Get(),
		FGuid::NewGuid(),
		SlotView);
	TestFalse(TEXT("Invalid Run instance does not show detail"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonInspectKeepsAndScrubsCardDetailSpec,
	"Wacom.UI.RunFirstPersonCardLayer.Detail.InspectKeepsAndScrubsCardDetail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonInspectKeepsAndScrubsCardDetailSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* First = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.RunInspect.First"), TEXT("Run Inspect First"), 1);
	UCardDefinition* Second = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.RunInspect.Second"), TEXT("Run Inspect Second"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { First, Second, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	TestTrue(TEXT("Run has first BattleDeck card"), Run->GetBattleDeck().IsValidIndex(0));
	TestTrue(TEXT("Run has second BattleDeck card"), Run->GetBattleDeck().IsValidIndex(1));
	const FGuid FirstId = Run->GetBattleDeck().IsValidIndex(0)
		? Run->GetBattleDeck()[0].InstanceId
		: FGuid();
	const FGuid SecondId = Run->GetBattleDeck().IsValidIndex(1)
		? Run->GetBattleDeck()[1].InstanceId
		: FGuid();

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	PC->SetRunFirstPersonCardLayerActive(true);

	UWacomFirstPersonCardAnchorComponent* Anchor =
		WacomRunFirstPersonCardLayerSpec::GetFirstPersonAnchorForTest(PC.Get());
	TestNotNull(TEXT("Probe pawn has first-person anchor"), Anchor);
	if (!Anchor)
	{
		return false;
	}

	const FWacomFirstPersonCardLayerSlotView FirstSlot =
		WacomRunFirstPersonCardLayerSpec::MakeProjectedSlotForRunDetail(
			FirstId,
			FVector2D(700.0f, 720.0f));
	const FWacomFirstPersonCardLayerSlotView SecondSlot =
		WacomRunFirstPersonCardLayerSpec::MakeProjectedSlotForRunDetail(
			SecondId,
			FVector2D(900.0f, 720.0f));

	FWacomFirstPersonCardDragView InspectFirst =
		WacomRunFirstPersonCardLayerSpec::MakeRunDetailDragView(
			FirstId,
			EWacomFirstPersonCardGestureState::Inspecting,
			FirstSlot);
	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerDragStarted(
		PC.Get(),
		FirstId,
		InspectFirst);
	TestTrue(TEXT("Inspect detail starts pending show"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailMotionPending(PC.Get()));
	TestFalse(TEXT("Inspect detail does not pop in immediately"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));
	FWacomPlayerControllerRunInteractionTestAccess::TickRunFirstPersonCardDetail(PC.Get(), 0.2f);
	TestTrue(TEXT("Inspect shows Run first-person card detail after motion tick"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));
	TestEqual(TEXT("Inspect detail uses first card"),
		FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailPanelNameText(PC.Get()).ToString(),
		FString(TEXT("Run Inspect First")));
	TestTrue(TEXT("Inspect does not start world drop probe"),
		FWacomPlayerControllerRunInteractionTestAccess::RunWorldCardDropDebugSummary(PC.Get()).IsEmpty());
	const int32 DataApplyCountAfterFirstInspect =
		FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailDataApplyCount(PC.Get());

	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerCardUnhovered(
		PC.Get(),
		FirstId,
		FirstSlot);
	TestTrue(TEXT("Unhover during inspect keeps detail visible"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));

	const FVector2D InitialPosition =
		FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailPanelPosition(PC.Get());
	FWacomFirstPersonCardDragView MovedInspectFirst = InspectFirst;
	MovedInspectFirst.SourceSlotView.ScreenPosition = FVector2D(1040.0f, 620.0f);
	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerDragUpdated(
		PC.Get(),
		FirstId,
		MovedInspectFirst);
	TestEqual(TEXT("Same-card inspect update reuses detail data"),
		FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailDataApplyCount(PC.Get()),
		DataApplyCountAfterFirstInspect);
	FWacomPlayerControllerRunInteractionTestAccess::TickRunFirstPersonCardDetail(PC.Get(), 0.05f);
	TestNotEqual(TEXT("Inspect drag update repositions detail"),
		FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailPanelPosition(PC.Get()),
		InitialPosition);

	FWacomFirstPersonCardDragView InspectSecond =
		WacomRunFirstPersonCardLayerSpec::MakeRunDetailDragView(
			SecondId,
			EWacomFirstPersonCardGestureState::Inspecting,
			SecondSlot);
	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerDragStarted(
		PC.Get(),
		SecondId,
		InspectSecond);
	TestTrue(TEXT("Inspect scrub applies detail data for second card"),
		FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailDataApplyCount(PC.Get())
			> DataApplyCountAfterFirstInspect);
	TestEqual(TEXT("Inspect scrub switches detail to second card"),
		FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailPanelNameText(PC.Get()).ToString(),
		FString(TEXT("Run Inspect Second")));

	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerDragReleased(
		PC.Get(),
		SecondId,
		InspectSecond);
	FWacomPlayerControllerRunInteractionTestAccess::TickRunFirstPersonCardDetail(PC.Get(), 0.2f);
	TestFalse(TEXT("Inspect release hides detail when no hover owns it"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));

	Anchor->bShowDetailDuringCardInspect = false;
	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerDragStarted(
		PC.Get(),
		FirstId,
		InspectFirst);
	FWacomPlayerControllerRunInteractionTestAccess::TickRunFirstPersonCardDetail(PC.Get(), 0.2f);
	TestFalse(TEXT("Inspect detail respects anchor disable flag"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));

	Anchor->bShowDetailDuringCardInspect = true;
	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerDragStarted(
		PC.Get(),
		FirstId,
		InspectFirst);
	FWacomPlayerControllerRunInteractionTestAccess::TickRunFirstPersonCardDetail(PC.Get(), 0.2f);
	TestTrue(TEXT("Inspect detail is visible before formal drag promotion"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));
	FWacomFirstPersonCardDragView FormalDrag = InspectFirst;
	FormalDrag.GestureState = EWacomFirstPersonCardGestureState::DraggingNoTargetCard;
	FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerDragUpdated(
		PC.Get(),
		FirstId,
		FormalDrag);
	FWacomPlayerControllerRunInteractionTestAccess::TickRunFirstPersonCardDetail(PC.Get(), 0.2f);
	TestFalse(TEXT("Formal drag promotion hides inspect detail"),
		FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(PC.Get()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRunStateUnchangedRevisionSkipsDefaultRewriteSpec,
	"Wacom.UI.RunFirstPersonCardLayer.RunStateChangedWithUnchangedStorageRevisionSkipsDefaultSourceRewrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRunStateUnchangedRevisionSkipsDefaultRewriteSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.StateChangedSkip"), TEXT("State Changed Skip Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	const int32 WritesAfterActivate = Source->WriteCount;
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Unchanged storage revision skips source rewrite"),
		Source->WriteCount,
		WritesAfterActivate);
	TestEqual(TEXT("Skip is reported"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("SkippedUnchangedRevision")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Revision skip count increments"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RevisionSkipCount,
		1);
	TestEqual(TEXT("Skipped refresh does not rebuild snapshot"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).DataBuildCount,
		0);
	TestEqual(TEXT("Skipped refresh does not apply runtime source"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RuntimeApplyCount,
		0);
#endif
	TestEqual(TEXT("Debug keeps previous entry count"),
		Source->GetRunFirstPersonCardSourceDebugView().EntryCount,
		1);

	Source->SetRunFirstPersonCardLayerActive(false);
	const int32 WritesAfterDeactivate = Source->WriteCount;
	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Inactive source ignores RunState change"),
		Source->WriteCount,
		WritesAfterDeactivate);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonMissingSessionOrAnchorClearsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MissingSessionOrAnchorClearsRuntimeSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonMissingSessionOrAnchorClearsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();

	Source->SetRunFirstPersonCardLayerActive(true);
	TestEqual(TEXT("Missing session clears runtime source"), Source->ClearCount, 1);
	TestEqual(TEXT("Missing session is reported"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("MissingRunSession")));
	TestTrue(TEXT("Missing session is pending instead of a final success state"),
		Source->GetRunFirstPersonCardSourceDebugView().bHasPendingDefaultSourceReconcile);
	TestEqual(TEXT("Missing session records its block reason"),
		Source->GetRunFirstPersonCardSourceDebugView().PendingDefaultSourceBlockReason,
		FName(TEXT("MissingRunSession")));

	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.MissingAnchor"), TEXT("Missing Anchor Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes for missing anchor branch"), Run->Initialize(Character));
	Source->BindRunSession(Run.Get());

	Source->ClearCount = 0;
	Source->AnchorForTest = nullptr;
	TestFalse(TEXT("Missing anchor refresh fails"), Source->RefreshRunFirstPersonCardLayer());
	TestEqual(TEXT("Missing anchor does not call anchor clear"), Source->ClearCount, 0);
	TestTrue(TEXT("Missing anchor remains pending"),
		Source->GetRunFirstPersonCardSourceDebugView().bHasPendingDefaultSourceReconcile);
	TestEqual(TEXT("Missing anchor records its block reason"),
		Source->GetRunFirstPersonCardSourceDebugView().PendingDefaultSourceBlockReason,
		FName(TEXT("MissingAnchor")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonGameMenuSuppressionClearsDefaultSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.GameMenuSuppressionClearsDefaultBattleDeckSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonGameMenuSuppressionClearsDefaultSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.MenuSuppress"), TEXT("Menu Suppress Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	TestEqual(TEXT("Default source writes once"), Source->WriteCount, 1);

	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);
	TestEqual(TEXT("Suppression writes empty frame instead of hard clearing source"),
		Source->ClearCount,
		0);
	TestTrue(TEXT("Suppression keeps runtime ownership so static preview cannot reappear"),
		Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Suppression writes an empty runtime layer"),
		Anchor->GetRuntimeCardLayerCardCount(),
		0);
	TestEqual(TEXT("Suppression uses its own runtime source"),
		Anchor->GetRuntimeCardLayerSourceId(),
		WacomFirstPersonCardLayerSourceIds::RunMenuSuppressed());
	TestEqual(TEXT("Suppression reports state"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("SuppressedByGameMenu")));
	TestTrue(TEXT("Debug marks GameMenu suppression"),
		Source->GetRunFirstPersonCardSourceDebugView().bSuppressedByGameMenu);
	TestEqual(TEXT("Suppression does not keep stale entries"),
		Source->GetRunFirstPersonCardSourceDebugView().EntryCount,
		0);
	TestEqual(TEXT("Suppression frame has no enter hints"),
		Source->LastWrittenTransitionHints.Num(),
		0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonMenuSuppressionBlocksDevelopmentPreviewSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.GameMenuSuppressionBlocksDevelopmentPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonMenuSuppressionBlocksDevelopmentPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.MenuSuppressStatic"), TEXT("Menu Suppress Static Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	Anchor->bDrawPreviewCardLayer = true;
	Anchor->PreviewCardCountFallback = 3;
	TestEqual(TEXT("Development preview is configured with placeholder cards"),
		Anchor->PreviewCardCountFallback,
		3);

	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	TestEqual(TEXT("Default runtime source has one run card"),
		Anchor->GetRuntimeCardLayerCardCount(),
		1);

	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);
	TestTrue(TEXT("Suppressed layer still has runtime data ownership"),
		Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Suppressed runtime entries are empty instead of exposing preview data"),
		Anchor->GetRuntimeCardLayerEntries().Num(),
		0);
	TestEqual(TEXT("Development preview is still configured, proving runtime ownership blocks preview data"),
		Anchor->PreviewCardCountFallback,
		3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonGameMenuSuppressionReleaseRestoresSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.ReleasingSuppressionRestoresBattleDeckSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonGameMenuSuppressionReleaseRestoresSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.MenuRestore"), TEXT("Menu Restore Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);
	const int32 WritesWhileSuppressed = Source->WriteCount;

	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(false);
	TestEqual(TEXT("Release suppression writes default source"),
		Source->WriteCount,
		WritesWhileSuppressed + 1);
	TestEqual(TEXT("Restored entry count"),
		Source->LastWrittenEntries.Num(),
		1);
	TestEqual(TEXT("Restore animates Run hand enter"),
		Source->LastWrittenTransitionHints.Num(),
		1);
	if (Source->LastWrittenTransitionHints.Num() == 1)
	{
		TestEqual(TEXT("Restore uses Run hand enter transition"),
			Source->LastWrittenTransitionHints[0].TransitionKind,
			EWacomFirstPersonCardSlotTransitionKind::RunHandEntered);
	}
	TestEqual(TEXT("Restore reports refreshed"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("Refreshed")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonPrepareExplorationClearsStaleMenuContextSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.PrepareExplorationClearsStaleMenuContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonPrepareExplorationClearsStaleMenuContextSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.PrepareRun"), TEXT("Prepare Run Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	UWacomRunFirstPersonCardSourceComponent* Source = PC->GetRunFirstPersonCardSourceComponent();
	UWacomFirstPersonCardAnchorComponent* Anchor =
		WacomRunFirstPersonCardLayerSpec::GetFirstPersonAnchorForTest(PC.Get());
	TestNotNull(TEXT("PC owns source"), Source);
	TestNotNull(TEXT("Pawn provides anchor"), Anchor);
	if (!Source || !Anchor)
	{
		return false;
	}

	FWacomPlayerControllerRunInteractionTestAccess::SetRunFirstPersonMenuLease(
		PC.Get(),
		TEXT("StaleMenuLease"));
	PC->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);
	TestTrue(TEXT("Fixture has stale menu lease"),
		Source->GetRunFirstPersonCardSourceDebugView().bHasActiveMenuLease);
	TestTrue(TEXT("Fixture has stale suppression"),
		Source->GetRunFirstPersonCardSourceDebugView().bSuppressedByGameMenu);

	FWacomPlayerControllerRunInteractionTestAccess::PrepareExplorationRunFirstPersonCardLayer(PC.Get());
	const FWacomRunFirstPersonCardSourceDebugView DebugView =
		Source->GetRunFirstPersonCardSourceDebugView();
	TestFalse(TEXT("Prepare clears stale menu lease"), DebugView.bHasActiveMenuLease);
	TestFalse(TEXT("Prepare clears stale GameMenu suppression"), DebugView.bSuppressedByGameMenu);
	TestEqual(TEXT("Prepare restores default Run source"),
		Anchor->GetRuntimeCardLayerSourceId(),
		Source->RunFirstPersonCardLayerSourceId);
	TestEqual(TEXT("Prepare writes BattleDeck card"),
		Anchor->GetRuntimeCardLayerCardCount(),
		1);
	TestEqual(TEXT("Prepare reports refreshed"),
		DebugView.LastRefreshResult,
		FName(TEXT("Refreshed")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRunStateSuppressedDoesNotRewriteSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.RunStateChangedDoesNotRewriteBattleDeckWhileSuppressed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRunStateSuppressedDoesNotRewriteSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.MenuState"), TEXT("Menu State Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);
	const int32 WritesWhileSuppressed = Source->WriteCount;

	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Suppressed source does not rewrite entries"),
		Source->WriteCount,
		WritesWhileSuppressed);
	TestEqual(TEXT("Suppressed state remains reported"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("SuppressedByGameMenu")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonMenuLeaseOverridesDefaultSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.MenuLeaseOverridesDefaultBattleDeckSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonMenuLeaseOverridesDefaultSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* DefaultCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.Default"), TEXT("Default Card"), 1);
	UCardDefinition* LeaseCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.Lease"), TEXT("Lease Card"), 2);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { DefaultCard, LeaseCard, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseResult LeaseResult;

	TestTrue(TEXT("Lease can be set"),
		WacomRunFirstPersonCardLayerSpec::SetDefinitionLease(
			*Source,
			TEXT("RunEventChoice"),
			LeaseCard,
			LeaseResult));
	TestEqual(TEXT("Lease writes its own source id"),
		Source->LastWrittenSourceId,
		FName(TEXT("RunEventChoiceSource")));
	TestEqual(TEXT("Lease writes candidate entries"),
		Source->LastWrittenEntries.Num(),
		1);
	TestTrue(TEXT("Debug marks active lease"),
		Source->GetRunFirstPersonCardSourceDebugView().bHasActiveMenuLease);
	TestEqual(TEXT("Debug reports lease entry count"),
		Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseEntryCount,
		1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonSuppressionDoesNotDisableLeaseSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.GameMenuSuppressionDoesNotDisableActiveMenuLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonSuppressionDoesNotDisableLeaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* LeaseCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.LeaseSuppressed"), TEXT("Lease While Suppressed"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { LeaseCard, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseResult LeaseResult;

	TestTrue(TEXT("Lease writes while GameMenu suppression is active"),
		WacomRunFirstPersonCardLayerSpec::SetDefinitionLease(
			*Source,
			TEXT("MenuLease"),
			LeaseCard,
			LeaseResult));
	TestEqual(TEXT("Lease source still writes"),
		Source->LastWrittenSourceId,
		FName(TEXT("MenuLeaseSource")));
	TestEqual(TEXT("Lease entry remains visible"),
		Source->LastWrittenEntries.Num(),
		1);
	TestTrue(TEXT("Suppression remains tracked"),
		Source->GetRunFirstPersonCardSourceDebugView().bSuppressedByGameMenu);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonLeaseReleaseRestoresStateSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.ReleasingMenuLeaseRestoresSuppressedOrDefaultState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonLeaseReleaseRestoresStateSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.LeaseRestore"), TEXT("Lease Restore Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease can be set"),
		WacomRunFirstPersonCardLayerSpec::SetDefinitionLease(
			*Source,
			TEXT("Lease"),
			Card,
			LeaseResult));
	const int32 WritesWithLease = Source->WriteCount;

	TestTrue(TEXT("Lease can be cleared"),
		Source->ClearRunFirstPersonCardLayerMenuLease(TEXT("Lease")));
	TestEqual(TEXT("Suppressed state writes empty runtime source after lease clears"),
		Source->WriteCount,
		WritesWithLease + 1);
	TestEqual(TEXT("Lease release restores suppression source"),
		Source->LastWrittenSourceId,
		WacomFirstPersonCardLayerSourceIds::RunMenuSuppressed());
	TestEqual(TEXT("Lease release does not expose stale lease entries"),
		Source->LastWrittenEntries.Num(),
		0);
	TestEqual(TEXT("Suppression is restored"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("SuppressedByGameMenu")));

	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(false);
	TestEqual(TEXT("Default returns after suppression ends"),
		Source->LastWrittenSourceId,
		Source->RunFirstPersonCardLayerSourceId);
	TestEqual(TEXT("Default has entries"),
		Source->LastWrittenEntries.Num(),
		1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonDifferentLeaseCannotStealSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.DifferentLeaseIdCannotStealActiveLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonDifferentLeaseCannotStealSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* LeaseCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.LeaseSteal"), TEXT("Lease"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { LeaseCard, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseResult LeaseResult;

	TestTrue(TEXT("First lease succeeds"),
		WacomRunFirstPersonCardLayerSpec::SetDefinitionLease(
			*Source,
			TEXT("LeaseA"),
			LeaseCard,
			LeaseResult));
	TestFalse(TEXT("Different lease cannot steal active lease"),
		WacomRunFirstPersonCardLayerSpec::SetDefinitionLease(
			*Source,
			TEXT("LeaseB"),
			LeaseCard,
			LeaseResult));
	TestEqual(TEXT("Conflict is reported"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("MenuLeaseConflict")));
	TestEqual(TEXT("Original lease source remains active"),
		Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseSourceId,
		FName(TEXT("LeaseASource")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonClearLayerClearsMenuContextSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.ClearRunFirstPersonCardLayerClearsLeaseAndSuppressionOutput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonClearLayerClearsMenuContextSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* LeaseCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.LeaseClear"), TEXT("Lease"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { LeaseCard, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease can be set"),
		WacomRunFirstPersonCardLayerSpec::SetDefinitionLease(
			*Source,
			TEXT("Lease"),
			LeaseCard,
			LeaseResult));

	Source->ClearRunFirstPersonCardLayer();
	const FWacomRunFirstPersonCardSourceDebugView Debug = Source->GetRunFirstPersonCardSourceDebugView();
	TestFalse(TEXT("Suppression is cleared"), Debug.bSuppressedByGameMenu);
	TestFalse(TEXT("Lease is cleared"), Debug.bHasActiveMenuLease);
	TestEqual(TEXT("Visible entry output is cleared"), Debug.EntryCount, 0);
	TestEqual(TEXT("Clear reports result"), Debug.LastRefreshResult, FName(TEXT("Cleared")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonDebugSummaryReportsMenuContextSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.DebugSummaryReportsMenuContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonDebugSummaryReportsMenuContextSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* LeaseCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.LeaseDebug"), TEXT("Lease"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { LeaseCard, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease can be set"),
		WacomRunFirstPersonCardLayerSpec::SetDefinitionLease(
			*Source,
			TEXT("Lease"),
			LeaseCard,
			LeaseResult));

	const FString Summary = Source->GetRunFirstPersonCardSourceDebugSummary();
	TestTrue(TEXT("Summary includes suppression"),
		Summary.Contains(TEXT("SuppressedByGameMenu=true")));
	TestTrue(TEXT("Summary includes lease id"),
		Summary.Contains(TEXT("LeaseId=Lease")));
	TestTrue(TEXT("Summary includes lease source"),
		Summary.Contains(TEXT("LeaseSource=LeaseSource")));
	TestTrue(TEXT("Summary includes lease entry count"),
		Summary.Contains(TEXT("LeaseEntries=1")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonMenuLeaseCanEnableDragProbeSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.MenuLeaseCanEnableFirstPersonDragProbe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonMenuLeaseCanEnableDragProbeSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* LeaseCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.LeaseDragProbe"), TEXT("Lease"), 1);
	LeaseCard->TargetMode = ECardTargetMode::SingleEnemyPart;
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { LeaseCard, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseResult LeaseResult;

	TestTrue(TEXT("Menu lease can be written"),
		WacomRunFirstPersonCardLayerSpec::SetDefinitionLease(
			*Source,
			TEXT("Lease"),
			LeaseCard,
			LeaseResult));
	TestTrue(TEXT("Menu lease enables first-person interaction for probe"),
		Anchor->IsFirstPersonCardLayerInteractionEnabled());

	Source->ClearRunFirstPersonCardLayerMenuLease(TEXT("Lease"));
	TestFalse(TEXT("Suppressed default source disables interaction after lease clears"),
		Anchor->IsFirstPersonCardLayerInteractionEnabled());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRequestBuildsLeaseEntriesFromDefinitionsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.RequestBuildsLeaseEntriesFromAllowedDefinitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRequestBuildsLeaseEntriesFromDefinitionsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("毒牙"), 0);
	UCardDefinition* Other = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Other"), TEXT("Other"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = {
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang),
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Other)
	};

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest();
	Request.AllowedCardDefinitions.Add(Fang);
	FWacomRunMenuCardLeaseResult Result;
	const int32 PresentationFrameWritesBeforeProvider =
		Source->PresentationFrameWriteCount;

	TestTrue(TEXT("Provider sets lease from allowed definition"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestTrue(TEXT("Result reports lease set"), Result.bLeaseSet);
	TestEqual(TEXT("Provider lease writes through presentation frame"),
		Source->PresentationFrameWriteCount,
		PresentationFrameWritesBeforeProvider + 1);
	TestEqual(TEXT("Only matching definition becomes a candidate"), Result.CandidateCount, 1);
	TestEqual(TEXT("Written lease entry preserves Fang instance"),
		Source->LastWrittenEntries[0].CardInstanceId,
		State.Backpack[0].InstanceId);
	TestEqual(TEXT("Written lease uses provider source id"),
		Source->LastWrittenSourceId,
		Request.SourceId);
	TestEqual(TEXT("Card face uses presentation data"),
		Source->LastWrittenEntries[0].CardViewData.Name.ToString(),
		FString(TEXT("毒牙")));
	TestEqual(TEXT("Provider lease animates filtered Run card entering the menu hand"),
		Source->LastWrittenTransitionHints.Num(),
		1);
	if (Source->LastWrittenTransitionHints.Num() == 1)
	{
		TestEqual(TEXT("Provider lease entry uses Run hand enter transition"),
			Source->LastWrittenTransitionHints[0].TransitionKind,
			EWacomFirstPersonCardSlotTransitionKind::RunHandEntered);
		TestEqual(TEXT("Provider lease enter hint targets Fang"),
			Source->LastWrittenTransitionHints[0].CardInstanceId,
			State.Backpack[0].InstanceId);
		TestEqual(TEXT("Provider lease enter hint sequence index"),
			Source->LastWrittenTransitionHints[0].SequenceIndex,
			0);
		TestEqual(TEXT("Provider lease enter hint sequence count"),
			Source->LastWrittenTransitionHints[0].SequenceCount,
			1);
	}
	FRunCardWorkspaceEntry WorkspaceEntry;
	TestTrue(TEXT("Provider source stores workspace metadata"),
		Source->FindCurrentRunFirstPersonCardWorkspaceEntry(
			State.Backpack[0].InstanceId,
			WorkspaceEntry));
	TestEqual(TEXT("Workspace metadata keeps physical source zone"),
		WorkspaceEntry.PhysicalZone,
		EZoneKind::Backpack);
	TestFalse(TEXT("Provider workspace entry is not projected"),
		WorkspaceEntry.bIsProjectedBattleDeckCard);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRequestMatchesAllowedCardIdsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.RequestMatchesAllowedCardIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRequestMatchesAllowedCardIdsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("毒牙"), 0);
	UCardDefinition* Other = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Other"), TEXT("Other"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.BattleDeck = {
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Other),
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang)
	};

	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("CardIdLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult Result;

	TestTrue(TEXT("Provider matches by CardId"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("One CardId match is shown"), Result.CandidateCount, 1);
	TestEqual(TEXT("Matched entry is Fang"),
		Source->LastWrittenEntries[0].CardInstanceId,
		State.BattleDeck[1].InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRequestMatchesExplicitInstanceIdsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.RequestMatchesExplicitInstanceIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRequestMatchesExplicitInstanceIdsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Shared = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Shared"), TEXT("Shared"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Shared });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = {
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Shared),
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Shared)
	};

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("InstanceLease"));
	Request.AllowedCardIds.Add(TEXT("Shared"));
	Request.ExplicitCardInstanceIds.Add(State.Backpack[1].InstanceId);
	FWacomRunMenuCardLeaseResult Result;

	TestTrue(TEXT("Provider matches explicit instance whitelist"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("Only whitelisted instance is shown"), Result.CandidateCount, 1);
	TestEqual(TEXT("Matched entry is second shared instance"),
		Source->LastWrittenEntries[0].CardInstanceId,
		State.Backpack[1].InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRequestUsesKeywordsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.RequestUsesRequiredAndBlockedKeywords",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRequestUsesKeywordsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Companion = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Companion"), TEXT("Companion"), 1);
	Companion->Keywords.AddTag(WacomTags::Card_Keyword_Companion);
	UCardDefinition* WeaponCompanion = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("WeaponCompanion"), TEXT("Weapon Companion"), 1);
	WeaponCompanion->Keywords.AddTag(WacomTags::Card_Keyword_Companion);
	WeaponCompanion->Keywords.AddTag(WacomTags::Card_Keyword_Weapon);
	UCardDefinition* Plain = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Plain"), TEXT("Plain"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Companion });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = {
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Companion),
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(WeaponCompanion),
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Plain)
	};

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("KeywordLease"));
	Request.RequiredKeywords.AddTag(WacomTags::Card_Keyword_Companion);
	Request.BlockedKeywords.AddTag(WacomTags::Card_Keyword_Weapon);
	FWacomRunMenuCardLeaseResult Result;

	TestTrue(TEXT("Provider applies required and blocked keywords"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("Only non-weapon companion is shown"), Result.CandidateCount, 1);
	TestEqual(TEXT("Matched entry is companion"),
		Source->LastWrittenEntries[0].CardInstanceId,
		State.Backpack[0].InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonAllHeldZonesNoProjectedDuplicatesSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.AllHeldZonesAreIncludedWithoutProjectedDuplicates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonAllHeldZonesNoProjectedDuplicatesSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Match = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Match"), TEXT("Match"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Match });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Match) };
	State.BattleDeck = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Match) };
	State.BurdenZone = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Match) };
	FSpecialZone SpecialZone;
	SpecialZone.OwnerInstanceId = FGuid::NewGuid();
	SpecialZone.Cards.Add(WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Match));
	SpecialZone.Cards[0].bBattleEnabledInSpecialZone = true;
	State.SpecialZones = { SpecialZone };

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("AllZonesLease"));
	Request.AllowedCardDefinitions.Add(Match);
	FWacomRunMenuCardLeaseResult Result;

	TestTrue(TEXT("Provider builds candidates from all physical owned zones"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("Backpack, BattleDeck, Burden and SpecialZone are included once each"),
		Result.CandidateCount,
		4);
	TestEqual(TEXT("Projected duplicates are not added"),
		Source->LastWrittenEntries.Num(),
		4);
	TestEqual(TEXT("SpecialZone physical card is the last candidate"),
		Source->LastWrittenEntries[3].CardInstanceId,
		State.SpecialZones[0].Cards[0].InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonNoMatchingClearsSameLeaseSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.NoMatchingCardsClearsExistingSameLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonNoMatchingClearsSameLeaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Match = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Match"), TEXT("Match"), 1);
	UCardDefinition* Other = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Other"), TEXT("Other"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Match });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Match) };

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("SameLease"));
	Request.AllowedCardDefinitions.Add(Match);
	FWacomRunMenuCardLeaseResult Result;
	TestTrue(TEXT("Initial matching lease succeeds"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestTrue(TEXT("Lease is active"), Source->HasActiveMenuLease());

	Request.AllowedCardDefinitions.Reset();
	Request.AllowedCardDefinitions.Add(Other);
	TestFalse(TEXT("No matching candidates rejects"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("No match reason is reported"),
		Result.RejectReason,
		FName(TEXT("NoMatchingCandidates")));
	TestFalse(TEXT("Same lease is cleared to avoid stale candidates"),
		Source->HasActiveMenuLease());
	TestEqual(TEXT("Suppression output is restored after clearing stale lease"),
		Source->LastWrittenEntries.Num(),
		0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonEmptyFilterRejectsUnlessAllowAllSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.EmptyFilterRejectsUnlessAllowAllIsEnabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonEmptyFilterRejectsUnlessAllowAllSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* First = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("First"), TEXT("First"), 1);
	UCardDefinition* Second = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Second"), TEXT("Second"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { First });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = {
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(First),
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Second)
	};

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("EmptyLease"));
	FWacomRunMenuCardLeaseResult Result;
	TestFalse(TEXT("Empty filter rejects by default"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("Empty filter reason is reported"),
		Result.RejectReason,
		FName(TEXT("EmptyFilter")));

	Request.bAllowAllOwnedCardsWhenNoFilter = true;
	TestTrue(TEXT("Allow all enables empty filter request"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestEqual(TEXT("All owned physical cards are shown"),
		Result.CandidateCount,
		2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonMenuWidgetOwnedLeaseClearsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.MenuWidgetOwnedLeaseAutoClearsOnDeactivate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonMenuWidgetOwnedLeaseClearsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("毒牙"), 0);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang) };

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	TStrongObjectPtr<AWacomPlayerCharacter> CharacterPawn(NewObject<AWacomPlayerCharacter>());
	PC->SetPawn(CharacterPawn.Get());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> Menu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	Menu->SetOwningWacomPlayerControllerForTest(PC.Get());

	FWacomRunMenuCardLeaseRequest Request;
	Request.AllowedCardDefinitions.Add(Fang);
	FWacomRunMenuCardLeaseResult Result;
	TestTrue(TEXT("Menu widget can set owned lease with generated ids"),
		Menu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));

	UWacomRunFirstPersonCardSourceComponent* Source = PC->GetRunFirstPersonCardSourceComponent();
	TestTrue(TEXT("PC source has active menu lease"),
		Source && Source->HasActiveMenuLease());
	TestFalse(TEXT("Generated lease id is non-empty"),
		Source->GetActiveMenuLeaseId().IsNone());

	Menu->DeactivateForTest();
	TestFalse(TEXT("Owned lease is cleared on deactivate"),
		Source->HasActiveMenuLease());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonPrototypeTestMenuRequestsOwnedLeaseSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.PrototypeTestMenuRequestsOwnedLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonPrototypeTestMenuRequestsOwnedLeaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("毒牙"), 0);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang) };

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	TStrongObjectPtr<AWacomPlayerCharacter> CharacterPawn(NewObject<AWacomPlayerCharacter>());
	PC->SetPawn(CharacterPawn.Get());

	TStrongObjectPtr<UWacomRunMenuCardLeaseTestMenuProbe> Menu(
		NewObject<UWacomRunMenuCardLeaseTestMenuProbe>(PC.Get()));
	Menu->SetOwningWacomPlayerControllerForTest(PC.Get());
	Menu->LeaseRequest.AllowedCardIds = { TEXT("PoisonFang") };

	TestTrue(TEXT("C++ test menu requests owned lease"),
		Menu->RequestOwnedLeaseNow());
	TestTrue(TEXT("Result reports one candidate"),
		Menu->GetLastLeaseResult().CandidateCount == 1);

	UWacomRunFirstPersonCardSourceComponent* Source = PC->GetRunFirstPersonCardSourceComponent();
	TestTrue(TEXT("PC source has active lease after C++ test menu request"),
		Source && Source->HasActiveMenuLease());

	Menu->DeactivateForTest();
	TestFalse(TEXT("C++ test menu owned lease clears on deactivate"),
		Source->HasActiveMenuLease());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonLeaseProviderPendingMissingAnchorSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.MissingAnchorKeepsPendingLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonLeaseProviderPendingMissingAnchorSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("毒牙"), 0);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang) };

	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("MissingAnchorLease"));
	Request.AllowedCardDefinitions.Add(Fang);
	FWacomRunMenuCardLeaseResult Result;

	TestTrue(TEXT("Provider accepts desired lease before anchor is available"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));
	TestTrue(TEXT("Missing anchor keeps an active lease to retry"),
		Source->HasActiveMenuLease());
	TestTrue(TEXT("Missing anchor is reported as pending menu lease reconcile"),
		Source->GetRunFirstPersonCardSourceDebugView().bHasPendingMenuLeaseReconcile);
	TestEqual(TEXT("Missing anchor pending reason is reported"),
		Source->GetRunFirstPersonCardSourceDebugView().PendingMenuLeaseBlockReason,
		FName(TEXT("MissingAnchorForMenuLease")));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	Source->AnchorForTest = Anchor.Get();
	TestTrue(TEXT("Anchor-ready manual refresh applies pending provider lease"),
		Source->RefreshRunFirstPersonCardLayer());
	TestEqual(TEXT("Pending lease writes candidate to anchor"),
		Anchor->GetRuntimeCardLayerCardCount(),
		1);
	TestFalse(TEXT("Anchor-ready refresh clears pending menu lease reconcile"),
		Source->GetRunFirstPersonCardSourceDebugView().bHasPendingMenuLeaseReconcile);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonLeaseProviderDebugReportsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuLeaseProvider.LeaseProviderDebugReportsCandidateResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonLeaseProviderDebugReportsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("毒牙"), 0);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = { WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang) };

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("DebugLease"));
	Request.AllowedCardDefinitions.Add(Fang);
	FWacomRunMenuCardLeaseResult Result;
	TestTrue(TEXT("Provider sets lease"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, Result));

	const FWacomRunFirstPersonCardSourceDebugView Debug =
		Source->GetRunFirstPersonCardSourceDebugView();
	TestEqual(TEXT("Debug stores provider lease id"),
		Debug.LastMenuLeaseProviderLeaseId,
		Request.LeaseId);
	TestEqual(TEXT("Debug stores provider source id"),
		Debug.LastMenuLeaseProviderSourceId,
		Request.SourceId);
	TestEqual(TEXT("Debug stores candidate count"),
		Debug.LastMenuLeaseProviderCandidateCount,
		1);
	TestTrue(TEXT("Debug summary includes provider result"),
		Source->GetRunFirstPersonCardSourceDebugSummary().Contains(TEXT("Provider{")));
	TestTrue(TEXT("Result summary includes candidate count"),
		Result.DebugSummary.Contains(TEXT("Candidates=1")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonDefaultBattleDeckEnablesRunWorldDragSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.DefaultBattleDeckSourceEnablesRunWorldDrag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonDefaultBattleDeckEnablesRunWorldDragSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.DefaultInteraction"), TEXT("Default Interaction Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());

	Source->SetRunFirstPersonCardLayerActive(true);
	TestTrue(TEXT("Default Run BattleDeck source enables run-world drag probe"),
		Anchor->IsFirstPersonCardLayerInteractionEnabled());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonDragUpdateOverMenuZoneReportsTargetSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.DragUpdateOverMenuZoneReportsZoneTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonDragUpdateOverMenuZoneReportsTargetSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomInteractionTargetHandle Handle;
	TestTrue(TEXT("Menu zone is probed"),
		FWacomPlayerControllerRunInteractionTestAccess::ProbeRunMenuDropTargetAtWidgetPosition(PC.Get(), FVector2D(100.0f, 200.0f), Handle));
	TestEqual(TEXT("Zone target id is reported"),
		Handle.ZoneId,
		FName(TEXT("RunEvent.Pay.Fang")));
	TestEqual(TEXT("Target receives probe preview"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Normal);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonDragReleaseOnMenuZoneProbeOnlySpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.DragReleaseOnMenuZoneProbeOnlyWhenMenuDoesNotAccept",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonDragReleaseOnMenuZoneProbeOnlySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* LeaseCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.RunEventLease.ProbeOnly"), TEXT("Lease"), 0);
	LeaseCard->TargetMode = ECardTargetMode::SingleEnemyPart;
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { LeaseCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	const FGuid LeaseCardInstanceId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), LeaseCard);
	TestTrue(TEXT("Lease card instance is valid"), LeaseCardInstanceId.IsValid());

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	UWacomRunFirstPersonCardSourceComponent* Source = PC->GetRunFirstPersonCardSourceComponent();
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	TStrongObjectPtr<UWacomShopScreenProbe> ActiveMenu(NewObject<UWacomShopScreenProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());

	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is accepted"),
		WacomRunFirstPersonCardLayerSpec::SetDefinitionLease(
			*PC.Get(),
			TEXT("RunEventLease"),
			LeaseCard,
			LeaseResult));
	TestTrue(TEXT("Lease result is set"), LeaseResult.bLeaseSet);

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = LeaseCardInstanceId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());
	FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(PC.Get(), LeaseCardInstanceId, DragView, /*bReleased*/ true);

	const FString Debug = FWacomPlayerControllerRunInteractionTestAccess::RunMenuDropProbeDebugSummary(PC.Get());
	TestTrue(TEXT("Release is probe-only"),
		Debug.Contains(TEXT("Intent=ProbeZoneTarget")));
	TestTrue(TEXT("Debug includes zone id"),
		Debug.Contains(TEXT("ZoneId=RunEvent.Pay.Fang")));

	if (Source)
	{
		TestTrue(TEXT("Run source lease still exists after probe release"),
			Source->HasActiveMenuLease());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonMenuDropProbeClearSpec,
	"Wacom.UI.RunFirstPersonCardLayer.MenuContext.DragCancelLeaseClearMenuCloseClearsZonePreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonMenuDropProbeClearSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* LeaseCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.RunEventLease.ProbeClear"), TEXT("Lease"), 0);
	LeaseCard->TargetMode = ECardTargetMode::SingleEnemyPart;
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { LeaseCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	const FGuid LeaseCardInstanceId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), LeaseCard);
	TestTrue(TEXT("Lease card instance is valid"), LeaseCardInstanceId.IsValid());

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	TStrongObjectPtr<UWacomShopScreenProbe> ActiveMenu(NewObject<UWacomShopScreenProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Clear.Zone");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());

	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is accepted"),
		WacomRunFirstPersonCardLayerSpec::SetDefinitionLease(
			*PC.Get(),
			TEXT("RunEventLease"),
			LeaseCard,
			LeaseResult));
	TestTrue(TEXT("Lease result is set"), LeaseResult.bLeaseSet);
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = LeaseCardInstanceId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;
	FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(PC.Get(), LeaseCardInstanceId, DragView, /*bReleased*/ false);
	TestEqual(TEXT("Probe preview is active before clear"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Probe);

	FWacomPlayerControllerRunInteractionTestAccess::ClearRunMenuDropTargetProbe(PC.Get());

	TestEqual(TEXT("Preview clears"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Normal);
	TestTrue(TEXT("Debug reports clear"),
		FWacomPlayerControllerRunInteractionTestAccess::RunMenuDropProbeDebugSummary(PC.Get()).Contains(TEXT("Cleared")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropRejectsWithoutLeaseSpec,
	"Wacom.UI.RunMenuCardDropIntent.ResolveRejectsWithoutActiveMenuLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropRejectsWithoutLeaseSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = FGuid::NewGuid();
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	const FWacomRunMenuCardDropResolveResult Result =
		FWacomPlayerControllerRunInteractionTestAccess::ResolveRunMenuCardDropIntent(PC.Get(), DragView.CardInstanceId, DragView);
	TestEqual(TEXT("Intent rejects without lease"),
		Result.IntentKind,
		EWacomRunMenuCardDropIntentKind::Reject);
	TestEqual(TEXT("Reject reason reports missing lease"),
		Result.RejectReason,
		EWacomRunMenuCardDropRejectReason::MissingMenuLease);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropProbeOnlySpec,
	"Wacom.UI.RunMenuCardDropIntent.ResolveProbeOnlyWhenMenuReturnsProbe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropProbeOnlySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance found"), DragView.CardInstanceId.IsValid());
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	const FWacomRunMenuCardDropResolveResult Result =
		FWacomPlayerControllerRunInteractionTestAccess::ResolveRunMenuCardDropIntent(PC.Get(), DragView.CardInstanceId, DragView);
	TestEqual(TEXT("Intent remains probe-only"),
		Result.IntentKind,
		EWacomRunMenuCardDropIntentKind::ProbeZoneTarget);
	TestEqual(TEXT("Menu does not accept by default"),
		Result.RejectReason,
		EWacomRunMenuCardDropRejectReason::MenuDoesNotAccept);
	TestFalse(TEXT("Probe-only intent cannot submit"), Result.bCanSubmit);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropAcceptedZoneSpec,
	"Wacom.UI.RunMenuCardDropIntent.AcceptedZoneResolvesControllerDestroyOwnedCardPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropAcceptedZoneSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptRunMenuFirstPersonCardDropForTest = true;
	ActiveMenu->SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance found"), DragView.CardInstanceId.IsValid());
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	const FWacomRunMenuCardDropResolveResult Result =
		FWacomPlayerControllerRunInteractionTestAccess::ResolveRunMenuCardDropIntent(PC.Get(), DragView.CardInstanceId, DragView);
	TestEqual(TEXT("Accepted zone resolves submit intent"),
		Result.IntentKind,
		EWacomRunMenuCardDropIntentKind::SubmitZoneTarget);
	TestEqual(TEXT("Submit policy is controller destroy"),
		Result.SubmitPolicy,
		EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard);
	TestTrue(TEXT("Submit intent can submit"), Result.bCanSubmit);
	TestEqual(TEXT("Zone id preserved"),
		Result.ZoneId,
		FName(TEXT("RunEvent.Pay.Fang")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropReleaseDestroysSpec,
	"Wacom.UI.RunMenuCardDropIntent.ReleaseWithControllerDestroyPolicyDestroysExactOwnedInstance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropReleaseDestroysSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptRunMenuFirstPersonCardDropForTest = true;
	ActiveMenu->SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	const FGuid PaidId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance found"), PaidId.IsValid());
	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = PaidId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(PC.Get(), PaidId, DragView, /*bReleased*/ true);

	FCardInstance Found;
	EZoneKind FoundZone = EZoneKind::Backpack;
	FGuid FoundOwner;
	TestFalse(TEXT("Paid instance is removed from owned zones"),
		Run->FindInstance(PaidId, Found, FoundZone, FoundOwner));
	TestEqual(TEXT("Drop target shows submitted preview"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Submitted);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropMenuHandledDoesNotDefaultDestroySpec,
	"Wacom.UI.RunMenuCardDropIntent.ReleaseWithMenuHandledPolicyUsesMenuSubmitResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropMenuHandledDoesNotDefaultDestroySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptRunMenuFirstPersonCardDropForTest = true;
	ActiveMenu->SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::MenuHandled;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	const FGuid PaidId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance found"), PaidId.IsValid());
	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = PaidId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	FWacomRunMenuCardDropResolveResult Result =
		FWacomPlayerControllerRunInteractionTestAccess::ResolveRunMenuCardDropIntent(PC.Get(), PaidId, DragView);
	TestEqual(TEXT("Menu-handled drop resolves submit intent"),
		Result.IntentKind,
		EWacomRunMenuCardDropIntentKind::SubmitZoneTarget);
	TestEqual(TEXT("Submit policy is menu handled"),
		Result.SubmitPolicy,
		EWacomRunMenuCardDropSubmitPolicy::MenuHandled);

	FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(PC.Get(), PaidId, DragView, /*bReleased*/ true);

	FCardInstance Found;
	EZoneKind FoundZone = EZoneKind::Backpack;
	FGuid FoundOwner;
	TestTrue(TEXT("Default destroy path does not remove card"),
		Run->FindInstance(PaidId, Found, FoundZone, FoundOwner));
	TestTrue(TEXT("Menu submit result is recorded"),
		ActiveMenu->LastDropResultForTest.bSubmitted);
	TestEqual(TEXT("Menu receives menu-handled submit result"),
		ActiveMenu->LastDropResultForTest.SubmitPolicy,
		EWacomRunMenuCardDropSubmitPolicy::MenuHandled);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropMenuHandledFailureSpec,
	"Wacom.UI.RunMenuCardDropIntent.MenuHandledSubmitFailureShowsRejectWithoutDefaultDestroy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropMenuHandledFailureSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptRunMenuFirstPersonCardDropForTest = true;
	ActiveMenu->SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::MenuHandled;
	ActiveMenu->bMenuSubmitSucceedsForTest = false;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	const FGuid PaidId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance found"), PaidId.IsValid());
	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = PaidId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(PC.Get(), PaidId, DragView, /*bReleased*/ true);

	FCardInstance Found;
	EZoneKind FoundZone = EZoneKind::Backpack;
	FGuid FoundOwner;
	TestTrue(TEXT("Failed menu submit does not default destroy card"),
		Run->FindInstance(PaidId, Found, FoundZone, FoundOwner));
	TestFalse(TEXT("Menu submit result is failed"),
		ActiveMenu->LastDropResultForTest.bSubmitted);
	TestEqual(TEXT("Menu submit failure becomes reject"),
		ActiveMenu->LastDropResultForTest.RejectReason,
		EWacomRunMenuCardDropRejectReason::SubmitFailed);
	TestEqual(TEXT("Drop target shows invalid preview"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Invalid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonEconomyRevisionSkipsDefaultSnapshotSpec,
	"Wacom.UI.RunFirstPersonCardLayer.RunStateChangedWithEconomyOnlyRevisionSkipsDefaultSourceSnapshotBuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonEconomyRevisionSkipsDefaultSnapshotSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.EconomySkip"), TEXT("Economy Skip Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	const int32 WritesAfterActivate = Source->WriteCount;
	const uint64 StorageRevisionAfterActivate = Run->GetBackpackStorageSnapshotRevision();
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Run->AddGold(1);
	TestEqual(TEXT("Gold-only change does not bump storage revision"),
		Run->GetBackpackStorageSnapshotRevision(),
		StorageRevisionAfterActivate);
	TestEqual(TEXT("Economy-only notification skips default source write"),
		Source->WriteCount,
		WritesAfterActivate);
	TestEqual(TEXT("Economy-only notification reports revision skip"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("SkippedUnchangedRevision")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Economy-only skip increments skip count"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RevisionSkipCount,
		1);
	TestEqual(TEXT("Economy-only skip does not rebuild backpack snapshot"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).DataBuildCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonDefaultSourceRefreshKeyBreaksSkipSpec,
	"Wacom.UI.RunFirstPersonCardLayer.RunFirstPersonDefaultSourceRefreshKeyBreaksSkipWhenSourceIdOrProjectedFlagChanges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonDefaultSourceRefreshKeyBreaksSkipSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.DefaultKey"), TEXT("Default Key Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	const int32 WritesAfterActivate = Source->WriteCount;
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Source->RunFirstPersonCardLayerSourceId = TEXT("RunFirstPersonChangedSource");
	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Source id change refreshes despite unchanged storage revision"),
		Source->WriteCount,
		WritesAfterActivate + 1);
	TestEqual(TEXT("Changed source id is written"),
		Source->LastWrittenSourceId,
		FName(TEXT("RunFirstPersonChangedSource")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Source id change rebuilds default data"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Source id change applies default source"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RuntimeApplyCount,
		1);
	TestEqual(TEXT("Source id change does not count as skip"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RevisionSkipCount,
		0);

	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	const int32 WritesAfterSourceChange = Source->WriteCount;
	Source->bIncludeProjectedRunBattleDeckCards =
		!Source->bIncludeProjectedRunBattleDeckCards;
	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Projected include flag change refreshes despite unchanged storage revision"),
		Source->WriteCount,
		WritesAfterSourceChange + 1);
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Projected include flag change rebuilds default data"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Projected include flag change applies default source"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RuntimeApplyCount,
		1);
	TestEqual(TEXT("Projected include flag change does not count as skip"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RevisionSkipCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseUnchangedRevisionSkipsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ProviderBackedLeaseRunStateChangedWithUnchangedStorageRevisionSkipsRebuildAndRewrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseUnchangedRevisionSkipsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderSkipLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	const int32 WritesAfterLease = Source->WriteCount;
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Unchanged storage revision skips provider source rewrite"),
		Source->WriteCount,
		WritesAfterLease);
	TestEqual(TEXT("Provider skip is reported"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("MenuLeaseProviderSkippedUnchangedRevision")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Provider skip count increments"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		1);
	TestEqual(TEXT("Provider candidate rebuild is skipped"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).DataBuildCount,
		0);
	TestEqual(TEXT("Provider runtime apply is skipped"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RuntimeApplyCount,
		0);
#endif
	TestEqual(TEXT("Candidate count is preserved"),
		Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseEntryCount,
		1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseRefreshKeyBreaksSkipSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ProviderBackedLeaseRefreshKeyBreaksSkipWhenProviderRequestChanges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseRefreshKeyBreaksSkipSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCardDefinition* Other = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Other"), TEXT("Other"), 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang, Other });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderRequestKeyLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	const int32 WritesAfterLease = Source->WriteCount;
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);

	FWacomRunMenuCardLeaseRequest ChangedRequest = Request;
	ChangedRequest.AllowedCardIds.Reset();
	ChangedRequest.AllowedCardIds.Add(TEXT("Other"));
	FWacomFirstPersonCardLayerTestAccess::SetActiveProviderLeaseRequest(*Source, ChangedRequest);
#endif

	Run->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("Provider request change refreshes despite unchanged storage revision"),
		Source->WriteCount,
		WritesAfterLease + 1);
	TestEqual(TEXT("Changed provider request reports other card"),
		Source->LastWrittenEntries[0].CardViewData.Name.ToString(),
		FString(TEXT("Other")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Provider request change rebuilds candidates"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Provider request change applies runtime source"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RuntimeApplyCount,
		1);
	TestEqual(TEXT("Provider request change does not count as skip"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseEconomyRevisionSkipsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ProviderBackedLeaseEconomyOnlyRevisionSkipsCandidateRebuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseEconomyRevisionSkipsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderEconomySkipLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	const int32 WritesAfterLease = Source->WriteCount;
	const uint64 StorageRevisionAfterLease = Run->GetBackpackStorageSnapshotRevision();
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Run->AddGold(1);
	TestEqual(TEXT("Economy-only mutation does not bump storage revision"),
		Run->GetBackpackStorageSnapshotRevision(),
		StorageRevisionAfterLease);
	TestEqual(TEXT("Economy-only notification skips provider rewrite"),
		Source->WriteCount,
		WritesAfterLease);
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Economy-only provider skip increments skip count"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		1);
	TestEqual(TEXT("Economy-only provider skip avoids rebuild"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).DataBuildCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseStorageRevisionRefreshesSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ProviderBackedLeaseStorageRevisionRefreshesCandidates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseStorageRevisionRefreshesSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderStorageRefreshLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	const int32 WritesAfterLease = Source->WriteCount;
	const uint64 StorageRevisionAfterLease = Run->GetBackpackStorageSnapshotRevision();
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Run->AcquireCardToRun(Fang);
	TestTrue(TEXT("Storage mutation bumps storage revision"),
		Run->GetBackpackStorageSnapshotRevision() > StorageRevisionAfterLease);
	TestEqual(TEXT("Provider source rewrites after storage revision change"),
		Source->WriteCount,
		WritesAfterLease + 1);
	TestEqual(TEXT("Provider candidate count refreshes"),
		Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseEntryCount,
		2);
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Provider rebuild happens once"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Provider apply happens once"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RuntimeApplyCount,
		1);
	TestEqual(TEXT("Storage revision refresh does not count as skip"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseUnchangedCandidatesUseStateRefreshSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ProviderBackedLeaseStorageRevisionWithoutNewCandidatesUsesStateRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseUnchangedCandidatesUseStateRefreshSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCardDefinition* Other = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Other"), TEXT("Other"), 1);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderStateRefreshLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	const int32 WritesAfterLease = Source->WriteCount;
	const uint64 StorageRevisionAfterLease = Run->GetBackpackStorageSnapshotRevision();
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Run->AcquireCardToRun(Other);
	TestTrue(TEXT("Storage mutation bumps storage revision"),
		Run->GetBackpackStorageSnapshotRevision() > StorageRevisionAfterLease);
	TestEqual(TEXT("Provider source rewrites after storage revision change"),
		Source->WriteCount,
		WritesAfterLease + 1);
	TestEqual(TEXT("Provider candidate count remains unchanged"),
		Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseEntryCount,
		1);
	TestEqual(TEXT("Unchanged candidates use state refresh commit"),
		Source->LastWrittenCommitMode,
		EWacomFirstPersonCardLayerFrameCommitMode::StateRefresh);
	TestEqual(TEXT("Unchanged candidates do not replay RunHandEntered"),
		Source->LastWrittenTransitionHints.Num(),
		0);
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Provider rebuild happens once"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Provider apply happens once"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RuntimeApplyCount,
		1);
	TestEqual(TEXT("Storage revision refresh does not count as skip"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseStorageRevisionCanClearSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ProviderBackedLeaseStorageRevisionCanClearWhenNoCandidatesRemain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseStorageRevisionCanClearSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderClearLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	const FGuid PaidId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance exists"), PaidId.IsValid());
	Request.ExplicitCardInstanceIds.Add(PaidId);
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	TestTrue(TEXT("Destroying the only candidate succeeds"),
		Run->DestroyCardByInstance(PaidId));
	TestFalse(TEXT("Provider lease clears when no candidates remain"),
		Source->HasActiveMenuLease());
	TestEqual(TEXT("Provider reports no candidates"),
		Source->GetRunFirstPersonCardSourceDebugView().LastMenuLeaseProviderResult,
		FName(TEXT("NoMatchingCandidates")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Clear path still rebuilds candidates"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Clear path does not skip"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseRequestOrSessionResetsGateSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ProviderBackedLeaseRequestOrRunSessionSwitchResetsRevisionGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseRequestOrSessionResetsGateSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCardDefinition* Other = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Other"), TEXT("Other"), 1);
	UCharacterDefinition* FirstCharacter = Fx.MakeCharacter(nullptr, nullptr, { Fang, Other });
	UCharacterDefinition* SecondCharacter = Fx.MakeCharacter(nullptr, nullptr, { Other });

	TStrongObjectPtr<URunSession> FirstRun(NewObject<URunSession>());
	TStrongObjectPtr<URunSession> SecondRun(NewObject<URunSession>());
	TestTrue(TEXT("First run initializes"), FirstRun->Initialize(FirstCharacter));
	TestTrue(TEXT("Second run initializes"), SecondRun->Initialize(SecondCharacter));
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(FirstRun.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseRequest FirstRequest =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderResetLease"));
	FirstRequest.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("First provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(FirstRequest, LeaseResult));
	const int32 WritesAfterFirstLease = Source->WriteCount;
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	FWacomRunMenuCardLeaseRequest ChangedRequest = FirstRequest;
	ChangedRequest.AllowedCardIds.Reset();
	ChangedRequest.AllowedCardIds.Add(TEXT("Other"));
	TestTrue(TEXT("Changed provider request refreshes same lease"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(ChangedRequest, LeaseResult));
	TestEqual(TEXT("Changed provider request rewrites active lease"),
		Source->WriteCount,
		WritesAfterFirstLease + 1);
	TestEqual(TEXT("Changed provider request reports other card"),
		Source->LastWrittenEntries[0].CardViewData.Name.ToString(),
		FString(TEXT("Other")));

	Source->BindRunSession(SecondRun.Get());
	TestEqual(TEXT("RunSession switch keeps active provider lease"),
		Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseId,
		FirstRequest.LeaseId);
	TestEqual(TEXT("RunSession switch refreshes provider entries from new run"),
		Source->LastWrittenEntries[0].CardViewData.Name.ToString(),
		FString(TEXT("Other")));
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	SecondRun->OnRunStateChangedNative.Broadcast();
	TestEqual(TEXT("New session stored provider key allows later unchanged skip"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("MenuLeaseProviderSkippedUnchangedRevision")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("New session skip count increments after key is restablished"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		1);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonProviderLeaseManualAndSuppressionBypassSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ManualProviderLeaseSetAndSuppressionReleaseBypassRevisionGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonProviderLeaseManualAndSuppressionBypassSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);

	FWacomRunMenuCardLeaseRequest Request =
		WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("ProviderManualBypassLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Provider lease is set"),
		Source->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	const int32 WritesAfterLease = Source->WriteCount;
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	TestTrue(TEXT("Manual refresh succeeds"),
		Source->RefreshRunFirstPersonCardLayer());
	TestEqual(TEXT("Manual refresh rewrites provider source"),
		Source->WriteCount,
		WritesAfterLease + 1);
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Manual refresh rebuilds provider lease"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Manual refresh applies provider source"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RuntimeApplyCount,
		1);
	TestEqual(TEXT("Manual refresh does not count as provider skip"),
		FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(*Source).RevisionSkipCount,
		0);
#endif

	TestTrue(TEXT("Clearing provider lease succeeds"),
		Source->ClearRunFirstPersonCardLayerMenuLease(Request.LeaseId));
	TestFalse(TEXT("Suppressed state remains active after lease clear"),
		Source->HasActiveMenuLease());
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(false);
	TestEqual(TEXT("Suppression release restores default source"),
		Source->LastWrittenSourceId,
		Source->RunFirstPersonCardLayerSourceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonStorageRevisionRefreshesDefaultSourceSpec,
	"Wacom.UI.RunFirstPersonCardLayer.RunStateChangedWithStorageRevisionRefreshesDefaultSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonStorageRevisionRefreshesDefaultSourceSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.StorageRefresh"), TEXT("Storage Refresh Card"), 1);
	UCardDefinition* NewCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.StorageRefresh.New"), TEXT("Storage Refresh New Card"), 0);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 3);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	const int32 WritesAfterActivate = Source->WriteCount;
	const uint64 StorageRevisionAfterActivate = Run->GetBackpackStorageSnapshotRevision();
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	Run->AcquireCardToRun(NewCard);
	TestTrue(TEXT("Storage mutation bumps storage revision"),
		Run->GetBackpackStorageSnapshotRevision() > StorageRevisionAfterActivate);
	TestEqual(TEXT("Storage revision change refreshes default source"),
		Source->WriteCount,
		WritesAfterActivate + 1);
	TestEqual(TEXT("Storage refresh reports refreshed"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("Refreshed")));
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Storage refresh rebuilds snapshot once"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Storage refresh applies runtime source once"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RuntimeApplyCount,
		1);
	TestEqual(TEXT("Storage refresh does not count as skip"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RevisionSkipCount,
		0);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonRunSessionSwitchResetsRevisionGateSpec,
	"Wacom.UI.RunFirstPersonCardLayer.RunSessionSwitchResetsRunFirstPersonSourceRevisionGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonRunSessionSwitchResetsRevisionGateSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* FirstCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.SessionA"), TEXT("Session A Card"), 1);
	UCardDefinition* SecondCard = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.SessionB"), TEXT("Session B Card"), 2);
	UCardDefinition* FirstPack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCardDefinition* SecondPack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* FirstCharacter = Fx.MakeCharacter(nullptr, nullptr, { FirstCard, FirstPack });
	UCharacterDefinition* SecondCharacter = Fx.MakeCharacter(nullptr, nullptr, { SecondCard, SecondPack });

	TStrongObjectPtr<URunSession> FirstRun(NewObject<URunSession>());
	TStrongObjectPtr<URunSession> SecondRun(NewObject<URunSession>());
	TestTrue(TEXT("First run initializes"), FirstRun->Initialize(FirstCharacter));
	TestTrue(TEXT("Second run initializes"), SecondRun->Initialize(SecondCharacter));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(FirstRun.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	const int32 WritesAfterFirstRun = Source->WriteCount;

	Source->BindRunSession(SecondRun.Get());
	TestEqual(TEXT("RunSession switch forces a new default source write"),
		Source->WriteCount,
		WritesAfterFirstRun + 1);
	TestEqual(TEXT("Switched run writes one card"),
		Source->LastWrittenEntries.Num(),
		1);
	TestEqual(TEXT("Switched run writes its own card"),
		Source->LastWrittenEntries[0].CardViewData.Name.ToString(),
		FString(TEXT("Session B Card")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonManualAndSuppressionBypassRevisionGateSpec,
	"Wacom.UI.RunFirstPersonCardLayer.ManualRefreshAndSuppressionReleaseBypassRevisionGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonManualAndSuppressionBypassRevisionGateSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.ManualBypass"), TEXT("Manual Bypass Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Card, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	const int32 WritesAfterActivate = Source->WriteCount;
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(*Source);
#endif

	TestTrue(TEXT("Manual refresh still succeeds"),
		Source->RefreshRunFirstPersonCardLayer());
	TestEqual(TEXT("Manual refresh bypasses revision skip"),
		Source->WriteCount,
		WritesAfterActivate + 1);
	TestEqual(TEXT("Manual refresh does not replay unchanged Run hand enter"),
		Source->LastWrittenTransitionHints.Num(),
		0);
#if WITH_AUTOMATION_TESTS
	TestEqual(TEXT("Manual refresh rebuilds snapshot"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).DataBuildCount,
		1);
	TestEqual(TEXT("Manual refresh does not count as revision skip"),
		FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(*Source).RevisionSkipCount,
		0);
#endif

	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(true);
	const int32 WritesWhileSuppressed = Source->WriteCount;
	Source->SetRunFirstPersonCardLayerSuppressedByGameMenu(false);
	TestEqual(TEXT("Suppression release forces default source write"),
		Source->WriteCount,
		WritesWhileSuppressed + 1);
	TestEqual(TEXT("Suppression release reports refreshed"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("Refreshed")));
	TestEqual(TEXT("Suppression release replays Run hand enter"),
		Source->LastWrittenTransitionHints.Num(),
		1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunFirstPersonNewDefaultCardGetsRunHandEnterSpec,
	"Wacom.UI.RunFirstPersonCardLayer.PresentationFrame.NewDefaultCardGetsRunHandEnterOnlyForNewCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunFirstPersonNewDefaultCardGetsRunHandEnterSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Initial = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.RunEnter.Initial"), TEXT("Initial Run Card"), 1);
	UCardDefinition* Extra = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("Test.RunEnter.Extra"), TEXT("Extra Run Card"), 1);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Initial, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);
	TestEqual(TEXT("Initial source animates one card"),
		Source->LastWrittenTransitionHints.Num(),
		1);

	Run->AcquireCardToRun(Extra);
	const FGuid ExtraInstanceId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(
			Run->GetRunState(),
			Extra);
	TestTrue(TEXT("Extra card is acquired"), ExtraInstanceId.IsValid());
	TestEqual(TEXT("Backpack acquire does not animate default BattleDeck source"),
		Source->LastWrittenTransitionHints.Num(),
		0);

	const int32 WritesAfterAcquire = Source->WriteCount;
	TestTrue(TEXT("Extra card moves to BattleDeck"),
		Run->MoveInstance(ExtraInstanceId, EZoneKind::BattleDeck, FGuid()));
	TestEqual(TEXT("BattleDeck move refreshes default source"),
		Source->WriteCount,
		WritesAfterAcquire + 1);
	TestEqual(TEXT("Default source now has two cards"),
		Source->LastWrittenEntries.Num(),
		2);
	TestEqual(TEXT("Only new BattleDeck card gets enter hint"),
		Source->LastWrittenTransitionHints.Num(),
		1);
	if (Source->LastWrittenTransitionHints.Num() == 1)
	{
		TestEqual(TEXT("New card hint uses Run hand enter"),
			Source->LastWrittenTransitionHints[0].TransitionKind,
			EWacomFirstPersonCardSlotTransitionKind::RunHandEntered);
		TestEqual(TEXT("New card hint targets moved card"),
			Source->LastWrittenTransitionHints[0].CardInstanceId,
			ExtraInstanceId);
		TestEqual(TEXT("New card enters as first item in its animation batch"),
			Source->LastWrittenTransitionHints[0].SequenceIndex,
			0);
		TestEqual(TEXT("New card animation batch count"),
			Source->LastWrittenTransitionHints[0].SequenceCount,
			1);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropValidationFailsSpec,
	"Wacom.UI.RunMenuCardDropIntent.ReleaseRejectsWhenRunSessionValidationFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropValidationFailsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptRunMenuFirstPersonCardDropForTest = true;
	ActiveMenu->SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	const FGuid PaidId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance found"), PaidId.IsValid());
	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	Request.ExplicitCardInstanceIds.Add(PaidId);
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = PaidId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;

	const FWacomRunMenuCardDropResolveResult Result =
		FWacomPlayerControllerRunInteractionTestAccess::ResolveRunMenuCardDropIntent(PC.Get(), PaidId, DragView);
	TestEqual(TEXT("Missing owned card rejects"),
		Result.IntentKind,
		EWacomRunMenuCardDropIntentKind::Reject);
	TestEqual(TEXT("Reject reason is card not owned"),
		Result.RejectReason,
		EWacomRunMenuCardDropRejectReason::CardNotOwned);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropRefreshesLeaseSpec,
	"Wacom.UI.RunMenuCardDropIntent.PaidCardRefreshesProviderBackedLeaseEntries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropRefreshesLeaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCardDefinition* Pack = WacomRunFirstPersonCardLayerSpec::MakeTypeAContainerCard(Fx, 2);
	UCharacterDefinition* Character = Fx.MakeCharacter(nullptr, nullptr, { Fang, Fang, Pack });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
	WacomRunFirstPersonCardLayerSpec::ResetRunOwnedZones(State);
	State.Backpack = {
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang),
		WacomRunFirstPersonCardLayerSpec::MakeRunCardInstance(Fang)
	};

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptRunMenuFirstPersonCardDropForTest = true;
	ActiveMenu->SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	Request.ExplicitCardInstanceIds.Add(State.Backpack[0].InstanceId);
	Request.ExplicitCardInstanceIds.Add(State.Backpack[1].InstanceId);
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());
	UWacomRunFirstPersonCardSourceComponent* Source = PC->GetRunFirstPersonCardSourceComponent();
	TestTrue(TEXT("Lease has two candidates"),
		Source && Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseEntryCount == 2);

	const FGuid PaidId = State.Backpack[0].InstanceId;
	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = PaidId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;
	FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(PC.Get(), PaidId, DragView, /*bReleased*/ true);

	TestTrue(TEXT("Provider-backed lease remains active"),
		Source && Source->HasActiveMenuLease());
	TestEqual(TEXT("Lease refreshes to remaining candidate"),
		Source->GetRunFirstPersonCardSourceDebugView().ActiveMenuLeaseEntryCount,
		1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuCardDropNoCandidatesClearsLeaseSpec,
	"Wacom.UI.RunMenuCardDropIntent.NoRemainingCandidatesClearsProviderBackedLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuCardDropNoCandidatesClearsLeaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = WacomRunFirstPersonCardLayerSpec::MakeNamedNoopCard(
		Fx, TEXT("PoisonFang"), TEXT("Poison Fang"), 0);
	UCharacterDefinition* Character = WacomRunFirstPersonCardLayerSpec::MakePaymentTestCharacter(Fx, Fang);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC.Get(), Run.Get());
	WacomRunFirstPersonCardLayerSpec::AttachFirstPersonPawnForTest(PC.Get());
	PC->SetRunFirstPersonCardLayerActive(true);
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> ActiveMenu(NewObject<UWacomMenuWidgetBaseProbe>(PC.Get()));
	ActiveMenu->SetOwningWacomPlayerControllerForTest(PC.Get());
	ActiveMenu->bAcceptRunMenuFirstPersonCardDropForTest = true;
	ActiveMenu->SubmitPolicyForTest =
		EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard;
	ActiveMenu->AcceptedZoneIdForTest = TEXT("RunEvent.Pay.Fang");
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Target(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(Target.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(PC.Get(), Target.Get());

	FWacomRunMenuCardLeaseRequest Request = WacomRunFirstPersonCardLayerSpec::MakeLeaseRequest(TEXT("RunEventLease"));
	Request.AllowedCardIds.Add(TEXT("PoisonFang"));
	const FGuid PaidId =
		WacomRunFirstPersonCardLayerSpec::FindOwnedInstanceIdByDefinition(Run->GetRunState(), Fang);
	TestTrue(TEXT("Fang instance found"), PaidId.IsValid());
	Request.ExplicitCardInstanceIds.Add(PaidId);
	FWacomRunMenuCardLeaseResult LeaseResult;
	TestTrue(TEXT("Lease is set"),
		ActiveMenu->SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult));
	PC->RegisterActiveGameMenuWidget(ActiveMenu.Get());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = PaidId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.CurrentScreenPosition = FVector2D(100.0f, 200.0f);
	DragView.PointerViewportPosition = FVector2D(100.0f, 200.0f);
	DragView.bHasPointerViewportPosition = true;
	FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(PC.Get(), PaidId, DragView, /*bReleased*/ true);

	UWacomRunFirstPersonCardSourceComponent* Source = PC->GetRunFirstPersonCardSourceComponent();
	TestTrue(TEXT("Source exists"), Source != nullptr);
	TestFalse(TEXT("Provider-backed lease clears after last candidate"),
		Source->HasActiveMenuLease());
	TestEqual(TEXT("Provider reports no candidates"),
		Source->GetRunFirstPersonCardSourceDebugView().LastMenuLeaseProviderResult,
		FName(TEXT("NoMatchingCandidates")));

	return true;
}
