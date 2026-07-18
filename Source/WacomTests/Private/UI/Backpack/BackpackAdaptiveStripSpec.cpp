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
float ResolveExpectedExposure(
	int32 CardCount,
	float AvailableWidth,
	float CardWidth,
	float DesiredExposure,
	float FocusSeparation)
{
	if (CardCount <= 1)
	{
		return 0.0f;
	}
	const float ReservedFocus = FocusSeparation * FMath::Min(2, CardCount - 1);
	return FMath::Min(
		DesiredExposure,
		FMath::Max(0.0f, AvailableWidth - CardWidth - ReservedFocus)
			/ static_cast<float>(CardCount - 1));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackAdaptiveStripLayoutSpec,
	"Wacom.UI.Backpack.Workspace.AdaptiveStrip.Layout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackAdaptiveStripLayoutSpec::RunTest(const FString& Parameters)
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
			const FWacomBackpackResolvedPileContentLayout Neutral =
				FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
					CardCount,
					FVector2D(48.0f, 120.0f),
					FVector2D(260.0f, 48.0f),
					WorkspaceSize,
					Style->CardRenderSize,
					true,
					Style->PileCollapsedExposurePixels,
					Style->AdaptiveStripExposurePixels,
					Style->AdaptiveStripFocusSeparationPixels,
					Style->PileEdgeMarginPixels,
					Style->ExpandedCardHoverLiftPixels);
			TestEqual(TEXT("Neutral hit bands match the expanded card count"),
				Neutral.FocusHitBands.Num(), CardCount);
			TestTrue(TEXT("Count-adaptive frame stays inside the workspace safe edge"),
				Neutral.FrameRect.Left >= Style->PileEdgeMarginPixels - 0.1f
					&& Neutral.FrameRect.Right <= WorkspaceSize.X - Style->PileEdgeMarginPixels + 0.1f);
			for (int32 Index = 1; Index < Neutral.FocusHitBands.Num(); ++Index)
			{
				TestTrue(TEXT("Neutral focus hit bands have no horizontal gap"),
					FMath::IsNearlyEqual(
						Neutral.FocusHitBands[Index - 1].Right,
						Neutral.FocusHitBands[Index].Left,
						0.1f));
			}
			for (int32 Index = 0; Index < Neutral.FocusHitBands.Num(); ++Index)
			{
				TestTrue(TEXT("Neutral hit band starts at the rendered card body's top edge"),
					FMath::IsNearlyEqual(
						Neutral.FocusHitBands[Index].Top,
						Neutral.Cards[Index].CardCenter.Y - Style->CardRenderSize.Y * 0.5f,
						0.1f));
				TestTrue(TEXT("Neutral hit band ends at the rendered card body's bottom edge"),
					FMath::IsNearlyEqual(
						Neutral.FocusHitBands[Index].Bottom,
						Neutral.Cards[Index].CardCenter.Y + Style->CardRenderSize.Y * 0.5f,
						0.1f));
			}
			for (const FWacomBackpackResolvedLayout& Card : Neutral.Cards)
			{
				TestTrue(TEXT("Expanded piles use a zero-rotation horizontal strip"),
					FMath::IsNearlyZero(Card.AngleDegrees));
			}
			if (CardCount <= 0)
			{
				continue;
			}

			const float AvailableWidth = WorkspaceSize.X - Style->PileEdgeMarginPixels * 2.0f;
			const float ExpectedExposure = ResolveExpectedExposure(
				CardCount,
				AvailableWidth,
				Style->CardRenderSize.X,
				Style->AdaptiveStripExposurePixels,
				Style->AdaptiveStripFocusSeparationPixels);
			const float ExpectedFocusReserve = Style->AdaptiveStripFocusSeparationPixels
				* FMath::Min(2, FMath::Max(0, CardCount - 1));
			const float ExpectedReservedWidth = Style->CardRenderSize.X
				+ ExpectedExposure * FMath::Max(0, CardCount - 1)
				+ ExpectedFocusReserve;
			TestTrue(TEXT("Expanded corridor width follows the actual card count"),
				FMath::IsNearlyEqual(
					Neutral.FocusCorridorRect.Right - Neutral.FocusCorridorRect.Left,
					ExpectedReservedWidth,
					0.1f));

			for (const int32 FocusIndex : { 0, CardCount / 2, CardCount - 1 })
			{
				const FWacomBackpackAdaptiveStripLayout Focused =
					FWacomBackpackWorkspaceLayoutSolver::BuildAdaptiveStripLayout(
						CardCount,
						FocusIndex,
						Neutral.FocusCorridorRect,
						Style->CardRenderSize,
						Neutral.Cards,
						Style->AdaptiveStripExposurePixels,
						Style->AdaptiveStripFocusSeparationPixels);
				TestEqual(TEXT("Adaptive strip preserves card order and count"),
					Focused.Cards.Num(), CardCount);
				TestEqual(TEXT("Adaptive strip publishes one actual-position hit band per card"),
					Focused.HitBands.Num(), CardCount);
				TestTrue(TEXT("Adaptive strip preserves the count-resolved exposure"),
					FMath::IsNearlyEqual(Focused.EffectiveExposurePixels, ExpectedExposure, 0.1f));
				int32 HighestRank = TNumericLimits<int32>::Lowest();
				const float StableAlignmentOffset = Focused.Cards[FocusIndex].CardCenter.X
					- Neutral.Cards[FocusIndex].CardCenter.X;
				for (int32 Index = 0; Index < Focused.Cards.Num(); ++Index)
				{
					const FWacomBackpackResolvedLayout& Card = Focused.Cards[Index];
					HighestRank = FMath::Max(HighestRank, Card.LayerRank);
					TestTrue(TEXT("Adaptive strip cards remain horizontal"),
						FMath::IsNearlyZero(Card.AngleDegrees));
					TestTrue(TEXT("Every adaptive-strip card remains inside the stable corridor"),
						Card.CardCenter.X - Style->CardRenderSize.X * 0.5f
							>= Neutral.FocusCorridorRect.Left - 0.1f
						&& Card.CardCenter.X + Style->CardRenderSize.X * 0.5f
							<= Neutral.FocusCorridorRect.Right + 0.1f);
					if (Focused.HitBands.IsValidIndex(Index))
					{
						TestTrue(TEXT("Actual-position hit band contains its rendered card center"),
							Focused.HitBands[Index].Left <= Card.CardCenter.X
								&& Focused.HitBands[Index].Right >= Card.CardCenter.X);
						TestTrue(TEXT("Focused hit band keeps the rendered card body's vertical size"),
							FMath::IsNearlyEqual(
								Focused.HitBands[Index].Bottom - Focused.HitBands[Index].Top,
								Style->CardRenderSize.Y,
								0.1f));
					}
					const float ExpectedShift = Index < FocusIndex
						? -Focused.EffectiveFocusSeparationPixels
						: Index > FocusIndex
							? Focused.EffectiveFocusSeparationPixels
							: 0.0f;
					TestTrue(TEXT("Only the two sides of the focus receive local separation"),
						FMath::IsNearlyEqual(
							Card.CardCenter.X,
							Neutral.Cards[Index].CardCenter.X
								+ StableAlignmentOffset + ExpectedShift,
							0.1f));
				}
				TestEqual(TEXT("Focused card has the highest group layer rank"),
					Focused.Cards[FocusIndex].LayerRank, HighestRank);
			}
		}
	}

	const FVector2D LargeWorkspace(1920.0f, 1080.0f);
	for (const TPair<int32, float>& Case : {
		TPair<int32, float>(2, 416.0f),
		TPair<int32, float>(3, 536.0f),
		TPair<int32, float>(5, 680.0f) })
	{
		const FWacomBackpackResolvedPileContentLayout Layout =
			FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
				Case.Key,
				FVector2D(48.0f, 120.0f),
				FVector2D(260.0f, 48.0f),
				LargeWorkspace,
				Style->CardRenderSize,
				true,
				Style->PileCollapsedExposurePixels,
				Style->AdaptiveStripExposurePixels,
				Style->AdaptiveStripFocusSeparationPixels,
				Style->PileEdgeMarginPixels,
				Style->ExpandedCardHoverLiftPixels);
		TestTrue(TEXT("Authored small-count width remains compact"),

			FMath::IsNearlyEqual(
				Layout.FocusCorridorRect.Right - Layout.FocusCorridorRect.Left,
				Case.Value,
				0.1f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackAdaptiveStripInteractionSpec,
	"Wacom.UI.Backpack.Workspace.AdaptiveStrip.Interaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackAdaptiveStripInteractionSpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	Workspace->TakeWidget();
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Model =
		MakeShared<FWacomBackpackWorkspaceInteractionModel>();
	Workspace->SetInteractionModel(Model, nullptr);
	const UWacomBackpackWorkspaceStyle* Style = GetDefault<UWacomBackpackWorkspaceStyle>();
	const FWacomBackpackResolvedPileContentLayout Neutral =
		FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
			3,
			FVector2D(120.0f, 20.0f),
			FVector2D(260.0f, 48.0f),
			FVector2D(1280.0f, 720.0f),
			Style->CardRenderSize,
			true,
			Style->PileCollapsedExposurePixels,
			Style->AdaptiveStripExposurePixels,
			Style->AdaptiveStripFocusSeparationPixels,
			Style->PileEdgeMarginPixels,
			Style->ExpandedCardHoverLiftPixels);

	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.AdaptiveStrip.Interaction");
	TArray<TStrongObjectPtr<UWacomDeckCardWidget>> OwnedCards;
	TArray<TObjectPtr<UWacomDeckCardWidget>> Cards;
	TArray<FWacomBackpackExpandedPileFocusCard> FocusCards;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TStrongObjectPtr<UWacomDeckCardWidget> Card(NewObject<UWacomDeckCardWidget>());
		FCardInstance Instance;
		Instance.InstanceId = FGuid(Index + 1, 41, 42, 43);
		Instance.Definition = Definition.Get();
		Card->SetCard(Instance, EZoneKind::BattleDeck, FGuid());
		Workspace->GetStaticCardLayer()->AddChildToCanvas(Card.Get());
		Workspace->PrimeCardBaseLayout(
			*Card,
			Neutral.Cards[Index].CardCenter,
			Style->CardRenderSize,
			0.0f,
			3000 + Index);
		FWacomBackpackExpandedPileFocusCard& Focus = FocusCards.AddDefaulted_GetRef();
		Focus.Card = Card.Get();
		Focus.NeutralCenter = Neutral.Cards[Index].CardCenter;
		Focus.NeutralLayerRank = 3000 + Index;
		Focus.NeutralHitBand = Neutral.FocusHitBands[Index];
		Focus.CurrentHitBand = Focus.NeutralHitBand;
		Cards.Add(Card.Get());
		OwnedCards.Add(MoveTemp(Card));
	}
	OwnedCards[1]->SetWorkspaceInteractionEnabled(false);
	OwnedCards[1]->SetWorkspaceReadOnlyKind(
		EWacomBackpackWorkspaceCardReadOnlyKind::BattleProjection);
	Workspace->BindWorkspaceCards(Cards, 5);
	Workspace->SetExpandedPileFocusContract(
		EZoneKind::BattleDeck,
		FGuid(),
		Neutral.HeaderRect,
		Neutral.FocusCorridorRect,
		FocusCards);

	UWacomDeckCardWidget* LastBroadcastCard = nullptr;
	Workspace->OnBrowseFocusChangedNative.AddLambda(
		[&LastBroadcastCard](UWacomDeckCardWidget* Card)
		{
			LastBroadcastCard = Card;
		});
	const FVector2D ReadOnlyBandCenter(
		(Neutral.FocusHitBands[1].Left + Neutral.FocusHitBands[1].Right) * 0.5f,
		(Neutral.FocusHitBands[1].Top + Neutral.FocusHitBands[1].Bottom) * 0.5f);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, ReadOnlyBandCenter);
	const FWacomBackpackWorkspaceAutomationTestView ReadOnlyFocused =
		Workspace->GetAutomationTestView();
	TestEqual(TEXT("Read-only projection participates in browse focus"),
		ReadOnlyFocused.ExpandedPileFocusIndex, 1);
	TestEqual(TEXT("Browse focus broadcasts the exact display widget identity"),
		LastBroadcastCard, OwnedCards[1].Get());
	const FVector2D HeaderCenter(
		(Neutral.HeaderRect.Left + Neutral.HeaderRect.Right) * 0.5f,
		(Neutral.HeaderRect.Top + Neutral.HeaderRect.Bottom) * 0.5f);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(*Workspace, HeaderCenter);
	TestEqual(TEXT("Pile header immediately takes priority over expanded-card browse focus"),
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex, INDEX_NONE);
	TestNull(TEXT("Entering the pile header clears the browse detail source"), LastBroadcastCard);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, ReadOnlyBandCenter);

	TArray<FWacomBackpackResolvedLayout> NeutralLayouts = Neutral.Cards;
	const FWacomBackpackAdaptiveStripLayout FocusedAtOne =
		FWacomBackpackWorkspaceLayoutSolver::BuildAdaptiveStripLayout(
			3,
			1,
			Neutral.FocusCorridorRect,
			Style->CardRenderSize,
			NeutralLayouts,
			Style->AdaptiveStripExposurePixels,
			Style->AdaptiveStripFocusSeparationPixels);
	const int32 RebuildBaseline =
		Workspace->GetAutomationTestView().ExpandedPileFocusLayoutRebuildCount;
	const FVector2D FocusOneBandCenter(
		(FocusedAtOne.HitBands[1].Left + FocusedAtOne.HitBands[1].Right) * 0.5f,
		FocusedAtOne.Cards[1].CardCenter.Y - Style->ExpandedCardHoverLiftPixels);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, FocusOneBandCenter);
	TestEqual(TEXT("Pointer movement inside the focused card's actual band does not rebuild layout"),
		Workspace->GetAutomationTestView().ExpandedPileFocusLayoutRebuildCount,
		RebuildBaseline);
	const FVector2D BelowFocusedCardBody(
		FocusOneBandCenter.X,
		FocusedAtOne.Cards[1].CardCenter.Y - Style->ExpandedCardHoverLiftPixels
			+ Style->CardRenderSize.Y * 0.5f + 2.0f);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, BelowFocusedCardBody);
	TestTrue(TEXT("Leaving the rendered card body vertically starts focus exit even inside the reserved corridor"),
		Workspace->GetAutomationTestView().bExpandedPileFocusExitPending);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, FocusOneBandCenter);
	TestFalse(TEXT("Returning to the rendered card body cancels focus exit"),
		Workspace->GetAutomationTestView().bExpandedPileFocusExitPending);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, FocusedAtOne.Cards[2].CardCenter);
	TestEqual(TEXT("Hovering the card at its adaptive position selects that exact card"),
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex, 2);
	TestEqual(TEXT("Crossing into a new actual-position band rebuilds exactly once"),
		Workspace->GetAutomationTestView().ExpandedPileFocusLayoutRebuildCount,
		RebuildBaseline + 1);

	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, FVector2D(1100.0f, 680.0f));
	TestTrue(TEXT("Leaving the stable count-adaptive frame starts the authored exit delay"),
		Workspace->GetAutomationTestView().bExpandedPileFocusExitPending);
	FWacomBackpackScreenTestAccess::TickWorkspaceBrowseExit(*Workspace, 0.06f);
	TestTrue(TEXT("Focus remains during the first half of the exit grace period"),
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex != INDEX_NONE);
	FWacomBackpackScreenTestAccess::TickWorkspaceBrowseExit(*Workspace, 0.07f);
	TestEqual(TEXT("Focus returns to neutral after the exit grace period"),
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex, INDEX_NONE);
	TestNull(TEXT("Clearing browse focus broadcasts no detail source"), LastBroadcastCard);

	Workspace->SetSimplifiedMotion(true);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, Neutral.Cards[0].CardCenter);
	TestEqual(TEXT("Simplified motion keeps the readable adaptive focus identity"),
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex, 0);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, FVector2D(1100.0f, 680.0f));
	TestFalse(TEXT("Simplified motion removes the focus exit delay"),
		Workspace->GetAutomationTestView().bExpandedPileFocusExitPending);
	TestEqual(TEXT("Simplified motion returns immediately to neutral"),
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex, INDEX_NONE);
	return true;
}

#endif
