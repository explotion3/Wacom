// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailRichTextRenderer.h"

#include "UI/Card/WacomCardDetailPlainTextRenderer.h"
#include "WacomCardDetailIconIds.h"

namespace
{
	FString EscapeRichText(const FString& In)
	{
		FString Out = In;
		Out.ReplaceInline(TEXT("&"), TEXT("&amp;"), ESearchCase::CaseSensitive);
		Out.ReplaceInline(TEXT("<"), TEXT("&lt;"), ESearchCase::CaseSensitive);
		Out.ReplaceInline(TEXT(">"), TEXT("&gt;"), ESearchCase::CaseSensitive);
		return Out;
	}

	FString EscapeRichTextAttribute(const FString& In)
	{
		FString Out = EscapeRichText(In);
		Out.ReplaceInline(TEXT("\""), TEXT("&quot;"), ESearchCase::CaseSensitive);
		return Out;
	}

	FString WrapStyle(const TCHAR* StyleName, const FString& Text)
	{
		return Text.IsEmpty()
			? FString()
			: FString::Printf(TEXT("<%s>%s</>"), StyleName, *EscapeRichText(Text));
	}

	FString InlineIconTag(EWacomCardDetailIcon Icon, const FString& Label)
	{
		return FString::Printf(
			TEXT("<wacom-icon id=\"%s\" label=\"%s\"/>"),
			*EscapeRichTextAttribute(WacomCardDetailIconIds::ToString(Icon)),
			*EscapeRichTextAttribute(Label));
	}

	FString InlineStatusTag(const FGameplayTag& StatusTag, const FString& Label)
	{
		return FString::Printf(
			TEXT("<wacom-status tag=\"%s\" label=\"%s\"/>"),
			*EscapeRichTextAttribute(StatusTag.IsValid() ? StatusTag.ToString() : FString()),
			*EscapeRichTextAttribute(Label));
	}

	bool ShouldRenderRichValueSourceText(const FWacomCardDetailRun& Run)
	{
		return Run.bHasValueSourceText
			&& !Run.ValueSourceText.IsEmpty()
			&& (!Run.bHasPreviewValue || Run.PreviewValue == Run.Value);
	}

	FString RenderRun(const FWacomCardDetailRun& Run)
	{
		switch (Run.Kind)
		{
		case EWacomCardDetailRunKind::Value:
			if (!Run.bHasValue)
			{
				return FString();
			}
			{
				const int32 DisplayValue = Run.bHasPreviewValue ? Run.PreviewValue : Run.Value;
				const TCHAR* ValueStyle = Run.bHasPreviewValue && Run.PreviewValue > Run.Value
					? TEXT("ValueBuffed")
					: Run.bHasPreviewValue && Run.PreviewValue < Run.Value
						? TEXT("ValueNerfed")
						: TEXT("Value");
				const FString StyledValue = WrapStyle(ValueStyle, FString::FromInt(DisplayValue));
				return ShouldRenderRichValueSourceText(Run)
					? EscapeRichText(Run.ValueSourceText.ToString() + TEXT(" ")) + StyledValue
					: StyledValue;
			}
		case EWacomCardDetailRunKind::Icon:
			return InlineIconTag(
				Run.Icon,
				UWacomCardDetailPlainTextRenderer::RenderRunPlainString(Run));
		case EWacomCardDetailRunKind::Status:
			{
				const FString Label = UWacomCardDetailPlainTextRenderer::RenderRunPlainString(Run);
				return InlineStatusTag(Run.Tag, Label) + TEXT(" ") + WrapStyle(TEXT("Status"), Label);
			}
		case EWacomCardDetailRunKind::Keyword:
			return WrapStyle(TEXT("Keyword"), UWacomCardDetailPlainTextRenderer::RenderRunPlainString(Run));
		case EWacomCardDetailRunKind::Muted:
			return WrapStyle(TEXT("Muted"), UWacomCardDetailPlainTextRenderer::RenderRunPlainString(Run));
		case EWacomCardDetailRunKind::PreviewDelta:
			return WrapStyle(TEXT("ValueBuffed"), UWacomCardDetailPlainTextRenderer::RenderRunPlainString(Run));
		case EWacomCardDetailRunKind::Text:
		default:
			return EscapeRichText(Run.Text.ToString());
		}
	}
}

FText UWacomCardDetailRichTextRenderer::RenderSectionRichText(
	const FWacomCardDetailSection& Section)
{
	TArray<FString> Lines;
	for (const FWacomCardDetailBlock& Block : Section.Blocks)
	{
		const FString Line = RenderBlockRichText(Block).ToString();
		if (!Line.IsEmpty())
		{
			Lines.Add(Line);
		}
	}
	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

FText UWacomCardDetailRichTextRenderer::RenderBlockRichText(
	const FWacomCardDetailBlock& Block)
{
	FString Result;
	for (const FWacomCardDetailRun& Run : Block.Runs)
	{
		Result += RenderRun(Run);
	}
	return FText::FromString(Result);
}
