// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "Cards/CardDefinition.h"
#include "Components/CanvasPanel.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "../BackpackScreenTestAccess.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
struct FExpectedFocusWindowMetrics
{
	int32 WindowCardCount = 0;
	float ExposurePixels = 0.0f;
	float UsedWidthPixels = 0.0f;
};

FExpectedFocusWindowMetrics ResolveExpectedMetrics(
	int32 CardCount,
	float AvailableWidth,
	const UWacomBackpackWorkspaceStyle& Style)
{
	FExpectedFocusWindowMetrics Result;
	if (CardCount <= 0)
	{
		return Result;
	}
	const int32 MaximumWindow = FMath::Clamp(
		Style.FocusWindowMaximumCards, 1, CardCount);
	for (int32 WindowCount = MaximumWindow; WindowCount >= 1; --WindowCount)
	{
		const float WindowWidth = Style.CardRenderSize.X * WindowCount
			+ Style.FocusWindowFullGapPixels * FMath::Max(0, WindowCount - 1);
		const int32 CompressedCount = CardCount - WindowCount;
		if (WindowCount == 1
			|| WindowWidth + Style.FocusWindowMinimumExposurePixels * CompressedCount
				<= AvailableWidth + KINDA_SMALL_NUMBER)
		{
			Result.WindowCardCount = WindowCount;
			Result.ExposurePixels = CompressedCount > 0
				? FMath::Clamp(
					(AvailableWidth - WindowWidth) / static_cast<float>(CompressedCount),
					Style.FocusWindowMinimumExposurePixels,
					Style.FocusWindowCompressedExposurePixels)
				: 0.0f;
			Result.UsedWidthPixels = WindowWidth
				+ Result.ExposurePixels * CompressedCount;
			return Result;
		}
	}
	return Result;
}

int32 ResolveExpectedWindowStart(int32 CardCount, int32 WindowCount, int32 FocusIndex)
{
	if (FocusIndex == INDEX_NONE)
	{
		return FMath::Max(0, (CardCount - WindowCount) / 2);
	}
	return FMath::Clamp(
		FocusIndex - FMath::Max(0, (WindowCount - 1) / 2),
		0,
		FMath::Max(0, CardCount - WindowCount));
}

bool BandsAreHorizontallyContinuous(TConstArrayView<FSlateRect> Bands)
{
	for (int32 Index = 1; Index < Bands.Num(); ++Index)
	{
		if (!FMath::IsNearlyEqual(Bands[Index - 1].Right, Bands[Index].Left, 0.1f))
		{
			return false;
		}
	}
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackFocusWindowStripLayoutSpec,
	"Wacom.UI.Backpack.Workspace.FocusWindowStrip.Layout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackFocusWindowStripLayoutSpec::RunTest(const FString& Parameters)
{
	const UWacomBackpackWorkspaceStyle* Style = GetDefault<UWacomBackpackWorkspaceStyle>();
	for (const FVector2D WorkspaceSize : {
		FVector2D(1280.0f, 720.0f),
		FVector2D(1920.0f, 1080.0f),
		FVector2D(1680.0f, 1050.0f),
		FVector2D(2560.0f, 1080.0f) })
	{
		for (const int32 CardCount : { 0, 1, 2, 3, 5, 7, 15, 21 })
		{
			const float AvailableWidth = WorkspaceSize.X
				- Style->PileEdgeMarginPixels * 2.0f;
			const FExpectedFocusWindowMetrics Expected = ResolveExpectedMetrics(
				CardCount, AvailableWidth, *Style);
			const FWacomBackpackResolvedPileContentLayout Initial =
				FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
					CardCount,
					FVector2D(48.0f, 120.0f),
					FVector2D(260.0f, 48.0f),
					WorkspaceSize,
					Style->CardRenderSize,
					true,
					Style->PileCollapsedExposurePixels,
					Style->FocusWindowMaximumCards,
					Style->FocusWindowFullGapPixels,
					Style->FocusWindowCompressedExposurePixels,
					Style->FocusWindowMinimumExposurePixels,
					Style->PileEdgeMarginPixels,
					Style->ExpandedCardHoverLiftPixels);

			TestEqual(TEXT("Initial strip preserves card count"), Initial.Cards.Num(), CardCount);
			TestEqual(TEXT("Initial strip publishes one visible hit band per card"),
				Initial.FocusHitBands.Num(), CardCount);
			TestTrue(TEXT("Initial visible hit bands have no horizontal holes"),
				BandsAreHorizontallyContinuous(Initial.FocusHitBands));
			TestTrue(TEXT("Pile frame stays inside the workspace safe edge"),
				Initial.FrameRect.Left >= Style->PileEdgeMarginPixels - 0.1f
					&& Initial.FrameRect.Right
						<= WorkspaceSize.X - Style->PileEdgeMarginPixels + 0.1f);
			if (CardCount <= 0)
			{
				continue;
			}

			TestEqual(TEXT("Largest fitting full window is selected"),
				Initial.FocusWindowCardCount, Expected.WindowCardCount);
			TestEqual(TEXT("First expansion uses the centered window"),
				Initial.FocusWindowStartIndex,
				ResolveExpectedWindowStart(CardCount, Expected.WindowCardCount, INDEX_NONE));
			TestTrue(TEXT("Expanded corridor width is stable for this count and geometry"),
				FMath::IsNearlyEqual(
					Initial.FocusCorridorRect.Right - Initial.FocusCorridorRect.Left,
					Expected.UsedWidthPixels,
					0.1f));
			if (CardCount > Expected.WindowCardCount)
			{
				TestTrue(TEXT("Compressed cards keep the authored minimum visible edge"),
					Initial.EffectiveCompressedExposurePixels
						>= Style->FocusWindowMinimumExposurePixels - 0.1f);
			}

			for (int32 Index = 0; Index < Initial.Cards.Num(); ++Index)
			{
				TestTrue(TEXT("Focus-window cards remain unrotated"),
					FMath::IsNearlyZero(Initial.Cards[Index].AngleDegrees));
				TestTrue(TEXT("Hit bands use the exact card body height"),
					FMath::IsNearlyEqual(
						Initial.FocusHitBands[Index].Bottom
							- Initial.FocusHitBands[Index].Top,
						Style->CardRenderSize.Y,
						0.1f));
			}

			for (const int32 FocusIndex : { 0, CardCount / 2, CardCount - 1 })
			{
				const FWacomBackpackFocusWindowStripLayout Focused =
					FWacomBackpackWorkspaceLayoutSolver::BuildFocusWindowStripLayout(
						CardCount,
						FocusIndex,
						Initial.FocusCorridorRect,
						Style->CardRenderSize,
						Initial.Cards,
						Style->FocusWindowMaximumCards,
						Style->FocusWindowFullGapPixels,
						Style->FocusWindowCompressedExposurePixels,
						Style->FocusWindowMinimumExposurePixels);
				TestEqual(TEXT("Focused strip preserves card order and count"),
					Focused.Cards.Num(), CardCount);
				TestEqual(TEXT("Focus does not change the resolved window size"),
					Focused.WindowCardCount, Expected.WindowCardCount);
				TestEqual(TEXT("Window follows the focus and clamps at both edges"),
					Focused.WindowStartIndex,
					ResolveExpectedWindowStart(
						CardCount, Expected.WindowCardCount, FocusIndex));
				TestTrue(TEXT("Focused visible bands remain continuous"),
					BandsAreHorizontallyContinuous(Focused.HitBands));
				int32 HighestRank = TNumericLimits<int32>::Lowest();
				for (int32 Index = 0; Index < Focused.Cards.Num(); ++Index)
				{
					const FWacomBackpackResolvedLayout& Card = Focused.Cards[Index];
					HighestRank = FMath::Max(HighestRank, Card.LayerRank);
					TestTrue(TEXT("Every card stays inside the stable pile corridor"),
						Card.CardCenter.X - Style->CardRenderSize.X * 0.5f
							>= Initial.FocusCorridorRect.Left - 0.1f
						&& Card.CardCenter.X + Style->CardRenderSize.X * 0.5f
							<= Initial.FocusCorridorRect.Right + 0.1f);
					if (Index > 0)
					{
						TestTrue(TEXT("Card identity order stays left-to-right"),
							Card.CardCenter.X > Focused.Cards[Index - 1].CardCenter.X);
					}
				}
				TestEqual(TEXT("Active focus owns the highest ZOrder"),
					Focused.Cards[FocusIndex].LayerRank, HighestRank);
			}
		}
	}

	const FWacomBackpackResolvedPileContentLayout TwentyOneAt1280 =
		FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
			21, FVector2D(24.0f, 80.0f), FVector2D(260.0f, 48.0f),
			FVector2D(1280.0f, 720.0f), Style->CardRenderSize, true,
			Style->PileCollapsedExposurePixels, Style->FocusWindowMaximumCards,
			Style->FocusWindowFullGapPixels, Style->FocusWindowCompressedExposurePixels,
			Style->FocusWindowMinimumExposurePixels, Style->PileEdgeMarginPixels,
			Style->ExpandedCardHoverLiftPixels);
	const FWacomBackpackResolvedPileContentLayout TwentyOneAt1920 =
		FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
			21, FVector2D(24.0f, 80.0f), FVector2D(260.0f, 48.0f),
			FVector2D(1920.0f, 1080.0f), Style->CardRenderSize, true,
			Style->PileCollapsedExposurePixels, Style->FocusWindowMaximumCards,
			Style->FocusWindowFullGapPixels, Style->FocusWindowCompressedExposurePixels,
			Style->FocusWindowMinimumExposurePixels, Style->PileEdgeMarginPixels,
			Style->ExpandedCardHoverLiftPixels);
	TestEqual(TEXT("1280-wide workspace resolves three full cards for 21-card pile"),
		TwentyOneAt1280.FocusWindowCardCount, 3);
	TestEqual(TEXT("1920-wide workspace resolves five full cards for 21-card pile"),
		TwentyOneAt1920.FocusWindowCardCount, 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackFocusWindowStripInteractionSpec,
	"Wacom.UI.Backpack.Workspace.FocusWindowStrip.Interaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackFocusWindowStripInteractionSpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	// Keep the Slate root alive for the whole pointer-routing fixture. Discarding
	// this reference lets UUserWidget rebuild during CaptureMouse(), and the old
	// Slate root's Destruct correctly cancels transient interaction state.
	const TSharedRef<SWidget> WorkspaceSlateRoot = Workspace->TakeWidget();
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Model =
		MakeShared<FWacomBackpackWorkspaceInteractionModel>();
	Workspace->SetInteractionModel(Model, nullptr);
	const UWacomBackpackWorkspaceStyle* Style = GetDefault<UWacomBackpackWorkspaceStyle>();
	const FWacomBackpackResolvedPileContentLayout Initial =
		FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
			7, FVector2D(120.0f, 20.0f), FVector2D(260.0f, 48.0f),
			FVector2D(1280.0f, 720.0f), Style->CardRenderSize, true,
			Style->PileCollapsedExposurePixels, Style->FocusWindowMaximumCards,
			Style->FocusWindowFullGapPixels, Style->FocusWindowCompressedExposurePixels,
			Style->FocusWindowMinimumExposurePixels, Style->PileEdgeMarginPixels,
			Style->ExpandedCardHoverLiftPixels);

	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.FocusWindow.Interaction");
	TArray<TStrongObjectPtr<UWacomDeckCardWidget>> OwnedCards;
	TArray<TObjectPtr<UWacomDeckCardWidget>> Cards;
	TArray<FWacomBackpackExpandedPileFocusCard> FocusCards;
	for (int32 Index = 0; Index < 7; ++Index)
	{
		TStrongObjectPtr<UWacomDeckCardWidget> Card(NewObject<UWacomDeckCardWidget>());
		FCardInstance Instance;
		Instance.InstanceId = FGuid(Index + 1, 41, 42, 43);
		Instance.Definition = Definition.Get();
		Card->SetCard(Instance, EZoneKind::BattleDeck, FGuid());
		Workspace->GetStaticCardLayer()->AddChildToCanvas(Card.Get());
		Workspace->PrimeCardBaseLayout(
			*Card, Initial.Cards[Index].CardCenter, Style->CardRenderSize,
			0.0f, 3000 + Initial.Cards[Index].LayerRank);
		FWacomBackpackExpandedPileFocusCard& Focus = FocusCards.AddDefaulted_GetRef();
		Focus.Card = Card.Get();
		Focus.NeutralCenter = Initial.Cards[Index].CardCenter;
		Focus.NeutralLayerRank = 3000 + Initial.Cards[Index].LayerRank;
		Focus.NeutralHitBand = Initial.FocusHitBands[Index];
		Focus.CurrentHitBand = Focus.NeutralHitBand;
		Cards.Add(Card.Get());
		OwnedCards.Add(MoveTemp(Card));
	}
	OwnedCards[3]->SetWorkspaceInteractionEnabled(false);
	OwnedCards[3]->SetWorkspaceReadOnlyKind(
		EWacomBackpackWorkspaceCardReadOnlyKind::BattleProjection);
	Workspace->BindWorkspaceCards(Cards, 5);
	Workspace->SetExpandedPileFocusContract(
		EZoneKind::BattleDeck, FGuid(), Initial.HeaderRect,
		Initial.FocusCorridorRect, FocusCards);
	for (const TStrongObjectPtr<UWacomDeckCardWidget>& Card : OwnedCards)
	{
		TestEqual(TEXT("Expanded pile cards defer Slate pointer routing to Workspace"),
			Card->GetVisibility(), ESlateVisibility::HitTestInvisible);
	}

	UWacomDeckCardWidget* LastBroadcastCard = nullptr;
	Workspace->OnBrowseFocusChangedNative.AddLambda(
		[&LastBroadcastCard](UWacomDeckCardWidget* Card)
		{
			LastBroadcastCard = Card;
		});
	const FVector2D ReadOnlyBandCenter(
		(Initial.FocusHitBands[3].Left + Initial.FocusHitBands[3].Right) * 0.5f,
		(Initial.FocusHitBands[3].Top + Initial.FocusHitBands[3].Bottom) * 0.5f);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, ReadOnlyBandCenter);
	const FWacomBackpackWorkspaceAutomationTestView Focused =
		Workspace->GetAutomationTestView();
	TestEqual(TEXT("Read-only card participates in browse focus"),
		Focused.ExpandedPileFocusIndex, 3);
	TestEqual(TEXT("Browse focus broadcasts the exact display widget"),
		LastBroadcastCard, OwnedCards[3].Get());
	TestEqual(TEXT("Active card also drives the frozen window position"),
		Focused.ExpandedPileWindowFocusIndex, 3);

	const FVector2D HeaderCenter(
		(Initial.HeaderRect.Left + Initial.HeaderRect.Right) * 0.5f,
		(Initial.HeaderRect.Top + Initial.HeaderRect.Bottom) * 0.5f);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(*Workspace, HeaderCenter);
	const FWacomBackpackWorkspaceAutomationTestView HeaderPriority =
		Workspace->GetAutomationTestView();
	TestEqual(TEXT("Pile header immediately clears active card focus"),
		HeaderPriority.ExpandedPileFocusIndex, INDEX_NONE);
	TestEqual(TEXT("Header priority does not reset the last focus window"),
		HeaderPriority.ExpandedPileWindowFocusIndex, 3);
	TestNull(TEXT("Header clears the browse detail source"), LastBroadcastCard);

	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, FVector2D(1100.0f, 680.0f));
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, ReadOnlyBandCenter);
	TestEqual(TEXT("Stable activation band can reacquire the same display card"),
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex, 3);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, FVector2D(1100.0f, 680.0f));
	TestTrue(TEXT("Leaving the card body starts the exit grace period"),
		Workspace->GetAutomationTestView().bExpandedPileFocusExitPending);
	FWacomBackpackScreenTestAccess::TickWorkspaceBrowseExit(*Workspace, 0.13f);
	const FWacomBackpackWorkspaceAutomationTestView Frozen =
		Workspace->GetAutomationTestView();
	TestEqual(TEXT("Exit clears the active lifted card"),
		Frozen.ExpandedPileFocusIndex, INDEX_NONE);
	TestEqual(TEXT("Exit freezes the last three-segment window"),
		Frozen.ExpandedPileWindowFocusIndex, 3);
	TestNull(TEXT("Exit clears the browse detail source"), LastBroadcastCard);

	const FSlateRect& LeftEdgeBand = Initial.FocusHitBands[0];
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace,
		FVector2D(
			(LeftEdgeBand.Left + LeftEdgeBand.Right) * 0.5f,
			(LeftEdgeBand.Top + LeftEdgeBand.Bottom) * 0.5f));
	const FWacomBackpackFocusWindowStripLayout FocusZeroLayout =
		FWacomBackpackWorkspaceLayoutSolver::BuildFocusWindowStripLayout(
			Initial.Cards.Num(),
			0,
			Initial.FocusCorridorRect,
			Style->CardRenderSize,
			Initial.Cards,
			Style->FocusWindowMaximumCards,
			Style->FocusWindowFullGapPixels,
			Style->FocusWindowCompressedExposurePixels,
			Style->FocusWindowMinimumExposurePixels);
	FSlateRect DetailAnchorRect;
	TestTrue(TEXT("Expanded focus exposes an authoritative detail anchor"),
		FWacomBackpackScreenTestAccess::ResolveWorkspaceCardDetailAnchorRect(
			*Workspace, *OwnedCards[0], DetailAnchorRect));
	const FVector2D DetailAnchorCenter(
		(DetailAnchorRect.Left + DetailAnchorRect.Right) * 0.5f,
		(DetailAnchorRect.Top + DetailAnchorRect.Bottom) * 0.5f);
	FVector2D ExpectedDetailAnchorCenter = FocusZeroLayout.Cards[0].CardCenter;
	ExpectedDetailAnchorCenter.Y -= Style->ExpandedCardHoverLiftPixels;
	TestTrue(TEXT("Detail panel anchor uses the final expanded card position"),
		DetailAnchorCenter.Equals(ExpectedDetailAnchorCenter, 0.1f));
	FWacomBackpackScreenTestAccess::TickWorkspaceCardMotion(*Workspace, 0.06f);
	const FVector2D CardOneVisualCenter = Initial.Cards[1].CardCenter
		+ OwnedCards[1]->GetBackpackLocalMotionTranslation();
	const FVector2D CardTwoVisualCenter = Initial.Cards[2].CardCenter
		+ OwnedCards[2]->GetBackpackLocalMotionTranslation();
	const float VisualOverlapLeft = FMath::Max(
		CardOneVisualCenter.X - Style->CardRenderSize.X * 0.5f,
		CardTwoVisualCenter.X - Style->CardRenderSize.X * 0.5f);
	const float VisualOverlapRight = FMath::Min(
		CardOneVisualCenter.X + Style->CardRenderSize.X * 0.5f,
		CardTwoVisualCenter.X + Style->CardRenderSize.X * 0.5f);
	TestTrue(TEXT("Reflow fixture has a visible overlap between moving cards"),
		VisualOverlapRight > VisualOverlapLeft);
	const FVector2D VisualOverlapPoint(
		(VisualOverlapLeft + VisualOverlapRight) * 0.5f,
		CardTwoVisualCenter.Y);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, VisualOverlapPoint);
	TestEqual(TEXT("Reflow hit testing follows the visually topmost card"),
		LastBroadcastCard, OwnedCards[2].Get());
	const FVector2D FirstVisualCenterBeforeFocus = Initial.Cards[0].CardCenter
		+ OwnedCards[0]->GetBackpackLocalMotionTranslation();
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace,
		FVector2D(
			FirstVisualCenterBeforeFocus.X - Style->CardRenderSize.X * 0.5f + 4.0f,
			FirstVisualCenterBeforeFocus.Y));
	TestEqual(TEXT("The first card's current visible edge activates that exact card"),
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex, 0);
	FWacomBackpackScreenTestAccess::TickWorkspaceCardMotion(
		*Workspace, Style->FocusReflowSeconds + 0.02f);
	const FVector2D FirstExpandedVisualCenter = Initial.Cards[0].CardCenter
		+ OwnedCards[0]->GetBackpackLocalMotionTranslation();
	const FVector2D PointInsideExpandedFirstCard(
		FirstExpandedVisualCenter.X + Style->CardRenderSize.X * 0.4f,
		FirstExpandedVisualCenter.Y);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, PointInsideExpandedFirstCard);
	TestEqual(TEXT("Moving inside the expanded first card keeps its focus identity"),
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex, 0);
	TestFalse(TEXT("The expanded first card body does not start focus exit"),
		Workspace->GetAutomationTestView().bExpandedPileFocusExitPending);
	TestEqual(TEXT("The expanded first card owns its full visible body"),
		LastBroadcastCard, OwnedCards[0].Get());
	TestTrue(TEXT("The expanded first card remains directly movable"),
		OwnedCards[0]->IsMoveEnabled());
	TestTrue(TEXT("The interaction model keeps the expanded first card movable"),
		Model->IsMovable(OwnedCards[0]->GetCardInstanceId()));
	TestTrue(TEXT("Unified visual pointer routing resolves the expanded pile card"),
		FWacomBackpackScreenTestAccess::PressExpandedPileVisualCard(
			*Workspace, PointInsideExpandedFirstCard));
	const FWacomBackpackWorkspaceAutomationTestView CarryView =
		Workspace->GetAutomationTestView();
	TestEqual(TEXT("Visual pointer-down begins a one-card carry"),
		CarryView.CarriedInstanceIds.Num(), 1);
	if (!CarryView.CarriedInstanceIds.IsEmpty())
	{
		TestEqual(TEXT("Visual pointer-down carries the same expanded first card as browse focus"),
			CarryView.CarriedInstanceIds[0], OwnedCards[0]->GetCardInstanceId());
	}
	Workspace->CancelInteraction();

	Workspace->SetExpandedPileFocusContract(
		EZoneKind::Backpack, FGuid(), FSlateRect(), FSlateRect(), {});
	for (const TStrongObjectPtr<UWacomDeckCardWidget>& Card : OwnedCards)
	{
		TestEqual(TEXT("Collapsed pile restores per-card Slate pointer routing"),
			Card->GetVisibility(),
			Card->IsWorkspaceInteractionEnabled()
				? ESlateVisibility::Visible
				: ESlateVisibility::HitTestInvisible);
	}
	const FWacomBackpackWorkspaceAutomationTestView Reset =
		Workspace->GetAutomationTestView();
	TestEqual(TEXT("Collapsing or switching the pile clears active focus"),
		Reset.ExpandedPileFocusIndex, INDEX_NONE);
	TestEqual(TEXT("Collapsing or switching the pile clears the frozen window"),
		Reset.ExpandedPileWindowFocusIndex, INDEX_NONE);
	return true;
}

#endif
