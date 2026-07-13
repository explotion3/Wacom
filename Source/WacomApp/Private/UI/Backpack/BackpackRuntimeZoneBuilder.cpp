// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/BackpackRuntimeZoneBuilder.h"

#include "UI/Backpack/BackpackFallbackLayoutBuilder.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "UI/Backpack/WacomBackpackScreen.h"

#define LOCTEXT_NAMESPACE "WacomBackpack"

namespace
{
void LogBackpackBindingWarningOnce(FName Key, const TCHAR* Message)
{
	static TSet<FName> LoggedKeys;
	if (!LoggedKeys.Contains(Key))
	{
		LoggedKeys.Add(Key);
		UE_LOG(LogTemp, Warning, TEXT("%s"), Message);
	}
}

UWrapBox* EnsureReadOnlyCardBox(
	UWidgetTree& WidgetTree,
	UPanelWidget* Host,
	TObjectPtr<UWrapBox>& CardBox,
	FName BoxName,
	FName SizeName,
	float MinimumHeight)
{
	if (CardBox || !Host)
	{
		return CardBox;
	}

	Host->ClearChildren();
	CardBox = WidgetTree.ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), BoxName);
	CardBox->SetInnerSlotPadding(FVector2D(8.f, 8.f));
	USizeBox* SizeBox = WidgetTree.ConstructWidget<USizeBox>(USizeBox::StaticClass(), SizeName);
	SizeBox->SetMinDesiredHeight(MinimumHeight);
	SizeBox->AddChild(CardBox);
	Host->AddChild(SizeBox);
	return CardBox;
}
}

void FBackpackRuntimeZoneBuilder::Ensure(const FBackpackRuntimeZoneBuilderContext& Context)
{
	UWidgetTree* WidgetTree = Context.WidgetTree;
	if (!Context.OwnerScreen || !WidgetTree)
	{
		return;
	}

	auto& CardDetailLayer = *Context.CardDetailLayer;
	auto& DeleteZoneHost = *Context.DeleteZoneHost;
	auto& BattleDeckZoneHost = *Context.BattleDeckZoneHost;
	auto& FluxContentHost = *Context.FluxContentDropTargetHost;
	auto& SpecialZonesHost = *Context.SpecialZonesHost;
	auto& BurdenZoneHost = *Context.BurdenZoneHost;
	auto& DeleteZoneTitleText = *Context.DeleteZoneTitleText;
	auto& BurdenZoneTitleText = *Context.BurdenZoneTitleText;
	auto& BattleDeckCardsBox = *Context.BattleDeckCardsBox;
	auto& FluxContentCardsBox = *Context.FluxContentCardsBox;
	auto& SpecialZonesPanel = *Context.SpecialZonesPanel;
	auto& BurdenCardsBox = *Context.BurdenCardsBox;

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

	if (DeleteZoneHost && DeleteZoneHost->GetChildrenCount() == 0)
	{
		if (!DeleteZoneTitleText)
		{
			DeleteZoneTitleText = FBackpackFallbackLayoutBuilder::CreateBackpackText(
				WidgetTree,
				TEXT("DeleteZoneTitleText"),
				LOCTEXT("DeleteZoneHint", "携带卡牌到销毁牌匣以置换金币"),
				14);
		}
		DeleteZoneHost->AddChild(DeleteZoneTitleText);
	}

	EnsureReadOnlyCardBox(*WidgetTree, BattleDeckZoneHost, BattleDeckCardsBox,
		TEXT("BattleDeckCardsBox"), TEXT("BattleDeckReadOnlyContent"), 220.f);
	EnsureReadOnlyCardBox(*WidgetTree, FluxContentHost, FluxContentCardsBox,
		TEXT("FluxContentCardsBox_Runtime"), TEXT("BackpackReadOnlyContent"), 220.f);

	if (!SpecialZonesPanel && SpecialZonesHost)
	{
		SpecialZonesHost->ClearChildren();
		SpecialZonesPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SpecialZonesPanel"));
		SpecialZonesHost->AddChild(SpecialZonesPanel);
	}

	if (!BurdenCardsBox && BurdenZoneHost)
	{
		BurdenZoneHost->ClearChildren();
		UVerticalBox* BurdenContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BurdenContent"));
		if (!BurdenZoneTitleText)
		{
			BurdenZoneTitleText = FBackpackFallbackLayoutBuilder::CreateBackpackText(
				WidgetTree,
				TEXT("BurdenZoneTitleText"),
				LOCTEXT("BurdenZoneTitle", "[ 负重区 ] 0"),
				15);
		}
		BurdenContent->AddChildToVerticalBox(BurdenZoneTitleText);
		BurdenCardsBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("BurdenCardsBox"));
		BurdenCardsBox->SetInnerSlotPadding(FVector2D(8.f, 8.f));
		USizeBox* BurdenSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BurdenReadOnlyContent"));
		BurdenSize->SetMinDesiredHeight(140.f);
		BurdenSize->AddChild(BurdenCardsBox);
		BurdenContent->AddChildToVerticalBox(BurdenSize);
		BurdenZoneHost->AddChild(BurdenContent);
	}
}

#undef LOCTEXT_NAMESPACE
