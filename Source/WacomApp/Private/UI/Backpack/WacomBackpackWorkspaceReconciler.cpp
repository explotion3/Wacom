// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceReconciler.h"

#include "Components/CanvasPanel.h"
#include "Components/PanelWidget.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"
#include "UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceRuntime.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"

void FWacomBackpackWorkspaceReconciler::Reconcile(
	UWacomBackpackWorkspaceWidget& Workspace,
	const FRunBackpackStorageSnapshot& Snapshot,
	FWacomBackpackWorkspaceStateStore& StateStore,
	const FWacomBackpackWorkspaceInteractionModel* InteractionModel,
	const UWacomBackpackWorkspaceStyle* Style,
	TFunctionRef<UWacomDeckCardWidget*(const FRunStorageCardView&)> CreateWidget,
	TFunctionRef<void(UWacomDeckCardWidget*)> OnRemovedWidget,
	TArray<TObjectPtr<UWacomDeckCardWidget>>* OutOrderedWidgets,
	uint64 StorageRevision)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Wacom_Backpack_WorkspaceReconcile);
	UCanvasPanel* StaticCanvas = Workspace.GetStaticCardLayer();
	if (!StaticCanvas)
	{
		return;
	}

	const UWacomBackpackWorkspaceStyle& ResolvedStyle = Style
		? *Style
		: *GetDefault<UWacomBackpackWorkspaceStyle>();
	FWacomBackpackWorkspaceCarryProjection CarryProjection;
	if (InteractionModel && InteractionModel->IsCarrying())
	{
		const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
		CarryProjection.bCarrying = true;
		CarryProjection.SourceZone = Carry.SourceZone;
		CarryProjection.InstanceIds.Reserve(Carry.RemainingInstanceIds.Num());
		for (const FGuid InstanceId : Carry.RemainingInstanceIds)
		{
			CarryProjection.InstanceIds.Add(InstanceId);
		}
	}
	const FWacomBackpackWorkspaceScene Scene = FWacomBackpackWorkspaceSceneBuilder::Build(
		Snapshot,
		StateStore,
		CarryProjection,
		ResolvedStyle,
		Workspace.GetLayoutSpaceSize());

	if (UCanvasPanel* PileCanvas = Workspace.GetPileCanvas())
	{
		Workspace.GetRuntime().Visuals.ReconcilePiles(
			Workspace,
			*PileCanvas,
			Workspace.PileWidgetClass,
			Scene.Piles,
			[&Workspace](UWacomBackpackZonePileWidget& Pile)
			{
				Pile.OnPilePointerDownNative.BindUObject(
					&Workspace,
					&UWacomBackpackWorkspaceWidget::HandlePilePointerDown);
			});
		Workspace.PileCount = Workspace.GetRuntime().Visuals.GetPileWidgets().Num();
	}

	TArray<UPanelWidget*> SearchPanels;
	SearchPanels.Add(StaticCanvas);
	if (UCanvasPanel* CarryCanvas = Workspace.GetCarryCanvas())
	{
		SearchPanels.Add(CarryCanvas);
	}
	if (UCanvasPanel* CarryActiveCanvas = Workspace.GetCarryActiveCanvas())
	{
		SearchPanels.Add(CarryActiveCanvas);
	}
	if (UCanvasPanel* SettlementCanvas = Workspace.GetSettlementCanvas())
	{
		SearchPanels.Add(SettlementCanvas);
	}

	Workspace.PrepareForWorkspaceCardReconcile();
	Workspace.GetRuntime().Visuals.ReconcileCards(
		SearchPanels,
		*StaticCanvas,
		Scene.Cards,
		[&Workspace](const UWacomDeckCardWidget* Widget)
		{
			return Workspace.ShouldPreserveCardParent(Widget);
		},
		CreateWidget,
		OnRemovedWidget);
	const TConstArrayView<TWeakObjectPtr<UWacomDeckCardWidget>> OrderedWidgets =
		Workspace.GetRuntime().Visuals.GetCardWidgets();

	const int32 AlignedCardCount = FMath::Min3(
		OrderedWidgets.Num(), Scene.Cards.Num(), Scene.CardLayouts.Num());
	for (int32 Index = 0; Index < AlignedCardCount; ++Index)
	{
		UWacomDeckCardWidget* CardWidget = OrderedWidgets[Index].Get();
		if (!CardWidget)
		{
			continue;
		}
		const FWacomBackpackResolvedLayout& Layout = Scene.CardLayouts[Index];
		if (!Workspace.HasCardBaseLayout(*CardWidget))
		{
			Workspace.PrimeCardBaseLayout(
				*CardWidget, Layout.CardCenter, ResolvedStyle.CardRenderSize,
				Layout.AngleDegrees, Layout.LayerRank);
		}
		Workspace.ApplyCardBaseLayout(
			*CardWidget, Layout.CardCenter, ResolvedStyle.CardRenderSize,
			Layout.AngleDegrees, Layout.LayerRank);
		CardWidget->SetWorkspaceInteractionEnabled(Scene.Cards[Index].bWorkspaceInteractive);
		CardWidget->SetWorkspaceReadOnlyKind(Scene.Cards[Index].ReadOnlyKind);
	}

	bool bAppliedExpandedFocusContract = false;
	if (Scene.bHasExpandedPile)
	{
		for (const FWacomBackpackWorkspaceScenePileEntry& Pile : Scene.Piles)
		{
			if (Pile.Zone != Scene.ExpandedZone || Pile.CardCount <= 0)
			{
				continue;
			}
			TArray<FWacomBackpackExpandedPileFocusCard> FocusCards;
			FocusCards.Reserve(Pile.CardCount);
			for (int32 LocalIndex = 0; LocalIndex < Pile.CardCount; ++LocalIndex)
			{
				const int32 CardIndex = Pile.CardStartIndex + LocalIndex;
				if (!OrderedWidgets.IsValidIndex(CardIndex)
					|| !Pile.ContentLayout.Cards.IsValidIndex(LocalIndex)
					|| !Pile.ContentLayout.FocusHitBands.IsValidIndex(LocalIndex))
				{
					continue;
				}
				FWacomBackpackExpandedPileFocusCard& FocusCard =
					FocusCards.AddDefaulted_GetRef();
				FocusCard.Card = OrderedWidgets[CardIndex];
				FocusCard.NeutralCenter = Pile.ContentLayout.Cards[LocalIndex].CardCenter;
				FocusCard.NeutralAngleDegrees =
					Pile.ContentLayout.Cards[LocalIndex].AngleDegrees;
				FocusCard.NeutralLayerRank =
					Pile.ContentLayout.Cards[LocalIndex].LayerRank + 3000;
				FocusCard.NeutralHitBand = Pile.ContentLayout.FocusHitBands[LocalIndex];
				FocusCard.CurrentHitBand = FocusCard.NeutralHitBand;
			}
			Workspace.SetExpandedPileFocusContract(
				Scene.ExpandedZone.Zone,
				Scene.ExpandedZone.OwnerInstanceId,
				Pile.HeaderRect,
				Pile.ContentLayout.FocusCorridorRect,
				FocusCards);
			bAppliedExpandedFocusContract = true;
			break;
		}
	}
	if (!bAppliedExpandedFocusContract)
	{
		Workspace.SetExpandedPileFocusContract(
			EZoneKind::Backpack, FGuid(), FSlateRect(), FSlateRect(), {});
	}

	if (Scene.bHasExpandedBounds)
	{
		Workspace.SetExpandedContentBounds(
			Scene.ExpandedZone.Zone,
			Scene.ExpandedZone.OwnerInstanceId,
			Scene.ExpandedBounds);
	}
	else
	{
		Workspace.SetExpandedContentBounds(EZoneKind::Backpack, FGuid(), FSlateRect());
	}
	Workspace.SetPresentedContentZone(
		Scene.ExpandedZone.Zone, Scene.ExpandedZone.OwnerInstanceId);
	Workspace.SetManualLayoutCount(Scene.ManualFluxLayoutCount);
	Workspace.SetEmptyStateVisible(Scene.bFluxEmpty);
	Workspace.BindRegisteredWorkspaceCards(StorageRevision);
	if (OutOrderedWidgets)
	{
		OutOrderedWidgets->Reset();
		OutOrderedWidgets->Reserve(OrderedWidgets.Num());
		for (const TWeakObjectPtr<UWacomDeckCardWidget>& Card : OrderedWidgets)
		{
			if (UWacomDeckCardWidget* Widget = Card.Get())
			{
				OutOrderedWidgets->Add(Widget);
			}
		}
	}
}
