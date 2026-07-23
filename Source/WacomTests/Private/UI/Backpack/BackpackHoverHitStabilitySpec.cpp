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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackExpandedCardHoverHitStabilitySpec,
	"Wacom.UI.Backpack.Workspace.HoverHitStability.BottomEdgeLift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackExpandedCardHoverHitStabilitySpec::RunTest(
	const FString& Parameters)
{
	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	const TSharedRef<SWidget> WorkspaceSlateRoot = Workspace->TakeWidget();
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Model =
		MakeShared<FWacomBackpackWorkspaceInteractionModel>();
	Workspace->SetInteractionModel(Model, nullptr);

	const UWacomBackpackWorkspaceStyle* Style =
		GetDefault<UWacomBackpackWorkspaceStyle>();
	const FWacomBackpackResolvedPileContentLayout Layout =
		FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
			1,
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
	if (!TestEqual(TEXT("Single-card layout is available"), Layout.Cards.Num(), 1)
		|| !TestEqual(TEXT("Single-card stable hit band is available"), Layout.FocusHitBands.Num(), 1))
	{
		return false;
	}

	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.HoverHitStability");
	TStrongObjectPtr<UWacomDeckCardWidget> Card(
		NewObject<UWacomDeckCardWidget>());
	FCardInstance Instance;
	Instance.InstanceId = FGuid(1, 91, 92, 93);
	Instance.Definition = Definition.Get();
	Card->SetCard(Instance, EZoneKind::BattleDeck, FGuid());
	Workspace->GetStaticCardLayer()->AddChildToCanvas(Card.Get());
	Workspace->PrimeCardBaseLayout(
		*Card,
		Layout.Cards[0].CardCenter,
		Style->GetCardDisplaySize(),
		Layout.Cards[0].AngleDegrees,
		3000 + Layout.Cards[0].LayerRank);

	FWacomBackpackExpandedPileFocusCard FocusCard;
	FocusCard.Card = Card.Get();
	FocusCard.NeutralCenter = Layout.Cards[0].CardCenter;
	FocusCard.NeutralAngleDegrees = Layout.Cards[0].AngleDegrees;
	FocusCard.NeutralLayerRank = 3000 + Layout.Cards[0].LayerRank;
	FocusCard.NeutralHitBand = Layout.FocusHitBands[0];
	FocusCard.CurrentHitBand = FocusCard.NeutralHitBand;
	const TObjectPtr<UWacomDeckCardWidget> BoundCard = Card.Get();
	Workspace->BindWorkspaceCards(MakeArrayView(&BoundCard, 1), 1);
	Workspace->SetExpandedPileFocusContract(
		EZoneKind::BattleDeck,
		FGuid(),
		Layout.HeaderRect,
		Layout.FocusCorridorRect,
		MakeArrayView(&FocusCard, 1));

	int32 FocusBroadcastCount = 0;
	int32 ClearBroadcastCount = 0;
	Workspace->OnBrowseFocusChangedNative.AddLambda(
		[&FocusBroadcastCount, &ClearBroadcastCount](UWacomDeckCardWidget* Focused)
		{
			if (Focused)
			{
				++FocusBroadcastCount;
			}
			else
			{
				++ClearBroadcastCount;
			}
		});

	const FSlateRect StableHitBand = Layout.FocusHitBands[0];
	const FVector2D BottomEdgePointer(
		(StableHitBand.Left + StableHitBand.Right) * 0.5f,
		StableHitBand.Bottom - 1.0f);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace,
		BottomEdgePointer);
	TestEqual(
		TEXT("Bottom-edge entry acquires the expanded card"),
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex,
		0);

	const float OscillationWindowSeconds =
		Style->HoverEnterSeconds
		+ Style->FocusExitDelaySeconds
		+ Style->HoverExitSeconds
		+ 0.4f;
	FWacomBackpackScreenTestAccess::TickWorkspaceCardMotion(
		*Workspace,
		OscillationWindowSeconds);
	TestEqual(
		TEXT("Stationary pointer retains focus after the card completes its lift"),
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex,
		0);
	TestEqual(
		TEXT("Lift does not rebroadcast focus"),
		FocusBroadcastCount,
		1);
	TestEqual(
		TEXT("Lift does not emit an unhover"),
		ClearBroadcastCount,
		0);

	const FVector2D LiftedBodyPointer(
		BottomEdgePointer.X,
		StableHitBand.Top - Style->ExpandedCardHoverLiftPixels * 0.5f);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace,
		LiftedBodyPointer);
	TestEqual(
		TEXT("Pointer following the lifted visual body retains focus"),
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex,
		0);

	const FVector2D OutsideBothRegions(
		BottomEdgePointer.X,
		StableHitBand.Bottom + Style->FocusHitHysteresisPixels + 4.0f);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace,
		OutsideBothRegions);
	FWacomBackpackScreenTestAccess::TickWorkspaceCardMotion(
		*Workspace,
		Style->FocusExitDelaySeconds + 0.05f);
	TestEqual(
		TEXT("Leaving both stable and visual regions clears focus"),
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex,
		INDEX_NONE);
	TestEqual(
		TEXT("A genuine exit emits exactly one unhover"),
		ClearBroadcastCount,
		1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCompressedPileHoverIdentityStabilitySpec,
	"Wacom.UI.Backpack.Workspace.HoverHitStability.CompressedPileBottomEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCompressedPileHoverIdentityStabilitySpec::RunTest(
	const FString& Parameters)
{
	constexpr int32 CardCount = 15;
	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	const TSharedRef<SWidget> WorkspaceSlateRoot = Workspace->TakeWidget();
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Model =
		MakeShared<FWacomBackpackWorkspaceInteractionModel>();
	Workspace->SetInteractionModel(Model, nullptr);

	const UWacomBackpackWorkspaceStyle* Style =
		GetDefault<UWacomBackpackWorkspaceStyle>();
	const FWacomBackpackResolvedPileContentLayout Initial =
		FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
			CardCount,
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
	if (!TestEqual(TEXT("Compressed-pile layout has every card"), Initial.Cards.Num(), CardCount)
		|| !TestEqual(TEXT("Compressed-pile layout has every stable band"), Initial.FocusHitBands.Num(), CardCount))
	{
		return false;
	}

	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.HoverHitStability.CompressedPile");
	TArray<TStrongObjectPtr<UWacomDeckCardWidget>> OwnedCards;
	TArray<TObjectPtr<UWacomDeckCardWidget>> Cards;
	TArray<FWacomBackpackExpandedPileFocusCard> FocusCards;
	OwnedCards.Reserve(CardCount);
	Cards.Reserve(CardCount);
	FocusCards.Reserve(CardCount);
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		TStrongObjectPtr<UWacomDeckCardWidget> Card(
			NewObject<UWacomDeckCardWidget>());
		FCardInstance Instance;
		Instance.InstanceId = FGuid(Index + 1, 94, 95, 96);
		Instance.Definition = Definition.Get();
		Card->SetCard(Instance, EZoneKind::BattleDeck, FGuid());
		Workspace->GetStaticCardLayer()->AddChildToCanvas(Card.Get());
		Workspace->PrimeCardBaseLayout(
			*Card,
			Initial.Cards[Index].CardCenter,
			Style->GetCardDisplaySize(),
			Initial.Cards[Index].AngleDegrees,
			3000 + Initial.Cards[Index].LayerRank);

		FWacomBackpackExpandedPileFocusCard& Focus =
			FocusCards.AddDefaulted_GetRef();
		Focus.Card = Card.Get();
		Focus.NeutralCenter = Initial.Cards[Index].CardCenter;
		Focus.NeutralAngleDegrees = Initial.Cards[Index].AngleDegrees;
		Focus.NeutralLayerRank = 3000 + Initial.Cards[Index].LayerRank;
		Focus.NeutralHitBand = Initial.FocusHitBands[Index];
		Focus.CurrentHitBand = Focus.NeutralHitBand;
		Cards.Add(Card.Get());
		OwnedCards.Add(MoveTemp(Card));
	}
	Workspace->BindWorkspaceCards(Cards, 15);
	Workspace->SetExpandedPileFocusContract(
		EZoneKind::BattleDeck,
		FGuid(),
		Initial.HeaderRect,
		Initial.FocusCorridorRect,
		FocusCards);

	// Find a self-consistent pointer X where at least two compressed right-stack
	// card bodies overlap. The runtime derives LensFocus from this same X, so the
	// chosen overlap remains the target after reflow.
	float PointerX = 0.0f;
	int32 CompressedCardIndex = INDEX_NONE;
	FWacomBackpackHandLensStripLayout SettledLayout;
	const float CorridorWidth =
		Initial.FocusCorridorRect.Right - Initial.FocusCorridorRect.Left;
	for (float CandidateX = Initial.FocusCorridorRect.Left + 1.0f;
		CandidateX < Initial.FocusCorridorRect.Right - 1.0f;
		CandidateX += 1.0f)
	{
		const float LensAlpha = FMath::Clamp(
			(CandidateX - Initial.FocusCorridorRect.Left) / CorridorWidth,
			0.0f,
			1.0f);
		const FWacomBackpackHandLensStripLayout CandidateLayout =
			FWacomBackpackWorkspaceLayoutSolver::BuildHandLensStripLayout(
				CardCount,
				LensAlpha * static_cast<float>(CardCount - 1),
				Initial.FocusCorridorRect,
				Style->GetCardDisplaySize(),
				Initial.Cards,
				Style->HandLensFullGapPixels,
				Style->HandLensCompressedExposurePixels,
				Style->HandLensMinimumExposurePixels,
				Style->HandLensPromotionOverlapTolerancePixels);
		const int32 RightStackStart =
			CandidateLayout.ExpandedStartIndex
			+ CandidateLayout.ExpandedCardCount;
		int32 OverlappingBodyCount = 0;
		int32 HighestLayerRank = MIN_int32;
		int32 HighestBodyIndex = INDEX_NONE;
		for (int32 Index = RightStackStart; Index < CardCount; ++Index)
		{
			const float CardLeft =
				CandidateLayout.Cards[Index].CardCenter.X
				- Style->GetCardDisplaySize().X * 0.5f;
			const float CardRight =
				CandidateLayout.Cards[Index].CardCenter.X
				+ Style->GetCardDisplaySize().X * 0.5f;
			if (CandidateX >= CardLeft && CandidateX <= CardRight)
			{
				++OverlappingBodyCount;
				if (CandidateLayout.Cards[Index].LayerRank > HighestLayerRank)
				{
					HighestLayerRank = CandidateLayout.Cards[Index].LayerRank;
					HighestBodyIndex = Index;
				}
			}
		}
		if (OverlappingBodyCount >= 2)
		{
			PointerX = CandidateX;
			CompressedCardIndex = HighestBodyIndex;
			SettledLayout = CandidateLayout;
			break;
		}
	}
	if (!TestTrue(
		TEXT("A stable pointer exists inside overlapping compressed cards"),
		CompressedCardIndex != INDEX_NONE))
	{
		return false;
	}

	// Enter above the card bodies first and let the reflow settle; this isolates
	// bottom-edge hover ownership from layout-transition changes.
	const FVector2D ReflowPointer(
		PointerX,
		Initial.FocusCorridorRect.Top + 1.0f);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace,
		ReflowPointer);
	FWacomBackpackScreenTestAccess::TickWorkspaceCardMotion(
		*Workspace,
		Style->FocusReflowSeconds + 0.1f);

	int32 FocusBroadcastCount = 0;
	TArray<FGuid> FocusSequence;
	Workspace->OnBrowseFocusChangedNative.AddLambda(
		[&FocusBroadcastCount, &FocusSequence](UWacomDeckCardWidget* Focused)
		{
			if (Focused)
			{
				++FocusBroadcastCount;
				FocusSequence.Add(Focused->GetCardInstanceId());
			}
		});

	const FVector2D BottomEdgePointer(
		PointerX,
		SettledLayout.Cards[CompressedCardIndex].CardCenter.Y
			+ Style->GetCardDisplaySize().Y * 0.5f
			- 1.0f);
	int32 VisualBodyCountAtPointer = 0;
	for (const TStrongObjectPtr<UWacomDeckCardWidget>& Card : OwnedCards)
	{
		const UCanvasPanelSlot* Slot =
			Card.IsValid() ? Cast<UCanvasPanelSlot>(Card->Slot) : nullptr;
		if (!Slot)
		{
			continue;
		}
		const FVector2D VisualTopLeft =
			Slot->GetPosition() + Card->GetBackpackLocalMotionTranslation();
		const FVector2D VisualBottomRight = VisualTopLeft + Slot->GetSize();
		if (BottomEdgePointer.X >= VisualTopLeft.X
			&& BottomEdgePointer.X <= VisualBottomRight.X
			&& BottomEdgePointer.Y >= VisualTopLeft.Y
			&& BottomEdgePointer.Y <= VisualBottomRight.Y)
		{
			++VisualBodyCountAtPointer;
		}
	}
	TestTrue(
		TEXT("The selected bottom-edge point overlaps multiple visual cards"),
		VisualBodyCountAtPointer >= 2);
	FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
		*Workspace,
		BottomEdgePointer);
	const int32 InitialFocusIndex =
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex;
	TestTrue(
		TEXT("Compressed-pile bottom edge acquires one card"),
		OwnedCards.IsValidIndex(InitialFocusIndex));

	FWacomBackpackScreenTestAccess::TickWorkspaceCardMotion(
		*Workspace,
		Style->HoverEnterSeconds
			+ Style->HoverExitSeconds
			+ Style->FocusReflowSeconds
			+ 0.8f);
	const int32 FinalFocusIndex =
		Workspace->GetAutomationTestView().ExpandedPileFocusIndex;
	TestEqual(
		TEXT("Stationary pointer keeps the same compressed-pile identity"),
		FinalFocusIndex,
		InitialFocusIndex);
	TestEqual(
		TEXT("Overlapping cards do not alternate hover ownership"),
		FocusBroadcastCount,
		1);
	if (FocusBroadcastCount != 1)
	{
		FString SequenceText;
		for (const FGuid& InstanceId : FocusSequence)
		{
			if (!SequenceText.IsEmpty())
			{
				SequenceText += TEXT(" -> ");
			}
			SequenceText += InstanceId.ToString(EGuidFormats::Digits);
		}
		AddInfo(FString::Printf(
			TEXT("Observed compressed-pile hover sequence: %s"),
			*SequenceText));
	}
	if (OwnedCards.IsValidIndex(InitialFocusIndex))
	{
		TestTrue(
			TEXT("Pointer-down accepts the retained browse identity"),
			FWacomBackpackScreenTestAccess::PressExpandedPileVisualCard(
				*Workspace,
				BottomEdgePointer));
		const FWacomBackpackWorkspaceAutomationTestView Carry =
			Workspace->GetAutomationTestView();
		TestTrue(
			TEXT("Pointer-down carries the same card that remained focused"),
			Carry.CarriedInstanceIds.Num() == 1
				&& Carry.CarriedInstanceIds[0]
					== OwnedCards[InitialFocusIndex]->GetCardInstanceId());
		Workspace->CancelInteraction();
	}
	return true;
}

#endif
