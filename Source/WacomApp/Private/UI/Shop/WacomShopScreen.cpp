// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopScreen.h"

#define LOCTEXT_NAMESPACE "WacomShopScreen"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "Cards/CardDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "RunState.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Shop/WacomShopOfferRowWidget.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"
#include "UI/Shop/WacomShopScreenFlow.h"

namespace
{
	uint32 HashGuidForShopRefresh(uint32 Hash, const FGuid& Value)
	{
		Hash = HashCombine(Hash, Value.A);
		Hash = HashCombine(Hash, Value.B);
		Hash = HashCombine(Hash, Value.C);
		Hash = HashCombine(Hash, Value.D);
		return Hash;
	}

	uint32 BuildShopOfferRefreshSignature(const FRunShopSnapshot& Snapshot, int32 CurrentGold)
	{
		uint32 Hash = 2166136261u;
		Hash = HashCombine(Hash, GetTypeHash(Snapshot.ShopId));
		Hash = HashCombine(Hash, Snapshot.bIsActive ? 1u : 0u);
		Hash = HashCombine(Hash, Snapshot.bHasPurchaseThisVisit ? 1u : 0u);
		Hash = HashCombine(Hash, static_cast<uint32>(CurrentGold));
		Hash = HashCombine(Hash, static_cast<uint32>(Snapshot.Offers.Num()));
		for (const FRunShopOffer& Offer : Snapshot.Offers)
		{
			Hash = HashGuidForShopRefresh(Hash, Offer.OfferId);
			Hash = HashCombine(Hash, GetTypeHash(Offer.CardDefinition ? Offer.CardDefinition->CardId : NAME_None));
			Hash = HashCombine(Hash, static_cast<uint32>(Offer.Price));
			Hash = HashCombine(Hash, Offer.bPurchased ? 1u : 0u);
		}
		return Hash;
	}

	UTextBlock* MakeShopText(UWidgetTree* Tree, FName Name, const FText& Text, int32 FontSize)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Block->SetText(Text);
		FSlateFontInfo Font = Block->GetFont();
		Font.Size = FontSize;
		Block->SetFont(Font);
		Block->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.f)));
		return Block;
	}

}

TSharedRef<SWidget> UWacomShopScreen::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		ResetShopOfferRefreshDirtyGate();
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		UBorder* PanelBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBg"));
		PanelBg->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.92f));
		PanelBg->SetPadding(FMargin(20.f));
		if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelBg))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetOffsets(FMargin(-360.f, -260.f, 720.f, 520.f));
			PanelSlot->SetAutoSize(false);
		}

		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
		PanelBg->AddChild(RootBox);

		TitleText = MakeShopText(WidgetTree, TEXT("TitleText"), LOCTEXT("Title", "商店"), 30);
		TitleText->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Center);
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		GoldText = MakeShopText(WidgetTree, TEXT("GoldText"), LOCTEXT("GoldInit", "金币：0"), 18);
		if (UVerticalBoxSlot* GoldSlot = RootBox->AddChildToVerticalBox(GoldText))
		{
			GoldSlot->SetHorizontalAlignment(HAlign_Center);
			GoldSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
		}

		EmptyText = MakeShopText(WidgetTree, TEXT("EmptyText"), LOCTEXT("Empty", "暂无商品"), 18);
		EmptyText->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* EmptySlot = RootBox->AddChildToVerticalBox(EmptyText))
		{
			EmptySlot->SetHorizontalAlignment(HAlign_Center);
			EmptySlot->SetPadding(FMargin(0.f, 12.f));
		}

		UScrollBox* OfferScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("OfferScroll"));
		if (UVerticalBoxSlot* ScrollSlot = RootBox->AddChildToVerticalBox(OfferScroll))
		{
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		OfferList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OfferList"));
		OfferScroll->AddChild(OfferList);

		CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		UTextBlock* CloseText = MakeShopText(WidgetTree, TEXT("CloseText"), LOCTEXT("Close", "关闭"), 18);
		CloseText->SetJustification(ETextJustify::Center);
		CloseButton->AddChild(CloseText);
		if (UVerticalBoxSlot* CloseSlot = RootBox->AddChildToVerticalBox(CloseButton))
		{
			CloseSlot->SetHorizontalAlignment(HAlign_Center);
			CloseSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
		}
	}
	return Super::RebuildWidget();
}

void UWacomShopScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UWacomShopScreen::HandleCloseClicked);
	}

	TrySubscribeRunSession();
	RefreshShop();
}

void UWacomShopScreen::NativeDestruct()
{
	UnsubscribeRunSession();
	Super::NativeDestruct();
}

void UWacomShopScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	bDidEndShopVisit = false;
	TrySubscribeRunSession();
	RefreshShop();
}

void UWacomShopScreen::NativeOnDeactivated()
{
	UnsubscribeRunSession();
	FWacomShopScreenFlow::EndShopVisitOnDeactivate(ResolveRunSession(), bDidEndShopVisit);
	Super::NativeOnDeactivated();
}

void UWacomShopScreen::RefreshShop()
{
	TrySubscribeRunSession();
	URunSession* Run = ResolveRunSession();
	if (LastShopRefreshRunSession.Get() != Run)
	{
		LastShopRefreshRunSession = Run;
		CachedShopSnapshot = FRunShopSnapshot();
		ResetShopOfferRefreshDirtyGate();
	}

	FRunShopSnapshot Snapshot;
	const int32 CurrentGold = Run ? Run->GetGold() : 0;
	if (Run)
	{
		const uint64 ShopRevision = Run->GetShopSnapshotRevision();
		if (bHasLastShopSnapshotRevision && LastShopSnapshotRevision == ShopRevision)
		{
#if WITH_AUTOMATION_TESTS
			++ShopSnapshotRevisionSkipCountForTest;
#endif
			Snapshot = CachedShopSnapshot;
		}
		else
		{
			LastShopSnapshotRevision = ShopRevision;
			bHasLastShopSnapshotRevision = true;
#if WITH_AUTOMATION_TESTS
			++ShopSnapshotBuildCountForTest;
#endif
			Snapshot = BuildShopSnapshot();
			CachedShopSnapshot = Snapshot;
		}
	}
	else
	{
		Snapshot = FRunShopSnapshot();
		CachedShopSnapshot = Snapshot;
		ResetShopOfferRefreshDirtyGate();
	}

	if (TitleText)
	{
		TitleText->SetText(Snapshot.ShopId.IsNone()
			? LOCTEXT("Title", "商店")
			: FText::Format(LOCTEXT("TitleWithId", "商店：{0}"), FText::FromName(Snapshot.ShopId)));
	}
	if (GoldText)
	{
		GoldText->SetText(FText::Format(
			LOCTEXT("GoldFmt", "金币：{0}"),
			FText::AsNumber(CurrentGold)));
	}

	RebuildOfferRows(Snapshot, CurrentGold);
}

void UWacomShopScreen::SuppressEndShopVisitOnNextDeactivate()
{
	bDidEndShopVisit = true;
}

void UWacomShopScreen::HandleCloseClicked()
{
	DeactivateWidget();
}

#if WITH_AUTOMATION_TESTS
bool UWacomShopScreen::PurchaseOfferByIndex(int32 Index)
{
	if (!CachedOfferIds.IsValidIndex(Index))
	{
		return false;
	}
	return PurchaseOffer(CachedOfferIds[Index]);
}

FWacomShopOfferPresentationView UWacomShopScreen::GetCachedOfferView(int32 Index) const
{
	return CachedOfferViews.IsValidIndex(Index)
		? CachedOfferViews[Index]
		: FWacomShopOfferPresentationView();
}

FText UWacomShopScreen::BuildPurchaseFailureToastText(FName DisabledReason)
{
	return FWacomShopScreenFlow::BuildPurchaseFailureToastText(DisabledReason);
}

FText UWacomShopScreen::GetDisplayedGoldText() const
{
	return GoldText ? GoldText->GetText() : FText::GetEmpty();
}

UWacomShopOfferRowWidget* UWacomShopScreen::GetOfferRowWidgetForTest(int32 Index) const
{
	return OfferList && Index >= 0 && OfferList->GetChildrenCount() > Index
		? Cast<UWacomShopOfferRowWidget>(OfferList->GetChildAt(Index))
		: nullptr;
}
#endif

void UWacomShopScreen::TrySubscribeRunSession()
{
	URunSession* Run = ResolveRunSession();
	if (SubscribedRunSession.Get() == Run)
	{
		return;
	}

	UnsubscribeRunSession();
	if (Run)
	{
		Run->OnRunStateChangedNative.AddUObject(this, &UWacomShopScreen::HandleRunStateChanged);
		SubscribedRunSession = Run;
	}
}

void UWacomShopScreen::UnsubscribeRunSession()
{
	if (URunSession* Run = SubscribedRunSession.Get())
	{
		Run->OnRunStateChangedNative.RemoveAll(this);
	}
	SubscribedRunSession.Reset();
}

void UWacomShopScreen::HandleRunStateChanged()
{
	RefreshShop();
}

URunSession* UWacomShopScreen::ResolveRunSession() const
{
	AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(GetOwningPlayer());
	return WacomPC ? WacomPC->GetRunSession() : nullptr;
}

FRunShopSnapshot UWacomShopScreen::BuildShopSnapshot() const
{
	if (URunSession* Run = ResolveRunSession())
	{
		return Run->BuildCurrentShopSnapshot();
	}
	return FRunShopSnapshot();
}

void UWacomShopScreen::RebuildOfferRows(const FRunShopSnapshot& Snapshot, int32 CurrentGold)
{
	if (EmptyText)
	{
		EmptyText->SetVisibility(Snapshot.Offers.Num() == 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (!OfferList)
	{
		CachedOfferIds.Reset();
		CachedOfferViews.Reset();
		return;
	}

	if (URunSession* Run = ResolveRunSession())
	{
		const uint64 EconomyRevision = Run->GetEconomySnapshotRevision();
		if (!bHasLastShopEconomyRevision || LastShopEconomyRevision != EconomyRevision)
		{
			bHasLastShopEconomyRevision = true;
			LastShopEconomyRevision = EconomyRevision;
		}
	}
	else
	{
		bHasLastShopEconomyRevision = false;
		LastShopEconomyRevision = 0;
	}

	const uint32 RefreshSignature = BuildShopOfferRefreshSignature(Snapshot, CurrentGold);
	if (bHasLastShopOfferRefreshSignature && LastShopOfferRefreshSignature == RefreshSignature)
	{
#if WITH_AUTOMATION_TESTS
		++ShopOfferRefreshSkipCountForTest;
#endif
		return;
	}

	LastShopOfferRefreshSignature = RefreshSignature;
	bHasLastShopOfferRefreshSignature = true;
#if WITH_AUTOMATION_TESTS
	++ShopOfferRefreshApplyCountForTest;
#endif

	TMap<FGuid, UWacomShopOfferRowWidget*> ExistingRowsByOfferId;
	for (int32 ChildIndex = 0; ChildIndex < OfferList->GetChildrenCount(); ++ChildIndex)
	{
		UWacomShopOfferRowWidget* ExistingRow = Cast<UWacomShopOfferRowWidget>(OfferList->GetChildAt(ChildIndex));
		if (!ExistingRow)
		{
			continue;
		}

		const FGuid ExistingOfferId = ExistingRow->GetOfferPresentationView().OfferId;
		if (ExistingOfferId.IsValid())
		{
			ExistingRowsByOfferId.Add(ExistingOfferId, ExistingRow);
		}
	}

	const TArray<FWacomShopOfferPresentationView> OfferViews =
		UWacomShopPresentationBuilder::BuildOfferPresentationViews(Snapshot, CurrentGold);
	CachedOfferIds.Reset();
	CachedOfferViews = OfferViews;
	TSet<UWacomShopOfferRowWidget*> UsedRows;
	for (int32 DesiredIndex = 0; DesiredIndex < OfferViews.Num(); ++DesiredIndex)
	{
		const FWacomShopOfferPresentationView& OfferView = OfferViews[DesiredIndex];
		CachedOfferIds.Add(OfferView.OfferId);
		UWacomShopOfferRowWidget* RowWidget = ExistingRowsByOfferId.FindRef(OfferView.OfferId);
		if (!RowWidget)
		{
			if (!WidgetTree)
			{
				continue;
			}

			RowWidget = WidgetTree->ConstructWidget<UWacomShopOfferRowWidget>(
				UWacomShopOfferRowWidget::StaticClass());
			if (RowWidget)
			{
				RowWidget->OnPurchaseRequestedNative.AddUObject(this, &UWacomShopScreen::HandleOfferPurchaseRequested);
			}
		}
		if (!RowWidget)
		{
			continue;
		}

		RowWidget->SetOfferPresentationView(OfferView);
		UsedRows.Add(RowWidget);
		if (RowWidget->GetParent() == OfferList)
		{
			OfferList->ShiftChild(DesiredIndex, RowWidget);
			if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(RowWidget->Slot))
			{
				RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
			}
		}
		else if (UVerticalBoxSlot* RowSlot = OfferList->AddChildToVerticalBox(RowWidget))
		{
			RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
			OfferList->ShiftChild(DesiredIndex, RowWidget);
		}
	}

	for (const TPair<FGuid, UWacomShopOfferRowWidget*>& ExistingPair : ExistingRowsByOfferId)
	{
		if (ExistingPair.Value && !UsedRows.Contains(ExistingPair.Value))
		{
			ExistingPair.Value->RemoveFromParent();
		}
	}
}

void UWacomShopScreen::ResetShopOfferRefreshDirtyGate()
{
	LastShopOfferRefreshSignature = 0;
	bHasLastShopOfferRefreshSignature = false;
	LastShopSnapshotRevision = 0;
	bHasLastShopSnapshotRevision = false;
	LastShopEconomyRevision = 0;
	bHasLastShopEconomyRevision = false;
	CachedShopSnapshot = FRunShopSnapshot();
}

void UWacomShopScreen::HandleOfferPurchaseRequested(FGuid OfferId)
{
	PurchaseOffer(OfferId);
}

bool UWacomShopScreen::PurchaseOffer(FGuid OfferId)
{
	URunSession* Run = ResolveRunSession();
	if (!Run)
	{
		return false;
	}

	return FWacomShopScreenFlow::PurchaseOffer(*this, Run, ResolveToastSubsystem(), OfferId, CachedOfferViews);
}

UWacomAppToastSubsystem* UWacomShopScreen::ResolveToastSubsystem() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UWacomAppToastSubsystem>() : nullptr;
}

#undef LOCTEXT_NAMESPACE
