// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/HandPanel.h"
#include "UI/Battle/CardWidget.h"
#include "UI/Battle/BattleHUD.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/BorderSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/OverlaySlot.h"
#include "Snapshots/BattleSnapshot.h"
#include "Types/WacomEnums.h"
#include "UObject/ConstructorHelpers.h"

UHandPanel::UHandPanel()
{
	static ConstructorHelpers::FClassFinder<UCardWidget> DefaultCardWidgetClass(
		TEXT("/Game/Wacom/UI/Battle/WBP_CardWidget"));
	if (DefaultCardWidgetClass.Succeeded())
	{
		CardWidgetClass = DefaultCardWidgetClass.Class;
	}
	else
	{
		CardWidgetClass = UCardWidget::StaticClass();
	}
}

TSharedRef<SWidget> UHandPanel::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UBorder* UnifiedFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("UnifiedHandFrame"));
		WidgetTree->RootWidget = UnifiedFrame;
		UnifiedFrame->SetBrushColor(FLinearColor(0.08f, 0.08f, 0.10f, 0.35f));
		UnifiedFrame->SetPadding(FMargin(4));

		UHorizontalBox* UnifiedInner = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("UnifiedHandSlot"));
		UnifiedFrame->SetContent(UnifiedInner);
		UnifiedHandSlot = UnifiedInner;
	}
	return Super::RebuildWidget();
}

void UHandPanel::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	ClearAllSlots();
	CurrentVisualEntries.Reset();
	CurrentVisualEntries = BuildVisualEntries(Snap.Hand);
	RebuildUnifiedHorizontalRenderer(CurrentVisualEntries);

	ApplyTargetingHighlight();
}

TArray<FHandCardVisualEntry> UHandPanel::BuildVisualEntries(const FHandQueueSnapshot& HandSnapshot)
{
	TArray<FHandCardVisualEntry> Entries;
	Entries.Reserve(HandSnapshot.Cards.Num());
	for (int32 Index = 0; Index < HandSnapshot.Cards.Num(); ++Index)
	{
		const FHandCardSnapshot& Card = HandSnapshot.Cards[Index];
		FHandCardVisualEntry Entry;
		Entry.Snapshot = Card;
		Entry.VisualIndex = Index;
		Entry.LogicalZone = Card.Zone;
		Entry.bIsAnchor = Card.bIsHandAnchor;
		Entries.Add(Entry);
	}
	return Entries;
}

void UHandPanel::RebuildUnifiedHorizontalRenderer(const TArray<FHandCardVisualEntry>& Entries)
{
	if (!UnifiedHandSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandPanel] UnifiedHorizontal renderer requires UnifiedHandSlot, but it is not bound."));
		return;
	}

	TArray<FHandCardVisualEntry> SortedEntries = Entries;
	SortedEntries.Sort([](const FHandCardVisualEntry& A, const FHandCardVisualEntry& B)
	{
		return A.VisualIndex < B.VisualIndex;
	});

	ApplyUnifiedHandSlotAlignment();

	for (int32 Index = 0; Index < SortedEntries.Num(); ++Index)
	{
		CreateAndPlaceCard(SortedEntries[Index], UnifiedHandSlot, Index, SortedEntries.Num());
	}
}

UCardWidget* UHandPanel::CreateAndPlaceCard(const FHandCardVisualEntry& Entry, UPanelWidget* TargetSlot, int32 CardIndex, int32 CardCount)
{
	if (!TargetSlot) { return nullptr; }

	TSubclassOf<UCardWidget> ClassToUse = Entry.bIsAnchor && AnchorCardWidgetClass
		? AnchorCardWidgetClass
		: CardWidgetClass;
	if (!ClassToUse) { ClassToUse = UCardWidget::StaticClass(); }

	UCardWidget* Card = CreateWidget<UCardWidget>(this, ClassToUse);
	if (!Card) { return nullptr; }

	TargetSlot->AddChild(Card);
	ApplyCardSlotLayout(Card, CardIndex, CardCount);
	Card->ApplyCardSnapshot(Entry.Snapshot);
	Card->OnCardClicked.AddDynamic(this, &UHandPanel::HandleCardClicked);
	Card->OnCardHoveredNative.AddUObject(this, &UHandPanel::HandleCardHovered);
	Card->OnCardUnhoveredNative.AddUObject(this, &UHandPanel::HandleCardUnhovered);

	SpawnedCards.Add(Card);
	return Card;
}

void UHandPanel::ApplyUnifiedHandSlotAlignment() const
{
	if (!UnifiedHandSlot)
	{
		return;
	}

	if (UHorizontalBox* HorizontalBox = Cast<UHorizontalBox>(UnifiedHandSlot))
	{
		if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(HorizontalBox->Slot))
		{
			HorizontalSlot->SetHorizontalAlignment(bCenterCardsWhenNotOverflow ? HAlign_Center : HAlign_Left);
			HorizontalSlot->SetVerticalAlignment(CardVerticalAlignment);
		}
		else if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(HorizontalBox->Slot))
		{
			BorderSlot->SetHorizontalAlignment(bCenterCardsWhenNotOverflow ? HAlign_Center : HAlign_Fill);
			BorderSlot->SetVerticalAlignment(CardVerticalAlignment);
		}
		else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(HorizontalBox->Slot))
		{
			OverlaySlot->SetHorizontalAlignment(bCenterCardsWhenNotOverflow ? HAlign_Center : HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(CardVerticalAlignment);
		}
		else if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(HorizontalBox->Slot))
		{
			CanvasSlot->SetAlignment(bCenterCardsWhenNotOverflow ? FVector2D(0.5f, 0.5f) : FVector2D(0.0f, 0.5f));
		}
	}
}

void UHandPanel::ApplyCardSlotLayout(UCardWidget* Card, int32 CardIndex, int32 CardCount) const
{
	if (!Card)
	{
		return;
	}

	if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(Card->Slot))
	{
		HorizontalSlot->SetPadding(BuildCardSlotPadding(CardIndex, CardCount));
		HorizontalSlot->SetVerticalAlignment(CardVerticalAlignment);
		HorizontalSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

FMargin UHandPanel::BuildCardSlotPadding(int32 CardIndex, int32 CardCount) const
{
	const float HalfSpacing = FMath::Max(0.0f, CardSpacing) * 0.5f;
	const bool bFirst = CardIndex <= 0;
	const bool bLast = CardIndex >= CardCount - 1;

	const float Left = HandContentPadding.Left + (bFirst ? 0.0f : HalfSpacing);
	const float Right = HandContentPadding.Right + (bLast ? 0.0f : HalfSpacing);
	return FMargin(Left, HandContentPadding.Top, Right, HandContentPadding.Bottom);
}

int32 UHandPanel::GetUnifiedHandSlotCardCount() const
{
	return UnifiedHandSlot ? UnifiedHandSlot->GetChildrenCount() : 0;
}

UCardWidget* UHandPanel::GetSpawnedCardForTest(int32 Index) const
{
	return SpawnedCards.IsValidIndex(Index) ? SpawnedCards[Index] : nullptr;
}

void UHandPanel::ClearAllSlots()
{
	auto ClearSlot = [](UPanelWidget* P) { if (P) { P->ClearChildren(); } };
	ClearSlot(UnifiedHandSlot);
	SpawnedCards.Reset();
}

void UHandPanel::ApplyTargetingHighlight()
{
	UBattleHUD* HUD = nullptr;
	for (UUserWidget* P = GetTypedOuter<UUserWidget>(); P; P = P->GetTypedOuter<UUserWidget>())
	{
		HUD = Cast<UBattleHUD>(P);
		if (HUD) { break; }
	}
	if (!HUD) { return; }

	const bool bTargeting = HUD->IsInTargetSelect();
	const FGuid PendingId = HUD->GetPendingTargetingCardId();

	for (const TObjectPtr<UCardWidget>& Card : SpawnedCards)
	{
		if (!Card) { continue; }
		const bool bSelf = bTargeting && Card->GetCardInstanceId() == PendingId;
		Card->SetTargetingHighlight(bSelf);
	}
}

void UHandPanel::HandleCardClicked(FGuid CardInstanceId)
{
	for (UUserWidget* P = GetTypedOuter<UUserWidget>(); P; P = P->GetTypedOuter<UUserWidget>())
	{
		if (UBattleHUD* HUD = Cast<UBattleHUD>(P))
		{
			HUD->OnCardClickedByUser(CardInstanceId);
			return;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[HandPanel] 未找到 UBattleHUD 父 Widget，点击被吞"));
}

void UHandPanel::HandleCardHovered(UCardWidget* SourceWidget)
{
	OnCardHoveredNative.Broadcast(SourceWidget);
}

void UHandPanel::HandleCardUnhovered(UCardWidget* SourceWidget)
{
	OnCardUnhoveredNative.Broadcast(SourceWidget);
}
