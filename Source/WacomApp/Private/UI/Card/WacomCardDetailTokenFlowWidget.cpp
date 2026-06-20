// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailTokenFlowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Card/WacomCardDetailTokenLineWidget.h"

UWacomCardDetailTokenFlowWidget::UWacomCardDetailTokenFlowWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (UClass* Loaded = LoadObject<UClass>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_CardDetailTokenLine.WBP_CardDetailTokenLine_C"));
		Loaded && Loaded->IsChildOf(UWacomCardDetailTokenLineWidget::StaticClass()))
	{
		TokenLineWidgetClass = Loaded;
	}
	else
	{
		TokenLineWidgetClass = UWacomCardDetailTokenLineWidget::StaticClass();
	}
}

TSharedRef<SWidget> UWacomCardDetailTokenFlowWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_CardDetailTokenFlow"));
		}

		LinesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LinesBox"));
		WidgetTree->RootWidget = LinesBox;
	}

	return Super::RebuildWidget();
}

void UWacomCardDetailTokenFlowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyCurrentDataToWidgets();
}

void UWacomCardDetailTokenFlowWidget::SetTokenLinesData(const TArray<FWacomCardDetailTokenLine>& InLines)
{
	CurrentLines = InLines;
	ApplyCurrentDataToWidgets();
}

void UWacomCardDetailTokenFlowWidget::ApplyCurrentDataToWidgets()
{
	if (!LinesBox)
	{
		return;
	}

	TSet<FName> DesiredKeys;
	TArray<TObjectPtr<UWacomCardDetailTokenLineWidget>> DesiredWidgets;
	DesiredWidgets.Reserve(CurrentLines.Num());

	for (int32 Index = 0; Index < CurrentLines.Num(); ++Index)
	{
		const FWacomCardDetailTokenLine& Line = CurrentLines[Index];
		if (Line.Tokens.IsEmpty())
		{
			continue;
		}

		const FName LineKey = MakeLineWidgetKey(Line, Index);
		DesiredKeys.Add(LineKey);

		UWacomCardDetailTokenLineWidget* LineWidget = FindOrCreateLineWidget(LineKey);
		if (!LineWidget)
		{
			continue;
		}

		LineWidget->SetTokenLineData(Line);
		DesiredWidgets.Add(LineWidget);
	}

	RemoveStaleLineWidgets(DesiredKeys);
	RebuildLineChildrenIfNeeded(DesiredWidgets);
	LinesBox->SetVisibility(DesiredWidgets.Num() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	SetVisibility(DesiredWidgets.Num() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

UWacomCardDetailTokenLineWidget* UWacomCardDetailTokenFlowWidget::FindOrCreateLineWidget(FName LineKey)
{
	if (TObjectPtr<UWacomCardDetailTokenLineWidget>* Existing = LineWidgetsByKey.Find(LineKey))
	{
		return Existing->Get();
	}

	UClass* WidgetClass = TokenLineWidgetClass
		? TokenLineWidgetClass.Get()
		: UWacomCardDetailTokenLineWidget::StaticClass();
	UWacomCardDetailTokenLineWidget* LineWidget = GetWorld()
		? CreateWidget<UWacomCardDetailTokenLineWidget>(this, WidgetClass)
		: NewObject<UWacomCardDetailTokenLineWidget>(this, WidgetClass);
	if (!LineWidget)
	{
		return nullptr;
	}

	LineWidgetsByKey.Add(LineKey, LineWidget);
	return LineWidget;
}

void UWacomCardDetailTokenFlowWidget::RemoveStaleLineWidgets(const TSet<FName>& DesiredKeys)
{
	TArray<FName> KeysToRemove;
	for (const TPair<FName, TObjectPtr<UWacomCardDetailTokenLineWidget>>& Pair : LineWidgetsByKey)
	{
		if (!DesiredKeys.Contains(Pair.Key))
		{
			if (Pair.Value && Pair.Value->GetParent() == LinesBox)
			{
				LinesBox->RemoveChild(Pair.Value);
			}
			KeysToRemove.Add(Pair.Key);
		}
	}

	for (const FName Key : KeysToRemove)
	{
		LineWidgetsByKey.Remove(Key);
	}
}

void UWacomCardDetailTokenFlowWidget::RebuildLineChildrenIfNeeded(
	const TArray<TObjectPtr<UWacomCardDetailTokenLineWidget>>& DesiredWidgets)
{
	bool bNeedsRebuild = LinesBox->GetChildrenCount() != DesiredWidgets.Num();
	if (!bNeedsRebuild)
	{
		for (int32 Index = 0; Index < DesiredWidgets.Num(); ++Index)
		{
			if (LinesBox->GetChildAt(Index) != DesiredWidgets[Index])
			{
				bNeedsRebuild = true;
				break;
			}
		}
	}

	if (!bNeedsRebuild)
	{
		return;
	}

	LinesBox->ClearChildren();
	for (UWacomCardDetailTokenLineWidget* LineWidget : DesiredWidgets)
	{
		if (!LineWidget)
		{
			continue;
		}

		if (UPanelSlot* LineSlot = LinesBox->AddChild(LineWidget))
		{
			if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(LineSlot))
			{
				VerticalSlot->SetPadding(LinePadding);
			}
		}
	}
}

FName UWacomCardDetailTokenFlowWidget::MakeLineWidgetKey(
	const FWacomCardDetailTokenLine& Line,
	int32 LineIndex) const
{
	if (!Line.LineId.IsNone())
	{
		return Line.LineId;
	}

	return FName(*FString::Printf(TEXT("TokenLine.%d"), LineIndex));
}
