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
#include "Misc/PackageName.h"

#include "Cards/CardDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Backpack/WacomBackpackZoneSectionWidget.h"
#include "UI/Backpack/WacomDeleteZoneDropTarget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomSpecialZoneWidget.h"
#include "UI/Backpack/WacomZoneDropTarget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/ViewModels/WacomRunViewModel.h"
#include "UI/ViewModels/WacomRunViewModelProvider.h"

namespace
{
template <typename TWidget>
TSubclassOf<TWidget> LoadOptionalWidgetClass(const TCHAR* ClassPath);
}

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

	if (!SpecialZoneWidgetClass)
	{
		if (UClass* Loaded = LoadObject<UClass>(
			nullptr,
			TEXT("/Game/Wacom/UI/Backpack/WBP_WacomSpecialZoneWidget.WBP_WacomSpecialZoneWidget_C")))
		{
			SpecialZoneWidgetClass = Loaded;
		}
		else
		{
			SpecialZoneWidgetClass = UWacomSpecialZoneWidget::StaticClass();
		}
	}

	if (!CardDetailPanelClass)
	{
		if (UClass* Loaded = LoadObject<UClass>(
			nullptr,
			TEXT("/Game/Wacom/UI/Card/WBP_CardDetailPanel.WBP_CardDetailPanel_C")))
		{
			CardDetailPanelClass = Loaded;
		}
		else
		{
			CardDetailPanelClass = UWacomCardDetailPanel::StaticClass();
		}
	}

	if (!DeleteZoneSectionWidgetClass)
	{
		DeleteZoneSectionWidgetClass = LoadOptionalWidgetClass<UWacomBackpackZoneSectionWidget>(
			TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackDeleteZone.WBP_BackpackDeleteZone_C"));
	}
	if (!BattleDeckZoneSectionWidgetClass)
	{
		BattleDeckZoneSectionWidgetClass = LoadOptionalWidgetClass<UWacomBackpackZoneSectionWidget>(
			TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackBattleDeckZone.WBP_BackpackBattleDeckZone_C"));
	}
	if (!FluxContentZoneSectionWidgetClass)
	{
		FluxContentZoneSectionWidgetClass = LoadOptionalWidgetClass<UWacomBackpackZoneSectionWidget>(
			TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackFluxContentZone.WBP_BackpackFluxContentZone_C"));
	}
	if (!SpecialZonesSectionWidgetClass)
	{
		SpecialZonesSectionWidgetClass = LoadOptionalWidgetClass<UWacomBackpackZoneSectionWidget>(
			TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackSpecialZones.WBP_BackpackSpecialZones_C"));
	}
	if (!BurdenZoneSectionWidgetClass)
	{
		BurdenZoneSectionWidgetClass = LoadOptionalWidgetClass<UWacomBackpackZoneSectionWidget>(
			TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackBurdenZone.WBP_BackpackBurdenZone_C"));
	}
}

namespace
{
const FVector2D CardDetailPanelEstimatedSize(360.f, 420.f);
constexpr float CardDetailPanelPadding = 12.f;

template <typename TWidget>
TSubclassOf<TWidget> LoadOptionalWidgetClass(const TCHAR* ClassPath)
{
	const FString ObjectPath(ClassPath);
	const FString PackagePath = FPackageName::ObjectPathToPackageName(ObjectPath);
	if (!FPackageName::DoesPackageExist(PackagePath))
	{
		return TWidget::StaticClass();
	}

	if (UClass* Loaded = LoadObject<UClass>(nullptr, ClassPath))
	{
		return Loaded;
	}
	return TWidget::StaticClass();
}

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

UPanelWidget* AddZoneSectionWidget(
	UUserWidget* Owner,
	UWidgetTree* WidgetTree,
	UVerticalBox* Parent,
	TSubclassOf<UWacomBackpackZoneSectionWidget> SectionClass,
	const FText& Title,
	const FMargin& Padding,
	UWacomBackpackZoneSectionWidget** OutSection = nullptr)
{
	if (!Owner || !WidgetTree || !Parent)
	{
		return nullptr;
	}

	auto AttachSection = [Parent, &Padding](UWacomBackpackZoneSectionWidget* SectionToAttach)
	{
		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(SectionToAttach))
		{
			Slot->SetPadding(Padding);
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	};

	auto CreateSection = [WidgetTree, &Title](UClass* ClassToUse)
	{
		UWacomBackpackZoneSectionWidget* Section = WidgetTree->ConstructWidget<UWacomBackpackZoneSectionWidget>(ClassToUse);
		if (Section)
		{
			Section->SetZoneTitleText(Title);
		}
		return Section;
	};

	UClass* ClassToUse = SectionClass ? SectionClass.Get() : UWacomBackpackZoneSectionWidget::StaticClass();
	UWacomBackpackZoneSectionWidget* Section = CreateSection(ClassToUse);
	if (!Section)
	{
		return nullptr;
	}

	AttachSection(Section);
	UPanelWidget* ContentHost = Section->EnsureContentHost();
	if (!ContentHost && ClassToUse != UWacomBackpackZoneSectionWidget::StaticClass())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Backpack] %s 缺少 ContentHost 绑定，当前区块回退到 C++ 默认布局。"),
			*GetNameSafe(ClassToUse));

		Parent->RemoveChild(Section);
		Section = CreateSection(UWacomBackpackZoneSectionWidget::StaticClass());
		if (!Section)
		{
			return nullptr;
		}
		AttachSection(Section);
		ContentHost = Section->EnsureContentHost();
	}

	if (OutSection)
	{
		*OutSection = Section;
	}
	return ContentHost;
}

void LogBackpackBindingWarningOnce(FName Key, const TCHAR* Message)
{
	static TSet<FName> LoggedKeys;
	if (!LoggedKeys.Contains(Key))
	{
		LoggedKeys.Add(Key);
		UE_LOG(LogTemp, Warning, TEXT("%s"), Message);
	}
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

		CardDetailLayer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CardDetailLayer"));
		CardDetailLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* DetailLayerSlot = Root->AddChildToCanvas(CardDetailLayer))
		{
			DetailLayerSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			DetailLayerSlot->SetOffsets(FMargin(0.f));
			DetailLayerSlot->SetAutoSize(false);
			DetailLayerSlot->SetZOrder(10);
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

		DeleteZoneHost = AddZoneSectionWidget(
			this,
			WidgetTree,
			MainVBox,
			DeleteZoneSectionWidgetClass,
			LOCTEXT("DeleteZoneTitle", "[ 删牌区 ]"),
			FMargin(0.f, 0.f, 0.f, 8.f));

		UScrollBox* ContentScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BackpackContentScroll"));
		if (UVerticalBoxSlot* S = MainVBox->AddChildToVerticalBox(ContentScroll))
		{
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UVerticalBox* ZonesVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BackpackZonesVBox"));
		ContentScroll->AddChild(ZonesVBox);

		UWacomBackpackZoneSectionWidget* BattleDeckSectionRaw = nullptr;
		BattleDeckZoneHost = AddZoneSectionWidget(
			this,
			WidgetTree,
			ZonesVBox,
			BattleDeckZoneSectionWidgetClass,
			LOCTEXT("BattleDeckTitle", "[ 备战区 ]"),
			FMargin(0.f, 0.f, 0.f, 8.f),
			&BattleDeckSectionRaw);
		BattleDeckZoneSection = BattleDeckSectionRaw;

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

		UVerticalBox* FluxContentColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FluxContentCardsColumn"));
		if (UVerticalBoxSlot* ContentSlot = FluxSection->AddChildToVerticalBox(FluxContentColumn))
		{
			ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UWacomBackpackZoneSectionWidget* FluxContentSectionRaw = nullptr;
		FluxContentDropTargetHost = AddZoneSectionWidget(
			this,
			WidgetTree,
			FluxContentColumn,
			FluxContentZoneSectionWidgetClass,
			LOCTEXT("FluxContentCardsTitle", "[ 通量内容 ]"),
			FMargin(0.f),
			&FluxContentSectionRaw);
		FluxContentZoneSection = FluxContentSectionRaw;

		UScrollBox* BackpackInnerScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BackpackInnerScroll"));
		if (UVerticalBoxSlot* S = BackpackVBox->AddChildToVerticalBox(BackpackInnerScroll))
		{
			S->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			S->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
		}

		UVerticalBox* BackpackInnerVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BackpackInnerVBox"));
		BackpackInnerScroll->AddChild(BackpackInnerVBox);

		SpecialZonesHost = AddZoneSectionWidget(
			this,
			WidgetTree,
			BackpackInnerVBox,
			SpecialZonesSectionWidgetClass,
			LOCTEXT("SpecialZonesTitle", "[ 特殊存放区 ]"),
			FMargin(0.f, 0.f, 0.f, 8.f));

		UWacomBackpackZoneSectionWidget* BurdenSectionRaw = nullptr;
		BurdenZoneHost = AddZoneSectionWidget(
			this,
			WidgetTree,
			BackpackInnerVBox,
			BurdenZoneSectionWidgetClass,
			LOCTEXT("BurdenZoneTitle", "[ 负重区 ] 0"),
			FMargin(0.f),
			&BurdenSectionRaw);
		BurdenZoneSection = BurdenSectionRaw;
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
	HideCardDetailPanel();

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
	return UWacomBackpackScreenPresenter::BuildSpecialZoneTitleText(OwnerName, CardCount, Capacity);
}

ESlateVisibility UWacomBackpackScreen::GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind OwnerZone)
{
	return UWacomBackpackScreenPresenter::GetSpecialZoneBattleReadyBadgeVisibility(OwnerZone);
}

FText UWacomBackpackScreen::BuildBurdenZoneTitleText(int32 CardCount)
{
	return UWacomBackpackScreenPresenter::BuildBurdenZoneTitleText(CardCount);
}

FVector2D UWacomBackpackScreen::ComputeCardDetailPanelPosition(
	FVector2D AnchorPosition,
	FVector2D AnchorSize,
	FVector2D LayerSize,
	FVector2D PanelSize,
	float Padding)
{
	return UWacomBackpackScreenPresenter::ComputeCardDetailPanelPosition(
		AnchorPosition,
		AnchorSize,
		LayerSize,
		PanelSize,
		Padding);
}

bool UWacomBackpackScreen::IsCardDetailPanelVisible() const
{
	return CardDetailPanel && CardDetailPanel->GetVisibility() != ESlateVisibility::Collapsed;
}

FText UWacomBackpackScreen::GetCardDetailPanelNameText() const
{
	return CardDetailPanel ? CardDetailPanel->GetNameText() : FText::GetEmpty();
}

void UWacomBackpackScreen::EnsureRuntimeZoneWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!CardDetailLayer)
	{
		if (UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget))
		{
			CardDetailLayer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CardDetailLayer_Runtime"));
			CardDetailLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (UCanvasPanelSlot* DetailLayerSlot = RootCanvas->AddChildToCanvas(CardDetailLayer))
			{
				DetailLayerSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
				DetailLayerSlot->SetOffsets(FMargin(0.f));
				DetailLayerSlot->SetAutoSize(false);
				DetailLayerSlot->SetZOrder(10);
			}
		}
		else
		{
			LogBackpackBindingWarningOnce(TEXT("CardDetailLayer"), TEXT("[Backpack] CardDetailLayer 未绑定，且 RootWidget 不是 CanvasPanel，卡牌悬浮详情不会显示"));
		}
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
				LOCTEXT("DeleteZoneHint", "拖入卡牌置换金币（白=1 / 蓝=2）"),
				14);
		}
		DeleteDropTarget->SetDropContent(DeleteZoneTitleText);
		DeleteZoneHost->AddChild(DeleteDropTarget);
	}
	else if (!DeleteZoneHost)
	{
		LogBackpackBindingWarningOnce(TEXT("DeleteZoneHost"), TEXT("[Backpack] DeleteZoneHost 未绑定，删牌区运行时内容不会显示"));
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
	else if (!BattleDeckZoneHost)
	{
		LogBackpackBindingWarningOnce(TEXT("BattleDeckZoneHost"), TEXT("[Backpack] BattleDeckZoneHost 未绑定，备战区运行时内容不会显示"));
	}

	if (!BackpackDropTarget && FluxContentDropTargetHost)
	{
		FluxContentDropTargetHost->ClearChildren();
		BackpackDropTarget = WidgetTree->ConstructWidget<UWacomZoneDropTarget>(UWacomZoneDropTarget::StaticClass(), TEXT("BackpackDropTarget"));
		BackpackDropTarget->Configure(EZoneKind::Backpack, FGuid());
		BackpackDropTarget->SetOwnerScreen(this);

		if (!FluxContentCardsBox)
		{
			FluxContentCardsBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("FluxContentCardsBox_Runtime"));
			FluxContentCardsBox->SetInnerSlotPadding(FVector2D(8.f, 8.f));
		}
		USizeBox* PackSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BackpackDropContent"));
		PackSize->SetMinDesiredHeight(220.f);
		PackSize->AddChild(FluxContentCardsBox);
		BackpackDropTarget->SetDropContent(PackSize);
		FluxContentDropTargetHost->AddChild(BackpackDropTarget);
	}
	else if (!FluxContentDropTargetHost && !FluxContentCardsBox)
	{
		LogBackpackBindingWarningOnce(TEXT("FluxContentDropTargetHost"), TEXT("[Backpack] FluxContentDropTargetHost/FluxContentCardsBox 未绑定，通量内容区不会显示"));
	}

	if (!SpecialZonesPanel && SpecialZonesHost)
	{
		SpecialZonesHost->ClearChildren();
		SpecialZonesPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SpecialZonesPanel"));
		SpecialZonesHost->AddChild(SpecialZonesPanel);
	}
	else if (!SpecialZonesHost && !SpecialZonesPanel)
	{
		LogBackpackBindingWarningOnce(TEXT("SpecialZonesHost"), TEXT("[Backpack] SpecialZonesHost 未绑定，特殊存放区不会显示"));
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
	else if (!BurdenZoneHost)
	{
		LogBackpackBindingWarningOnce(TEXT("BurdenZoneHost"), TEXT("[Backpack] BurdenZoneHost 未绑定，负重区运行时内容不会显示"));
	}
}

void UWacomBackpackScreen::ClearCardBoxes()
{
	HideCardDetailPanel();
	if (BattleDeckCardsBox) { BattleDeckCardsBox->ClearChildren(); }
	if (FluxContentCardsBox) { FluxContentCardsBox->ClearChildren(); }
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
	CardWidget->SetCard(Inst, FromZone, FromZoneOwnerInstanceId);

	URunSession* Run = GetRunSession();
	if (Run)
	{
		// Stage 4.5.3a：主体按钮只作为展示和拖拽热区，不再绑定点击移动语义。
		CardWidget->SetMoveEnabled(true);
	}

	CardWidget->OnBattleEnabledToggleRequestedNative.AddUObject(this, &UWacomBackpackScreen::HandleBattleEnabledToggle);
	CardWidget->OnCardHoveredNative.AddUObject(this, &UWacomBackpackScreen::HandleCardHovered);
	CardWidget->OnCardUnhoveredNative.AddUObject(this, &UWacomBackpackScreen::HandleCardUnhovered);

	return CardWidget;
}

UWacomDeckCardWidget* UWacomBackpackScreen::CreateCardWidget(const FRunStorageCardView& CardView)
{
	return CreateCardWidget(CardView.Instance, CardView.PhysicalZone, CardView.ZoneOwnerInstanceId);
}

void UWacomBackpackScreen::RebuildAll()
{
	UWacomRunViewModel* VM = GetViewModel();
	URunSession* Run = GetRunSession();

	RebuildTopStats(VM);

	if (!Run)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Backpack] RebuildAll: RunSession 未就位，列表跳过重建"));
		ClearCardBoxes();
		return;
	}

	const FRunBackpackStorageSnapshot Snapshot = Run->BuildBackpackStorageSnapshot();
	ClearCardBoxes();
	RebuildBattleDeckZone(Snapshot);
	RebuildBackpackZone(Snapshot);
	RebuildSpecialZones(Snapshot);
	RebuildBurdenZone(Snapshot);
}

void UWacomBackpackScreen::RebuildTopStats(UWacomRunViewModel* VM)
{
	if (VM)
	{
		const FText BattleDeckTitle = UWacomBackpackScreenPresenter::BuildBattleDeckTitleText(
			VM->GetBattleDeckCount(),
			VM->GetBattleDeckCapacity());
		if (BattleDeckTitleText)
		{
			BattleDeckTitleText->SetText(BattleDeckTitle);
		}
		if (BattleDeckZoneSection)
		{
			BattleDeckZoneSection->SetZoneTitleText(BattleDeckTitle);
		}

		if (BackpackTitleText)
		{
			BackpackTitleText->SetText(UWacomBackpackScreenPresenter::BuildBackpackTitleText());
		}
		if (GoldText)
		{
			GoldText->SetText(UWacomBackpackScreenPresenter::BuildGoldText(VM->GetGold()));
		}
	}
}

void UWacomBackpackScreen::RebuildBattleDeckZone(const FRunBackpackStorageSnapshot& Snapshot)
{
	if (BattleDeckCardsBox)
	{
		for (const FRunStorageCardView& CardView : Snapshot.BattleDeckPhysicalCards)
		{
			UWacomDeckCardWidget* W = CreateCardWidget(CardView);
			if (!W) { continue; }
			BattleDeckCardsBox->AddChildToWrapBox(W);
		}

		for (const FRunStorageCardView& ProjectedView : Snapshot.BattleDeckProjectedCards)
		{
			UWacomDeckCardWidget* W = CreateCardWidget(ProjectedView);
			if (!W)
			{
				continue;
			}

			const FText ProjectedBadgeText = UWacomBackpackScreenPresenter::BuildBattleDeckProjectedFromBadgeText(ProjectedView, Snapshot);
			if (!ProjectedBadgeText.IsEmpty())
			{
				W->SetProjectedFromBadgeText(ProjectedBadgeText);
			}
			W->SetRightClickToggleEnabled(true);
			BattleDeckCardsBox->AddChildToWrapBox(W);
		}
	}
}

void UWacomBackpackScreen::RebuildBackpackZone(const FRunBackpackStorageSnapshot& Snapshot)
{
	RebuildFluxContentCards(Snapshot);
}

void UWacomBackpackScreen::RebuildFluxMainCards(const FRunBackpackStorageSnapshot& Snapshot)
{
	// 兼容旧 API：通量区已不再有 A 类主卡槽，默认不渲染 Flux.MainCards。
}

void UWacomBackpackScreen::RebuildFluxContentCards(const FRunBackpackStorageSnapshot& Snapshot)
{
	if (FluxContentZoneSection)
	{
		FluxContentZoneSection->SetZoneTitleText(UWacomBackpackScreenPresenter::BuildFluxContentTitleText(
			Snapshot.FluxContentCount,
			Snapshot.FluxCapacity));
	}

	if (FluxContentCardsBox)
	{
		for (const FRunStorageCardView& CardView : Snapshot.Flux.ContentCards)
		{
			UWacomDeckCardWidget* W = CreateCardWidget(CardView);
			if (!W) { continue; }
			FluxContentCardsBox->AddChildToWrapBox(W);
		}
	}
}

void UWacomBackpackScreen::RebuildSpecialZones(const FRunBackpackStorageSnapshot& Snapshot)
{
	if (SpecialZonesPanel)
	{
		for (const FRunSpecialStorageView& SpecialView : Snapshot.SpecialZones)
		{
			UClass* ZoneWidgetClass = SpecialZoneWidgetClass ? SpecialZoneWidgetClass.Get() : UWacomSpecialZoneWidget::StaticClass();
			UWacomSpecialZoneWidget* ZoneWidget = CreateWidget<UWacomSpecialZoneWidget>(this, ZoneWidgetClass);
			if (!ZoneWidget)
			{
				continue;
			}
			ZoneWidget->SetSpecialZoneView(SpecialView, this, CardWidgetClass);
			ZoneWidget->OnBattleEnabledToggleRequestedNative.AddUObject(this, &UWacomBackpackScreen::HandleBattleEnabledToggle);
			ZoneWidget->OnCardHoveredNative.AddUObject(this, &UWacomBackpackScreen::HandleCardHovered);
			ZoneWidget->OnCardUnhoveredNative.AddUObject(this, &UWacomBackpackScreen::HandleCardUnhovered);
			if (UVerticalBoxSlot* ZoneSlot = SpecialZonesPanel->AddChildToVerticalBox(ZoneWidget))
			{
				ZoneSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
			}
		}
	}
}

void UWacomBackpackScreen::RebuildBurdenZone(const FRunBackpackStorageSnapshot& Snapshot)
{
	const FText BurdenTitle = UWacomBackpackScreenPresenter::BuildBurdenZoneTitleText(Snapshot.BurdenCount);
	if (BurdenZoneTitleText)
	{
		BurdenZoneTitleText->SetText(BurdenTitle);
	}
	if (BurdenZoneSection)
	{
		BurdenZoneSection->SetZoneTitleText(BurdenTitle);
	}

	if (BurdenCardsBox)
	{
		for (const FRunStorageCardView& CardView : Snapshot.BurdenCards)
		{
			UWacomDeckCardWidget* W = CreateCardWidget(CardView);
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

void UWacomBackpackScreen::HandleCardHovered(UWacomDeckCardWidget* SourceWidget)
{
	ShowCardDetailForCardWidget(SourceWidget);
}

void UWacomBackpackScreen::HandleCardUnhovered(UWacomDeckCardWidget* SourceWidget)
{
	HideCardDetailPanel();
}

bool UWacomBackpackScreen::ShowCardDetailForCardWidget(UWacomDeckCardWidget* SourceWidget)
{
	if (!SourceWidget || !SourceWidget->GetCard())
	{
		HideCardDetailPanel();
		return false;
	}

	UWacomCardDetailPanel* Panel = EnsureCardDetailPanel();
	if (!Panel)
	{
		return false;
	}

	Panel->SetCardDetailData(UWacomBackpackScreenPresenter::BuildCardDetailViewData(SourceWidget->GetCard()));
	PositionCardDetailPanelNear(SourceWidget);
	Panel->SetRenderOpacity(1.f);
	Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	return true;
}

void UWacomBackpackScreen::HideCardDetailPanel()
{
	if (CardDetailPanel)
	{
		CardDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UWacomCardDetailPanel* UWacomBackpackScreen::EnsureCardDetailPanel()
{
	EnsureRuntimeZoneWidgets();
	if (!CardDetailLayer)
	{
		return nullptr;
	}

	if (CardDetailPanel)
	{
		return CardDetailPanel;
	}

	UClass* PanelClass = CardDetailPanelClass
		? CardDetailPanelClass.Get()
		: UWacomCardDetailPanel::StaticClass();
	CardDetailPanel = GetWorld()
		? CreateWidget<UWacomCardDetailPanel>(this, PanelClass)
		: NewObject<UWacomCardDetailPanel>(this, PanelClass);
	if (!CardDetailPanel)
	{
		return nullptr;
	}

	CardDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	CardDetailPanel->SetIsEnabled(true);
	CardDetailPanel->SetRenderOpacity(1.f);
	if (UCanvasPanelSlot* DetailSlot = CardDetailLayer->AddChildToCanvas(CardDetailPanel))
	{
		DetailSlot->SetAutoSize(false);
		DetailSlot->SetSize(CardDetailPanelEstimatedSize);
		DetailSlot->SetZOrder(1);
	}
	return CardDetailPanel;
}

void UWacomBackpackScreen::PositionCardDetailPanelNear(UWacomDeckCardWidget* SourceWidget)
{
	if (!SourceWidget || !CardDetailLayer || !CardDetailPanel)
	{
		return;
	}

	const FGeometry& LayerGeometry = CardDetailLayer->GetCachedGeometry();
	const FGeometry& SourceGeometry = SourceWidget->GetCachedGeometry();
	const FVector2D AnchorPosition = LayerGeometry.AbsoluteToLocal(SourceGeometry.GetAbsolutePosition());
	const FVector2D AnchorSize = SourceGeometry.GetLocalSize();
	const FVector2D LayerSize = LayerGeometry.GetLocalSize();
	const FVector2D Position = UWacomBackpackScreenPresenter::ComputeCardDetailPanelPosition(
		AnchorPosition,
		AnchorSize,
		LayerSize,
		CardDetailPanelEstimatedSize,
		CardDetailPanelPadding);

	if (UCanvasPanelSlot* DetailSlot = Cast<UCanvasPanelSlot>(CardDetailPanel->Slot))
	{
		DetailSlot->SetPosition(Position);
		DetailSlot->SetSize(CardDetailPanelEstimatedSize);
	}
}

void UWacomBackpackScreen::HandleCloseClicked()
{
	HideCardDetailPanel();
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
