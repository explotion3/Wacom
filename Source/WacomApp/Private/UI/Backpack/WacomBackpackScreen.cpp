// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackScreen.h"

#define LOCTEXT_NAMESPACE "WacomBackpack"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"

#include "Cards/CardDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Menus/WacomConfirmDialog.h"
#include "UI/ViewModels/WacomRunViewModel.h"
#include "UI/ViewModels/WacomRunViewModelProvider.h"

UWacomBackpackScreen::UWacomBackpackScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (!CardWidgetClass)
	{
		CardWidgetClass = UWacomDeckCardWidget::StaticClass();
	}
}

TSharedRef<SWidget> UWacomBackpackScreen::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// 全屏半透明黑色背景，挡住游戏世界 + 兜住操作区
		UBorder* DimBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DimBg"));
		DimBg->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.7f));
		DimBg->SetPadding(FMargin(0.f));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(DimBg))
		{
			S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			S->SetOffsets(FMargin(0.f));
		}

		// 主面板：面板背景 + 内边距
		UBorder* MainPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainPanel"));
		MainPanel->SetBrushColor(FLinearColor(0.04f, 0.05f, 0.08f, 0.92f));
		MainPanel->SetPadding(FMargin(16.f, 12.f));
		if (UCanvasPanelSlot* MainPanelSlot = Root->AddChildToCanvas(MainPanel))
		{
			MainPanelSlot->SetAnchors(FAnchors(0.05f, 0.05f, 0.95f, 0.95f));
			MainPanelSlot->SetOffsets(FMargin(0.f));
			MainPanelSlot->SetAutoSize(false);
		}

		UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
		MainPanel->AddChild(MainVBox);

		// 顶部标题行（Title + Gold + Close）
		UHorizontalBox* TopRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopRow"));
		if (UVerticalBoxSlot* TopSlot = MainVBox->AddChildToVerticalBox(TopRow))
		{
			TopSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		if (!TitleText)
		{
			TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
			TitleText->SetText(LOCTEXT("Title", "背包"));
			FSlateFontInfo Font = TitleText->GetFont();
			Font.Size = 28;
			TitleText->SetFont(Font);
			if (UHorizontalBoxSlot* S = TopRow->AddChildToHorizontalBox(TitleText))
			{
				S->SetPadding(FMargin(8.f, 4.f));
				S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				S->SetVerticalAlignment(VAlign_Center);
			}
		}

		if (!GoldText)
		{
			GoldText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GoldText"));
			GoldText->SetText(LOCTEXT("GoldPlaceholder", "金币：0"));
			FSlateFontInfo Font = GoldText->GetFont();
			Font.Size = 18;
			GoldText->SetFont(Font);
			if (UHorizontalBoxSlot* S = TopRow->AddChildToHorizontalBox(GoldText))
			{
				S->SetPadding(FMargin(8.f, 4.f));
				S->SetVerticalAlignment(VAlign_Center);
			}
		}

		if (!CloseButton)
		{
			CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Label->SetText(LOCTEXT("Close", "关闭"));
			Label->SetJustification(ETextJustify::Center);
			CloseButton->AddChild(Label);
			USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			CloseSize->SetWidthOverride(80.f);
			CloseSize->SetHeightOverride(36.f);
			CloseSize->AddChild(CloseButton);
			if (UHorizontalBoxSlot* S = TopRow->AddChildToHorizontalBox(CloseSize))
			{
				S->SetPadding(FMargin(8.f, 4.f));
				S->SetVerticalAlignment(VAlign_Center);
			}
		}

		// ---- 删牌区 ----
		UBorder* DeleteZoneBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DeleteZoneBorder"));
		DeleteZoneBorder->SetBrushColor(FLinearColor(0.18f, 0.06f, 0.06f, 0.85f));
		DeleteZoneBorder->SetPadding(FMargin(12.f, 8.f));
		if (UVerticalBoxSlot* S = MainVBox->AddChildToVerticalBox(DeleteZoneBorder))
		{
			S->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		if (!DeleteZoneTitleText)
		{
			DeleteZoneTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeleteZoneTitleText"));
			DeleteZoneTitleText->SetText(LOCTEXT("DeleteZoneHint", "[ 删牌区 ] 点击卡的 X 按钮置换金币（白=1 / 蓝=2）"));
			FSlateFontInfo Font = DeleteZoneTitleText->GetFont();
			Font.Size = 14;
			DeleteZoneTitleText->SetFont(Font);
			DeleteZoneBorder->AddChild(DeleteZoneTitleText);
		}

		// ---- 备战区 ----
		UBorder* BattleDeckBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BattleDeckBorder"));
		BattleDeckBorder->SetBrushColor(FLinearColor(0.06f, 0.10f, 0.18f, 0.85f));
		BattleDeckBorder->SetPadding(FMargin(12.f, 10.f));
		if (UVerticalBoxSlot* S = MainVBox->AddChildToVerticalBox(BattleDeckBorder))
		{
			S->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UVerticalBox* BattleDeckVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BattleDeckVBox"));
		BattleDeckBorder->AddChild(BattleDeckVBox);

		if (!BattleDeckTitleText)
		{
			BattleDeckTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BattleDeckTitleText"));
			BattleDeckTitleText->SetText(LOCTEXT("BattleDeckTitle", "[ 备战区 ]"));
			FSlateFontInfo Font = BattleDeckTitleText->GetFont();
			Font.Size = 16;
			BattleDeckTitleText->SetFont(Font);
			if (UVerticalBoxSlot* S = BattleDeckVBox->AddChildToVerticalBox(BattleDeckTitleText))
			{
				S->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
			}
		}

		if (!BattleDeckCardsBox)
		{
			BattleDeckCardsBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("BattleDeckCardsBox"));
			BattleDeckCardsBox->SetInnerSlotPadding(FVector2D(8.f, 8.f));
			USizeBox* DeckSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			DeckSize->SetMinDesiredHeight(220.f); // 一行卡的高度
			DeckSize->AddChild(BattleDeckCardsBox);
			if (UVerticalBoxSlot* S = BattleDeckVBox->AddChildToVerticalBox(DeckSize))
			{
				S->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			}
		}

		// ---- 背包区 ----
		UBorder* BackpackBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackpackBorder"));
		BackpackBorder->SetBrushColor(FLinearColor(0.08f, 0.12f, 0.08f, 0.85f));
		BackpackBorder->SetPadding(FMargin(12.f, 10.f));
		if (UVerticalBoxSlot* S = MainVBox->AddChildToVerticalBox(BackpackBorder))
		{
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UVerticalBox* BackpackVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BackpackVBox"));
		BackpackBorder->AddChild(BackpackVBox);

		if (!BackpackTitleText)
		{
			BackpackTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BackpackTitleText"));
			BackpackTitleText->SetText(LOCTEXT("BackpackTitle", "[ 背包区 ]"));
			FSlateFontInfo Font = BackpackTitleText->GetFont();
			Font.Size = 16;
			BackpackTitleText->SetFont(Font);
			if (UVerticalBoxSlot* S = BackpackVBox->AddChildToVerticalBox(BackpackTitleText))
			{
				S->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
			}
		}

		if (!BackpackCardsBox)
		{
			BackpackCardsBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("BackpackCardsBox"));
			BackpackCardsBox->SetInnerSlotPadding(FVector2D(8.f, 8.f));
			USizeBox* PackSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			PackSize->SetMinDesiredHeight(220.f);
			PackSize->AddChild(BackpackCardsBox);
			if (UVerticalBoxSlot* S = BackpackVBox->AddChildToVerticalBox(PackSize))
			{
				S->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			}
		}
	}
	return Super::RebuildWidget();
}

void UWacomBackpackScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UWacomBackpackScreen::HandleCloseClicked);
	}

	TrySubscribeAndRefresh();
}

void UWacomBackpackScreen::NativeDestruct()
{
	if (UWacomRunViewModelProvider* Provider = SubscribedProvider.Get())
	{
		Provider->OnRunViewModelRefreshedNative.RemoveAll(this);
	}
	SubscribedProvider = nullptr;

	Super::NativeDestruct();
}

void UWacomBackpackScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	// CommonUI Stack 重新激活时（背包从 GameMenu 顶层重新显示），事件订阅可能错过期间的广播；
	// 无条件刷新一次保底。
	TrySubscribeAndRefresh();
}

void UWacomBackpackScreen::TrySubscribeAndRefresh()
{
	if (!SubscribedProvider.Get())
	{
		if (UWacomRunViewModelProvider* Provider = GetProvider())
		{
			Provider->OnRunViewModelRefreshedNative.AddUObject(
				this, &UWacomBackpackScreen::HandleViewModelRefreshed);
			SubscribedProvider = Provider;
		}
	}
	RebuildAll();
}

void UWacomBackpackScreen::HandleViewModelRefreshed()
{
	RebuildAll();
}

UWacomRunViewModelProvider* UWacomBackpackScreen::GetProvider() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UWacomRunViewModelProvider>() : nullptr;
}

UWacomRunViewModel* UWacomBackpackScreen::GetViewModel() const
{
	UWacomRunViewModelProvider* Provider = GetProvider();
	return Provider ? Provider->GetRunViewModel() : nullptr;
}

URunSession* UWacomBackpackScreen::GetRunSession() const
{
	APlayerController* PC = GetOwningPlayer();
	AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC);
	return WacomPC ? WacomPC->GetRunSession() : nullptr;
}

void UWacomBackpackScreen::ClearCardBoxes()
{
	if (BattleDeckCardsBox) { BattleDeckCardsBox->ClearChildren(); }
	if (BackpackCardsBox)   { BackpackCardsBox->ClearChildren(); }
}

UWacomDeckCardWidget* UWacomBackpackScreen::CreateCardWidget(const FCardInstance& Inst, EZoneKind FromZone, FGuid FromZoneOwnerInstanceId)
{
	if (!CardWidgetClass)
	{
		return nullptr;
	}

	UWacomDeckCardWidget* CardWidget = CreateWidget<UWacomDeckCardWidget>(this, CardWidgetClass);
	if (!CardWidget)
	{
		return nullptr;
	}
	UCardDefinition* Card = Inst.Definition;
	CardWidget->SetCard(Inst, FromZone, FromZoneOwnerInstanceId);

	URunSession* Run = GetRunSession();
	if (Run)
	{
		// Stage 4.5.3a：主体按钮只作为展示和拖拽热区，不再绑定点击移动语义。
		CardWidget->SetMoveEnabled(true);

		// 删除按钮：Intrinsic / 最后 BagProvider 禁用（视觉与业务规则保持一致）
		bool bDeleteEnabled = true;
		if (URunSession::IsIntrinsicCard(Card))
		{
			bDeleteEnabled = false;
		}
		else if (URunSession::IsBagProviderCard(Card))
		{
			// 计算"销毁后还剩几张 BagProvider"
			int32 RemainingProviders = -1; // 减去本张
			// Stage 4.5.0：zone 元素是 FCardInstance，按 instance.Definition 计数
			for (const FCardInstance& BackpackInst : Run->GetBackpack())
			{
				if (URunSession::IsBagProviderCard(BackpackInst.Definition)) { ++RemainingProviders; }
			}
			for (const FCardInstance& BattleDeckInst : Run->GetBattleDeck())
			{
				if (URunSession::IsBagProviderCard(BattleDeckInst.Definition)) { ++RemainingProviders; }
			}
			if (RemainingProviders <= 0)
			{
				bDeleteEnabled = false;
			}
		}
		CardWidget->SetDeleteEnabled(bDeleteEnabled);
	}

	CardWidget->OnDeleteRequestedNative.AddUObject(this, &UWacomBackpackScreen::HandleDeleteCard);

	return CardWidget;
}

void UWacomBackpackScreen::RebuildAll()
{
	UWacomRunViewModel* VM = GetViewModel();
	URunSession* Run = GetRunSession();

	// 顶部统计：从 ViewModel 读（标量数据已被 Provider 同步）
	if (VM)
	{
		if (BattleDeckTitleText)
		{
			BattleDeckTitleText->SetText(FText::Format(
				LOCTEXT("BattleDeckTitleFmt", "[ 备战区 ]   {0} / {1}"),
				FText::AsNumber(VM->GetBattleDeckCount()),
				FText::AsNumber(VM->GetBattleDeckCapacity())));
		}
		if (BackpackTitleText)
		{
			BackpackTitleText->SetText(FText::Format(
				LOCTEXT("BackpackTitleFmt", "[ 背包区 ]   {0} / {1}（容量来自容器卡）"),
				FText::AsNumber(VM->GetBackpackCount()),
				FText::AsNumber(VM->GetFluxCapacity())));
		}
		if (GoldText)
		{
			GoldText->SetText(FText::Format(
				LOCTEXT("GoldFmt", "金币：{0}"),
				FText::AsNumber(VM->GetGold())));
		}
	}

	// WrapBox 列表内容：仍读 RunSession（数组数据，MVVM 不擅长，保留命令式）
	if (!Run)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Backpack] RebuildAll: RunSession 未就位，列表跳过重建"));
		ClearCardBoxes();
		return;
	}

	ClearCardBoxes();

	if (BattleDeckCardsBox)
	{
		// Stage 4.5.0：zone 元素是 FCardInstance，按 instance.Definition 渲染卡 widget
		for (const FCardInstance& Inst : Run->GetBattleDeck())
		{
			UCardDefinition* Card = Inst.Definition;
			if (!Card) { continue; }
			UWacomDeckCardWidget* W = CreateCardWidget(Inst, EZoneKind::BattleDeck, FGuid());
			if (!W) { continue; }
			BattleDeckCardsBox->AddChildToWrapBox(W);
		}
	}

	if (BackpackCardsBox)
	{
		for (const FCardInstance& Inst : Run->GetBackpack())
		{
			UCardDefinition* Card = Inst.Definition;
			if (!Card) { continue; }
			UWacomDeckCardWidget* W = CreateCardWidget(Inst, EZoneKind::Backpack, FGuid());
			if (!W) { continue; }
			BackpackCardsBox->AddChildToWrapBox(W);
		}
	}
}

void UWacomBackpackScreen::HandleDeleteCard(UCardDefinition* Card)
{
	if (!Card)
	{
		return;
	}

	UWacomConfirmDialog::Show(
		this,
		LOCTEXT("DeleteConfirmTitle", "删除卡牌"),
		FText::Format(
			LOCTEXT("DeleteConfirmMsg", "确认永久销毁 {0} 并置换金币？"),
			Card->DisplayName),
		[this, Card]()
		{
			URunSession* Run = GetRunSession();
			if (Run)
			{
				Run->DeleteCardForGold(Card);
			}
			// 同上，RunSession 广播会触发我们的 HandleViewModelRefreshed → RebuildAll。
		});
}

void UWacomBackpackScreen::HandleCloseClicked()
{
	DeactivateWidget();
}

FReply UWacomBackpackScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// B 键关闭：和打开是同一个键，CommonUI Menu 模式下 EnhancedInput IA 被屏蔽，
	// 必须在 widget 层自己拦。父类已经处理 ESC。
	if (InKeyEvent.GetKey() == EKeys::B)
	{
		DeactivateWidget();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
