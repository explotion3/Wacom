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
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"

#include "Cards/CardDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Backpack/WacomDeleteZoneDropTarget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Backpack/WacomZoneDropTarget.h"
#include "UI/ViewModels/WacomRunViewModel.h"
#include "UI/ViewModels/WacomRunViewModelProvider.h"

UWacomBackpackScreen::UWacomBackpackScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (!CardWidgetClass)
	{
		if (UClass* Loaded = LoadObject<UClass>(
			nullptr,
			TEXT("/Game/Wacom/UI/Card/WBP_WacomDeckCardWidget.WBP_WacomDeckCardWidget_C")))
		{
			CardWidgetClass = Loaded;
		}
		else
		{
			CardWidgetClass = UWacomDeckCardWidget::StaticClass();
		}
	}
}

namespace
{
UTextBlock* CreateBackpackText(UWidgetTree* WidgetTree, FName Name, const FText& Text, int32 FontSize)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	TextBlock->SetText(Text);
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	return TextBlock;
}

UBorder* CreateBackpackSectionBorder(UWidgetTree* WidgetTree, FName Name, const FLinearColor& Color)
{
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
	Border->SetBrushColor(Color);
	Border->SetPadding(FMargin(12.f, 10.f));
	return Border;
}

UVerticalBox* AddVerticalHostSection(UWidgetTree* WidgetTree, UVerticalBox* Parent, FName BorderName, const FLinearColor& Color, const FMargin& Padding)
{
	UBorder* Border = CreateBackpackSectionBorder(WidgetTree, BorderName, Color);
	if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Border))
	{
		Slot->SetPadding(Padding);
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	UVerticalBox* Host = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Border->AddChild(Host);
	return Host;
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

		DeleteZoneHost = AddVerticalHostSection(
			WidgetTree,
			MainVBox,
			TEXT("DeleteZoneBorder"),
			FLinearColor(0.18f, 0.06f, 0.06f, 0.85f),
			FMargin(0.f, 0.f, 0.f, 8.f));

		UScrollBox* ContentScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BackpackContentScroll"));
		if (UVerticalBoxSlot* S = MainVBox->AddChildToVerticalBox(ContentScroll))
		{
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UVerticalBox* ZonesVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BackpackZonesVBox"));
		ContentScroll->AddChild(ZonesVBox);

		UVerticalBox* BattleDeckVBox = AddVerticalHostSection(
			WidgetTree,
			ZonesVBox,
			TEXT("BattleDeckBorder"),
			FLinearColor(0.06f, 0.10f, 0.18f, 0.85f),
			FMargin(0.f, 0.f, 0.f, 8.f));
		BattleDeckTitleText = CreateBackpackText(WidgetTree, TEXT("BattleDeckTitleText"), LOCTEXT("BattleDeckTitle", "[ 备战区 ]"), 16);
		if (UVerticalBoxSlot* S = BattleDeckVBox->AddChildToVerticalBox(BattleDeckTitleText))
		{
			S->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}
		BattleDeckZoneHost = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BattleDeckZoneHost"));
		BattleDeckVBox->AddChildToVerticalBox(BattleDeckZoneHost);

		UVerticalBox* BackpackVBox = AddVerticalHostSection(
			WidgetTree,
			ZonesVBox,
			TEXT("BackpackBorder"),
			FLinearColor(0.08f, 0.12f, 0.08f, 0.85f),
			FMargin(0.f));
		BackpackTitleText = CreateBackpackText(WidgetTree, TEXT("BackpackTitleText"), LOCTEXT("BackpackTitle", "[ 背包区 ]"), 16);
		if (UVerticalBoxSlot* S = BackpackVBox->AddChildToVerticalBox(BackpackTitleText))
		{
			S->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		UVerticalBox* FluxSection = AddVerticalHostSection(
			WidgetTree,
			BackpackVBox,
			TEXT("FluxZoneBorder"),
			FLinearColor(0.09f, 0.16f, 0.10f, 0.8f),
			FMargin(0.f, 0.f, 0.f, 8.f));
		FluxZoneHost = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FluxZoneHost"));
		FluxSection->AddChildToVerticalBox(FluxZoneHost);

		UScrollBox* BackpackInnerScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BackpackInnerScroll"));
		if (UVerticalBoxSlot* S = BackpackVBox->AddChildToVerticalBox(BackpackInnerScroll))
		{
			S->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			S->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
		}

		UVerticalBox* BackpackInnerVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BackpackInnerVBox"));
		BackpackInnerScroll->AddChild(BackpackInnerVBox);

		UVerticalBox* SpecialSection = AddVerticalHostSection(
			WidgetTree,
			BackpackInnerVBox,
			TEXT("SpecialZonesBorder"),
			FLinearColor(0.11f, 0.09f, 0.16f, 0.82f),
			FMargin(0.f, 0.f, 0.f, 8.f));
		SpecialZonesHost = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SpecialZonesHost"));
		SpecialSection->AddChildToVerticalBox(SpecialZonesHost);

		UVerticalBox* BurdenSection = AddVerticalHostSection(
			WidgetTree,
			BackpackInnerVBox,
			TEXT("BurdenZoneBorder"),
			FLinearColor(0.18f, 0.12f, 0.05f, 0.85f),
			FMargin(0.f));
		BurdenZoneHost = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BurdenZoneHost"));
		BurdenSection->AddChildToVerticalBox(BurdenZoneHost);
	}
	return Super::RebuildWidget();
}

void UWacomBackpackScreen::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureRuntimeZoneWidgets();

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
	EnsureRuntimeZoneWidgets();

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

FText UWacomBackpackScreen::BuildSpecialZoneTitleText(const FText& OwnerName, int32 CardCount, int32 Capacity)
{
	return FText::Format(
		LOCTEXT("SpecialZoneTitleFmt", "[ 特殊存放区 ] {0}   {1} / {2}"),
		OwnerName,
		FText::AsNumber(CardCount),
		FText::AsNumber(Capacity));
}

ESlateVisibility UWacomBackpackScreen::GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind OwnerZone)
{
	return OwnerZone == EZoneKind::BattleDeck ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
}

FText UWacomBackpackScreen::BuildBurdenZoneTitleText(int32 CardCount)
{
	return FText::Format(
		LOCTEXT("BurdenZoneTitleFmt", "[ 负重区 ] {0}"),
		FText::AsNumber(CardCount));
}

void UWacomBackpackScreen::EnsureRuntimeZoneWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!DeleteDropTarget && DeleteZoneHost)
	{
		DeleteZoneHost->ClearChildren();
		DeleteDropTarget = WidgetTree->ConstructWidget<UWacomDeleteZoneDropTarget>(UWacomDeleteZoneDropTarget::StaticClass(), TEXT("DeleteDropTarget"));
		DeleteDropTarget->Configure(EZoneKind::Backpack, FGuid());
		DeleteDropTarget->SetOwnerScreen(this);

		if (!DeleteZoneTitleText)
		{
			DeleteZoneTitleText = CreateBackpackText(
				WidgetTree,
				TEXT("DeleteZoneTitleText"),
				LOCTEXT("DeleteZoneHint", "[ 删牌区 ] 拖入卡牌置换金币（白=1 / 蓝=2）"),
				14);
		}
		DeleteDropTarget->SetDropContent(DeleteZoneTitleText);
		DeleteZoneHost->AddChild(DeleteDropTarget);
	}

	if (!BattleDeckDropTarget && BattleDeckZoneHost)
	{
		BattleDeckZoneHost->ClearChildren();
		BattleDeckDropTarget = WidgetTree->ConstructWidget<UWacomZoneDropTarget>(UWacomZoneDropTarget::StaticClass(), TEXT("BattleDeckDropTarget"));
		BattleDeckDropTarget->Configure(EZoneKind::BattleDeck, FGuid());
		BattleDeckDropTarget->SetOwnerScreen(this);

		BattleDeckCardsBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("BattleDeckCardsBox"));
		BattleDeckCardsBox->SetInnerSlotPadding(FVector2D(8.f, 8.f));
		USizeBox* DeckSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BattleDeckDropContent"));
		DeckSize->SetMinDesiredHeight(220.f);
		DeckSize->AddChild(BattleDeckCardsBox);
		BattleDeckDropTarget->SetDropContent(DeckSize);
		BattleDeckZoneHost->AddChild(BattleDeckDropTarget);
	}

	if (!BackpackDropTarget && FluxZoneHost)
	{
		FluxZoneHost->ClearChildren();
		BackpackDropTarget = WidgetTree->ConstructWidget<UWacomZoneDropTarget>(UWacomZoneDropTarget::StaticClass(), TEXT("BackpackDropTarget"));
		BackpackDropTarget->Configure(EZoneKind::Backpack, FGuid());
		BackpackDropTarget->SetOwnerScreen(this);

		BackpackCardsBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("BackpackCardsBox"));
		BackpackCardsBox->SetInnerSlotPadding(FVector2D(8.f, 8.f));
		USizeBox* PackSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BackpackDropContent"));
		PackSize->SetMinDesiredHeight(220.f);
		PackSize->AddChild(BackpackCardsBox);
		BackpackDropTarget->SetDropContent(PackSize);
		FluxZoneHost->AddChild(BackpackDropTarget);
	}

	if (!SpecialZonesPanel && SpecialZonesHost)
	{
		SpecialZonesHost->ClearChildren();
		SpecialZonesPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SpecialZonesPanel"));
		SpecialZonesHost->AddChild(SpecialZonesPanel);
	}

	if (!BurdenDropTarget && BurdenZoneHost)
	{
		BurdenZoneHost->ClearChildren();
		if (!BurdenZoneTitleText)
		{
			BurdenZoneTitleText = CreateBackpackText(
				WidgetTree,
				TEXT("BurdenZoneTitleText"),
				LOCTEXT("BurdenZoneTitle", "[ 负重区 ] 0"),
				15);
			BurdenZoneHost->AddChild(BurdenZoneTitleText);
		}

		BurdenDropTarget = WidgetTree->ConstructWidget<UWacomZoneDropTarget>(UWacomZoneDropTarget::StaticClass(), TEXT("BurdenDropTarget"));
		BurdenDropTarget->Configure(EZoneKind::BurdenZone, FGuid());
		BurdenDropTarget->SetOwnerScreen(this);

		BurdenCardsBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("BurdenCardsBox"));
		BurdenCardsBox->SetInnerSlotPadding(FVector2D(8.f, 8.f));
		USizeBox* BurdenSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BurdenDropContent"));
		BurdenSize->SetMinDesiredHeight(140.f);
		BurdenSize->AddChild(BurdenCardsBox);
		BurdenDropTarget->SetDropContent(BurdenSize);
		BurdenZoneHost->AddChild(BurdenDropTarget);
	}
}

void UWacomBackpackScreen::ClearCardBoxes()
{
	if (BattleDeckCardsBox) { BattleDeckCardsBox->ClearChildren(); }
	if (BackpackCardsBox)   { BackpackCardsBox->ClearChildren(); }
	if (SpecialZonesPanel)  { SpecialZonesPanel->ClearChildren(); }
	if (BurdenCardsBox)     { BurdenCardsBox->ClearChildren(); }
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
	}

	CardWidget->OnBattleEnabledToggleRequestedNative.AddUObject(this, &UWacomBackpackScreen::HandleBattleEnabledToggle);

	return CardWidget;
}

void UWacomBackpackScreen::RebuildAll()
{
	UWacomRunViewModel* VM = GetViewModel();
	URunSession* Run = GetRunSession();

	RebuildTopStats(VM, Run);

	// WrapBox 列表内容：仍读 RunSession（数组数据，MVVM 不擅长，保留命令式）
	if (!Run)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Backpack] RebuildAll: RunSession 未就位，列表跳过重建"));
		ClearCardBoxes();
		return;
	}

	ClearCardBoxes();
	RebuildBattleDeckZone(Run);
	RebuildBackpackZone(Run);
	RebuildSpecialZones(Run);
	RebuildBurdenZone(Run);
}

void UWacomBackpackScreen::RebuildTopStats(UWacomRunViewModel* VM, URunSession* Run)
{
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

	if (BurdenZoneTitleText && Run)
	{
		BurdenZoneTitleText->SetText(BuildBurdenZoneTitleText(Run->GetRunState().BurdenZone.Num()));
	}
}

void UWacomBackpackScreen::RebuildBattleDeckZone(URunSession* Run)
{
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

		AddBattleEnabledSpecialZoneCardsToBattleDeckView(Run);
	}
}

void UWacomBackpackScreen::AddBattleEnabledSpecialZoneCardsToBattleDeckView(URunSession* Run)
{
	if (!BattleDeckCardsBox || !Run)
	{
		return;
	}

	for (const FSpecialZone& SZ : Run->GetRunState().SpecialZones)
	{
		FCardInstance OwnerInst;
		EZoneKind OwnerZone = EZoneKind::Backpack;
		FGuid IgnoredOwner;
		const bool bOwnerFound = Run->FindInstance(SZ.OwnerInstanceId, OwnerInst, OwnerZone, IgnoredOwner);
		if (!bOwnerFound || OwnerZone != EZoneKind::BattleDeck || !OwnerInst.Definition)
		{
			continue;
		}

		for (const FCardInstance& Inst : SZ.Cards)
		{
			if (!Inst.Definition || !Inst.bBattleEnabledInSpecialZone)
			{
				continue;
			}

			UWacomDeckCardWidget* W = CreateCardWidget(Inst, EZoneKind::SpecialZone, SZ.OwnerInstanceId);
			if (!W)
			{
				continue;
			}

			W->SetRightClickToggleEnabled(true);
			W->SetProjectedFromBadgeText(FText::Format(
				LOCTEXT("ProjectedFromBadgeFmt", "来自 {0}"),
				OwnerInst.Definition->DisplayName));
			BattleDeckCardsBox->AddChildToWrapBox(W);
		}
	}
}

void UWacomBackpackScreen::RebuildBackpackZone(URunSession* Run)
{
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

void UWacomBackpackScreen::RebuildSpecialZones(URunSession* Run)
{
	if (SpecialZonesPanel)
	{
		for (const FSpecialZone& SZ : Run->GetRunState().SpecialZones)
		{
			FCardInstance OwnerInst;
			EZoneKind OwnerZone = EZoneKind::Backpack;
			FGuid IgnoredOwner;
			const bool bOwnerFound = Run->FindInstance(SZ.OwnerInstanceId, OwnerInst, OwnerZone, IgnoredOwner);
			UCardDefinition* OwnerCard = bOwnerFound ? OwnerInst.Definition : nullptr;
			const int32 Capacity = Run->GetSpecialZoneCapacityFor(SZ.OwnerInstanceId);

			UBorder* ZoneBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			ZoneBorder->SetBrushColor(FLinearColor(0.11f, 0.09f, 0.16f, 0.82f));
			ZoneBorder->SetPadding(FMargin(12.f, 10.f));
			if (UVerticalBoxSlot* BorderSlot = SpecialZonesPanel->AddChildToVerticalBox(ZoneBorder))
			{
				BorderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
			}

			UVerticalBox* ZoneVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			ZoneBorder->AddChild(ZoneVBox);

			UHorizontalBox* ZoneTitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			if (UVerticalBoxSlot* TitleRowSlot = ZoneVBox->AddChildToVerticalBox(ZoneTitleRow))
			{
				TitleRowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
			}

			UTextBlock* ZoneTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			const FText OwnerName = OwnerCard ? OwnerCard->DisplayName : LOCTEXT("UnknownSpecialZoneOwner", "未知主卡");
			ZoneTitle->SetText(BuildSpecialZoneTitleText(OwnerName, SZ.Cards.Num(), Capacity));
			FSlateFontInfo TitleFont = ZoneTitle->GetFont();
			TitleFont.Size = 15;
			ZoneTitle->SetFont(TitleFont);
			if (UHorizontalBoxSlot* TitleSlot = ZoneTitleRow->AddChildToHorizontalBox(ZoneTitle))
			{
				TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				TitleSlot->SetVerticalAlignment(VAlign_Center);
			}

			UTextBlock* BattleReadyBadge = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpecialZoneBattleReadyBadge"));
			BattleReadyBadge->SetText(LOCTEXT("SpecialZoneBattleReadyBadge", "已入战"));
			BattleReadyBadge->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.75f, 1.f, 1.f)));
			FSlateFontInfo BadgeFont = BattleReadyBadge->GetFont();
			BadgeFont.Size = 13;
			BattleReadyBadge->SetFont(BadgeFont);
			BattleReadyBadge->SetVisibility(GetSpecialZoneBattleReadyBadgeVisibility(OwnerZone));
			if (UHorizontalBoxSlot* BadgeSlot = ZoneTitleRow->AddChildToHorizontalBox(BattleReadyBadge))
			{
				BadgeSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
				BadgeSlot->SetVerticalAlignment(VAlign_Center);
			}

			UWacomZoneDropTarget* SpecialDropTarget = WidgetTree->ConstructWidget<UWacomZoneDropTarget>(UWacomZoneDropTarget::StaticClass());
			SpecialDropTarget->Configure(EZoneKind::SpecialZone, SZ.OwnerInstanceId);
			SpecialDropTarget->SetOwnerScreen(this);

			UWrapBox* SpecialCardsBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass());
			SpecialCardsBox->SetInnerSlotPadding(FVector2D(8.f, 8.f));
			USizeBox* SpecialSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			SpecialSize->SetMinDesiredHeight(220.f);
			SpecialSize->AddChild(SpecialCardsBox);
			SpecialDropTarget->SetDropContent(SpecialSize);
			if (UVerticalBoxSlot* DropSlot = ZoneVBox->AddChildToVerticalBox(SpecialDropTarget))
			{
				DropSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			}

			for (const FCardInstance& Inst : SZ.Cards)
			{
				if (!Inst.Definition) { continue; }
				UWacomDeckCardWidget* W = CreateCardWidget(Inst, EZoneKind::SpecialZone, SZ.OwnerInstanceId);
				if (!W) { continue; }
				W->SetRightClickToggleEnabled(true);
				SpecialCardsBox->AddChildToWrapBox(W);
			}
		}
	}
}

void UWacomBackpackScreen::RebuildBurdenZone(URunSession* Run)
{
	if (BurdenCardsBox)
	{
		for (const FCardInstance& Inst : Run->GetRunState().BurdenZone)
		{
			if (!Inst.Definition) { continue; }
			UWacomDeckCardWidget* W = CreateCardWidget(Inst, EZoneKind::BurdenZone, FGuid());
			if (!W) { continue; }
			BurdenCardsBox->AddChildToWrapBox(W);
		}
	}
}

void UWacomBackpackScreen::HandleBattleEnabledToggle(FGuid InstanceId)
{
	URunSession* Run = GetRunSession();
	if (!Run || !InstanceId.IsValid())
	{
		return;
	}

	FCardInstance Inst;
	EZoneKind Zone = EZoneKind::Backpack;
	FGuid ZoneOwner;
	if (!Run->FindInstance(InstanceId, Inst, Zone, ZoneOwner) || Zone != EZoneKind::SpecialZone)
	{
		return;
	}

	Run->SetSpecialZoneCardBattleEnabled(InstanceId, !Inst.bBattleEnabledInSpecialZone);
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
