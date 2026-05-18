// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomKnockdownChoiceDialog.h"

#define LOCTEXT_NAMESPACE "WacomKnockdownChoice"

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

#include "Commands/BattleCommand.h"
#include "Session/BattleSession.h"
#include "UI/Battle/BattleHUD.h"
#include "Types/WacomEnums.h"

namespace
{
	UButton* MakeChoiceButton(UWidgetTree* Tree, FName Name, const FText& Label,
		UHorizontalBox* Parent, float MinWidth)
	{
		USizeBox* Sizer = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Sizer->SetMinDesiredWidth(MinWidth);
		Sizer->SetMinDesiredHeight(48.f);

		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(Label);
		Text->SetJustification(ETextJustify::Center);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = 16;
		Text->SetFont(Font);
		Btn->AddChild(Text);
		Sizer->AddChild(Btn);

		if (UHorizontalBoxSlot* S = Parent->AddChildToHorizontalBox(Sizer))
		{
			S->SetPadding(FMargin(8.f, 0.f));
			S->SetVerticalAlignment(VAlign_Center);
		}
		return Btn;
	}
}

TSharedRef<SWidget> UWacomKnockdownChoiceDialog::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// 半透明背景
		UBorder* DimBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DimBg"));
		DimBg->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
		DimBg->SetPadding(FMargin(0.f));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(DimBg))
		{
			S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			S->SetOffsets(FMargin(0.f));
		}

		// 居中面板
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
		Panel->SetBrushColor(FLinearColor(0.04f, 0.05f, 0.08f, 0.95f));
		Panel->SetPadding(FMargin(28.f, 20.f));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Panel))
		{
			S->SetAnchors(FAnchors(0.5f, 0.5f));
			S->SetAlignment(FVector2D(0.5f, 0.5f));
			S->SetOffsets(FMargin(-260.f, -90.f, 520.f, 180.f));
			S->SetAutoSize(false);
		}

		UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
		Panel->AddChild(VBox);

		if (!TitleText)
		{
			TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
			TitleText->SetText(LOCTEXT("Title", "击倒"));
			TitleText->SetJustification(ETextJustify::Center);
			FSlateFontInfo Font = TitleText->GetFont();
			Font.Size = 24;
			TitleText->SetFont(Font);
			if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(TitleText))
			{
				S->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
				S->SetHorizontalAlignment(HAlign_Center);
			}
		}

		if (!PartNameText)
		{
			PartNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PartNameText"));
			PartNameText->SetText(LOCTEXT("PartNameInit", "部位"));
			PartNameText->SetJustification(ETextJustify::Center);
			FSlateFontInfo Font = PartNameText->GetFont();
			Font.Size = 14;
			PartNameText->SetFont(Font);
			PartNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f)));
			if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(PartNameText))
			{
				S->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
				S->SetHorizontalAlignment(HAlign_Center);
			}
		}

		// 三个按钮（援助 / 撤离 / 破坏）
		UHorizontalBox* BtnRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BtnRow"));
		if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(BtnRow))
		{
			S->SetHorizontalAlignment(HAlign_Center);
		}

		if (!AidButton)
		{
			AidButton = MakeChoiceButton(WidgetTree, TEXT("AidButton"), LOCTEXT("Aid", "援助（左手）"), BtnRow, 140.f);
		}
		if (!WithdrawButton)
		{
			WithdrawButton = MakeChoiceButton(WidgetTree, TEXT("WithdrawButton"), LOCTEXT("Withdraw", "撤离"), BtnRow, 100.f);
		}
		if (!DestroyButton)
		{
			DestroyButton = MakeChoiceButton(WidgetTree, TEXT("DestroyButton"), LOCTEXT("Destroy", "破坏（右手）"), BtnRow, 140.f);
		}
	}
	return Super::RebuildWidget();
}

void UWacomKnockdownChoiceDialog::NativeConstruct()
{
	Super::NativeConstruct();

	if (AidButton)      { AidButton     ->OnClicked.AddUniqueDynamic(this, &UWacomKnockdownChoiceDialog::HandleAidClicked); }
	if (WithdrawButton) { WithdrawButton->OnClicked.AddUniqueDynamic(this, &UWacomKnockdownChoiceDialog::HandleWithdrawClicked); }
	if (DestroyButton)  { DestroyButton ->OnClicked.AddUniqueDynamic(this, &UWacomKnockdownChoiceDialog::HandleDestroyClicked); }

	// 应用 SetContext 传入的状态
	if (PartNameText && !PendingPartName.IsEmpty())
	{
		PartNameText->SetText(PendingPartName);
	}
	if (AidButton)     { AidButton    ->SetIsEnabled(bLeftHandAvailable); }
	if (DestroyButton) { DestroyButton->SetIsEnabled(bRightHandAvailable); }
}

void UWacomKnockdownChoiceDialog::SetContext(UBattleHUD* InHUD, const FText& InPartName,
	bool bInLeftHandAvailable, bool bInRightHandAvailable)
{
	OwningHUD = InHUD;
	PendingPartName = InPartName;
	bLeftHandAvailable = bInLeftHandAvailable;
	bRightHandAvailable = bInRightHandAvailable;

	// 即时应用（如果 NativeConstruct 已经跑过）
	if (PartNameText) { PartNameText->SetText(PendingPartName); }
	if (AidButton)    { AidButton->SetIsEnabled(bLeftHandAvailable); }
	if (DestroyButton){ DestroyButton->SetIsEnabled(bRightHandAvailable); }
}

void UWacomKnockdownChoiceDialog::HandleAidClicked()
{
	if (OwningHUD)
	{
		OwningHUD->OnKnockdownChoiceSelected(EKnockdownChoice::Aid);
	}
	DeactivateWidget();
}

void UWacomKnockdownChoiceDialog::HandleWithdrawClicked()
{
	if (OwningHUD)
	{
		OwningHUD->OnKnockdownChoiceSelected(EKnockdownChoice::Withdraw);
	}
	DeactivateWidget();
}

void UWacomKnockdownChoiceDialog::HandleDestroyClicked()
{
	if (OwningHUD)
	{
		OwningHUD->OnKnockdownChoiceSelected(EKnockdownChoice::Destroy);
	}
	DeactivateWidget();
}

FReply UWacomKnockdownChoiceDialog::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// GDD：必须三选一，ESC 不允许关闭。
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		return FReply::Handled();  // 吃掉 ESC，不调父类 DeactivateWidget
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
