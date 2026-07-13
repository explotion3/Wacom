// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "Blueprint/WidgetTree.h"
#include "Cards/CardDefinition.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"

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

	const TArray<FWacomBackpackCarriedFanLayout> DefaultFan =
		FWacomBackpackWorkspaceLayoutSolver::BuildCarriedFanLayout(
			15, 14, 14, FVector2D(600.0f, 400.0f), 36.0f, 72.0f, 56.0f);
	TestEqual(TEXT("Fan returns one transform per carried card"), DefaultFan.Num(), 15);
	TestTrue(TEXT("Rightmost card is current"), DefaultFan[14].bCurrent);
	TestFalse(TEXT("Default current is not lifted"), DefaultFan[14].bLifted);
	TestEqual(TEXT("Rightmost card has highest stable Z"), DefaultFan[14].Transform.LayerRank, 14);

	Model.StepCurrentByWheel(1.0f);
	TestEqual(TEXT("Wheel up moves current left"), Model.GetCarry().CurrentIndex, 13);
	const TArray<FWacomBackpackCarriedFanLayout> LiftedFan =
		FWacomBackpackWorkspaceLayoutSolver::BuildCarriedFanLayout(
			15, 13, 14, FVector2D(600.0f, 400.0f), 36.0f, 72.0f, 56.0f);
	TestTrue(TEXT("Only non-default current lifts"), LiftedFan[13].bLifted);
	TestFalse(TEXT("Default card remains unlifted after wheel"), LiftedFan[14].bLifted);
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
	TestEqual(TEXT("Single release leaves stable remaining fan"), Model.GetCarry().RemainingInstanceIds.Num(), 14);
	TestEqual(TEXT("Remaining default/current clamp to new rightmost"), Model.GetCarry().CurrentIndex, 13);
	const FWacomBackpackWorkspaceReleaseIntent All = Model.BuildReleaseIntent(true);
	TestEqual(TEXT("Right release targets every remaining card"), All.InstanceIds.Num(), 14);
	Model.CommitReleasedCards(All.InstanceIds);
	TestFalse(TEXT("Successful all release exits carry"), Model.IsCarrying());

	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(NewObject<UWacomBackpackWorkspaceWidget>());
	Workspace->TakeWidget();
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
	PassiveCard->UnbindWorkspacePointerEvents();
	TestFalse(TEXT("Card can release all workspace input forwarding on reuse"), PassiveCard->OnWorkspacePointerDownNative.IsBound());
	return true;
}

#endif
