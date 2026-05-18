// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomDeckCardWidget.h"

#define LOCTEXT_NAMESPACE "WacomDeckCard"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Cards/CardDefinition.h"
#include "UI/Backpack/WacomCardDragOperation.h"

namespace
{
	/** 卡片固定尺寸。让 WrapBox 自动换行。 */
	constexpr float CardWidth  = 160.f;
	constexpr float CardHeight = 210.f;
}

TSharedRef<SWidget> UWacomDeckCardWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		// Root = SizeBox 强制固定尺寸，否则 WrapBox 里的卡片塌缩成 0x0 全部重叠
		USizeBox* SizeRoot = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeRoot"));
		SizeRoot->SetWidthOverride(CardWidth);
		SizeRoot->SetHeightOverride(CardHeight);
		WidgetTree->RootWidget = SizeRoot;

		// 卡片本体：CanvasPanel（让 MoveButton 占满，DeleteButton 锚到右上）
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CardCanvas"));
		SizeRoot->AddChild(Root);

		// 卡片主体（占满）。不能用 UButton，否则按钮会吃掉 MouseDown，导致 UserWidget 拿不到拖拽起点。
		if (!CardBody)
		{
			CardBody = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardBody"));
			CardBody->SetBrushColor(FLinearColor(0.70f, 0.70f, 0.70f, 1.f));
			CardBody->SetPadding(FMargin(0.f));
			if (UCanvasPanelSlot* BodySlot = Root->AddChildToCanvas(CardBody))
			{
				BodySlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
				BodySlot->SetOffsets(FMargin(0.f));
			}

			// 卡片内容：VerticalBox（Cost / Name / Keywords / Capacity）
			UVerticalBox* InnerVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InnerVBox"));
			CardBody->AddChild(InnerVBox);

			// Cost 行
			if (!CostText)
			{
				CostText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CostText"));
				CostText->SetText(LOCTEXT("CostPlaceholder", "Cost: 0"));
				FSlateFontInfo Font = CostText->GetFont();
				Font.Size = 11;
				CostText->SetFont(Font);
				if (UVerticalBoxSlot* SubSlot = InnerVBox->AddChildToVerticalBox(CostText))
				{
					SubSlot->SetPadding(FMargin(6.f, 6.f, 24.f, 0.f)); // 右侧给 X 按钮让位
				}
			}

			// 名字
			if (!NameText)
			{
				NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
				NameText->SetText(LOCTEXT("NamePlaceholder", "Card"));
				NameText->SetJustification(ETextJustify::Center);
				NameText->SetAutoWrapText(true);
				FSlateFontInfo Font = NameText->GetFont();
				Font.Size = 13;
				NameText->SetFont(Font);
				if (UVerticalBoxSlot* SubSlot = InnerVBox->AddChildToVerticalBox(NameText))
				{
					SubSlot->SetPadding(FMargin(6.f, 8.f, 6.f, 8.f));
					SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				}
			}

			// 关键词
			if (!KeywordsText)
			{
				KeywordsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("KeywordsText"));
				KeywordsText->SetText(FText::GetEmpty());
				KeywordsText->SetAutoWrapText(true);
				FSlateFontInfo Font = KeywordsText->GetFont();
				Font.Size = 9;
				KeywordsText->SetFont(Font);
				if (UVerticalBoxSlot* SubSlot = InnerVBox->AddChildToVerticalBox(KeywordsText))
				{
					SubSlot->SetPadding(FMargin(6.f, 0.f, 6.f, 0.f));
				}
			}

			// Capacity（仅容器卡显示）
			if (!CapacityText)
			{
				CapacityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CapacityText"));
				CapacityText->SetText(FText::GetEmpty());
				FSlateFontInfo Font = CapacityText->GetFont();
				Font.Size = 9;
				CapacityText->SetFont(Font);
				if (UVerticalBoxSlot* SubSlot = InnerVBox->AddChildToVerticalBox(CapacityText))
				{
					SubSlot->SetPadding(FMargin(6.f, 0.f, 6.f, 6.f));
				}
			}
		}

		// 删除按钮：右上角小 X
		if (!DeleteButton)
		{
			DeleteButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DeleteButton"));
			UTextBlock* XText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			XText->SetText(LOCTEXT("DeleteX", "X"));
			XText->SetJustification(ETextJustify::Center);
			DeleteButton->AddChild(XText);

			if (UCanvasPanelSlot* DelSlot = Root->AddChildToCanvas(DeleteButton))
			{
				// 右上角，固定 20x20
				DelSlot->SetAnchors(FAnchors(1.f, 0.f));
				DelSlot->SetAlignment(FVector2D(1.f, 0.f));
				DelSlot->SetOffsets(FMargin(-2.f, 2.f, 20.f, 20.f));
				DelSlot->SetAutoSize(false);
			}
		}

		if (!BattleEnabledBadge)
		{
			BattleEnabledBadge = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BattleEnabledBadge"));
			BattleEnabledBadge->SetText(LOCTEXT("BattleEnabledBadge", "已选"));
			BattleEnabledBadge->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.9f, 0.45f, 1.f)));
			FSlateFontInfo BadgeFont = BattleEnabledBadge->GetFont();
			BadgeFont.Size = 11;
			BattleEnabledBadge->SetFont(BadgeFont);
			BattleEnabledBadge->SetVisibility(bBattleEnabledBadgeVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			if (UCanvasPanelSlot* BadgeSlot = Root->AddChildToCanvas(BattleEnabledBadge))
			{
				BadgeSlot->SetAnchors(FAnchors(0.f, 0.f));
				BadgeSlot->SetAlignment(FVector2D(0.f, 0.f));
				BadgeSlot->SetOffsets(FMargin(6.f, 4.f, 44.f, 18.f));
				BadgeSlot->SetAutoSize(false);
			}
		}

		if (!ProjectedFromBadge)
		{
			ProjectedFromBadge = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ProjectedFromBadge"));
			ProjectedFromBadge->SetText(ProjectedFromBadgeText);
			ProjectedFromBadge->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.8f, 1.f, 1.f)));
			FSlateFontInfo FromFont = ProjectedFromBadge->GetFont();
			FromFont.Size = 10;
			ProjectedFromBadge->SetFont(FromFont);
			ProjectedFromBadge->SetVisibility(ProjectedFromBadgeText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
			if (UCanvasPanelSlot* FromSlot = Root->AddChildToCanvas(ProjectedFromBadge))
			{
				FromSlot->SetAnchors(FAnchors(0.f, 1.f));
				FromSlot->SetAlignment(FVector2D(0.f, 1.f));
				FromSlot->SetOffsets(FMargin(6.f, -24.f, 120.f, 18.f));
				FromSlot->SetAutoSize(false);
			}
		}

		// 整体卡片尺寸由父容器（WrapBox / Slot）决定。这里在容器侧设置 size。
	}
	return Super::RebuildWidget();
}

void UWacomDeckCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (DeleteButton)
	{
		DeleteButton->OnClicked.AddUniqueDynamic(this, &UWacomDeckCardWidget::HandleDeleteClicked);
	}

	RefreshContentFromCard();
}

void UWacomDeckCardWidget::SetCard(const FCardInstance& Inst, EZoneKind InFromZone, FGuid InFromZoneOwnerInstanceId)
{
	Card = Inst.Definition;
	InstanceId = Inst.InstanceId;
	FromZone = InFromZone;
	FromZoneOwnerInstanceId = (FromZone == EZoneKind::SpecialZone) ? InFromZoneOwnerInstanceId : FGuid();
	SetBattleEnabledBadgeVisible(FromZone == EZoneKind::SpecialZone && Inst.bBattleEnabledInSpecialZone);
	RefreshContentFromCard();
}

void UWacomDeckCardWidget::SetMoveEnabled(bool bEnabled)
{
	if (CardBody) { CardBody->SetIsEnabled(bEnabled); }
}

void UWacomDeckCardWidget::SetDeleteEnabled(bool bEnabled)
{
	if (DeleteButton) { DeleteButton->SetIsEnabled(bEnabled); }
}

void UWacomDeckCardWidget::SetBattleEnabledBadgeVisible(bool bVisible)
{
	bBattleEnabledBadgeVisible = bVisible;
	if (BattleEnabledBadge)
	{
		BattleEnabledBadge->SetVisibility(bBattleEnabledBadgeVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UWacomDeckCardWidget::SetProjectedFromBadgeText(const FText& InText)
{
	ProjectedFromBadgeText = InText;
	if (ProjectedFromBadge)
	{
		ProjectedFromBadge->SetText(ProjectedFromBadgeText);
		ProjectedFromBadge->SetVisibility(ProjectedFromBadgeText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UWacomDeckCardWidget::SetRightClickToggleEnabled(bool bEnabled)
{
	bRightClickToggleEnabled = bEnabled;
}

void UWacomDeckCardWidget::RefreshContentFromCard()
{
	if (!Card)
	{
		if (NameText)     { NameText->SetText(LOCTEXT("EmptyCard", "(none)")); }
		if (CostText)     { CostText->SetText(FText::GetEmpty()); }
		if (KeywordsText) { KeywordsText->SetText(FText::GetEmpty()); }
		if (CapacityText) { CapacityText->SetText(FText::GetEmpty()); }
		return;
	}

	if (NameText)
	{
		NameText->SetText(Card->DisplayName);
	}
	if (CostText)
	{
		CostText->SetText(FText::Format(LOCTEXT("CostFmt", "Cost: {0}"), FText::AsNumber(Card->BaseCost)));
	}
	if (KeywordsText)
	{
		// 简单展示首个关键词的最后一段（如 Card.Keyword.Companion → "Companion"）
		FString Joined;
		for (const FGameplayTag& Tag : Card->Keywords)
		{
			FString TagName = Tag.GetTagName().ToString();
			int32 LastDot = INDEX_NONE;
			TagName.FindLastChar(TEXT('.'), LastDot);
			const FString Short = (LastDot != INDEX_NONE) ? TagName.Mid(LastDot + 1) : TagName;
			if (!Joined.IsEmpty()) { Joined.Append(TEXT(" / ")); }
			Joined.Append(Short);
		}
		KeywordsText->SetText(FText::FromString(Joined));
	}
	if (CapacityText)
	{
		if (Card->Physique.Capacity > 0)
		{
			CapacityText->SetText(FText::Format(
				LOCTEXT("CapacityFmt", "Capacity: {0}"),
				FText::AsNumber(Card->Physique.Capacity)));
		}
		else
		{
			CapacityText->SetText(FText::GetEmpty());
		}
	}
}

FReply UWacomDeckCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && InstanceId.IsValid())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && RequestBattleEnabledToggle())
	{
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UWacomDeckCardWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	OutOperation = BuildDragOperation();
}

UDragDropOperation* UWacomDeckCardWidget::BuildDragOperation()
{
	if (!Card || !InstanceId.IsValid())
	{
		return nullptr;
	}

	UWacomCardDragOperation* DragOp = NewObject<UWacomCardDragOperation>(this);
	DragOp->InstanceId = InstanceId;
	DragOp->FromZone = FromZone;
	DragOp->FromZoneOwnerInstanceId = (FromZone == EZoneKind::SpecialZone) ? FromZoneOwnerInstanceId : FGuid();
	DragOp->Definition = Card;
	DragOp->DefaultDragVisual = this;
	DragOp->Pivot = EDragPivot::MouseDown;
	return DragOp;
}

bool UWacomDeckCardWidget::HasMoveButtonClickBindings() const
{
	return false;
}

bool UWacomDeckCardWidget::IsBattleEnabledBadgeVisible() const
{
	return BattleEnabledBadge ? BattleEnabledBadge->GetVisibility() != ESlateVisibility::Collapsed : bBattleEnabledBadgeVisible;
}

bool UWacomDeckCardWidget::IsProjectedFromBadgeVisible() const
{
	return ProjectedFromBadge ? ProjectedFromBadge->GetVisibility() != ESlateVisibility::Collapsed : !ProjectedFromBadgeText.IsEmpty();
}

FText UWacomDeckCardWidget::GetProjectedFromBadgeText() const
{
	return ProjectedFromBadge ? ProjectedFromBadge->GetText() : ProjectedFromBadgeText;
}

bool UWacomDeckCardWidget::RequestBattleEnabledToggle()
{
	if (!InstanceId.IsValid() || !bRightClickToggleEnabled)
	{
		return false;
	}

	OnBattleEnabledToggleRequestedNative.Broadcast(InstanceId);
	return true;
}

void UWacomDeckCardWidget::HandleDeleteClicked()
{
	if (Card)
	{
		OnDeleteRequestedNative.Broadcast(Card);
	}
}

#undef LOCTEXT_NAMESPACE
