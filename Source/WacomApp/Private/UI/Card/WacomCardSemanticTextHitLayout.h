// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "UI/Card/WacomCardPresentationTypes.h"

namespace WacomCardSemanticTextHitLayout
{
	/**
	 * Resolves a semantic token using the same font metrics, available width,
	 * ordered source ranges and justification used by the visible TypeText.
	 * Separators and whitespace are deliberately excluded from token bounds.
	 */
	WACOMAPP_API bool ResolveTokenAtLocalPosition(
		const FString& FullText,
		const TArray<FWacomCardFaceSemanticTokenView>& Tokens,
		const FSlateFontInfo& Font,
		const FVector2D& AvailableSize,
		ETextJustify::Type Justification,
		const FVector2D& LocalPosition,
		FWacomCardFaceSemanticTokenView& OutToken);
}
