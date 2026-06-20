// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailTokenLineWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "UI/Card/WacomCardDetailTokenWidget.h"

UWacomCardDetailTokenLineWidget::UWacomCardDetailTokenLineWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (UClass* Loaded = LoadObject<UClass>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_CardDetailToken.WBP_CardDetailToken_C"));
		Loaded && Loaded->IsChildOf(UWacomCardDetailTokenWidget::StaticClass()))
	{
		TokenWidgetClass = Loaded;
	}
	else
	{
		TokenWidgetClass = UWacomCardDetailTokenWidget::StaticClass();
	}
}

TSharedRef<SWidget> UWacomCardDetailTokenLineWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_CardDetailTokenLine"));
		}

		UWrapBox* RootWrapBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("TokensBox"));
		RootWrapBox->SetInnerSlotPadding(FVector2D(TokenPadding.Right, TokenPadding.Bottom));
		WidgetTree->RootWidget = RootWrapBox;
		TokensBox = RootWrapBox;
	}

	return Super::RebuildWidget();
}

void UWacomCardDetailTokenLineWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyCurrentDataToWidgets();
}

void UWacomCardDetailTokenLineWidget::SetTokenLineData(const FWacomCardDetailTokenLine& InData)
{
	CurrentData = InData;
	ApplyCurrentDataToWidgets();
}

void UWacomCardDetailTokenLineWidget::ApplyCurrentDataToWidgets()
{
	if (!TokensBox)
	{
		return;
	}

	TArray<FWacomCardDetailToken> DesiredTokens;
	if (HasSkippedToken())
	{
		DesiredTokens.Add(MakeSkippedPrefixToken());
	}
	DesiredTokens.Append(CurrentData.Tokens);

	TSet<FName> DesiredKeys;
	TArray<TObjectPtr<UWacomCardDetailTokenWidget>> DesiredWidgets;
	DesiredWidgets.Reserve(DesiredTokens.Num());

	for (int32 Index = 0; Index < DesiredTokens.Num(); ++Index)
	{
		const FWacomCardDetailToken& Token = DesiredTokens[Index];
		const FName TokenKey = MakeTokenWidgetKey(Token, Index);
		DesiredKeys.Add(TokenKey);

		UWacomCardDetailTokenWidget* TokenWidget = FindOrCreateTokenWidget(TokenKey);
		if (!TokenWidget)
		{
			continue;
		}

		TokenWidget->SetTokenData(Token);
		DesiredWidgets.Add(TokenWidget);
	}

	RemoveStaleTokenWidgets(DesiredKeys);
	RebuildTokenChildrenIfNeeded(DesiredWidgets);
	TokensBox->SetVisibility(DesiredWidgets.Num() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	SetVisibility(DesiredWidgets.Num() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

UWacomCardDetailTokenWidget* UWacomCardDetailTokenLineWidget::FindOrCreateTokenWidget(FName TokenKey)
{
	if (TObjectPtr<UWacomCardDetailTokenWidget>* Existing = TokenWidgetsByKey.Find(TokenKey))
	{
		return Existing->Get();
	}

	UClass* WidgetClass = TokenWidgetClass
		? TokenWidgetClass.Get()
		: UWacomCardDetailTokenWidget::StaticClass();
	UWacomCardDetailTokenWidget* TokenWidget = GetWorld()
		? CreateWidget<UWacomCardDetailTokenWidget>(this, WidgetClass)
		: NewObject<UWacomCardDetailTokenWidget>(this, WidgetClass);
	if (!TokenWidget)
	{
		return nullptr;
	}

	TokenWidgetsByKey.Add(TokenKey, TokenWidget);
	return TokenWidget;
}

void UWacomCardDetailTokenLineWidget::RemoveStaleTokenWidgets(const TSet<FName>& DesiredKeys)
{
	TArray<FName> KeysToRemove;
	for (const TPair<FName, TObjectPtr<UWacomCardDetailTokenWidget>>& Pair : TokenWidgetsByKey)
	{
		if (!DesiredKeys.Contains(Pair.Key))
		{
			if (Pair.Value && Pair.Value->GetParent() == TokensBox)
			{
				TokensBox->RemoveChild(Pair.Value);
			}
			KeysToRemove.Add(Pair.Key);
		}
	}

	for (const FName Key : KeysToRemove)
	{
		TokenWidgetsByKey.Remove(Key);
	}
}

void UWacomCardDetailTokenLineWidget::RebuildTokenChildrenIfNeeded(
	const TArray<TObjectPtr<UWacomCardDetailTokenWidget>>& DesiredWidgets)
{
	bool bNeedsRebuild = TokensBox->GetChildrenCount() != DesiredWidgets.Num();
	if (!bNeedsRebuild)
	{
		for (int32 Index = 0; Index < DesiredWidgets.Num(); ++Index)
		{
			if (TokensBox->GetChildAt(Index) != DesiredWidgets[Index])
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

	TokensBox->ClearChildren();
	for (UWacomCardDetailTokenWidget* TokenWidget : DesiredWidgets)
	{
		if (!TokenWidget)
		{
			continue;
		}

		if (UPanelSlot* TokenSlot = TokensBox->AddChild(TokenWidget))
		{
			if (UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot>(TokenSlot))
			{
				WrapSlot->SetPadding(TokenPadding);
				if (ShouldTokenFillLine(TokenWidget->GetTokenData()))
				{
					WrapSlot->SetNewLine(true);
					WrapSlot->SetFillEmptySpace(true);
					WrapSlot->SetFillSpanWhenLessThan(100000.0f);
				}
				else
				{
					WrapSlot->SetNewLine(false);
					WrapSlot->SetFillEmptySpace(false);
					WrapSlot->SetFillSpanWhenLessThan(0.0f);
				}
			}
		}
	}
}

FName UWacomCardDetailTokenLineWidget::MakeTokenWidgetKey(
	const FWacomCardDetailToken& Token,
	int32 TokenIndex) const
{
	if (!Token.StableId.IsNone())
	{
		return Token.StableId;
	}

	const FString LinePrefix = CurrentData.LineId.IsNone()
		? TEXT("Line")
		: CurrentData.LineId.ToString();
	return FName(*FString::Printf(TEXT("%s.Token.%d"), *LinePrefix, TokenIndex));
}

bool UWacomCardDetailTokenLineWidget::ShouldTokenFillLine(const FWacomCardDetailToken& Token) const
{
	return Token.Kind == EWacomCardDetailTokenKind::Text
		&& CurrentData.Kind == EWacomCardDetailTokenLineKind::Description
		&& CurrentData.Tokens.Num() == 1;
}

FWacomCardDetailToken UWacomCardDetailTokenLineWidget::MakeSkippedPrefixToken() const
{
	FWacomCardDetailToken Token;
	Token.StableId = CurrentData.LineId.IsNone()
		? FName(TEXT("SkippedPrefix"))
		: FName(*FString::Printf(TEXT("%s.SkippedPrefix"), *CurrentData.LineId.ToString()));
	Token.Kind = EWacomCardDetailTokenKind::Text;
	Token.Text = FText::FromString(TEXT("不会生效："));
	Token.bEmphasized = true;
	return Token;
}

bool UWacomCardDetailTokenLineWidget::HasSkippedToken() const
{
	for (const FWacomCardDetailToken& Token : CurrentData.Tokens)
	{
		if (Token.bSkipped)
		{
			return true;
		}
	}
	return false;
}
