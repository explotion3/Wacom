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
#include "UI/Backpack/WacomDeleteZoneDropTarget.h"
#include "UI/Backpack/WacomZoneDropTarget.h"

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
	auto& FluxContentDropTargetHost = *Context.FluxContentDropTargetHost;
	auto& SpecialZonesHost = *Context.SpecialZonesHost;
	auto& BurdenZoneHost = *Context.BurdenZoneHost;
	auto& DeleteZoneTitleText = *Context.DeleteZoneTitleText;
	auto& BurdenZoneTitleText = *Context.BurdenZoneTitleText;
	auto& BattleDeckCardsBox = *Context.BattleDeckCardsBox;
	auto& FluxContentCardsBox = *Context.FluxContentCardsBox;
	auto& SpecialZonesPanel = *Context.SpecialZonesPanel;
	auto& BurdenCardsBox = *Context.BurdenCardsBox;
	auto& DeleteDropTarget = *Context.DeleteDropTarget;
	auto& BattleDeckDropTarget = *Context.BattleDeckDropTarget;
	auto& BackpackDropTarget = *Context.BackpackDropTarget;

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
		DeleteDropTarget->SetOwnerScreen(Context.OwnerScreen);

		if (!DeleteZoneTitleText)
		{
			DeleteZoneTitleText = FBackpackFallbackLayoutBuilder::CreateBackpackText(
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
		BattleDeckDropTarget->SetOwnerScreen(Context.OwnerScreen);

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
		BackpackDropTarget->SetOwnerScreen(Context.OwnerScreen);

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
		BurdenZoneTitleText->RemoveFromParent();
		BurdenContent->AddChildToVerticalBox(BurdenZoneTitleText);

		BurdenCardsBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("BurdenCardsBox"));
		BurdenCardsBox->SetInnerSlotPadding(FVector2D(8.f, 8.f));
		USizeBox* BurdenSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BurdenCardContent"));
		BurdenSize->SetMinDesiredHeight(140.f);
		BurdenSize->AddChild(BurdenCardsBox);
		BurdenContent->AddChildToVerticalBox(BurdenSize);
		BurdenZoneHost->AddChild(BurdenContent);
	}
	else if (!BurdenZoneHost)
	{
		LogBackpackBindingWarningOnce(TEXT("BurdenZoneHost"), TEXT("[Backpack] BurdenZoneHost 未绑定，负重区运行时内容不会显示"));
	}
}

#undef LOCTEXT_NAMESPACE
