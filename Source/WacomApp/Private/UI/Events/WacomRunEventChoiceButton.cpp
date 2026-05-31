// Copyright Wacom. All Rights Reserved.

#include "UI/Events/WacomRunEventChoiceButton.h"

#define LOCTEXT_NAMESPACE "WacomRunEventChoiceButton"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

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

		RequirementList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RequirementList"));
		if (UVerticalBoxSlot* RequirementSlot = RootBox->AddChildToVerticalBox(RequirementList))
		{
			RequirementSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		}

		ConsequenceList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ConsequenceList"));
		if (UVerticalBoxSlot* ConsequenceSlot = RootBox->AddChildToVerticalBox(ConsequenceList))
		{
			ConsequenceSlot->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
		}

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
	ConsequenceView = UWacomRunEventPresentationBuilder::BuildChoiceConsequenceView(ChoiceSnapshot);
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

int32 UWacomRunEventChoiceButton::GetDisplayedRequirementItemCountForTest() const
{
	return RequirementList ? RequirementList->GetChildrenCount() : 0;
}

FText UWacomRunEventChoiceButton::GetDisplayedRequirementItemTextForTest(int32 Index) const
{
	if (!RequirementList || !RequirementList->GetChildAt(Index))
	{
		return FText::GetEmpty();
	}
	const UTextBlock* TextBlock = Cast<UTextBlock>(RequirementList->GetChildAt(Index));
	return TextBlock ? TextBlock->GetText() : FText::GetEmpty();
}

int32 UWacomRunEventChoiceButton::GetDisplayedConsequenceItemCountForTest() const
{
	return ConsequenceList ? ConsequenceList->GetChildrenCount() : 0;
}

FText UWacomRunEventChoiceButton::GetDisplayedConsequenceItemTextForTest(int32 Index) const
{
	if (!ConsequenceList || !ConsequenceList->GetChildAt(Index))
	{
		return FText::GetEmpty();
	}
	const UTextBlock* TextBlock = Cast<UTextBlock>(ConsequenceList->GetChildAt(Index));
	return TextBlock ? TextBlock->GetText() : FText::GetEmpty();
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

void UWacomRunEventChoiceButton::RefreshRequirementList()
{
	if (!RequirementList || !WidgetTree)
	{
		return;
	}

	RequirementList->ClearChildren();
	for (const FWacomRunEventChoiceRequirementItemView& Item : RequirementView.RequirementItems)
	{
		if (Item.Kind == ERunEventChoiceRequirementKind::CardPayment)
		{
			continue;
		}

		UTextBlock* RequirementText = MakeEventChoiceText(
			WidgetTree,
			NAME_None,
			Item.Text,
			13);
		RequirementText->SetColorAndOpacity(Item.bSatisfied
			? FSlateColor(FLinearColor(0.62f, 0.78f, 0.64f, 1.f))
			: FSlateColor(FLinearColor(0.9f, 0.55f, 0.45f, 1.f)));
		if (UVerticalBoxSlot* RequirementItemSlot = RequirementList->AddChildToVerticalBox(RequirementText))
		{
			RequirementItemSlot->SetPadding(FMargin(0.f, 1.f));
		}
	}

	RequirementList->SetVisibility(RequirementList->GetChildrenCount() > 0
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);
}

void UWacomRunEventChoiceButton::RefreshConsequenceList()
{
	if (!ConsequenceList || !WidgetTree)
	{
		return;
	}

	ConsequenceList->ClearChildren();
	for (const FWacomRunEventChoiceConsequenceItemView& Item : ConsequenceView.ConsequenceItems)
	{
		UTextBlock* ConsequenceText = MakeEventChoiceText(
			WidgetTree,
			NAME_None,
			Item.Text,
			13);
		ConsequenceText->SetColorAndOpacity(Item.Tone == EWacomRunEventChoiceAvailabilityTone::Ready
			? FSlateColor(FLinearColor(0.62f, 0.78f, 0.64f, 1.f))
			: FSlateColor(FLinearColor(0.72f, 0.78f, 0.92f, 1.f)));
		if (UVerticalBoxSlot* ConsequenceItemSlot = ConsequenceList->AddChildToVerticalBox(ConsequenceText))
		{
			ConsequenceItemSlot->SetPadding(FMargin(0.f, 1.f));
		}
	}

	ConsequenceList->SetVisibility(ConsequenceList->GetChildrenCount() > 0
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);
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
	RefreshRequirementList();
	RefreshConsequenceList();
	if (DisabledReasonText)
	{
		DisabledReasonText->SetText(RequirementView.BlockedReasonText);
		DisabledReasonText->SetVisibility(RequirementView.BlockedReasonText.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}
}

#undef LOCTEXT_NAMESPACE
