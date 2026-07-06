// Copyright Wacom. All Rights Reserved.

#include "WacomCardDetailDocumentBuilder.h"

#include "Cards/CardDefinition.h"
#include "WacomCardDetailTextCompiler.h"

#define LOCTEXT_NAMESPACE "WacomCardDetailDocumentBuilder"

namespace WacomCardDetailDocumentBuilder
{
	namespace
	{
		FText GetCardDisplayName(const UCardDefinition* Card)
		{
			if (!Card)
			{
				return LOCTEXT("UnknownCardName", "未知卡牌");
			}
			return Card->DisplayName.IsEmpty()
				? FText::FromName(Card->CardId)
				: Card->DisplayName;
		}

		bool ShouldAppendGeneratedActiveEffectTokenLines(const UCardDefinition* Card)
		{
			if (!Card)
			{
				return false;
			}

			// 手写 Description 是卡牌详情的主阅读文本；自动 Effect 行只在没有描述时补规则。
			return Card->Description.IsEmpty();
		}
	}

	FWacomCardDetailViewData BuildCardDetailViewData(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext)
	{
		FWacomCardDetailViewData Data;
		Data.Name = GetCardDisplayName(Card);
		if (!Card)
		{
			return Data;
		}

		TArray<FWacomCardDetailTokenLine> DescriptionSectionLines =
			WacomCardDetailTextCompiler::BuildAuthoredTextTokenLines(
				Card,
				Card->Effects,
				RuntimeContext,
				Card->Description,
				EWacomCardDetailTokenLineKind::Description,
				TEXT("Description"));
		if (ShouldAppendGeneratedActiveEffectTokenLines(Card))
		{
			TArray<FWacomCardDetailTokenLine> EffectLines =
				WacomCardDetailTextCompiler::BuildEffectTokenLines(Card, RuntimeContext);
			DescriptionSectionLines.Append(MoveTemp(EffectLines));
		}

		TArray<FWacomCardDetailTokenLine> PassiveSectionLines;
		WacomCardDetailTextCompiler::BuildPassiveTokenLines(
			Card,
			RuntimeContext,
			PassiveSectionLines);

		WacomCardDetailTextCompiler::AddCardDetailSection(
			Data,
			FName(TEXT("Description")),
			EWacomCardDetailSectionKind::Description,
			LOCTEXT("DescriptionSectionTitle", "描述"),
			MoveTemp(DescriptionSectionLines));
		WacomCardDetailTextCompiler::AddCardDetailSection(
			Data,
			FName(TEXT("Passive")),
			EWacomCardDetailSectionKind::Passive,
			LOCTEXT("PassivesSectionTitle", "被动"),
			MoveTemp(PassiveSectionLines));
		return Data;
	}
}

#undef LOCTEXT_NAMESPACE
