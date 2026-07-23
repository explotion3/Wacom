// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceAccessibility.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceOverlayPainter.h"
#include "Blueprint/WidgetTree.h"
#include "Cards/CardDefinition.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "../BackpackScreenTestAccess.h"

struct FWacomBackpackWorkspaceCardTestAccess
{
	static bool IsFocused(const UWacomDeckCardWidget& Card)
	{
		return Card.bWorkspaceNavigationFocused;
	}

	static EWacomBackpackWorkspaceCardSemanticIcon SemanticIcon(
		const UWacomDeckCardWidget& Card)
	{
		return Card.WorkspaceSemanticIcon;
	}

	static bool HasFocusBrush(const UWacomDeckCardWidget& Card)
	{
		return Card.WorkspaceFocusPaintBrush.GetResourceObject() != nullptr;
	}

	static bool HasSemanticBrush(const UWacomDeckCardWidget& Card)
	{
		return Card.WorkspaceSemanticPaintBrush.GetResourceObject() != nullptr;
	}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceOverlayOwnershipSpec,
	"Wacom.UI.Backpack.Workspace.OverlayPaint.Ownership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceOverlayOwnershipSpec::RunTest(const FString& Parameters)
{
	UClass* DeckCardClass = LoadClass<UWacomDeckCardWidget>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_WacomDeckCardWidget.WBP_WacomDeckCardWidget_C"));
	UWacomBackpackWorkspaceStyle* Style = LoadObject<UWacomBackpackWorkspaceStyle>(
		nullptr,
		TEXT("/Game/Wacom/UI/Backpack/DA_BackpackWorkspaceStyle.DA_BackpackWorkspaceStyle"));
	TestNotNull(TEXT("Formal deck card class is loadable"), DeckCardClass);
	TestNotNull(TEXT("Formal workspace style is loadable"), Style);
	if (!DeckCardClass || !Style)
	{
		return false;
	}

	TStrongObjectPtr<UWacomDeckCardWidget> Card(
		NewObject<UWacomDeckCardWidget>(GetTransientPackage(), DeckCardClass));
	const TSharedRef<SWidget> RetainedCardSlate = Card->TakeWidget();
	Card->SetWorkspaceAccessibilityState(
		true,
		EWacomBackpackWorkspaceCardSemanticIcon::Selected,
		*Style);
	TestTrue(TEXT("Native marker state retains independent navigation focus"),
		FWacomBackpackWorkspaceCardTestAccess::IsFocused(*Card));
	TestEqual(TEXT("Native marker state retains the resolved semantic icon"),
		FWacomBackpackWorkspaceCardTestAccess::SemanticIcon(*Card),
		EWacomBackpackWorkspaceCardSemanticIcon::Selected);
	TestTrue(TEXT("Native focus marker retains the authored Style brush"),
		FWacomBackpackWorkspaceCardTestAccess::HasFocusBrush(*Card));
	TestTrue(TEXT("Native semantic marker retains the authored Style brush"),
		FWacomBackpackWorkspaceCardTestAccess::HasSemanticBrush(*Card));

	const UImage* FocusImage = Card->WidgetTree
		? Cast<UImage>(Card->WidgetTree->FindWidget(TEXT("WorkspaceFocusIcon")))
		: nullptr;
	const UImage* StateImage = Card->WidgetTree
		? Cast<UImage>(Card->WidgetTree->FindWidget(TEXT("WorkspaceStateIcon")))
		: nullptr;
	TestNotNull(TEXT("Formal card retains the focus compatibility binding"), FocusImage);
	TestNotNull(TEXT("Formal card retains the semantic compatibility binding"), StateImage);
	if (FocusImage && StateImage)
	{
		TestEqual(TEXT("Compatibility focus image never competes with Workspace overlay paint"),
			FocusImage->GetVisibility(), ESlateVisibility::Collapsed);
		TestEqual(TEXT("Compatibility semantic image never competes with Workspace overlay paint"),
			StateImage->GetVisibility(), ESlateVisibility::Collapsed);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceOverlayLayerContractSpec,
	"Wacom.UI.Backpack.Workspace.OverlayPaint.LayerContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceOverlayLayerContractSpec::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Marquee fill and border consume two layers above the complete child tree"),
		FWacomBackpackWorkspaceOverlayPainter::ResolveMarqueeMaxLayer(41, true, true),
		43);
	TestEqual(TEXT("Inactive marquee preserves the complete child-tree layer"),
		FWacomBackpackWorkspaceOverlayPainter::ResolveMarqueeMaxLayer(41, false, true),
		41);
	TestEqual(TEXT("Zero-area marquee preserves the complete child-tree layer"),
		FWacomBackpackWorkspaceOverlayPainter::ResolveMarqueeMaxLayer(41, true, false),
		41);
	const int32 CompleteWorkspaceChildMaxLayer = 41;
	const int32 WorkspaceMarkerMaxLayer =
		FWacomBackpackWorkspaceOverlayPainter::ResolveCardMarkerMaxLayer(
			CompleteWorkspaceChildMaxLayer, true, true);
	TestEqual(TEXT("Focus and semantic markers share one layer above the complete Workspace tree"),
		WorkspaceMarkerMaxLayer, 42);
	TestTrue(TEXT("A per-card marker layer cannot guarantee ordering above a later sibling subtree"),
		FWacomBackpackWorkspaceOverlayPainter::ResolveCardMarkerMaxLayer(17, true, true)
			< CompleteWorkspaceChildMaxLayer);
	TestEqual(TEXT("Active marquee remains above the centralized card marker pass"),
		FWacomBackpackWorkspaceOverlayPainter::ResolveMarqueeMaxLayer(
			WorkspaceMarkerMaxLayer, true, true),
		44);
	TestEqual(TEXT("A card without markers preserves its child-tree layer"),
		FWacomBackpackWorkspaceOverlayPainter::ResolveCardMarkerMaxLayer(17, false, false),
		17);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceMarqueeHotPathSpec,
	"Wacom.UI.Backpack.Workspace.OverlayPaint.MarqueeHotPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceMarqueeHotPathSpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	const TSharedRef<SWidget> RetainedWorkspaceSlate = Workspace->TakeWidget();
	Workspace->SetInteractionModel(
		MakeShared<FWacomBackpackWorkspaceInteractionModel>(),
		nullptr);
	const FWacomBackpackMarqueePaintHotPathProbe Probe =
		FWacomBackpackScreenTestAccess::ProbeMarqueePaintHotPath(
			*Workspace,
			FVector2D(20.0f, 20.0f),
			FVector2D(360.0f, 240.0f));
	TestTrue(TEXT("Active marquee pointer movement remains handled"), Probe.bMoveHandled);
	TestTrue(TEXT("Active marquee pointer movement preserves marquee ownership"),
		Probe.bMarqueeRemainsActive);
	TestEqual(TEXT("Marquee pointer movement is paint-only"),
		Probe.FullPresentationRefreshCountAfter,
		Probe.FullPresentationRefreshCountBefore);
	TestEqual(TEXT("Marquee pointer movement never rebinds the Scene"),
		Probe.WorkspaceSceneBindCountAfter,
		Probe.WorkspaceSceneBindCountBefore);
	TestEqual(TEXT("Marquee pointer movement never rebuilds the carry strip"),
		Probe.CarryStripLayoutRebuildCountAfter,
		Probe.CarryStripLayoutRebuildCountBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceOverlayOcclusionSpec,
	"Wacom.UI.Backpack.Workspace.OverlayPaint.CardOcclusion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceOverlayOcclusionSpec::RunTest(const FString& Parameters)
{
	FWacomBackpackCardMarkerOccluder LowerCard;
	LowerCard.Order.LayerPriority = 3;
	LowerCard.Order.ZOrder = 2;
	LowerCard.Center = FVector2D(100.0f, 200.0f);
	LowerCard.Size = FVector2D(230.88f, 327.60f);

	FWacomBackpackCardMarkerOccluder HigherCard;
	HigherCard.Order.LayerPriority = 3;
	HigherCard.Order.ZOrder = 3;
	HigherCard.Center = FVector2D(150.0f, 200.0f);
	HigherCard.Size = LowerCard.Size;
	const TArray<FWacomBackpackCardMarkerOccluder> CardBodies = { LowerCard, HigherCard };

	const FVector2D LowerSemanticCenter =
		FWacomBackpackWorkspaceOverlayPainter::ResolveCardMarkerCenter(
			LowerCard.Center, LowerCard.Size, 0.0f, true);
	TestTrue(TEXT("A lower carried card marker is hidden when a higher card body overlaps it"),
		FWacomBackpackWorkspaceOverlayPainter::IsMarkerOccludedByHigherCard(
			LowerSemanticCenter,
			FVector2D(32.0f, 32.0f),
			0.0f,
			LowerCard.Order,
			CardBodies));

	const FVector2D HigherSemanticCenter =
		FWacomBackpackWorkspaceOverlayPainter::ResolveCardMarkerCenter(
			HigherCard.Center, HigherCard.Size, 0.0f, true);
	TestFalse(TEXT("The top card marker remains visible"),
		FWacomBackpackWorkspaceOverlayPainter::IsMarkerOccludedByHigherCard(
			HigherSemanticCenter,
			FVector2D(32.0f, 32.0f),
			0.0f,
			HigherCard.Order,
			CardBodies));

	HigherCard.Center = FVector2D(500.0f, 200.0f);
	const TArray<FWacomBackpackCardMarkerOccluder> SpacedBodies = { LowerCard, HigherCard };
	TestFalse(TEXT("A fully exposed lower marker remains visible"),
		FWacomBackpackWorkspaceOverlayPainter::IsMarkerOccludedByHigherCard(
			LowerSemanticCenter,
			FVector2D(32.0f, 32.0f),
			0.0f,
			LowerCard.Order,
			SpacedBodies));

	HigherCard.Order.LayerPriority = 4;
	HigherCard.Order.ZOrder = -100;
	HigherCard.Center = FVector2D(150.0f, 200.0f);
	const TArray<FWacomBackpackCardMarkerOccluder> ActiveLayerBodies = { LowerCard, HigherCard };
	TestTrue(TEXT("CarryActive layer occludes CarryCache regardless of local ZOrder"),
		FWacomBackpackWorkspaceOverlayPainter::IsMarkerOccludedByHigherCard(
			LowerSemanticCenter,
			FVector2D(32.0f, 32.0f),
			0.0f,
			LowerCard.Order,
			ActiveLayerBodies));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceOverlayIdentitySpec,
	"Wacom.UI.Backpack.Workspace.OverlayPaint.IdentityAndCarry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceOverlayIdentitySpec::RunTest(const FString& Parameters)
{
	UClass* DeckCardClass = LoadClass<UWacomDeckCardWidget>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_WacomDeckCardWidget.WBP_WacomDeckCardWidget_C"));
	UWacomBackpackWorkspaceStyle* Style = LoadObject<UWacomBackpackWorkspaceStyle>(
		nullptr,
		TEXT("/Game/Wacom/UI/Backpack/DA_BackpackWorkspaceStyle.DA_BackpackWorkspaceStyle"));
	TestNotNull(TEXT("Formal deck card class is loadable"), DeckCardClass);
	TestNotNull(TEXT("Formal workspace style is loadable"), Style);
	if (!DeckCardClass || !Style)
	{
		return false;
	}

	TStrongObjectPtr<UCardDefinition> SharedDefinition(NewObject<UCardDefinition>());
	SharedDefinition->CardId = TEXT("Backpack.Overlay.SharedDefinition");
	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	const TSharedRef<SWidget> RetainedWorkspaceSlate = Workspace->TakeWidget();
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Model =
		MakeShared<FWacomBackpackWorkspaceInteractionModel>();
	Workspace->SetInteractionModel(Model, Style);

	TArray<TStrongObjectPtr<UWacomDeckCardWidget>> OwnedCards;
	TArray<TObjectPtr<UWacomDeckCardWidget>> BoundCards;
	TArray<FGuid> PhysicalIds;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		FCardInstance Instance;
		Instance.InstanceId = FGuid(Index + 1, 33, 44, 55);
		Instance.Definition = SharedDefinition.Get();
		TStrongObjectPtr<UWacomDeckCardWidget> Card(
			NewObject<UWacomDeckCardWidget>(Workspace.Get(), DeckCardClass));
		Card->SetCard(Instance, EZoneKind::BattleDeck, FGuid());
		Card->SetWorkspaceDisplayZone(EZoneKind::BattleDeck, FGuid());
		if (Index == 3)
		{
			Card->SetWorkspaceReadOnlyKind(
				EWacomBackpackWorkspaceCardReadOnlyKind::BattleProjection);
		}
		else
		{
			PhysicalIds.Add(Instance.InstanceId);
		}
		if (UCanvasPanelSlot* Slot = Workspace->GetStaticCardLayer()->AddChildToCanvas(Card.Get()))
		{
			Slot->SetPosition(FVector2D(40.0f + Index * 120.0f, 70.0f));
			Slot->SetSize(FVector2D(100.0f, 150.0f));
			Slot->SetZOrder(Index);
		}
		BoundCards.Add(Card.Get());
		OwnedCards.Add(MoveTemp(Card));
	}
	Workspace->BindWorkspaceCards(BoundCards, 1);

	const FWacomBackpackZoneKey BattleZone =
		FWacomBackpackZoneKey::Make(EZoneKind::BattleDeck);
	Model->BeginMarquee(BattleZone, FVector2D::ZeroVector, false);
	Model->UpdateMarquee(FVector2D(600.0f, 300.0f));
	Model->CompleteMarquee();
	Workspace->RefreshInteractionPresentation();
	TestEqual(TEXT("Marquee selects every physical InstanceId sharing one card definition"),
		Model->GetSelection().OrderedSelectedInstanceIds,
		PhysicalIds);
	for (int32 Index = 0; Index < OwnedCards.Num(); ++Index)
	{
		const EWacomBackpackWorkspaceCardSemanticIcon Expected = Index < 3
			? EWacomBackpackWorkspaceCardSemanticIcon::Selected
			: EWacomBackpackWorkspaceCardSemanticIcon::None;
		TestEqual(
			*FString::Printf(TEXT("Card %d receives its InstanceId-derived semantic marker"), Index),
			FWacomBackpackWorkspaceCardTestAccess::SemanticIcon(*OwnedCards[Index]),
			Expected);
	}

	OwnedCards[1]->SetBackpackRealtimePresentation(true, FVector2D::ZeroVector, false);
	TestTrue(TEXT("Clicking any selected physical card begins group carry"),
		Model->BeginCarry(PhysicalIds[1], FVector2D(300.0f, 200.0f), 1));
	Workspace->RefreshInteractionPresentation();
	TestEqual(TEXT("Carry identity set exactly matches the visible selected-marker set"),
		Model->GetCarry().RemainingInstanceIds,
		PhysicalIds);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestEqual(
			*FString::Printf(TEXT("Carried physical card %d keeps its selected marker"), Index),
			FWacomBackpackWorkspaceCardTestAccess::SemanticIcon(*OwnedCards[Index]),
			EWacomBackpackWorkspaceCardSemanticIcon::Selected);
	}
	TestEqual(TEXT("Realtime Retainer presentation cannot erase the marker state"),
		FWacomBackpackWorkspaceCardTestAccess::SemanticIcon(*OwnedCards[1]),
		EWacomBackpackWorkspaceCardSemanticIcon::Selected);
	return true;
}

#endif
