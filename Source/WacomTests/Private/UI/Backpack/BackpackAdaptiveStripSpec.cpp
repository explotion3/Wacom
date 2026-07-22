// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "Cards/CardDefinition.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "../BackpackScreenTestAccess.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
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

TArray<FWacomBackpackResolvedLayout> BuildHorizontalBases(int32 CardCount, float CenterY)
{
	TArray<FWacomBackpackResolvedLayout> Result;
	Result.SetNum(CardCount);
	for (FWacomBackpackResolvedLayout& Layout : Result)
	{
		Layout.CardCenter.Y = CenterY;
	}
	return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackHandLensStripLayoutSpec,
	"Wacom.UI.Backpack.Workspace.HandLensStrip.Layout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackHandLensStripLayoutSpec::RunTest(const FString& Parameters)
{
	const UWacomBackpackWorkspaceStyle* Style = GetDefault<UWacomBackpackWorkspaceStyle>();
	for (const int32 CardCount : { 0, 1, 2, 3, 5, 7, 10, 15, 21 })
	{
		for (const float CorridorWidth : { 1232.0f, 1632.0f, 1872.0f, 2512.0f })
		{
			const FSlateRect Corridor(24.0f, 100.0f, 24.0f + CorridorWidth, 520.0f);
			const TArray<FWacomBackpackResolvedLayout> Bases =
				BuildHorizontalBases(CardCount, 310.0f);
			for (const float Focus : {
				0.0f,
				CardCount > 0 ? static_cast<float>(CardCount - 1) * 0.5f : 0.0f,
				CardCount > 0 ? static_cast<float>(CardCount - 1) : 0.0f })
			{
				const FWacomBackpackHandLensStripLayout Layout =
					FWacomBackpackWorkspaceLayoutSolver::BuildHandLensStripLayout(
						CardCount,
						Focus,
						Corridor,
						Style->GetCardDisplaySize(),
						Bases,
						Style->HandLensFullGapPixels,
						Style->HandLensCompressedExposurePixels,
						Style->HandLensMinimumExposurePixels,
						Style->HandLensPromotionOverlapTolerancePixels);
				TestEqual(TEXT("Hand Lens preserves card count"), Layout.Cards.Num(), CardCount);
				TestEqual(TEXT("Hand Lens publishes one visible band per card"),
					Layout.VisibleBands.Num(), CardCount);
				if (CardCount <= 0)
				{
					continue;
				}
				TestEqual(TEXT("Three segments cover every card"),
					Layout.LeftStackCount + Layout.ExpandedCardCount
						+ Layout.RightStackCount,
					CardCount);
				TestTrue(TEXT("Hand Lens always has a non-empty complete region"),
					Layout.ExpandedCardCount >= 1);
				TestTrue(TEXT("Target visible bands remain horizontally continuous"),
					BandsAreHorizontallyContinuous(Layout.VisibleBands));
				for (int32 Index = 0; Index < Layout.Cards.Num(); ++Index)
				{
					const FWacomBackpackResolvedLayout& Card = Layout.Cards[Index];
					TestTrue(TEXT("Hand Lens cards remain unrotated"),
						FMath::IsNearlyZero(Card.AngleDegrees));
					TestTrue(TEXT("Every card remains inside the stable corridor"),
						Card.CardCenter.X - Style->GetCardDisplaySize().X * 0.5f
							>= Corridor.Left - 0.1f
						&& Card.CardCenter.X + Style->GetCardDisplaySize().X * 0.5f
							<= Corridor.Right + 0.1f);
					if (Index > 0)
					{
						TestTrue(TEXT("Card identity order stays left-to-right"),
							Card.CardCenter.X > Layout.Cards[Index - 1].CardCenter.X);
					}
				}
				if (Layout.LeftStackCount > 1 || Layout.RightStackCount > 1)
				{
					TestTrue(TEXT("Strict compressed exposure keeps the authored minimum"),
						Layout.EffectiveCompressedExposurePixels
							>= Style->HandLensMinimumExposurePixels - 0.1f);
				}
			}
		}
	}

	const int32 FittingCount = 3;
	const float FullWidth = FittingCount * Style->GetCardDisplaySize().X
		+ (FittingCount - 1) * Style->HandLensFullGapPixels;
	const FSlateRect FittingCorridor(0.0f, 0.0f, FullWidth + 80.0f, 420.0f);
	const FWacomBackpackHandLensStripLayout AllFit =
		FWacomBackpackWorkspaceLayoutSolver::BuildHandLensStripLayout(
			FittingCount,
			1.0f,
			FittingCorridor,
			Style->GetCardDisplaySize(),
			BuildHorizontalBases(FittingCount, 210.0f),
			Style->HandLensFullGapPixels,
			Style->HandLensCompressedExposurePixels,
			Style->HandLensMinimumExposurePixels,
			Style->HandLensPromotionOverlapTolerancePixels);
	TestEqual(TEXT("A fitting hand shows every card completely"),
		AllFit.ExpandedCardCount, FittingCount);
	TestEqual(TEXT("A fitting hand has no left stack"), AllFit.LeftStackCount, 0);
	TestEqual(TEXT("A fitting hand has no right stack"), AllFit.RightStackCount, 0);

	const FWacomBackpackResolvedPileContentLayout StablePile =
		FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
			21,
			FVector2D(48.0f, 120.0f),
			FVector2D(260.0f, 48.0f),
			FVector2D(1920.0f, 1080.0f),
			Style->GetCardDisplaySize(),
			true,
			Style->PileCollapsedExposurePixels,
			Style->HandLensFullGapPixels,
			Style->HandLensCompressedExposurePixels,
			Style->HandLensMinimumExposurePixels,
			Style->HandLensPromotionOverlapTolerancePixels,
			Style->PileEdgeMarginPixels,
			Style->ExpandedCardHoverLiftPixels);
	TestEqual(TEXT("Pile scene publishes every Hand Lens card"), StablePile.Cards.Num(), 21);
	TestTrue(TEXT("Pile frame contains the stable Hand Lens corridor"),
		StablePile.FrameRect.Left <= StablePile.FocusCorridorRect.Left
			&& StablePile.FrameRect.Right >= StablePile.FocusCorridorRect.Right);
	if (StablePile.Cards.Num() > 0)
	{
		const float FocusedCardTop = StablePile.Cards[0].CardCenter.Y
			- Style->GetCardDisplaySize().Y * 0.5f
			- Style->ExpandedCardHoverLiftPixels;
		const float FocusedCardBottom = StablePile.Cards[0].CardCenter.Y
			+ Style->GetCardDisplaySize().Y * 0.5f
			- Style->ExpandedCardHoverLiftPixels;
		const bool bHeaderClear = FocusedCardTop >= StablePile.HeaderRect.Bottom + 7.9f
			|| FocusedCardBottom <= StablePile.HeaderRect.Top - 7.9f;
		TestTrue(TEXT("Expanded hover lift preserves title drag-handle clearance"), bHeaderClear);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCarryFocusWindowCompatibilitySpec,
	"Wacom.UI.Backpack.Workspace.HandLensStrip.CarryCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCarryFocusWindowCompatibilitySpec::RunTest(const FString& Parameters)
{
	const UWacomBackpackWorkspaceStyle* Style = GetDefault<UWacomBackpackWorkspaceStyle>();
	TestEqual(TEXT("Carry default exposes only the current card as a full card"),
		Style->FocusWindowMaximumCards, 1);
	int32 WindowStart = INDEX_NONE;
	const FVector2D Pointer(800.0f, 500.0f);
	const TArray<FWacomBackpackCarriedStripLayout> Carry =
		FWacomBackpackWorkspaceLayoutSolver::BuildCarriedFocusWindowLayout(
			15,
			7,
			14,
			Pointer,
			1600.0f,
			Style->GetCardDisplaySize().X,
			Style->FocusWindowMaximumCards,
			Style->FocusWindowFullGapPixels,
			Style->FocusWindowCompressedExposurePixels,
			Style->FocusWindowMinimumExposurePixels,
			Style->CurrentCardLiftPixels,
			INDEX_NONE,
			&WindowStart);
	TestEqual(TEXT("Carry still uses the existing FocusWindow card count"), Carry.Num(), 15);
	TestEqual(TEXT("The one-card full window follows the current card"), WindowStart, 7);
	TestTrue(TEXT("Carry current card remains anchored horizontally to the pointer"),
		Carry.IsValidIndex(7)
			&& FMath::IsNearlyEqual(Carry[7].Transform.CardCenter.X, Pointer.X, 0.1f));
	TestTrue(TEXT("The immediate left card stays in the compressed segment"),
		Carry.IsValidIndex(6)
			&& Pointer.X - Carry[6].Transform.CardCenter.X < Style->GetCardDisplaySize().X);
	TestTrue(TEXT("The immediate right card stays in the compressed segment"),
		Carry.IsValidIndex(8)
			&& Carry[8].Transform.CardCenter.X - Pointer.X < Style->GetCardDisplaySize().X);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackHandLensStripInteractionSpec,
	"Wacom.UI.Backpack.Workspace.HandLensStrip.Interaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackHandLensStripInteractionSpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	const TSharedRef<SWidget> WorkspaceSlateRoot = Workspace->TakeWidget();
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Model =
		MakeShared<FWacomBackpackWorkspaceInteractionModel>();
	Workspace->SetInteractionModel(Model, nullptr);
	const UWacomBackpackWorkspaceStyle* Style = GetDefault<UWacomBackpackWorkspaceStyle>();
	const FWacomBackpackResolvedPileContentLayout Initial =
		FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
			7,
			FVector2D(120.0f, 20.0f),
			FVector2D(260.0f, 48.0f),
			FVector2D(1280.0f, 720.0f),
			Style->GetCardDisplaySize(),
			true,
			Style->PileCollapsedExposurePixels,
			Style->HandLensFullGapPixels,
			Style->HandLensCompressedExposurePixels,
			Style->HandLensMinimumExposurePixels,
			Style->HandLensPromotionOverlapTolerancePixels,
			Style->PileEdgeMarginPixels,
			Style->ExpandedCardHoverLiftPixels);

	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.HandLens.Interaction");
	TArray<TStrongObjectPtr<UWacomDeckCardWidget>> OwnedCards;
	TArray<TObjectPtr<UWacomDeckCardWidget>> Cards;
	TArray<FWacomBackpackExpandedPileFocusCard> FocusCards;
	for (int32 Index = 0; Index < 7; ++Index)
	{
		TStrongObjectPtr<UWacomDeckCardWidget> Card(NewObject<UWacomDeckCardWidget>());
		FCardInstance Instance;
		Instance.InstanceId = FGuid(Index + 1, 51, 52, 53);
		Instance.Definition = Definition.Get();
		Card->SetCard(Instance, EZoneKind::BattleDeck, FGuid());
		Workspace->GetStaticCardLayer()->AddChildToCanvas(Card.Get());
		Workspace->PrimeCardBaseLayout(
			*Card,
			Initial.Cards[Index].CardCenter,
			Style->GetCardDisplaySize(),
			0.0f,
			3000 + Initial.Cards[Index].LayerRank);
		FWacomBackpackExpandedPileFocusCard& Focus = FocusCards.AddDefaulted_GetRef();
		Focus.Card = Card.Get();
		Focus.NeutralCenter = Initial.Cards[Index].CardCenter;
		Focus.NeutralLayerRank = 3000 + Initial.Cards[Index].LayerRank;
		Focus.NeutralHitBand = Initial.FocusHitBands[Index];
		Focus.CurrentHitBand = Focus.NeutralHitBand;
		Cards.Add(Card.Get());
		OwnedCards.Add(MoveTemp(Card));
	}
	Workspace->BindWorkspaceCards(Cards, 9);
	Workspace->SetExpandedPileFocusContract(
		EZoneKind::BattleDeck,
		FGuid(),
		Initial.HeaderRect,
		Initial.FocusCorridorRect,
		FocusCards);

	UWacomDeckCardWidget* LastBrowseCard = nullptr;
	Workspace->OnBrowseFocusChangedNative.AddLambda(
		[&LastBrowseCard](UWacomDeckCardWidget* Card)
		{
			LastBrowseCard = Card;
		});
	const FVector2D LeftLensPointer(
		Initial.FocusCorridorRect.Left + 4.0f,
		Initial.Cards[0].CardCenter.Y);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, LeftLensPointer);
	const FWacomBackpackWorkspaceAutomationTestView LeftLens =
		Workspace->GetAutomationTestView();
	TestTrue(TEXT("Pointer X drives the continuous lens toward the left edge"),
		LeftLens.ExpandedPileLensFocus < 0.1f);
	TestTrue(TEXT("Visual card under the pointer drives browse detail"),
		OwnedCards.IsValidIndex(LeftLens.ExpandedPileFocusIndex)
			&& LastBrowseCard == OwnedCards[LeftLens.ExpandedPileFocusIndex].Get());
	const int32 RebuildCount = LeftLens.ExpandedPileFocusLayoutRebuildCount;
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace, LeftLensPointer + FVector2D(1.0f, 0.0f));
	TestEqual(TEXT("Movement inside the same Hand Lens segment does not reflow"),
		Workspace->GetAutomationTestView().ExpandedPileFocusLayoutRebuildCount,
		RebuildCount);

	FWacomBackpackScreenTestAccess::TickWorkspaceCardMotion(
		*Workspace, Style->FocusReflowSeconds * 0.5f);
	const int32 VisualFocusIndex = Workspace->GetAutomationTestView().ExpandedPileFocusIndex;
	TestTrue(TEXT("Motion tick re-resolves the visual card beneath a stationary pointer"),
		OwnedCards.IsValidIndex(VisualFocusIndex)
			&& LastBrowseCard == OwnedCards[VisualFocusIndex].Get());
	if (OwnedCards.IsValidIndex(VisualFocusIndex))
	{
		TestTrue(TEXT("Pointer-down carries the same visual card used by details"),
			FWacomBackpackScreenTestAccess::PressExpandedPileVisualCard(
				*Workspace, LeftLensPointer));
		const FWacomBackpackWorkspaceAutomationTestView Carry =
			Workspace->GetAutomationTestView();
		TestTrue(TEXT("Visual pointer-down begins carry with the browse identity"),
			Carry.CarriedInstanceIds.Num() == 1
				&& Carry.CarriedInstanceIds[0]
					== OwnedCards[VisualFocusIndex]->GetCardInstanceId());
		Workspace->CancelInteraction();
	}

	Workspace->SetExpandedPileFocusContract(
		EZoneKind::Backpack, FGuid(), FSlateRect(), FSlateRect(), {});
	const FWacomBackpackWorkspaceAutomationTestView Reset =
		Workspace->GetAutomationTestView();
	TestEqual(TEXT("Collapsing clears active browse focus"),
		Reset.ExpandedPileFocusIndex, INDEX_NONE);
	TestEqual(TEXT("Collapsing clears the transient Hand Lens segment"),
		Reset.ExpandedPileLensExpandedStartIndex, INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackHandLensStripShiftLockSpec,
	"Wacom.UI.Backpack.Workspace.HandLensStrip.ShiftLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackHandLensStripShiftLockSpec::RunTest(const FString& Parameters)
{
	const UWacomBackpackWorkspaceStyle* Style = GetDefault<UWacomBackpackWorkspaceStyle>();
	for (const int32 CardCount : { 7, 15, 21 })
	{
		TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
			NewObject<UWacomBackpackWorkspaceWidget>());
		const TSharedRef<SWidget> WorkspaceSlateRoot = Workspace->TakeWidget();
		TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Model =
			MakeShared<FWacomBackpackWorkspaceInteractionModel>();
		Workspace->SetInteractionModel(Model, nullptr);
		const FWacomBackpackResolvedPileContentLayout Initial =
			FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
				CardCount,
				FVector2D(120.0f, 20.0f),
				FVector2D(260.0f, 48.0f),
				FVector2D(1920.0f, 1080.0f),
				Style->GetCardDisplaySize(),
				true,
				Style->PileCollapsedExposurePixels,
				Style->HandLensFullGapPixels,
				Style->HandLensCompressedExposurePixels,
				Style->HandLensMinimumExposurePixels,
				Style->HandLensPromotionOverlapTolerancePixels,
				Style->PileEdgeMarginPixels,
				Style->ExpandedCardHoverLiftPixels);

		TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
		Definition->CardId = FName(*FString::Printf(TEXT("Backpack.HandLens.ShiftLock.%d"), CardCount));
		TArray<TStrongObjectPtr<UWacomDeckCardWidget>> OwnedCards;
		TArray<TObjectPtr<UWacomDeckCardWidget>> Cards;
		TArray<FWacomBackpackExpandedPileFocusCard> FocusCards;
		OwnedCards.Reserve(CardCount);
		Cards.Reserve(CardCount);
		FocusCards.Reserve(CardCount);
		for (int32 Index = 0; Index < CardCount; ++Index)
		{
			TStrongObjectPtr<UWacomDeckCardWidget> Card(NewObject<UWacomDeckCardWidget>());
			FCardInstance Instance;
			Instance.InstanceId = FGuid(Index + 1, CardCount, 61, 62);
			Instance.Definition = Definition.Get();
			Card->SetCard(Instance, EZoneKind::BattleDeck, FGuid());
			Workspace->GetStaticCardLayer()->AddChildToCanvas(Card.Get());
			Workspace->PrimeCardBaseLayout(
				*Card,
				Initial.Cards[Index].CardCenter,
				Style->GetCardDisplaySize(),
				0.0f,
				3000 + Initial.Cards[Index].LayerRank);
			FWacomBackpackExpandedPileFocusCard& Focus = FocusCards.AddDefaulted_GetRef();
			Focus.Card = Card.Get();
			Focus.NeutralCenter = Initial.Cards[Index].CardCenter;
			Focus.NeutralLayerRank = 3000 + Initial.Cards[Index].LayerRank;
			Focus.NeutralHitBand = Initial.FocusHitBands[Index];
			Focus.CurrentHitBand = Focus.NeutralHitBand;
			Cards.Add(Card.Get());
			OwnedCards.Add(MoveTemp(Card));
		}
		Workspace->BindWorkspaceCards(Cards, static_cast<uint64>(CardCount));
		Workspace->SetExpandedPileFocusContract(
			EZoneKind::BattleDeck,
			FGuid(),
			Initial.HeaderRect,
			Initial.FocusCorridorRect,
			FocusCards);

		const FVector2D LeftPointer(
			Initial.FocusCorridorRect.Left + 4.0f,
			Initial.Cards[0].CardCenter.Y);
		const FVector2D RightPointer(
			Initial.FocusCorridorRect.Right - 4.0f,
			Initial.Cards.Last().CardCenter.Y);
		FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
			*Workspace, LeftPointer);
		const FWacomBackpackWorkspaceAutomationTestView BeforeLock =
			Workspace->GetAutomationTestView();
		TestTrue(TEXT("Left Shift is handled while a pile can browse"),
			FWacomBackpackScreenTestAccess::SetWorkspaceHandLensLock(*Workspace, true));
		TestTrue(TEXT("The Hand Lens exposes its transient lock state"),
			Workspace->GetAutomationTestView().bExpandedPileLensInputLocked);
		TestTrue(TEXT("Repeated Shift keydown is consumed without changing state"),
			FWacomBackpackScreenTestAccess::SetWorkspaceHandLensLock(
				*Workspace, true, true));

		FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
			*Workspace, RightPointer);
		const FWacomBackpackWorkspaceAutomationTestView WhileLocked =
			Workspace->GetAutomationTestView();
		TestTrue(TEXT("Locked pointer movement keeps the continuous lens focus"),
			FMath::IsNearlyEqual(
				WhileLocked.ExpandedPileLensFocus,
				BeforeLock.ExpandedPileLensFocus,
				0.01f));
		TestEqual(TEXT("Locked pointer movement does not rebuild Hand Lens layout"),
			WhileLocked.ExpandedPileFocusLayoutRebuildCount,
			BeforeLock.ExpandedPileFocusLayoutRebuildCount);
		TestTrue(TEXT("Visual browsing remains active while layout is locked"),
			WhileLocked.ExpandedPileFocusIndex != INDEX_NONE);
		TestTrue(TEXT("Releasing Shift is handled while the lock is active"),
			FWacomBackpackScreenTestAccess::SetWorkspaceHandLensLock(*Workspace, false));
		const FWacomBackpackWorkspaceAutomationTestView AfterRelease =
			Workspace->GetAutomationTestView();
		TestFalse(TEXT("Releasing Shift clears the transient lens lock"),
			AfterRelease.bExpandedPileLensInputLocked);
		TestTrue(TEXT("Shift release immediately resumes from the latest pointer"),
			AfterRelease.ExpandedPileLensFocus
				> static_cast<float>(CardCount - 1) - 0.1f);
		const bool bSegmentChanged =
			AfterRelease.ExpandedPileLensLeftStackCount
				!= WhileLocked.ExpandedPileLensLeftStackCount
			|| AfterRelease.ExpandedPileLensExpandedStartIndex
				!= WhileLocked.ExpandedPileLensExpandedStartIndex
			|| AfterRelease.ExpandedPileLensExpandedCardCount
				!= WhileLocked.ExpandedPileLensExpandedCardCount
			|| AfterRelease.ExpandedPileLensRightStackCount
				!= WhileLocked.ExpandedPileLensRightStackCount;
		TestEqual(TEXT("Immediate resume rebuilds only when the Hand Lens segment changes"),
			AfterRelease.ExpandedPileFocusLayoutRebuildCount,
			WhileLocked.ExpandedPileFocusLayoutRebuildCount + (bSegmentChanged ? 1 : 0));

		FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointerWithShiftState(
			*Workspace, LeftPointer, false, true);
		const FWacomBackpackWorkspaceAutomationTestView RightShift =
			Workspace->GetAutomationTestView();
		TestFalse(TEXT("Right Shift does not lock Hand Lens input"),
			RightShift.bExpandedPileLensInputLocked);
		TestTrue(TEXT("Right Shift pointer movement still drives Hand Lens"),
			RightShift.ExpandedPileLensFocus < 0.1f);
		FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointerWithShiftState(
			*Workspace, RightPointer, true);
		const FWacomBackpackWorkspaceAutomationTestView PointerLock =
			Workspace->GetAutomationTestView();
		TestTrue(TEXT("A left-Shift pointer event recovers a missed keydown"),
			PointerLock.bExpandedPileLensInputLocked);
		TestTrue(TEXT("Recovered pointer lock keeps the previous lens focus"),
			PointerLock.ExpandedPileLensFocus < 0.1f);
		FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointerWithShiftState(
			*Workspace, RightPointer, false);
		const FWacomBackpackWorkspaceAutomationTestView PointerUnlock =
			Workspace->GetAutomationTestView();
		TestFalse(TEXT("A pointer event without left Shift recovers a missed keyup"),
			PointerUnlock.bExpandedPileLensInputLocked);
		TestTrue(TEXT("Recovered keyup resumes from the latest pointer"),
			PointerUnlock.ExpandedPileLensFocus
				> static_cast<float>(CardCount - 1) - 0.1f);

		if (CardCount == 7 && OwnedCards.IsValidIndex(PointerUnlock.ExpandedPileFocusIndex))
		{
			const FGuid ExpectedCarryId =
				OwnedCards[PointerUnlock.ExpandedPileFocusIndex]->GetCardInstanceId();
			FWacomBackpackScreenTestAccess::SetWorkspaceHandLensLock(*Workspace, true);
			TestTrue(TEXT("Shift-locked pointer-down still begins normal carry"),
				FWacomBackpackScreenTestAccess::PressExpandedPileVisualCard(
					*Workspace, RightPointer, true));
			const FWacomBackpackWorkspaceAutomationTestView Carry =
				Workspace->GetAutomationTestView();
			TestTrue(TEXT("Shift does not enter the Ctrl multi-selection path"),
				Carry.CarriedInstanceIds.Num() == 1
					&& Carry.CarriedInstanceIds[0] == ExpectedCarryId);
			TestFalse(TEXT("Beginning carry clears the transient lens lock"),
				Carry.bExpandedPileLensInputLocked);
			Workspace->CancelInteraction();
			FWacomBackpackScreenTestAccess::ClearWorkspaceSelection(*Workspace);
			FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
				*Workspace, LeftPointer);
			FWacomBackpackScreenTestAccess::SetWorkspaceHandLensLock(*Workspace, true);
			FWacomBackpackScreenTestAccess::LoseWorkspaceKeyboardFocus(*Workspace);
			TestFalse(TEXT("Losing keyboard focus clears the transient lens lock"),
				Workspace->GetAutomationTestView().bExpandedPileLensInputLocked);
			FWacomBackpackScreenTestAccess::SetWorkspaceHandLensLock(*Workspace, true);
			Workspace->SetExpandedPileFocusContract(
				EZoneKind::Backpack, FGuid(), FSlateRect(), FSlateRect(), {});
			TestFalse(TEXT("Collapsing the pile clears the transient lens lock"),
				Workspace->GetAutomationTestView().bExpandedPileLensInputLocked);
			continue;
		}
	}
	return true;
}

#endif
