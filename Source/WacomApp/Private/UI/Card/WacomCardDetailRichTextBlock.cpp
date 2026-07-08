// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailRichTextBlock.h"

#include "UI/Card/WacomCardDetailTheme.h"

void UWacomCardDetailRichTextBlock::SetCardDetailRichText(
	FText InText,
	const UWacomCardDetailTheme* InTheme)
{
	if (InTheme && InTheme->BodyTextStyleSet)
	{
		SetTextStyleSet(InTheme->BodyTextStyleSet);
	}

	SetText(InText);
}
