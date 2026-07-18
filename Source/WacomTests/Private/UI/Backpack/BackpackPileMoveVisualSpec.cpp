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
