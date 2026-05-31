// Copyright Wacom. All Rights Reserved.

#include "UI/Events/WacomRunEventChoiceButton.h"

#define LOCTEXT_NAMESPACE "WacomRunEventChoiceButton"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

namespace
{
	UTextBlock* MakeEventChoiceText(UWidgetTree* Tree, FName Name, const FText& Text, int32 FontSize)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Block->SetText(Text);
		FSlateFontInfo Font = Block->GetFont();
		Font.Size = FontSize;
		Block->SetFont(Font);
		Block->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.f)));
		return Block;
	}
}

TSharedRef<SWidget> UWacomRunEventChoiceButton::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		ChoiceButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ChoiceButton"));
		WidgetTree->RootWidget = ChoiceButton;

		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
		ChoiceButton->AddChild(RootBox);

		LabelText = MakeEventChoiceText(WidgetTree, TEXT("LabelText"), LOCTEXT("Choice", "选项"), 18);
		RootBox->AddChildToVerticalBox(LabelText);

		PaymentStatusText = MakeEventChoiceText(WidgetTree, TEXT("PaymentStatusText"), FText::GetEmpty(), 14);
		PaymentStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.58f, 0.82f, 0.92f, 1.f)));
		RootBox->AddChildToVerticalBox(PaymentStatusText);

		DisabledReasonText = MakeEventChoiceText(WidgetTree, TEXT("DisabledReasonText"), FText::GetEmpty(), 14);
		DisabledReasonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.55f, 0.45f, 1.f)));
		RootBox->AddChildToVerticalBox(DisabledReasonText);
	}
	return Super::RebuildWidget();
}

void UWacomRunEventChoiceButton::NativeConstruct()
{
	Super::NativeConstruct();
	bHasConstructed = true;
	if (ChoiceButton)
	{
		ChoiceButton->OnClicked.AddUniqueDynamic(this, &UWacomRunEventChoiceButton::HandleClicked);
	}
	RefreshVisuals();
	if (bNeedsSnapshotAppliedNotifyAfterConstruct)
	{
		NotifyChoiceSnapshotApplied();
	}
}

void UWacomRunEventChoiceButton::SetChoiceSnapshot(const FRunEventChoiceSnapshot& InChoice)
{
	ChoiceSnapshot = InChoice;
	RequirementView = UWacomRunEventPresentationBuilder::BuildChoiceRequirementView(ChoiceSnapshot);
	bHasAppliedChoiceSnapshot = true;
	RefreshVisuals();
	NotifyChoiceSnapshotApplied();
}

#if WITH_AUTOMATION_TESTS
FText UWacomRunEventChoiceButton::GetDisplayedPaymentStatusTextForTest() const
{
	return PaymentStatusText ? PaymentStatusText->GetText() : FText::GetEmpty();
}

ESlateVisibility UWacomRunEventChoiceButton::GetPaymentStatusVisibilityForTest() const
{
	return PaymentStatusText ? PaymentStatusText->GetVisibility() : ESlateVisibility::Collapsed;
}

FText UWacomRunEventChoiceButton::GetDisplayedDisabledReasonTextForTest() const
{
	return DisabledReasonText ? DisabledReasonText->GetText() : FText::GetEmpty();
}

ESlateVisibility UWacomRunEventChoiceButton::GetDisabledReasonVisibilityForTest() const
{
	return DisabledReasonText ? DisabledReasonText->GetVisibility() : ESlateVisibility::Collapsed;
}
#endif

void UWacomRunEventChoiceButton::HandleClicked()
{
	OnChoiceClickedNative.Broadcast(ChoiceSnapshot.ChoiceId);
}

void UWacomRunEventChoiceButton::BP_OnRunEventChoiceSnapshotApplied_Implementation(
	const FRunEventChoiceSnapshot& AppliedChoiceSnapshot)
{
}

void UWacomRunEventChoiceButton::NotifyChoiceSnapshotApplied()
{
	if (!bHasAppliedChoiceSnapshot)
	{
		return;
	}
	if (!bHasConstructed)
	{
		bNeedsSnapshotAppliedNotifyAfterConstruct = true;
		return;
	}

	bNeedsSnapshotAppliedNotifyAfterConstruct = false;
	BP_OnRunEventChoiceSnapshotApplied(ChoiceSnapshot);
}

void UWacomRunEventChoiceButton::RefreshVisuals()
{
	if (LabelText)
	{
		LabelText->SetText(ChoiceSnapshot.LabelText.IsEmpty()
			? FText::FromName(ChoiceSnapshot.ChoiceId)
			: ChoiceSnapshot.LabelText);
	}
	if (ChoiceButton)
	{
		ChoiceButton->SetIsEnabled(true);
		ChoiceButton->SetRenderOpacity(ChoiceSnapshot.bAvailable ? 1.f : 0.62f);
	}
	if (PaymentStatusText)
	{
		PaymentStatusText->SetText(RequirementView.RequirementText);
		PaymentStatusText->SetColorAndOpacity(RequirementView.PaymentCandidateCount > 0
			? FSlateColor(FLinearColor(0.58f, 0.82f, 0.92f, 1.f))
			: FSlateColor(FLinearColor(0.9f, 0.55f, 0.45f, 1.f)));
		PaymentStatusText->SetVisibility(RequirementView.bRequiresCardPayment
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (DisabledReasonText)
	{
		DisabledReasonText->SetText(RequirementView.BlockedReasonText);
		DisabledReasonText->SetVisibility(RequirementView.BlockedReasonText.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}
}

#undef LOCTEXT_NAMESPACE
