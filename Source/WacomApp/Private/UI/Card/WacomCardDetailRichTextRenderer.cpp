// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailRichTextRenderer.h"

#include "UI/Card/WacomCardDetailPlainTextRenderer.h"

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

	FString WrapStyle(const TCHAR* StyleName, const FString& Text)
	{
		return Text.IsEmpty()
			? FString()
			: FString::Printf(TEXT("<%s>%s</>"), StyleName, *EscapeRichText(Text));
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
			if (Run.bHasPreviewValue)
			{
				const TCHAR* PreviewStyle = Run.PreviewValue > Run.Value
					? TEXT("ValueBuffed")
					: Run.PreviewValue < Run.Value
						? TEXT("ValueNerfed")
						: TEXT("Value");
				return WrapStyle(PreviewStyle, FString::FromInt(Run.PreviewValue));
			}
			return WrapStyle(TEXT("Value"), FString::FromInt(Run.Value));
		case EWacomCardDetailRunKind::Icon:
			return WrapStyle(TEXT("Icon"), UWacomCardDetailPlainTextRenderer::RenderRunPlainString(Run));
		case EWacomCardDetailRunKind::Status:
			return WrapStyle(TEXT("Status"), UWacomCardDetailPlainTextRenderer::RenderRunPlainString(Run));
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
	if (Block.bSkipped)
	{
		Result += WrapStyle(TEXT("Muted"), TEXT("不会生效："));
	}

	for (const FWacomCardDetailRun& Run : Block.Runs)
	{
		Result += RenderRun(Run);
	}
	return FText::FromString(Result);
}
