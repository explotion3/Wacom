// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailRichTextBlock.h"

#include "UI/Card/WacomCardDetailRichTextDecorator.h"
#include "UI/Card/WacomCardDetailTheme.h"

void UWacomCardDetailRichTextBlock::SetCardDetailRichText(
	FText InText,
	const UWacomCardDetailTheme* InTheme)
{
	CardDetailTheme = const_cast<UWacomCardDetailTheme*>(InTheme);
	if (InTheme && InTheme->BodyTextStyleSet)
	{
		SetTextStyleSet(InTheme->BodyTextStyleSet);
	}

	EnsureCardDetailDecoratorRegistered();
	SetText(InText);
}

bool UWacomCardDetailRichTextBlock::HasCardDetailDecoratorRegistered() const
{
	return DecoratorClasses.Contains(UWacomCardDetailRichTextDecorator::StaticClass());
}

void UWacomCardDetailRichTextBlock::EnsureCardDetailDecoratorRegistered()
{
	if (HasCardDetailDecoratorRegistered())
	{
		return;
	}

	TArray<TSubclassOf<URichTextBlockDecorator>> Decorators = DecoratorClasses;
	Decorators.Add(UWacomCardDetailRichTextDecorator::StaticClass());
	SetDecorators(Decorators);
}
