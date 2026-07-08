// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailPlainTextRenderer.h"

namespace
{
	FString ShortTagName(const FGameplayTag& Tag)
	{
		FString TagText = Tag.IsValid() ? Tag.ToString() : FString();
		int32 DotIndex = INDEX_NONE;
		TagText.FindLastChar(TEXT('.'), DotIndex);
		return DotIndex == INDEX_NONE ? TagText : TagText.Mid(DotIndex + 1);
	}

	FString IconFallbackText(EWacomCardDetailIcon Icon)
	{
		switch (Icon)
		{
		case EWacomCardDetailIcon::Damage: return TEXT("[伤]");
		case EWacomCardDetailIcon::Heal: return TEXT("[疗]");
		case EWacomCardDetailIcon::Shield: return TEXT("[盾]");
		case EWacomCardDetailIcon::Poison: return TEXT("[毒]");
		case EWacomCardDetailIcon::Cost: return TEXT("[费]");
		case EWacomCardDetailIcon::Initiative: return TEXT("[机]");
		case EWacomCardDetailIcon::Draw: return TEXT("[抽]");
		case EWacomCardDetailIcon::Discard: return TEXT("[弃]");
		case EWacomCardDetailIcon::Exhaust: return TEXT("[耗]");
		case EWacomCardDetailIcon::Slow: return TEXT("[缓]");
		case EWacomCardDetailIcon::Freeze: return TEXT("[冻]");
		case EWacomCardDetailIcon::Twilight: return TEXT("[暮]");
		case EWacomCardDetailIcon::Keyword: return TEXT("[词]");
		case EWacomCardDetailIcon::None:
		default:
			return FString();
		}
	}

	FString StatusDisplayText(const FGameplayTag& StatusTag, const FText& FallbackText)
	{
		if (!FallbackText.IsEmpty())
		{
			return FallbackText.ToString();
		}
		return ShortTagName(StatusTag);
	}

	bool ShouldRenderValueSourceText(const FWacomCardDetailRun& Run)
	{
		return Run.bHasValueSourceText
			&& !Run.ValueSourceText.IsEmpty()
			&& (!Run.bHasPreviewValue || Run.PreviewValue == Run.Value);
	}
}

FText UWacomCardDetailPlainTextRenderer::RenderSectionPlainText(
	const FWacomCardDetailSection& Section)
{
	TArray<FString> Lines;
	for (const FWacomCardDetailBlock& Block : Section.Blocks)
	{
		const FString Line = RenderBlockPlainText(Block).ToString();
		if (!Line.IsEmpty())
		{
			Lines.Add(Line);
		}
	}
	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

FText UWacomCardDetailPlainTextRenderer::RenderBlockPlainText(
	const FWacomCardDetailBlock& Block)
{
	FString Result;
	for (const FWacomCardDetailRun& Run : Block.Runs)
	{
		Result += RenderRunPlainString(Run);
	}
	return FText::FromString(Result);
}

FText UWacomCardDetailPlainTextRenderer::RenderDocumentPlainText(
	const FWacomCardDetailViewData& DetailData)
{
	TArray<FString> Sections;
	for (const FWacomCardDetailSection& Section : DetailData.Sections)
	{
		const FString Body = RenderSectionPlainText(Section).ToString();
		if (Body.IsEmpty())
		{
			continue;
		}

		const FString Title = Section.Title.ToString();
		Sections.Add(Title.IsEmpty() ? Body : FString::Printf(TEXT("%s\n%s"), *Title, *Body));
	}
	return FText::FromString(FString::Join(Sections, TEXT("\n\n")));
}

FString UWacomCardDetailPlainTextRenderer::RenderRunPlainString(
	const FWacomCardDetailRun& Run)
{
	switch (Run.Kind)
	{
	case EWacomCardDetailRunKind::Text:
	case EWacomCardDetailRunKind::Muted:
	case EWacomCardDetailRunKind::Keyword:
		return Run.Text.IsEmpty() && Run.Tag.IsValid()
			? ShortTagName(Run.Tag)
			: Run.Text.ToString();
	case EWacomCardDetailRunKind::Value:
		if (!Run.bHasValue)
		{
			return FString();
		}
		{
			const FString ValueText = FString::FromInt(Run.bHasPreviewValue ? Run.PreviewValue : Run.Value);
			return ShouldRenderValueSourceText(Run)
				? FString::Printf(TEXT("%s %s"), *Run.ValueSourceText.ToString(), *ValueText)
				: ValueText;
		}
	case EWacomCardDetailRunKind::PreviewDelta:
		return Run.bHasValue ? FString::FromInt(Run.Value) : FString();
	case EWacomCardDetailRunKind::Icon:
		return IconFallbackText(Run.Icon);
	case EWacomCardDetailRunKind::Status:
		return StatusDisplayText(Run.Tag, Run.Text);
	default:
		return Run.Text.ToString();
	}
}
