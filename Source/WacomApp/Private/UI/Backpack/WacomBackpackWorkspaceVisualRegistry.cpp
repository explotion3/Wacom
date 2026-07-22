// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceVisualRegistry.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Blueprint/UserWidget.h"
#include "UI/Backpack/WacomBackpackCardWidgetTransfer.h"
#include "UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"

namespace
{
bool IsPhysicalVisualRole(EWacomBackpackDeckCardListReuseRole Role)
{
	return Role == EWacomBackpackDeckCardListReuseRole::PhysicalList
		|| Role == EWacomBackpackDeckCardListReuseRole::SpecialContent;
}

FWacomBackpackWorkspaceCardViewKey MakeKey(
	const FRunStorageCardView& Card,
	EWacomBackpackDeckCardListReuseRole Role)
{
	FWacomBackpackWorkspaceCardViewKey Key;
	Key.InstanceId = Card.Instance.InstanceId;
	Key.PhysicalZone = Card.PhysicalZone;
	Key.OwnerInstanceId = Card.PhysicalZone == EZoneKind::SpecialZone
		? Card.ZoneOwnerInstanceId
		: FGuid();
	Key.Role = Role;
	return Key;
}

FWacomBackpackWorkspaceCardViewKey MakeKey(const UWacomDeckCardWidget& Widget)
{
	FWacomBackpackWorkspaceCardViewKey Key;
	Key.InstanceId = Widget.GetCardInstanceId();
	Key.PhysicalZone = Widget.GetFromZone();
	Key.OwnerInstanceId = Key.PhysicalZone == EZoneKind::SpecialZone
		? Widget.GetFromZoneOwnerInstanceId()
		: FGuid();
	Key.Role = Widget.GetBackpackListReuseRole();
	return Key;
}

UPanelWidget* GetPanelParent(UWacomDeckCardWidget* Widget)
{
	return Widget ? Cast<UPanelWidget>(Widget->GetParent()) : nullptr;
}

void MoveToPanelIndex(UPanelWidget& Panel, UWacomDeckCardWidget& Widget, int32 Index)
{
	if (GetPanelParent(&Widget) != &Panel)
	{
		Wacom::Backpack::ReparentCardPreservingSlate(Panel, Widget);
	}
	Panel.ShiftChild(Index, &Widget);
}
}

uint32 GetTypeHash(const FWacomBackpackWorkspaceCardViewKey& Key)
{
	uint32 Hash = GetTypeHash(Key.InstanceId);
	Hash = HashCombine(Hash, GetTypeHash(Key.OwnerInstanceId));
	Hash = HashCombine(Hash, static_cast<uint32>(Key.PhysicalZone));
	Hash = HashCombine(Hash, static_cast<uint32>(Key.Role));
	return Hash;
}

void FWacomBackpackWorkspaceVisualRegistry::RebuildCardIndexes(
	TConstArrayView<UPanelWidget*> SearchPanels,
	TFunctionRef<bool(const UWacomDeckCardWidget*)> PreserveCurrentParent)
{
	CardsByViewKey.Reset();
	PhysicalCardsByInstanceId.Reset();
	for (UPanelWidget* Panel : SearchPanels)
	{
		if (!Panel)
		{
			continue;
		}
		for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
		{
			UWacomDeckCardWidget* Card = Cast<UWacomDeckCardWidget>(Panel->GetChildAt(Index));
			if (!Card)
			{
				continue;
			}

			const FWacomBackpackWorkspaceCardViewKey Key = MakeKey(*Card);
			TWeakObjectPtr<UWacomDeckCardWidget>& Exact = CardsByViewKey.FindOrAdd(Key);
			if (!Exact.IsValid()
				|| (PreserveCurrentParent(Card) && !PreserveCurrentParent(Exact.Get())))
			{
				Exact = Card;
			}
			if (IsPhysicalVisualRole(Key.Role))
			{
				TWeakObjectPtr<UWacomDeckCardWidget>& Physical =
					PhysicalCardsByInstanceId.FindOrAdd(Key.InstanceId);
				if (!Physical.IsValid()
					|| (PreserveCurrentParent(Card)
						&& !PreserveCurrentParent(Physical.Get())))
				{
					Physical = Card;
				}
			}
		}
	}
}

void FWacomBackpackWorkspaceVisualRegistry::ReconcileCards(
	TConstArrayView<UPanelWidget*> SearchPanels,
	UPanelWidget& DestinationPanel,
	TConstArrayView<FWacomBackpackWorkspaceSceneCardEntry> DesiredCards,
	TFunctionRef<bool(const UWacomDeckCardWidget*)> PreserveCurrentParent,
	TFunctionRef<UWacomDeckCardWidget*(const FRunStorageCardView&)> CreateWidget,
	TFunctionRef<void(UWacomDeckCardWidget*)> OnRemovedWidget)
{
	OrderedCards.Reset();
	OrderedCards.Reserve(DesiredCards.Num());
	CardsByViewKey.Reset();
	PhysicalCardsByInstanceId.Reset();

	TArray<UWacomDeckCardWidget*> DuplicateWidgets;
	TMap<UWacomDeckCardWidget*, FWacomBackpackWorkspaceCardViewKey> ExistingKeysByWidget;
	for (UPanelWidget* Panel : SearchPanels)
	{
		if (!Panel)
		{
			continue;
		}
		for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
		{
			UWacomDeckCardWidget* Card = Cast<UWacomDeckCardWidget>(Panel->GetChildAt(Index));
			if (!Card)
			{
				continue;
			}
			const FWacomBackpackWorkspaceCardViewKey Key = MakeKey(*Card);
			ExistingKeysByWidget.Add(Card, Key);
			if (TWeakObjectPtr<UWacomDeckCardWidget>* Existing = CardsByViewKey.Find(Key))
			{
				UWacomDeckCardWidget* ExistingCard = Existing->Get();
				if (PreserveCurrentParent(Card)
					&& (!ExistingCard || !PreserveCurrentParent(ExistingCard)))
				{
					if (ExistingCard)
					{
						DuplicateWidgets.Add(ExistingCard);
					}
					*Existing = Card;
				}
				else
				{
					DuplicateWidgets.Add(Card);
				}
			}
			else
			{
				CardsByViewKey.Add(Key, Card);
			}
			if (IsPhysicalVisualRole(Key.Role))
			{
				TWeakObjectPtr<UWacomDeckCardWidget>& Physical =
					PhysicalCardsByInstanceId.FindOrAdd(Key.InstanceId);
				if (!Physical.IsValid()
					|| (PreserveCurrentParent(Card)
						&& !PreserveCurrentParent(Physical.Get())))
				{
					Physical = Card;
				}
			}
		}
	}

	TSet<UWacomDeckCardWidget*> Used;
	int32 StaticIndex = 0;
	for (const FWacomBackpackWorkspaceSceneCardEntry& Desired : DesiredCards)
	{
		const FWacomBackpackWorkspaceCardViewKey Key = MakeKey(Desired.CardView, Desired.Role);
		UWacomDeckCardWidget* Widget = CardsByViewKey.FindRef(Key).Get();
		if (!Widget && IsPhysicalVisualRole(Desired.Role))
		{
			Widget = PhysicalCardsByInstanceId.FindRef(
				Desired.CardView.Instance.InstanceId).Get();
		}
		if (!Widget)
		{
			Widget = CreateWidget(Desired.CardView);
		}
		if (!Widget)
		{
			continue;
		}
		if (const FWacomBackpackWorkspaceCardViewKey* ExistingKey =
			ExistingKeysByWidget.Find(Widget);
			ExistingKey && !(*ExistingKey == Key))
		{
			// A physical card may keep its Widget while moving between zones. Any
			// detail view anchored to the old logical view must still be closed
			// before that Widget is rebound to its new identity.
			OnRemovedWidget(Widget);
		}

		const bool bPreserveTransientPresentation = PreserveCurrentParent(Widget);
		Widget->PrepareForBackpackListReuse(bPreserveTransientPresentation);
		Widget->SetStorageCardView(Desired.CardView);
		Widget->SetMoveEnabled(true);
		Widget->SetWorkspaceInteractionEnabled(Desired.bWorkspaceInteractive);
		Widget->SetWorkspaceReadOnlyKind(Desired.ReadOnlyKind);
		Widget->SetWorkspaceDisplayZone(
			Desired.DisplayZone.Zone, Desired.DisplayZone.OwnerInstanceId);
		Widget->SetBackpackListReuseRole(Desired.Role);
		Widget->SetProjectedFromBadgeText(Desired.ProjectedBadgeText);
		Used.Add(Widget);
		OrderedCards.Add(Widget);
		if (!PreserveCurrentParent(Widget))
		{
			MoveToPanelIndex(DestinationPanel, *Widget, StaticIndex++);
		}
	}

	for (const TPair<FWacomBackpackWorkspaceCardViewKey,
		TWeakObjectPtr<UWacomDeckCardWidget>>& Pair : CardsByViewKey)
	{
		if (UWacomDeckCardWidget* Card = Pair.Value.Get(); Card && !Used.Contains(Card))
		{
			OnRemovedWidget(Card);
			Card->RemoveFromParent();
		}
	}
	for (UWacomDeckCardWidget* Duplicate : DuplicateWidgets)
	{
		if (Duplicate && !Used.Contains(Duplicate))
		{
			OnRemovedWidget(Duplicate);
			Duplicate->RemoveFromParent();
		}
	}

	CardsByViewKey.Reset();
	PhysicalCardsByInstanceId.Reset();
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : OrderedCards)
	{
		UWacomDeckCardWidget* Card = WeakCard.Get();
		if (!Card)
		{
			continue;
		}
		const FWacomBackpackWorkspaceCardViewKey Key = MakeKey(*Card);
		CardsByViewKey.Add(Key, Card);
		if (IsPhysicalVisualRole(Key.Role))
		{
			PhysicalCardsByInstanceId.Add(Key.InstanceId, Card);
		}
	}
}

void FWacomBackpackWorkspaceVisualRegistry::ReplaceOrderedCards(
	TConstArrayView<TObjectPtr<UWacomDeckCardWidget>> Cards)
{
	OrderedCards.Reset();
	OrderedCards.Reserve(Cards.Num());
	for (UWacomDeckCardWidget* Card : Cards)
	{
		if (Card)
		{
			OrderedCards.Add(Card);
		}
	}
}

void FWacomBackpackWorkspaceVisualRegistry::ReconcilePiles(
	UUserWidget& Owner,
	UCanvasPanel& Canvas,
	UClass* PileWidgetClass,
	TConstArrayView<FWacomBackpackWorkspaceScenePileEntry> Piles,
	TFunctionRef<void(UWacomBackpackZonePileWidget&)> BindWidget)
{
	PilesByZone.Reset();
	TArray<UWacomBackpackZonePileWidget*> Existing;
	for (int32 Index = 0; Index < Canvas.GetChildrenCount(); ++Index)
	{
		if (UWacomBackpackZonePileWidget* Pile =
			Cast<UWacomBackpackZonePileWidget>(Canvas.GetChildAt(Index)))
		{
			Existing.Add(Pile);
			const FWacomBackpackZoneKey Key = FWacomBackpackZoneKey::Make(
				Pile->GetPileView().Zone, Pile->GetPileView().OwnerInstanceId);
			if (!PilesByZone.Contains(Key))
			{
				PilesByZone.Add(Key, Pile);
			}
		}
	}

	TSet<UWacomBackpackZonePileWidget*> Used;
	OrderedPiles.Reset();
	OrderedPiles.Reserve(Piles.Num());
	UClass* ResolvedClass = PileWidgetClass
		? PileWidgetClass
		: UWacomBackpackZonePileWidget::StaticClass();
	for (const FWacomBackpackWorkspaceScenePileEntry& Entry : Piles)
	{
		if (!Entry.Zone.IsValid())
		{
			continue;
		}
		UWacomBackpackZonePileWidget* Pile = PilesByZone.FindRef(Entry.Zone).Get();
		if (!Pile || Used.Contains(Pile))
		{
			Pile = CreateWidget<UWacomBackpackZonePileWidget>(&Owner, ResolvedClass);
			if (Pile)
			{
				Canvas.AddChildToCanvas(Pile);
			}
		}
		if (!Pile)
		{
			continue;
		}
		Used.Add(Pile);
		OrderedPiles.Add(Pile);
		Pile->SetPileView(Entry.View);
		Pile->SetResolvedGeometry(Entry.FrameRect, Entry.HeaderRect);
		BindWidget(*Pile);
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Pile->Slot))
		{
			Slot->SetAnchors(FAnchors(0.0f, 0.0f));
			Slot->SetAlignment(FVector2D::ZeroVector);
			Slot->SetPosition(FVector2D(Entry.FrameRect.Left, Entry.FrameRect.Top));
			Slot->SetSize(FVector2D(
				Entry.FrameRect.Right - Entry.FrameRect.Left,
				Entry.FrameRect.Bottom - Entry.FrameRect.Top));
			Slot->SetZOrder(2000 + Entry.LayerRank);
		}
	}
	for (UWacomBackpackZonePileWidget* Pile : Existing)
	{
		if (Pile && !Used.Contains(Pile))
		{
			Pile->OnPilePointerDownNative.Unbind();
			Pile->RemoveFromParent();
		}
	}
	PilesByZone.Reset();
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile : OrderedPiles)
	{
		if (UWacomBackpackZonePileWidget* Pile = WeakPile.Get())
		{
			PilesByZone.Add(
				FWacomBackpackZoneKey::Make(
					Pile->GetPileView().Zone, Pile->GetPileView().OwnerInstanceId),
				Pile);
		}
	}
}

UWacomDeckCardWidget* FWacomBackpackWorkspaceVisualRegistry::FindPhysicalCard(
	FGuid InstanceId) const
{
	return PhysicalCardsByInstanceId.FindRef(InstanceId).Get();
}

UWacomBackpackZonePileWidget* FWacomBackpackWorkspaceVisualRegistry::FindPile(
	const FWacomBackpackZoneKey& Zone) const
{
	return PilesByZone.FindRef(Zone).Get();
}

void FWacomBackpackWorkspaceVisualRegistry::ResetPiles(bool bRemoveFromParent)
{
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile : OrderedPiles)
	{
		if (UWacomBackpackZonePileWidget* Pile = WeakPile.Get())
		{
			Pile->OnPilePointerDownNative.Unbind();
			if (bRemoveFromParent)
			{
				Pile->RemoveFromParent();
			}
		}
	}
	PilesByZone.Reset();
	OrderedPiles.Reset();
}

void FWacomBackpackWorkspaceVisualRegistry::ResetIndexes()
{
	CardsByViewKey.Reset();
	PhysicalCardsByInstanceId.Reset();
	OrderedCards.Reset();
	PilesByZone.Reset();
}
