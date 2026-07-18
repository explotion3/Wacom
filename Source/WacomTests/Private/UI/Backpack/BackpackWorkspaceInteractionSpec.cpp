// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "Blueprint/WidgetTree.h"
#include "Cards/CardDefinition.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "../BackpackScreenTestAccess.h"

namespace
{
void PrepareCarryModel(
	FWacomBackpackWorkspaceInteractionModel& Model,
	TArray<FWacomBackpackWorkspaceCardHitRecord>& OutCards,
	int32 CardCount)
{
	OutCards.Reset();
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		FWacomBackpackWorkspaceCardHitRecord Card;
		Card.InstanceId = FGuid(Index + 1, 10, 20, 30);
		Card.CardCenter = FVector2D(100.0f + Index * 30.0f, 200.0f);
		Card.LayerRank = Index;
		OutCards.Add(Card);
	}
	Model.ReconcileCards(FWacomBackpackZoneKey::Make(EZoneKind::Backpack), OutCards);
	Model.SelectAllMovable();
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspacePersistentCarrySpec,
	"Wacom.UI.Backpack.Workspace.PersistentCarryContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspacePersistentCarrySpec::RunTest(const FString& Parameters)
{
	int32 MisplacedDuringPickup = 0;
	for (int32 Repetition = 0; Repetition < 50; ++Repetition)
	{
		FWacomBackpackWorkspaceInteractionModel RepeatedModel;
		TArray<FWacomBackpackWorkspaceCardHitRecord> RepeatedCards;
		PrepareCarryModel(RepeatedModel, RepeatedCards, 15);
		TestTrue(TEXT("Selected cards enter carry"),
			RepeatedModel.BeginCarry(RepeatedCards[3].InstanceId, FVector2D(600.0f, 400.0f), 42));
		const FWacomBackpackWorkspaceReleaseIntent PickupRelease = RepeatedModel.BuildReleaseIntent(false);
		MisplacedDuringPickup += PickupRelease.InstanceIds.Num();
		TestTrue(TEXT("Pickup release is consumed by initial guard"), PickupRelease.bConsumedByInitialReleaseGuard);
		TestTrue(TEXT("Pickup release leaves carry active"), RepeatedModel.IsCarrying());
	}
	TestEqual(TEXT("Fifty pickup releases place zero cards"), MisplacedDuringPickup, 0);

	FWacomBackpackWorkspaceInteractionModel Model;
	TArray<FWacomBackpackWorkspaceCardHitRecord> Cards;
	PrepareCarryModel(Model, Cards, 15);
	TestTrue(TEXT("Carry begins"), Model.BeginCarry(Cards[0].InstanceId, FVector2D(600.0f, 400.0f), 77));
	TestEqual(TEXT("Default current is rightmost card"), Model.GetCarry().DefaultIndex, 14);
	TestEqual(TEXT("Current starts at default rightmost card"), Model.GetCarry().CurrentIndex, 14);

	const TArray<FWacomBackpackCarriedStripLayout> DefaultStrip =
		FWacomBackpackWorkspaceLayoutSolver::BuildCarriedFocusWindowLayout(
			15, 14, 14, FVector2D(600.0f, 400.0f), 1280.0f, 296.0f,
			5, 24.0f, 56.0f, 16.0f, 56.0f);
	TestEqual(TEXT("Strip returns one transform per carried card"), DefaultStrip.Num(), 15);
	for (const FWacomBackpackCarriedStripLayout& Card : DefaultStrip)
	{
		TestTrue(TEXT("Carried cards use a zero-rotation horizontal strip"),
			FMath::IsNearlyZero(Card.Transform.AngleDegrees));
		TestTrue(TEXT("Non-current carried cards share one horizontal baseline"),
			Card.bCurrent || FMath::IsNearlyEqual(Card.Transform.CardCenter.Y, 400.0f));
	}
	TestTrue(TEXT("Rightmost card is current"), DefaultStrip[14].bCurrent);
	TestFalse(TEXT("Default current is not lifted"), DefaultStrip[14].bLifted);
	TestTrue(TEXT("Default current stays anchored to the pointer"),
		DefaultStrip[14].Transform.CardCenter.Equals(FVector2D(600.0f, 400.0f), 0.1f));
	TestTrue(TEXT("Rightmost current card has highest stable Z"),
		DefaultStrip[14].Transform.LayerRank > DefaultStrip[13].Transform.LayerRank);

	Model.StepCurrentByWheel(1.0f);
	TestEqual(TEXT("Wheel up moves current left"), Model.GetCarry().CurrentIndex, 13);
	const TArray<FWacomBackpackCarriedStripLayout> LiftedStrip =
		FWacomBackpackWorkspaceLayoutSolver::BuildCarriedFocusWindowLayout(
			15, 13, 14, FVector2D(600.0f, 400.0f), 1280.0f, 296.0f,
			5, 24.0f, 56.0f, 16.0f, 56.0f);
	TestTrue(TEXT("Only non-default current lifts"), LiftedStrip[13].bLifted);
	TestFalse(TEXT("Default card remains unlifted after wheel"), LiftedStrip[14].bLifted);
	TestTrue(TEXT("Wheel-selected current remains horizontally anchored to the pointer"),
		FMath::IsNearlyEqual(LiftedStrip[13].Transform.CardCenter.X, 600.0f, 0.1f));
	for (int32 Index = 0; Index < 30; ++Index)
	{
		Model.StepCurrentByWheel(1.0f);
	}
	TestEqual(TEXT("Wheel clamps at left edge without wrap"), Model.GetCarry().CurrentIndex, 0);
	for (int32 Index = 0; Index < 30; ++Index)
	{
		Model.StepCurrentByWheel(-1.0f);
	}
	TestEqual(TEXT("Wheel clamps at right edge without wrap"), Model.GetCarry().CurrentIndex, 14);

	const FWacomBackpackWorkspaceReleaseIntent Guard = Model.BuildReleaseIntent(false);
	TestTrue(TEXT("First release consumes guard"), Guard.bConsumedByInitialReleaseGuard);
	const FWacomBackpackWorkspaceReleaseIntent Single = Model.BuildReleaseIntent(false);
	TestEqual(TEXT("Later left release targets current only"), Single.InstanceIds, TArray<FGuid>{ Cards[14].InstanceId });
	Model.CommitReleasedCards(Single.InstanceIds);
	TestEqual(TEXT("Single release leaves stable remaining strip"), Model.GetCarry().RemainingInstanceIds.Num(), 14);
	TestEqual(TEXT("Remaining default/current clamp to new rightmost"), Model.GetCarry().CurrentIndex, 13);
	const FWacomBackpackWorkspaceReleaseIntent All = Model.BuildReleaseIntent(true);
	TestEqual(TEXT("Right release targets every remaining card"), All.InstanceIds.Num(), 14);
	Model.CommitReleasedCards(All.InstanceIds);
	TestFalse(TEXT("Successful all release exits carry"), Model.IsCarrying());

	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(NewObject<UWacomBackpackWorkspaceWidget>());
	const TSharedRef<SWidget> WorkspaceSlateWidget = Workspace->TakeWidget();
	TestNull(TEXT("Workspace exposes no carry index/count label"), Workspace->WidgetTree->FindWidget(TEXT("CarryIndexText")));

	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.Workspace.PassiveItem");
	FCardInstance Instance;
	Instance.InstanceId = FGuid::NewGuid();
	Instance.Definition = Definition.Get();
	TStrongObjectPtr<UWacomDeckCardWidget> PassiveCard(NewObject<UWacomDeckCardWidget>());
	PassiveCard->SetCard(Instance, EZoneKind::Backpack, FGuid());
	Workspace->SetInteractionModel(MakeShared<FWacomBackpackWorkspaceInteractionModel>(), nullptr);
	TArray<TObjectPtr<UWacomDeckCardWidget>> BoundCards{ PassiveCard.Get() };
	Workspace->BindWorkspaceCards(BoundCards, 1);
	TestTrue(TEXT("Workspace installs the only pointer-down owner"), PassiveCard->OnWorkspacePointerDownNative.IsBound());
	TestTrue(TEXT("Workspace installs the only pointer-move owner"), PassiveCard->OnWorkspacePointerMoveNative.IsBound());
	TestTrue(TEXT("Workspace installs the only pointer-up owner"), PassiveCard->OnWorkspacePointerUpNative.IsBound());
	const FWacomBackpackPickupPointerSequenceProbe PickupProbe =
		FWacomBackpackScreenTestAccess::ProbeCardPickupPointerSequence(*Workspace, *PassiveCard);
	TestTrue(TEXT("Pickup fixture card is movable"), PickupProbe.bCardMovable);
	TestTrue(TEXT("Pickup fixture card is selected before pointer down"),
		PickupProbe.bSelectedBeforePointerDown);
	TestTrue(TEXT("Pickup fixture sends a left-button pointer event"),
		PickupProbe.bPointerEventIsLeftMouseButton);
	TestFalse(TEXT("Pickup fixture does not hold Control"), PickupProbe.bPointerEventControlDown);
	TestFalse(TEXT("Pickup fixture starts outside carry"), PickupProbe.bCarryingBeforePointerDown);
	TestTrue(TEXT("Workspace handles pointer down on the selected card"), PickupProbe.bPointerDownHandled);
	TestTrue(TEXT("Pressing an already-selected card starts carry without pointer movement"),
		PickupProbe.bCarryStartedOnPointerDown);
	TestTrue(TEXT("Pickup release keeps the selected strip carried"), PickupProbe.bPickupReleaseKeptCarry);
	TestTrue(TEXT("Pickup release consumes the initial release guard"),
		PickupProbe.bInitialReleaseGuardCleared);
	TestEqual(TEXT("The next left release immediately targets the current card"),
		PickupProbe.NextLeftReleaseCount, 1);
	TestEqual(TEXT("The next right release immediately targets every carried card"),
		PickupProbe.NextRightReleaseCount, 1);
	TestEqual(TEXT("A new left click releases immediately even when pickup-up was lost"),
		PickupProbe.FirstLeftReleaseAfterMissedPickupUpCount, 1);
	TestEqual(TEXT("A new right click releases immediately even when pickup-up was lost"),
		PickupProbe.FirstRightReleaseAfterMissedPickupUpCount, 1);
	const FWacomBackpackPickupPointerSequenceProbe FirstClickPickupProbe =
		FWacomBackpackScreenTestAccess::ProbeCardPickupPointerSequence(
			*Workspace,
			*PassiveCard,
			false);
	TestFalse(TEXT("First-click pickup fixture starts with an unselected card"),
		FirstClickPickupProbe.bSelectedBeforePointerDown);
	TestTrue(TEXT("Pressing an unselected movable card starts carry without pointer movement"),
		FirstClickPickupProbe.bCarryStartedOnPointerDown);
	TestTrue(TEXT("The matching pickup release keeps the newly selected card carried"),
		FirstClickPickupProbe.bPickupReleaseKeptCarry);
	TestTrue(TEXT("The matching pickup release consumes the initial release guard"),
		FirstClickPickupProbe.bInitialReleaseGuardCleared);
	PassiveCard->UnbindWorkspacePointerEvents();
	TestFalse(TEXT("Card can release all workspace input forwarding on reuse"), PassiveCard->OnWorkspacePointerDownNative.IsBound());
	return true;
}

#endif
