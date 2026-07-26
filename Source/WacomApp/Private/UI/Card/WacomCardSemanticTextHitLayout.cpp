// Copyright Wacom. All Rights Reserved.

#include "WacomCardSemanticTextHitLayout.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Text/ILayoutBlock.h"
#include "Framework/Text/PlainTextLayoutMarshaller.h"
#include "Framework/Text/SlateTextLayout.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SNullWidget.h"

namespace WacomCardSemanticTextHitLayout
{
	namespace
	{
		bool IsInsideBlockRange(
			const FVector2D& LocalPosition,
			const TSharedRef<ILayoutBlock>& Block,
			const int32 BeginIndex,
			const int32 EndIndex)
		{
			const FTextRange BlockRange = Block->GetTextRange();
			const int32 OverlapBegin =
				FMath::Max(BeginIndex, BlockRange.BeginIndex);
			const int32 OverlapEnd =
				FMath::Min(EndIndex, BlockRange.EndIndex);
			if (OverlapBegin >= OverlapEnd)
			{
				return false;
			}

			const TSharedRef<IRun> Run = Block->GetRun();
			const FVector2D Start = Run->GetLocationAt(
				Block,
				OverlapBegin - BlockRange.BeginIndex,
				1.0f);
			const FVector2D End = Run->GetLocationAt(
				Block,
				OverlapEnd - BlockRange.BeginIndex,
				1.0f);
			const FVector2D BlockOffset = Block->GetLocationOffset();
			const FVector2D BlockSize = Block->GetSize();
			const float MinX = FMath::Min(Start.X, End.X);
			const float MaxX = FMath::Max(Start.X, End.X);
			return LocalPosition.X >= MinX
				&& LocalPosition.X <= MaxX
				&& LocalPosition.Y >= BlockOffset.Y
				&& LocalPosition.Y <= BlockOffset.Y + BlockSize.Y;
		}
	}

	bool ResolveTokenAtLocalPosition(
		const FString& FullText,
		const TArray<FWacomCardFaceSemanticTokenView>& Tokens,
		const FSlateFontInfo& Font,
		const FVector2D& AvailableSize,
		const ETextJustify::Type Justification,
		const FVector2D& LocalPosition,
		FWacomCardFaceSemanticTokenView& OutToken)
	{
		if (!FSlateApplication::IsInitialized()
			|| FullText.IsEmpty()
			|| Tokens.IsEmpty()
			|| AvailableSize.X <= 0.0f
			|| AvailableSize.Y <= 0.0f
			|| LocalPosition.X < 0.0f
			|| LocalPosition.Y < 0.0f
			|| LocalPosition.X > AvailableSize.X
			|| LocalPosition.Y > AvailableSize.Y)
		{
			return false;
		}

		FTextBlockStyle TextStyle =
			FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>(
				TEXT("NormalText"));
		TextStyle.SetFont(Font);
		TSharedRef<FSlateTextLayout> TextLayout =
			FSlateTextLayout::Create(
				&SNullWidget::NullWidget.Get(),
				MoveTemp(TextStyle));
		FPlainTextLayoutMarshaller::Create()->SetText(
			FullText,
			*TextLayout);
		TextLayout->SetScale(1.0f);
		TextLayout->SetWrappingWidth(AvailableSize.X);
		TextLayout->SetJustification(Justification);
		TextLayout->SetVisibleRegion(
			AvailableSize,
			FVector2D::ZeroVector);
		TextLayout->UpdateIfNeeded();

		for (const FTextLayout::FLineView& Line :
			TextLayout->GetLineViews())
		{
			if (LocalPosition.Y < Line.Offset.Y
				|| LocalPosition.Y > Line.Offset.Y + Line.Size.Y)
			{
				continue;
			}
			for (const TSharedRef<ILayoutBlock>& Block : Line.Blocks)
			{
				for (const FWacomCardFaceSemanticTokenView& Token : Tokens)
				{
					if (Token.HasValidRangeFor(FullText)
						&& IsInsideBlockRange(
							LocalPosition,
							Block,
							Token.StartIndex,
							Token.StartIndex + Token.Length))
					{
						OutToken = Token;
						return true;
					}
				}
			}
		}
		return false;
	}
}
