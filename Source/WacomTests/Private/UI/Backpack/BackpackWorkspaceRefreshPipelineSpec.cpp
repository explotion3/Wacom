// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "Cards/CardDefinition.h"
#include "Components/CanvasPanel.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "../BackpackScreenTestAccess.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceRefreshPipelineHotPathSpec,
	"Wacom.UI.Backpack.Workspace.RefreshPipeline.TargetedHotPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceRefreshPipelineHotPathSpec::RunTest(
	const FString& Parameters)
{
	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	const TSharedRef<SWidget> WorkspaceSlate = Workspace->TakeWidget();
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Model =
		MakeShared<FWacomBackpackWorkspaceInteractionModel>();
	Workspace->SetInteractionModel(Model, nullptr);

	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.RefreshPipeline");
	TArray<TStrongObjectPtr<UWacomDeckCardWidget>> OwnedCards;
	TArray<TObjectPtr<UWacomDeckCardWidget>> Cards;
	for (int32 Index = 0; Index < 100; ++Index)
	{
		TStrongObjectPtr<UWacomDeckCardWidget> Card(
			NewObject<UWacomDeckCardWidget>());
		FCardInstance Instance;
		Instance.InstanceId = FGuid(Index + 1, 71, 19, 23);
		Instance.Definition = Definition.Get();
		Card->SetCard(Instance, EZoneKind::Backpack, FGuid());
		Workspace->GetStaticCardLayer()->AddChildToCanvas(Card.Get());
		Workspace->PrimeCardBaseLayout(
			*Card,
			FVector2D(120.0f + static_cast<float>(Index % 10) * 24.0f,
				160.0f + static_cast<float>(Index / 10) * 16.0f),
			FVector2D(220.0f, 320.0f),
			0.0f,
			Index);
		Cards.Add(Card.Get());
		OwnedCards.Add(MoveTemp(Card));
	}
	Workspace->BindWorkspaceCards(Cards, 41);

	const FWacomBackpackWorkspaceAutomationTestView BeforeSelection =
		Workspace->GetAutomationTestView();
	TestTrue(TEXT("Ctrl click selection is handled through the production input path"),
		FWacomBackpackScreenTestAccess::ToggleWorkspaceCardSelection(
			*Workspace,
			*Cards[37]));
	const FWacomBackpackWorkspaceAutomationTestView AfterSelection =
		Workspace->GetAutomationTestView();
	TestTrue(TEXT("Single-card selection remains a local presentation request"),
		!AfterSelection.bLastPresentationAppliedAllCards);
	TestEqual(TEXT("Single-card selection applies exactly one InstanceId"),
		AfterSelection.LastPresentationAppliedInstanceIds.Num(), 1);
	TestTrue(TEXT("The selected InstanceId is the only local request member"),
		AfterSelection.LastPresentationAppliedInstanceIds.Contains(
			Cards[37]->GetCardInstanceId()));
	TestEqual(TEXT("Single-card selection updates one static card"),
		AfterSelection.StaticCardPresentationUpdateCount
			- BeforeSelection.StaticCardPresentationUpdateCount,
		1);
	TestEqual(TEXT("Single-card selection does not rebuild navigation targets"),
		AfterSelection.NavigationTargetsApplyCount,
		BeforeSelection.NavigationTargetsApplyCount);
	TestEqual(TEXT("Single-card selection does not rebuild the carry strip"),
		AfterSelection.CarryStripLayoutRebuildCount,
		BeforeSelection.CarryStripLayoutRebuildCount);

	Model->SelectAllMovable();
	TestTrue(TEXT("100-card stress carry starts"),
		Model->BeginCarry(
			Cards[0]->GetCardInstanceId(),
			FVector2D(480.0f, 320.0f),
			41));
	FWacomBackpackScreenTestAccess::RefreshWorkspacePresentation(*Workspace);
	const FWacomBackpackWorkspaceAutomationTestView PointerBaseline =
		Workspace->GetAutomationTestView();
	TArray<FVector2D> PointerSamples;
	PointerSamples.Reserve(100);
	for (int32 Index = 0; Index < 100; ++Index)
	{
		PointerSamples.Add(FVector2D(
			300.0f + static_cast<float>(Index) * 2.0f,
			260.0f + static_cast<float>(Index % 7)));
	}
	FWacomBackpackScreenTestAccess::SendWorkspaceCarryPointerEvents(
		*Workspace,
		*Cards[0],
		PointerSamples);
	const FWacomBackpackWorkspaceAutomationTestView AfterPointerSamples =
		Workspace->GetAutomationTestView();
	TestEqual(TEXT("100 carry pointer samples do not flush presentation"),
		AfterPointerSamples.PresentationFlushCount,
		PointerBaseline.PresentationFlushCount);
	TestEqual(TEXT("100 carry pointer samples do not rebuild Scene cards"),
		AfterPointerSamples.WorkspaceSceneBindCount,
		PointerBaseline.WorkspaceSceneBindCount);
	TestEqual(TEXT("100 carry pointer samples do not rebuild navigation"),
		AfterPointerSamples.NavigationTargetsApplyCount,
		PointerBaseline.NavigationTargetsApplyCount);
	TestEqual(TEXT("100 carry pointer samples do not rebuild the carry strip"),
		AfterPointerSamples.CarryStripLayoutRebuildCount,
		PointerBaseline.CarryStripLayoutRebuildCount);
	TestEqual(TEXT("100 carry pointer samples do not touch static card presentation"),
		AfterPointerSamples.StaticCardPresentationUpdateCount,
		PointerBaseline.StaticCardPresentationUpdateCount);

	TestTrue(TEXT("Wheel current-card step is handled"),
		FWacomBackpackScreenTestAccess::StepWorkspaceCarryCurrentByWheel(
			*Workspace,
			1.0f));
	const FWacomBackpackWorkspaceAutomationTestView AfterWheel =
		Workspace->GetAutomationTestView();
	TestFalse(TEXT("Wheel handoff remains locally scoped"),
		AfterWheel.bLastPresentationAppliedAllCards);
	TestEqual(TEXT("Wheel handoff updates only outgoing and incoming current cards"),
		AfterWheel.LastPresentationAppliedInstanceIds.Num(), 2);
	TestEqual(TEXT("Wheel handoff does not rebuild navigation targets"),
		AfterWheel.NavigationTargetsApplyCount,
		AfterPointerSamples.NavigationTargetsApplyCount);
	TestEqual(TEXT("Wheel handoff rebuilds the carry strip once"),
		AfterWheel.CarryStripLayoutRebuildCount,
		AfterPointerSamples.CarryStripLayoutRebuildCount + 1);

	Workspace->SetCarryDropFeedbackState(true, false);
	const FWacomBackpackWorkspaceAutomationTestView AfterDropFeedback =
		Workspace->GetAutomationTestView();
	TestFalse(TEXT("Drop feedback scopes by carried InstanceIds instead of all cards"),
		AfterDropFeedback.bLastPresentationAppliedAllCards);
	TestEqual(TEXT("Drop feedback covers the exact 100 carried identities"),
		AfterDropFeedback.LastPresentationAppliedInstanceIds.Num(), 100);
	TestEqual(TEXT("Drop feedback does not rebuild navigation"),
		AfterDropFeedback.NavigationTargetsApplyCount,
		AfterWheel.NavigationTargetsApplyCount);
	TestEqual(TEXT("Drop feedback does not rebuild the carry strip"),
		AfterDropFeedback.CarryStripLayoutRebuildCount,
		AfterWheel.CarryStripLayoutRebuildCount);
	TestEqual(TEXT("Drop feedback does not touch static card presentation"),
		AfterDropFeedback.StaticCardPresentationUpdateCount,
		AfterWheel.StaticCardPresentationUpdateCount);
	TestEqual(TEXT("Drop feedback applies one accessibility stage"),
		AfterDropFeedback.AccessibilityApplyCount,
		AfterWheel.AccessibilityApplyCount + 1);

	const TArray<FGuid> CarryIdsBeforeMotionMode =
		Model->GetCarry().RemainingInstanceIds;
	const int32 CurrentIndexBeforeMotionMode = Model->GetCarry().CurrentIndex;
	Workspace->SetSimplifiedMotion(true);
	TestEqual(TEXT("Simplified Motion preserves carry identity"),
		Model->GetCarry().RemainingInstanceIds,
		CarryIdsBeforeMotionMode);
	TestEqual(TEXT("Simplified Motion preserves current carry identity"),
		Model->GetCarry().CurrentIndex,
		CurrentIndexBeforeMotionMode);
	return true;
}

#endif
