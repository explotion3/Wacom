// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/BackpackFallbackLayoutBuilder.h"

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
#include "UI/Backpack/WacomBackpackZoneSectionWidget.h"

#define LOCTEXT_NAMESPACE "WacomBackpack"

namespace
{
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
}

UTextBlock* FBackpackFallbackLayoutBuilder::CreateBackpackText(UWidgetTree* WidgetTree, FName Name, const FText& Text, int32 FontSize)
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	TextBlock->SetText(Text);
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	return TextBlock;
}

void FBackpackFallbackLayoutBuilder::Build(const FBackpackFallbackLayoutBuilderContext& Context)
{
	UWidgetTree* WidgetTree = Context.WidgetTree;
	if (!Context.Owner || !WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* DimBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DimBg"));
	DimBg->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.7f));
	DimBg->SetPadding(FMargin(0.f));
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(DimBg))
	{
		Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		Slot->SetOffsets(FMargin(0.f));
	}

	UBorder* MainPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainPanel"));
	MainPanel->SetBrushColor(FLinearColor(0.04f, 0.05f, 0.08f, 0.92f));
	MainPanel->SetPadding(FMargin(16.f, 12.f));
	if (UCanvasPanelSlot* MainPanelSlot = Root->AddChildToCanvas(MainPanel))
	{
		MainPanelSlot->SetAnchors(FAnchors(0.05f, 0.05f, 0.95f, 0.95f));
		MainPanelSlot->SetOffsets(FMargin(0.f));
		MainPanelSlot->SetAutoSize(false);
	}

	if (Context.CardDetailLayer)
	{
		*Context.CardDetailLayer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CardDetailLayer"));
		(*Context.CardDetailLayer)->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* DetailLayerSlot = Root->AddChildToCanvas(Context.CardDetailLayer->Get()))
		{
			DetailLayerSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			DetailLayerSlot->SetOffsets(FMargin(0.f));
			DetailLayerSlot->SetAutoSize(false);
			DetailLayerSlot->SetZOrder(10);
		}
	}

	UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
	MainPanel->AddChild(MainVBox);

	UHorizontalBox* TopRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopRow"));
	if (UVerticalBoxSlot* TopSlot = MainVBox->AddChildToVerticalBox(TopRow))
	{
		TopSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	if (Context.TitleText && !*Context.TitleText)
	{
		*Context.TitleText = CreateBackpackText(WidgetTree, TEXT("TitleText"), LOCTEXT("Title", "背包"), 28);
		if (UHorizontalBoxSlot* Slot = TopRow->AddChildToHorizontalBox(Context.TitleText->Get()))
		{
			Slot->SetPadding(FMargin(8.f, 4.f));
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	}

	if (Context.GoldText && !*Context.GoldText)
	{
		*Context.GoldText = CreateBackpackText(WidgetTree, TEXT("GoldText"), LOCTEXT("GoldPlaceholder", "金币：0"), 18);
		if (UHorizontalBoxSlot* Slot = TopRow->AddChildToHorizontalBox(Context.GoldText->Get()))
		{
			Slot->SetPadding(FMargin(8.f, 4.f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	}

	if (Context.CloseButton && !*Context.CloseButton)
	{
		*Context.CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(LOCTEXT("Close", "关闭"));
		Label->SetJustification(ETextJustify::Center);
		(*Context.CloseButton)->AddChild(Label);

		USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		CloseSize->SetWidthOverride(80.f);
		CloseSize->SetHeightOverride(36.f);
		CloseSize->AddChild(Context.CloseButton->Get());
		if (UHorizontalBoxSlot* Slot = TopRow->AddChildToHorizontalBox(CloseSize))
		{
			Slot->SetPadding(FMargin(8.f, 4.f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	}

	if (Context.DeleteZoneHost)
	{
		*Context.DeleteZoneHost = AddZoneSectionWidget(
			Context.Owner,
			WidgetTree,
			MainVBox,
			Context.DeleteZoneSectionWidgetClass,
			LOCTEXT("DeleteZoneTitle", "[ 删牌区 ]"),
			FMargin(0.f, 0.f, 0.f, 8.f));
	}

	UScrollBox* ContentScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BackpackContentScroll"));
	if (UVerticalBoxSlot* Slot = MainVBox->AddChildToVerticalBox(ContentScroll))
	{
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UVerticalBox* ZonesVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BackpackZonesVBox"));
	ContentScroll->AddChild(ZonesVBox);

	UWacomBackpackZoneSectionWidget* BattleDeckSectionRaw = nullptr;
	if (Context.BattleDeckZoneHost)
	{
		*Context.BattleDeckZoneHost = AddZoneSectionWidget(
			Context.Owner,
			WidgetTree,
			ZonesVBox,
			Context.BattleDeckZoneSectionWidgetClass,
			LOCTEXT("BattleDeckTitle", "[ 备战区 ]"),
			FMargin(0.f, 0.f, 0.f, 8.f),
			&BattleDeckSectionRaw);
	}
	if (Context.BattleDeckZoneSection)
	{
		*Context.BattleDeckZoneSection = BattleDeckSectionRaw;
	}

	UVerticalBox* BackpackVBox = AddVerticalHostSection(
		WidgetTree,
		ZonesVBox,
		TEXT("BackpackBorder"),
		FLinearColor(0.08f, 0.12f, 0.08f, 0.85f),
		FMargin(0.f));

	if (Context.BackpackTitleText)
	{
		*Context.BackpackTitleText = CreateBackpackText(WidgetTree, TEXT("BackpackTitleText"), LOCTEXT("BackpackTitle", "[ 背包区 ]"), 16);
		if (UVerticalBoxSlot* Slot = BackpackVBox->AddChildToVerticalBox(Context.BackpackTitleText->Get()))
		{
			Slot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}
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
	if (Context.FluxContentDropTargetHost)
	{
		*Context.FluxContentDropTargetHost = AddZoneSectionWidget(
			Context.Owner,
			WidgetTree,
			FluxContentColumn,
			Context.FluxContentZoneSectionWidgetClass,
			LOCTEXT("FluxContentCardsTitle", "[ 通量内容 ]"),
			FMargin(0.f),
			&FluxContentSectionRaw);
	}
	if (Context.FluxContentZoneSection)
	{
		*Context.FluxContentZoneSection = FluxContentSectionRaw;
	}

	UScrollBox* BackpackInnerScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BackpackInnerScroll"));
	if (UVerticalBoxSlot* Slot = BackpackVBox->AddChildToVerticalBox(BackpackInnerScroll))
	{
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		Slot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}

	UVerticalBox* BackpackInnerVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BackpackInnerVBox"));
	BackpackInnerScroll->AddChild(BackpackInnerVBox);

	if (Context.SpecialZonesHost)
	{
		*Context.SpecialZonesHost = AddZoneSectionWidget(
			Context.Owner,
			WidgetTree,
			BackpackInnerVBox,
			Context.SpecialZonesSectionWidgetClass,
			LOCTEXT("SpecialZonesTitle", "[ 特殊存放区 ]"),
			FMargin(0.f, 0.f, 0.f, 8.f));
	}

	UWacomBackpackZoneSectionWidget* BurdenSectionRaw = nullptr;
	if (Context.BurdenZoneHost)
	{
		*Context.BurdenZoneHost = AddZoneSectionWidget(
			Context.Owner,
			WidgetTree,
			BackpackInnerVBox,
			Context.BurdenZoneSectionWidgetClass,
			LOCTEXT("BurdenZoneTitle", "[ 负重区 ] 0"),
			FMargin(0.f),
			&BurdenSectionRaw);
	}
	if (Context.BurdenZoneSection)
	{
		*Context.BurdenZoneSection = BurdenSectionRaw;
	}
}

#undef LOCTEXT_NAMESPACE
