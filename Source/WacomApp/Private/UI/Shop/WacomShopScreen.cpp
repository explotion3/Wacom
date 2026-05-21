// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopScreen.h"

#define LOCTEXT_NAMESPACE "WacomShopScreen"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "RunState.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Shop/WacomShopOfferRowWidget.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"

namespace
{
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

	RefreshShop();
}

void UWacomShopScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	bDidEndShopVisit = false;
	RefreshShop();
}

void UWacomShopScreen::NativeOnDeactivated()
{
	if (!bDidEndShopVisit)
	{
		if (URunSession* Run = GetRunSession())
		{
			Run->EndShopVisit();
		}
		bDidEndShopVisit = true;
	}
	Super::NativeOnDeactivated();
}

void UWacomShopScreen::RefreshShop()
{
	if (URunSession* Run = GetRunSession())
	{
		const FRunShopSnapshot Snapshot = Run->BuildCurrentShopSnapshot();
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
				FText::AsNumber(Run->GetGold())));
		}
	}

	RebuildOfferRows();
}

bool UWacomShopScreen::PurchaseOfferByIndexForTest(int32 Index)
{
	if (!CachedOfferIds.IsValidIndex(Index))
	{
		return false;
	}
	return PurchaseOffer(CachedOfferIds[Index]);
}

FWacomShopOfferPresentationView UWacomShopScreen::GetOfferPresentationViewForTest(int32 Index) const
{
	return CachedOfferViews.IsValidIndex(Index)
		? CachedOfferViews[Index]
		: FWacomShopOfferPresentationView();
}

FText UWacomShopScreen::BuildPurchaseFailureToastTextForTest(FName DisabledReason)
{
	return BuildPurchaseFailureToastText(DisabledReason);
}

FText UWacomShopScreen::GetGoldTextForTest() const
{
	return GoldText ? GoldText->GetText() : FText::GetEmpty();
}

void UWacomShopScreen::HandleCloseClicked()
{
	DeactivateWidget();
}

URunSession* UWacomShopScreen::GetRunSession() const
{
	if (RunSessionOverride)
	{
		return RunSessionOverride;
	}
	AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(GetOwningPlayer());
	return WacomPC ? WacomPC->GetRunSession() : nullptr;
}

void UWacomShopScreen::RebuildOfferRows()
{
	CachedOfferIds.Reset();
	CachedOfferViews.Reset();
	if (OfferList)
	{
		OfferList->ClearChildren();
	}

	URunSession* Run = GetRunSession();
	const FRunShopSnapshot Snapshot = Run ? Run->BuildCurrentShopSnapshot() : FRunShopSnapshot();
	if (EmptyText)
	{
		EmptyText->SetVisibility(Snapshot.Offers.Num() == 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (!OfferList)
	{
		return;
	}

	const int32 CurrentGold = Run ? Run->GetGold() : 0;
	const TArray<FWacomShopOfferPresentationView> OfferViews =
		UWacomShopPresentationBuilder::BuildOfferPresentationViews(Snapshot, CurrentGold);
	CachedOfferViews = OfferViews;
	for (const FWacomShopOfferPresentationView& OfferView : OfferViews)
	{
		CachedOfferIds.Add(OfferView.OfferId);
		AddOfferRow(OfferView);
	}
}

void UWacomShopScreen::AddOfferRow(const FWacomShopOfferPresentationView& OfferView)
{
	if (!WidgetTree || !OfferList)
	{
		return;
	}

	UWacomShopOfferRowWidget* RowWidget = WidgetTree->ConstructWidget<UWacomShopOfferRowWidget>(
		UWacomShopOfferRowWidget::StaticClass());
	RowWidget->SetOfferPresentationView(OfferView);
	RowWidget->OnPurchaseRequestedNative.AddUObject(this, &UWacomShopScreen::HandleOfferPurchaseRequested);
	if (UVerticalBoxSlot* RowSlot = OfferList->AddChildToVerticalBox(RowWidget))
	{
		RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}
}

void UWacomShopScreen::HandleOfferPurchaseRequested(FGuid OfferId)
{
	PurchaseOffer(OfferId);
}

bool UWacomShopScreen::PurchaseOffer(FGuid OfferId)
{
	URunSession* Run = GetRunSession();
	if (!Run)
	{
		return false;
	}

	const FWacomShopOfferPresentationView* OfferView = FindCachedOfferView(OfferId);
	const bool bPurchased = Run->PurchaseShopOffer(OfferId);
	if (bPurchased)
	{
		ShowPurchaseSuccessToast(OfferView);
	}
	else
	{
		ShowPurchaseFailureToast(OfferView);
	}
	RefreshShop();
	return bPurchased;
}

const FWacomShopOfferPresentationView* UWacomShopScreen::FindCachedOfferView(FGuid OfferId) const
{
	return CachedOfferViews.FindByPredicate(
		[OfferId](const FWacomShopOfferPresentationView& View)
		{
			return View.OfferId == OfferId;
		});
}

UWacomAppToastSubsystem* UWacomShopScreen::GetToastSubsystem() const
{
	if (ToastSubsystemOverride)
	{
		return ToastSubsystemOverride;
	}

	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UWacomAppToastSubsystem>() : nullptr;
}

void UWacomShopScreen::ShowPurchaseSuccessToast(const FWacomShopOfferPresentationView* OfferView) const
{
	if (UWacomAppToastSubsystem* ToastSubsystem = GetToastSubsystem())
	{
		ToastSubsystem->ShowCardGained(OfferView ? OfferView->CardDefinition.Get() : nullptr);
	}
}

FText UWacomShopScreen::BuildPurchaseFailureToastText(FName DisabledReason)
{
	if (DisabledReason == FName(TEXT("InsufficientGold")))
	{
		return LOCTEXT("PurchaseFailedInsufficientGold", "金币不足");
	}
	if (DisabledReason == FName(TEXT("Purchased")))
	{
		return LOCTEXT("PurchaseFailedPurchased", "该商品已购买");
	}
	if (DisabledReason == FName(TEXT("MissingCard")))
	{
		return LOCTEXT("PurchaseFailedMissingCard", "商品不可购买");
	}
	return LOCTEXT("PurchaseFailed", "购买失败");
}

void UWacomShopScreen::ShowPurchaseFailureToast(const FWacomShopOfferPresentationView* OfferView) const
{
	const FText Message = BuildPurchaseFailureToastText(OfferView ? OfferView->DisabledReason : NAME_None);

	if (UWacomAppToastSubsystem* ToastSubsystem = GetToastSubsystem())
	{
		ToastSubsystem->ShowWarning(Message);
	}
}

#undef LOCTEXT_NAMESPACE
