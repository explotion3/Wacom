// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomKnockdownChoiceDialog.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Battle/WacomKnockdownChoiceOptionWidget.h"

#define LOCTEXT_NAMESPACE "WacomKnockdownChoiceDialog"

namespace
{
	UWacomKnockdownChoiceOptionWidget* AddFallbackOption(
		UWidgetTree* Tree,
		UHorizontalBox* Row,
		FName Name,
		float Width)
	{
		USizeBox* Sizer = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass());
		Sizer->SetWidthOverride(Width);
		Sizer->SetHeightOverride(470.0f);

		UWacomKnockdownChoiceOptionWidget* Option =
			Tree->ConstructWidget<UWacomKnockdownChoiceOptionWidget>(
				UWacomKnockdownChoiceOptionWidget::StaticClass(), Name);
		Sizer->SetContent(Option);
		if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Sizer))
		{
			Slot->SetPadding(FMargin(8.0f, 0.0f));
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
		return Option;
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

		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		WidgetTree->RootWidget = Root;

		UBorder* DimBackdrop = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("DimBackdrop"));
		DimBackdrop->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.70f));
		if (UOverlaySlot* BackdropSlot = Root->AddChildToOverlay(DimBackdrop))
		{
			BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
			BackdropSlot->SetVerticalAlignment(VAlign_Fill);
		}

		USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("PanelSize"));
		PanelSize->SetWidthOverride(1040.0f);
		PanelSize->SetHeightOverride(620.0f);
		if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(PanelSize))
		{
			PanelSlot->SetHorizontalAlignment(HAlign_Center);
			PanelSlot->SetVerticalAlignment(VAlign_Center);
		}

		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("Panel"));
		Panel->SetBrushColor(FLinearColor(0.025f, 0.032f, 0.052f, 0.99f));
		Panel->SetPadding(FMargin(28.0f, 22.0f));
		PanelSize->SetContent(Panel);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("ContentColumn"));
		Panel->SetContent(Column);

		TitleText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(LOCTEXT("FallbackTitle", "选择击倒结果"));
		TitleText->SetJustification(ETextJustify::Center);
		FSlateFontInfo TitleFont = TitleText->GetFont();
		TitleFont.Size = 28;
		TitleText->SetFont(TitleFont);
		if (UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		PartNameText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("PartNameText"));
		PartNameText->SetJustification(ETextJustify::Center);
		PartNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.77f, 0.88f)));
		if (UVerticalBoxSlot* PartNameSlot = Column->AddChildToVerticalBox(PartNameText))
		{
			PartNameSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 18.0f));
			PartNameSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		UHorizontalBox* OptionsRow = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("OptionsRow"));
		if (UVerticalBoxSlot* OptionsSlot = Column->AddChildToVerticalBox(OptionsRow))
		{
			OptionsSlot->SetHorizontalAlignment(HAlign_Center);
			OptionsSlot->SetVerticalAlignment(VAlign_Fill);
		}

		AidOption = AddFallbackOption(
			WidgetTree, OptionsRow, TEXT("AidOption"), 340.0f);
		WithdrawOption = AddFallbackOption(
			WidgetTree, OptionsRow, TEXT("WithdrawOption"), 236.0f);
		DestroyOption = AddFallbackOption(
			WidgetTree, OptionsRow, TEXT("DestroyOption"), 340.0f);
	}

	return Super::RebuildWidget();
}

void UWacomKnockdownChoiceDialog::NativeConstruct()
{
	Super::NativeConstruct();
	BindOptionDelegates();
	ApplyCurrentViewData();
}

void UWacomKnockdownChoiceDialog::NativeDestruct()
{
	ResetTransientState();
	Super::NativeDestruct();
}

void UWacomKnockdownChoiceDialog::NativeOnDeactivated()
{
	ResetTransientState();
	Super::NativeOnDeactivated();
}

void UWacomKnockdownChoiceDialog::Configure(
	const FWacomKnockdownChoiceDialogViewData& InViewData,
	FWacomKnockdownChoiceSubmitDelegate InSubmitDelegate)
{
	CurrentViewData = InViewData;
	SubmitDelegate = MoveTemp(InSubmitDelegate);
	bSubmitPending = false;
	BindOptionDelegates();
	ApplyCurrentViewData();
}

void UWacomKnockdownChoiceDialog::ApplyCurrentViewData()
{
	if (TitleText)
	{
		TitleText->SetText(CurrentViewData.TitleText);
	}
	if (PartNameText)
	{
		PartNameText->SetText(CurrentViewData.PartNameText);
	}
	if (AidOption)
	{
		AidOption->SetOptionViewData(CurrentViewData.AidOption);
	}
	if (WithdrawOption)
	{
		WithdrawOption->SetOptionViewData(CurrentViewData.WithdrawOption);
	}
	if (DestroyOption)
	{
		DestroyOption->SetOptionViewData(CurrentViewData.DestroyOption);
	}
}

void UWacomKnockdownChoiceDialog::BindOptionDelegates()
{
	UnbindOptionDelegates();
	if (AidOption)
	{
		AidOption->OnChoiceRequestedNative().AddUObject(
			this, &UWacomKnockdownChoiceDialog::HandleChoiceRequested);
	}
	if (WithdrawOption)
	{
		WithdrawOption->OnChoiceRequestedNative().AddUObject(
			this, &UWacomKnockdownChoiceDialog::HandleChoiceRequested);
	}
	if (DestroyOption)
	{
		DestroyOption->OnChoiceRequestedNative().AddUObject(
			this, &UWacomKnockdownChoiceDialog::HandleChoiceRequested);
	}
}

void UWacomKnockdownChoiceDialog::UnbindOptionDelegates()
{
	if (AidOption)
	{
		AidOption->OnChoiceRequestedNative().RemoveAll(this);
	}
	if (WithdrawOption)
	{
		WithdrawOption->OnChoiceRequestedNative().RemoveAll(this);
	}
	if (DestroyOption)
	{
		DestroyOption->OnChoiceRequestedNative().RemoveAll(this);
	}
}

const FWacomKnockdownChoiceOptionViewData*
UWacomKnockdownChoiceDialog::FindOptionViewData(EKnockdownChoice Choice) const
{
	switch (Choice)
	{
	case EKnockdownChoice::Aid:
		return &CurrentViewData.AidOption;
	case EKnockdownChoice::Withdraw:
		return &CurrentViewData.WithdrawOption;
	case EKnockdownChoice::Destroy:
		return &CurrentViewData.DestroyOption;
	case EKnockdownChoice::None:
	default:
		return nullptr;
	}
}

void UWacomKnockdownChoiceDialog::HandleChoiceRequested(
	EKnockdownChoice Choice)
{
	const FWacomKnockdownChoiceOptionViewData* OptionView =
		FindOptionViewData(Choice);
	if (bSubmitPending || !OptionView || !OptionView->bAvailable)
	{
		return;
	}

	bSubmitPending = true;
	SetAllOptionsInteractionEnabled(false);
	const bool bSubmitted = SubmitDelegate.IsBound()
		&& SubmitDelegate.Execute(Choice);
	if (bSubmitted)
	{
		DeactivateWidget();
		return;
	}

	bSubmitPending = false;
	ApplyCurrentViewData();
	if (SubmissionRejectedAnimation)
	{
		PlayAnimation(
			SubmissionRejectedAnimation,
			0.0f,
			1,
			EUMGSequencePlayMode::Forward,
			1.0f,
			/*bRestoreState*/true);
	}
	BP_OnChoiceSubmissionRejected(Choice);
}

void UWacomKnockdownChoiceDialog::SetAllOptionsInteractionEnabled(
	bool bEnabled)
{
	const auto ApplyEnabled = [bEnabled](
		UWacomKnockdownChoiceOptionWidget* Option)
	{
		if (!Option)
		{
			return;
		}
		Option->SetIsEnabled(bEnabled);
		Option->SetIsInteractionEnabled(bEnabled);
		Option->BP_OnInteractabilityChanged(bEnabled);
	};
	ApplyEnabled(AidOption);
	ApplyEnabled(WithdrawOption);
	ApplyEnabled(DestroyOption);
}

UWidget* UWacomKnockdownChoiceDialog::NativeGetDesiredFocusTarget() const
{
	const auto IsFocusableOption = [](const UWacomKnockdownChoiceOptionWidget* Option)
	{
		return Option
			&& Option->GetIsEnabled()
			&& Option->IsInteractionEnabled()
			&& Option->GetVisibility() != ESlateVisibility::Collapsed
			&& Option->GetVisibility() != ESlateVisibility::Hidden;
	};

	// 固定顺序：Aid -> Destroy -> Withdraw。Withdraw 永不作为正常默认项。
	if (IsFocusableOption(AidOption))
	{
		return AidOption;
	}
	if (IsFocusableOption(DestroyOption))
	{
		return DestroyOption;
	}
	return IsFocusableOption(WithdrawOption) ? WithdrawOption.Get() : nullptr;
}

FReply UWacomKnockdownChoiceDialog::NativeHandleBackRequested()
{
	// 击倒事件必须完成一个合法选择，Back / Escape / Gamepad B 只消费不关闭。
	return FReply::Handled();
}

void UWacomKnockdownChoiceDialog::ResetTransientState()
{
	UnbindOptionDelegates();
	SubmitDelegate.Unbind();
	bSubmitPending = false;
	CurrentViewData = FWacomKnockdownChoiceDialogViewData();
}

#undef LOCTEXT_NAMESPACE
