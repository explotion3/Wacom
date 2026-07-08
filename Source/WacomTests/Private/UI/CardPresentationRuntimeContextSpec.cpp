// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UCardDefinition* MakeRuntimePresentationCardForTest(UObject* Outer)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = TEXT("UI.CardPresentation.Runtime");
		Card->DisplayName = FText::FromString(TEXT("运行时展示测试卡"));
		Card->Description = FText::FromString(TEXT("测试运行时卡面展示。"));
		Card->BaseCost = 2;

		FCardEffect RuntimeDamage;
		RuntimeDamage.EffectType = WacomTags::Effect_Damage;
		RuntimeDamage.Magnitude = 11;
		RuntimeDamage.MagnitudeSource = WacomTags::Magnitude_Source_RuntimeCost;

		FCardEffect RuntimePoison;
		RuntimePoison.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		RuntimePoison.Magnitude = 7;
		RuntimePoison.MagnitudeSource = WacomTags::Magnitude_Source_RuntimeCost;

		FCardEffect LegacyRuntimeHeal;
		LegacyRuntimeHeal.EffectType = WacomTags::Effect_Heal;
		LegacyRuntimeHeal.Magnitude = 9;
		LegacyRuntimeHeal.bMagnitudeFromRuntimeCost = true;

		FCardEffect Shield;
		Shield.EffectType = WacomTags::Status_Shield;
		Shield.Magnitude = 6;

		Card->Effects = { RuntimeDamage, RuntimePoison, LegacyRuntimeHeal, Shield };
		return Card;
	}

	FWacomCardPresentationRuntimeContext MakeRuntimeContextForTest()
	{
		FWacomCardPresentationRuntimeContext Context;
		Context.bHasRuntimeCost = true;
		Context.RuntimeCost = 5;
		Context.bHasPlayableState = true;
		Context.bIsPlayable = false;
		return Context;
	}

	void TestBadge(
		FAutomationTestBase& Test,
		const TArray<FWacomCardViewEffectBadge>& Badges,
		int32 Index,
		EWacomCardViewEffectBadgeKind ExpectedKind,
		int32 ExpectedValue)
	{
		if (!Test.TestTrue(TEXT("Badge index exists"), Badges.IsValidIndex(Index)))
		{
			return;
		}

		Test.TestEqual(TEXT("Badge kind"), Badges[Index].Kind, ExpectedKind);
		Test.TestEqual(TEXT("Badge value"), Badges[Index].Value, ExpectedValue);
	}

	bool HasDetailValueRun(
		const FWacomCardDetailViewData& DetailData,
		int32 ExpectedValue,
		int32 ExpectedPreviewValue)
	{
		for (const FWacomCardDetailSection& Section : DetailData.Sections)
		{
			for (const FWacomCardDetailBlock& Block : Section.Blocks)
			{
				for (const FWacomCardDetailRun& Run : Block.Runs)
				{
					if (Run.Kind == EWacomCardDetailRunKind::Value
						&& Run.bHasValue
						&& Run.Value == ExpectedValue
						&& Run.bHasPreviewValue
						&& Run.PreviewValue == ExpectedPreviewValue)
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	bool HasSkippedDetailStatusRun(
		const FWacomCardDetailViewData& DetailData,
		const FGameplayTag& ExpectedStatusTag)
	{
		for (const FWacomCardDetailSection& Section : DetailData.Sections)
		{
			for (const FWacomCardDetailBlock& Block : Section.Blocks)
			{
				for (const FWacomCardDetailRun& Run : Block.Runs)
				{
					if (Run.Kind == EWacomCardDetailRunKind::Status
						&& Run.Tag.MatchesTagExact(ExpectedStatusTag)
						&& (Run.bSkipped || Block.bSkipped))
					{
						return true;
					}
				}
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardPresentationRuntimeContextSpec,
	"Wacom.UI.CardPresentation.RuntimeContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardPresentationRuntimeContextSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(MakeRuntimePresentationCardForTest(GetTransientPackage()));
	const FWacomCardPresentationRuntimeContext RuntimeContext = MakeRuntimeContextForTest();

	const FWacomCardViewData ViewData =
		UWacomCardPresentationBuilder::BuildCardViewData(Card.Get(), RuntimeContext);
	TestEqual(TEXT("Runtime cost drives card face cost"), ViewData.Cost, 5);
	TestTrue(TEXT("Runtime playable state drives disabled card face"), ViewData.bDisabled);
	TestEqual(TEXT("Runtime badges include all supported effect badges"), ViewData.EffectBadges.Num(), 4);
	TestBadge(*this, ViewData.EffectBadges, 0, EWacomCardViewEffectBadgeKind::Damage, 5);
	TestBadge(*this, ViewData.EffectBadges, 1, EWacomCardViewEffectBadgeKind::Poison, 5);
	TestBadge(*this, ViewData.EffectBadges, 2, EWacomCardViewEffectBadgeKind::Heal, 5);
	TestBadge(*this, ViewData.EffectBadges, 3, EWacomCardViewEffectBadgeKind::Shield, 6);

	FWacomCardPresentationRuntimeContext PreviewContext = RuntimeContext;
	FWacomCardPresentationRuntimeContext::FEffectPreview DamageOverride;
	DamageOverride.EffectIndex = 0;
	DamageOverride.bHasMagnitude = true;
	DamageOverride.Magnitude = 9;
	PreviewContext.EffectPreviews.Add(DamageOverride);

	FWacomCardPresentationRuntimeContext::FEffectPreview PoisonSkip;
	PoisonSkip.EffectIndex = 1;
	PoisonSkip.bSkip = true;
	PreviewContext.EffectPreviews.Add(PoisonSkip);

	FWacomCardPresentationRuntimeContext::FEffectPreview ShieldOverride;
	ShieldOverride.EffectIndex = 3;
	ShieldOverride.bHasMagnitude = true;
	ShieldOverride.Magnitude = 12;
	PreviewContext.EffectPreviews.Add(ShieldOverride);

	const FWacomCardViewData PreviewViewData =
		UWacomCardPresentationBuilder::BuildCardViewData(Card.Get(), PreviewContext);
	TestEqual(TEXT("Preview skip removes one effect badge"), PreviewViewData.EffectBadges.Num(), 3);
	TestBadge(*this, PreviewViewData.EffectBadges, 0, EWacomCardViewEffectBadgeKind::Damage, 9);
	TestBadge(*this, PreviewViewData.EffectBadges, 1, EWacomCardViewEffectBadgeKind::Heal, 5);
	TestBadge(*this, PreviewViewData.EffectBadges, 2, EWacomCardViewEffectBadgeKind::Shield, 12);

	Card->Description = FText::GetEmpty();
	const FWacomCardDetailViewData RuntimeDetail =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get(), RuntimeContext);
	TestTrue(TEXT("Runtime detail emits semantic sections"), RuntimeDetail.Sections.Num() > 0);

	const FWacomCardDetailViewData PreviewDetail =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get(), PreviewContext);
	TestTrue(TEXT("Preview detail run records damage override"),
		HasDetailValueRun(PreviewDetail, 5, 9));
	TestTrue(TEXT("Preview detail run marks skipped poison status"),
		HasSkippedDetailStatusRun(PreviewDetail, WacomTags::Status_Poison));
	TestTrue(TEXT("Preview detail run records shield override"),
		HasDetailValueRun(PreviewDetail, 6, 12));

	return true;
}
