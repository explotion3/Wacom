// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "Components/CanvasPanel.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomBackpackZonePileTypes.h"
#include "../BackpackScreenTestAccess.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackZonePileVisualTreeReconcileSpec,
	"Wacom.UI.Backpack.Workspace.ZonePileVisualTreeReconcile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackZonePileVisualTreeReconcileSpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	const TSharedRef<SWidget> WorkspaceSlate = Workspace->TakeWidget();
	Workspace->SetInteractionModel(
		MakeShared<FWacomBackpackWorkspaceInteractionModel>(), nullptr);

	FWacomBackpackZonePileView BattleDeck;
	BattleDeck.Zone = EZoneKind::BattleDeck;
	BattleDeck.Title = FText::FromString(TEXT("备战区"));
	BattleDeck.CardCount = 7;
	BattleDeck.Capacity = 18;
	BattleDeck.bHasCapacity = true;

	FWacomBackpackZonePileView Special;
	Special.Zone = EZoneKind::SpecialZone;
	Special.OwnerInstanceId = FGuid(11, 12, 13, 14);
	Special.Title = FText::FromString(TEXT("蛛茧绒囊"));
	Special.Capacity = 2;
	Special.bHasCapacity = true;

	TArray<FWacomBackpackZonePileView> Views { BattleDeck, Special };
	const TArray<FSlateRect> Frames {
		FSlateRect(32.0f, 32.0f, 332.0f, 420.0f),
		FSlateRect(380.0f, 32.0f, 680.0f, 420.0f)
	};
	const TArray<FSlateRect> Headers {
		FSlateRect(32.0f, 32.0f, 292.0f, 80.0f),
		FSlateRect(380.0f, 32.0f, 640.0f, 80.0f)
	};
	const TArray<int32> Ranks { 1, 2 };

	FWacomBackpackScreenTestAccess::ReconcileWorkspacePilesForTest(
		*Workspace, Views, Frames, Headers, Ranks);
	UCanvasPanel* PileCanvas = Workspace->GetPileCanvas();
	TestNotNull(TEXT("Workspace exposes its pile visual layer"), PileCanvas);
	if (!PileCanvas)
	{
		return false;
	}
	TestEqual(TEXT("Initial scene owns exactly one visual per pile"),
		PileCanvas->GetChildrenCount(), Views.Num());

	for (int32 Cycle = 0; Cycle < 5; ++Cycle)
	{
		// Mirrors a Slate reconstruct/destruct boundary where the UMG panel can retain
		// visual children after the transient registry has been cleared.
		FWacomBackpackScreenTestAccess::ForgetWorkspacePileRegistry(*Workspace);
		Views[Cycle % Views.Num()].bExpanded = !Views[Cycle % Views.Num()].bExpanded;
		FWacomBackpackScreenTestAccess::ReconcileWorkspacePilesForTest(
			*Workspace, Views, Frames, Headers, Ranks);
		TestEqual(
			*FString::Printf(TEXT("Cycle %d leaves no orphaned pile visuals"), Cycle + 1),
			PileCanvas->GetChildrenCount(),
			Views.Num());
		TestEqual(
			*FString::Printf(TEXT("Cycle %d rebuilds the authoritative pile registry"), Cycle + 1),
			Workspace->GetAutomationTestView().PileCount,
			Views.Num());
	}

	return true;
}

#endif
