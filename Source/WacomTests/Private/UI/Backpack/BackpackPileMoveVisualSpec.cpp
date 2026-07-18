// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "Blueprint/WidgetTree.h"
#include "Cards/CardDefinition.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "../BackpackScreenTestAccess.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackPileMoveVisualHandoffSpec,
	"Wacom.UI.Backpack.Workspace.PileMoveVisualHandoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackPileMoveVisualHandoffSpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	const TSharedRef<SWidget> WorkspaceSlate = Workspace->TakeWidget();
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Model =
		MakeShared<FWacomBackpackWorkspaceInteractionModel>();
	Workspace->SetInteractionModel(Model, nullptr);
	Workspace->SetSimplifiedMotion(false);

	FWacomBackpackZonePileView PileView;
	PileView.Zone = EZoneKind::BattleDeck;
	PileView.bMovable = true;
	const FSlateRect FrameRect(100.0f, 100.0f, 420.0f, 500.0f);
	const FSlateRect HeaderRect(100.0f, 100.0f, 360.0f, 148.0f);
	const TArray<FWacomBackpackZonePileView> PileViews { PileView };
	const TArray<FSlateRect> FrameRects { FrameRect };
	const TArray<FSlateRect> HeaderRects { HeaderRect };
	const TArray<int32> LayerRanks { 1 };
	FWacomBackpackScreenTestAccess::ReconcileWorkspacePilesForTest(
		*Workspace, PileViews, FrameRects, HeaderRects, LayerRanks);

	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.PileMove.VisualHandoff");
	FCardInstance Instance;
	Instance.InstanceId = FGuid(91, 92, 93, 94);
	Instance.Definition = Definition.Get();
	TStrongObjectPtr<UWacomDeckCardWidget> Card(NewObject<UWacomDeckCardWidget>());
	Card->SetCard(Instance, EZoneKind::BattleDeck, FGuid());
	Card->SetWorkspaceDisplayZone(EZoneKind::BattleDeck, FGuid());
	Workspace->GetStaticCardLayer()->AddChildToCanvas(Card.Get());
	const FVector2D SourceCenter(250.0f, 300.0f);
	Workspace->PrimeCardBaseLayout(
		*Card, SourceCenter, FVector2D(220.0f, 320.0f), 0.0f, 1);
	TArray<TObjectPtr<UWacomDeckCardWidget>> Cards { Card.Get() };
	Workspace->BindWorkspaceCards(Cards, 3);

	const FVector2D HeaderStart(100.0f, 100.0f);
	const FVector2D PointerEnd(496.0f, 304.0f);
	const FVector2D TargetCenter = SourceCenter + (PointerEnd - HeaderStart);
	int32 CancelledCommitCount = 0;
	const FDelegateHandle CancelledCommitHandle =
		Workspace->OnPileMoveCommittedNative.AddLambda(
			[&CancelledCommitCount](EZoneKind, FGuid, FVector2D)
			{
				++CancelledCommitCount;
			});
	const FWacomBackpackPileMoveCancelProbe CancelProbe =
		FWacomBackpackScreenTestAccess::CancelWorkspacePileMove(
			*Workspace,
			EZoneKind::BattleDeck,
			FGuid(),
			HeaderStart,
			PointerEnd);
	Workspace->OnPileMoveCommittedNative.Remove(CancelledCommitHandle);
	TestTrue(TEXT("Pile move begins before cancellation"), CancelProbe.bBeganMove);
	TestFalse(TEXT("Pile frame visibly moved before cancellation"),
		CancelProbe.PilePositionWhileMoving.Equals(CancelProbe.PilePositionBefore, 0.1f));
	TestEqual(TEXT("Active pile move temporarily owns the front layer"),
		CancelProbe.PileZOrderWhileMoving, 9000);
	TestTrue(TEXT("ESC-style cancellation restores the exact pile frame position"),
		CancelProbe.PilePositionAfterCancel.Equals(CancelProbe.PilePositionBefore, 0.1f));
	TestEqual(TEXT("ESC-style cancellation restores the exact pile ZOrder"),
		CancelProbe.PileZOrderAfterCancel, CancelProbe.PileZOrderBefore);
	TestEqual(TEXT("Cancelled pile move never commits Workspace State Store intent"),
		CancelledCommitCount, 0);
	const UCanvasPanelSlot* CancelledCardSlot = Cast<UCanvasPanelSlot>(Card->Slot);
	TestNotNull(TEXT("Cancelled pile card remains on a canvas"), CancelledCardSlot);
	if (CancelledCardSlot)
	{
		TestTrue(TEXT("Cancelled pile card returns atomically to A"),
			CancelledCardSlot->GetPosition().Equals(
				SourceCenter - FVector2D(110.0f, 160.0f), 1.0f));
	}

	TestTrue(TEXT("Pile release reaches the synchronous target reconcile"),
		FWacomBackpackScreenTestAccess::CommitWorkspacePileMoveWithSynchronousTargetReconcile(
			*Workspace,
			*Card,
			EZoneKind::BattleDeck,
			HeaderStart,
			PointerEnd,
			TargetCenter));

	const FWacomBackpackWorkspaceAutomationTestView View = Workspace->GetAutomationTestView();
	TestEqual(TEXT("Committed pile cards do not start an A-to-B base transition"),
		View.ActiveBaseCardLayoutTransitionCount, 0);
	const UCanvasPanelSlot* CardSlot = Cast<UCanvasPanelSlot>(Card->Slot);
	TestNotNull(TEXT("Moved pile card remains on a canvas"), CardSlot);
	if (CardSlot)
	{
		TestTrue(TEXT("Moved pile card lands directly at B"),
			CardSlot->GetPosition().Equals(TargetCenter - FVector2D(110.0f, 160.0f), 1.0f));
	}
	return true;
}

#endif
