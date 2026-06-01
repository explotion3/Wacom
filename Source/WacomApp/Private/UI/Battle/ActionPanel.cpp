// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/ActionPanel.h"
#include "UI/Battle/BattleHUD.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Snapshots/BattleSnapshot.h"

namespace
{
	UBattleHUD* ResolveOwningHUD(const UUserWidget* Widget)
	{
		for (UUserWidget* P = Widget ? Widget->GetTypedOuter<UUserWidget>() : nullptr;
			P;
			P = P->GetTypedOuter<UUserWidget>())
		{
			if (UBattleHUD* HUD = Cast<UBattleHUD>(P))
			{
				return HUD;
			}
		}
		return nullptr;
	}

	UButton* MakeLabeledButton(UWidgetTree* Tree, const FName& BtnName, const FName& LabelName,
	                           const FString& LabelText, FLinearColor BgColor,
	                           TObjectPtr<UTextBlock>& OutLabel)
	{
		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass(), BtnName);
		FButtonStyle Bs = Btn->GetStyle();
		Bs.Normal.TintColor   = FSlateColor(BgColor);
		Bs.Hovered.TintColor  = FSlateColor(BgColor * 1.3f);
		Bs.Pressed.TintColor  = FSlateColor(BgColor * 0.7f);
		Bs.Disabled.TintColor = FSlateColor(BgColor * 0.4f);
		Btn->SetStyle(Bs);

		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
		Label->SetText(FText::FromString(LabelText));
		Label->SetJustification(ETextJustify::Center);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));

		Btn->AddChild(Label);
		OutLabel = Label;
		return Btn;
	}
}

TSharedRef<SWidget> UActionPanel::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// WaitValueText
		WaitValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WaitValueText"));
		WaitValueText->SetText(FText::FromString(TEXT("Wait Value: 2")));
		WaitValueText->SetJustification(ETextJustify::Center);
		WaitValueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f)));
		if (UVerticalBoxSlot* S = Root->AddChildToVerticalBox(WaitValueText))
		{
			S->SetPadding(FMargin(0, 0, 0, 6));
		}

		// Wait button
		USizeBox* WaitBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WaitBox"));
		WaitBox->SetWidthOverride(140.0f);
		WaitBox->SetHeightOverride(44.0f);
		WaitButton = MakeLabeledButton(WidgetTree, TEXT("WaitButton"), TEXT("WaitLabel"),
			TEXT("Wait"), FLinearColor(0.2f, 0.3f, 0.55f), WaitLabel);
		WaitBox->AddChild(WaitButton);
		if (UVerticalBoxSlot* S = Root->AddChildToVerticalBox(WaitBox))
		{
			S->SetPadding(FMargin(0, 0, 0, 6));
		}

		// End Turn button
		USizeBox* EndBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EndTurnBox"));
		EndBox->SetWidthOverride(140.0f);
		EndBox->SetHeightOverride(44.0f);
		EndTurnButton = MakeLabeledButton(WidgetTree, TEXT("EndTurnButton"), TEXT("EndTurnLabel"),
			TEXT("End Turn"), FLinearColor(0.55f, 0.25f, 0.20f), EndTurnLabel);
		EndBox->AddChild(EndTurnButton);
		Root->AddChildToVerticalBox(EndBox);
	}
	return Super::RebuildWidget();
}

void UActionPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// Button 绑定移到 NativeConstruct（RebuildWidget 之后，BindWidget 字段已就位）。
}

void UActionPanel::NativeConstruct()
{
	Super::NativeConstruct();
	if (WaitButton && !WaitButton->OnClicked.IsBound())
	{
		WaitButton->OnClicked.AddDynamic(this, &UActionPanel::HandleWaitClicked);
	}
	if (EndTurnButton && !EndTurnButton->OnClicked.IsBound())
	{
		EndTurnButton->OnClicked.AddDynamic(this, &UActionPanel::HandleEndTurnClicked);
	}
}

void UActionPanel::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	UBattleHUD* HUD = ResolveOwningHUD(this);
	if (WaitValueText)
	{
		WaitValueText->SetText(HUD && HUD->HasPendingTurnBoundaryCommand()
			? HUD->GetPendingTurnBoundaryCommandText()
			: FText::Format(
				FText::FromString(TEXT("Wait Value: {0}")),
				FFormatOrderedArguments{ FFormatArgumentValue(Snap.CurrentWaitValue) }));
	}
	UpdateButtonEnabledState();
}

void UActionPanel::UpdateButtonEnabledState()
{
	UBattleHUD* HUD = ResolveOwningHUD(this);
	const bool bCanAct = HUD && HUD->CanSubmitPlayerActionCommand();

	if (WaitButton)    { WaitButton->SetIsEnabled(bCanAct); }
	if (EndTurnButton) { EndTurnButton->SetIsEnabled(bCanAct); }
}

void UActionPanel::HandleWaitClicked()
{
	if (UBattleHUD* HUD = ResolveOwningHUD(this))
	{
		HUD->OnWaitRequested();
	}
}

void UActionPanel::HandleEndTurnClicked()
{
	if (UBattleHUD* HUD = ResolveOwningHUD(this))
	{
		HUD->OnEndTurnRequested();
	}
}
