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

	FName ExpectedBadgePresentationKey(EWacomCardViewEffectBadgeKind Kind)
	{
		switch (Kind)
		{
		case EWacomCardViewEffectBadgeKind::Damage: return TEXT("Badge.Damage");
		case EWacomCardViewEffectBadgeKind::Poison: return TEXT("Badge.Poison");
		case EWacomCardViewEffectBadgeKind::Heal: return TEXT("Badge.Heal");
		case EWacomCardViewEffectBadgeKind::Shield: return TEXT("Badge.Shield");
		default: return NAME_None;
		}
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
		Test.TestEqual(TEXT("Badge identity follows its semantic kind"),
			Badges[Index].PresentationKey, ExpectedBadgePresentationKey(ExpectedKind));
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

	bool HasAuthoritativeDetailValueRun(
		const FWacomCardDetailViewData& DetailData,
		int32 ExpectedValue)
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
						&& !Run.bHasPreviewValue)
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

	FWacomCardPresentationRuntimeContext CurrentMagnitudeContext = RuntimeContext;
	FWacomCardPresentationRuntimeContext::FCurrentEffectMagnitude CurrentPoison;
	CurrentPoison.EffectIndex = 1;
	CurrentPoison.Magnitude = 8;
	CurrentMagnitudeContext.CurrentEffectMagnitudes.Add(CurrentPoison);
	const FWacomCardViewData CurrentMagnitudeViewData =
		UWacomCardPresentationBuilder::BuildCardViewData(
			Card.Get(),
			CurrentMagnitudeContext);
	TestBadge(*this,
		CurrentMagnitudeViewData.EffectBadges,
		1,
		EWacomCardViewEffectBadgeKind::Poison,
		8);

	Card->Description = FText::GetEmpty();
	const FWacomCardDetailViewData CurrentMagnitudeDetail =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(
			Card.Get(),
			CurrentMagnitudeContext);
	TestTrue(TEXT("Detail uses authoritative current effect magnitude without preview"),
		HasAuthoritativeDetailValueRun(CurrentMagnitudeDetail, 8));

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
	TestEqual(TEXT("Preview keeps all authoritative effect badges"), PreviewViewData.EffectBadges.Num(), 4);
	TestBadge(*this, PreviewViewData.EffectBadges, 0, EWacomCardViewEffectBadgeKind::Damage, 5);
	TestTrue(TEXT("Damage preview has a predicted value"), PreviewViewData.EffectBadges[0].bHasPreviewValue);
	TestEqual(TEXT("Damage preview value remains separate"), PreviewViewData.EffectBadges[0].PreviewValue, 9);
	TestBadge(*this, PreviewViewData.EffectBadges, 1, EWacomCardViewEffectBadgeKind::Poison, 5);
	TestTrue(TEXT("Skipped poison remains visible"), PreviewViewData.EffectBadges[1].bPreviewSkipped);
	TestFalse(TEXT("Skipped poison does not invent a new numeric value"), PreviewViewData.EffectBadges[1].bHasPreviewValue);
	TestBadge(*this, PreviewViewData.EffectBadges, 2, EWacomCardViewEffectBadgeKind::Heal, 5);
	TestBadge(*this, PreviewViewData.EffectBadges, 3, EWacomCardViewEffectBadgeKind::Shield, 6);
	TestTrue(TEXT("Shield preview has a predicted value"), PreviewViewData.EffectBadges[3].bHasPreviewValue);
	TestEqual(TEXT("Shield preview value remains separate"), PreviewViewData.EffectBadges[3].PreviewValue, 12);

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

	TStrongObjectPtr<UCardDefinition> ConditionalCard(NewObject<UCardDefinition>());
	ConditionalCard->CardId = TEXT("UI.CardPresentation.AggregatedDamage");
	FCardEffect BaseDamage;
	BaseDamage.EffectType = WacomTags::Effect_Damage;
	BaseDamage.Magnitude = 4;
	FCardEffect ConditionalDamage;
	ConditionalDamage.EffectType = WacomTags::Effect_Damage;
	ConditionalDamage.Magnitude = 5;
	ConditionalDamage.Condition.ConditionType = WacomTags::Condition_Target_HasStatus;
	ConditionalDamage.Condition.ParamTag = WacomTags::Status_Poison;
	FCardEffect SeparatePoison;
	SeparatePoison.EffectType = WacomTags::Effect_ApplyStatus_Poison;
	SeparatePoison.Magnitude = 1;
	ConditionalCard->Effects = { BaseDamage, ConditionalDamage, SeparatePoison };

	const FWacomCardViewData ConditionalBaseView =
		UWacomCardPresentationBuilder::BuildCardViewData(ConditionalCard.Get());
	TestEqual(TEXT("Same-kind damage effects aggregate into one compact badge"),
		ConditionalBaseView.EffectBadges.Num(), 2);
	TestBadge(*this, ConditionalBaseView.EffectBadges, 0,
		EWacomCardViewEffectBadgeKind::Damage, 4);
	TestBadge(*this, ConditionalBaseView.EffectBadges, 1,
		EWacomCardViewEffectBadgeKind::Poison, 1);

	FWacomCardPresentationRuntimeContext SkippedConditionalContext;
	FWacomCardPresentationRuntimeContext::FEffectPreview BaseDamagePreview;
	BaseDamagePreview.EffectIndex = 0;
	BaseDamagePreview.bHasMagnitude = true;
	BaseDamagePreview.Magnitude = 4;
	FWacomCardPresentationRuntimeContext::FEffectPreview SkippedBonusPreview;
	SkippedBonusPreview.EffectIndex = 1;
	SkippedBonusPreview.bSkip = true;
	FWacomCardPresentationRuntimeContext::FEffectPreview PoisonPreview;
	PoisonPreview.EffectIndex = 2;
	PoisonPreview.bHasMagnitude = true;
	PoisonPreview.Magnitude = 1;
	SkippedConditionalContext.EffectPreviews = {
		BaseDamagePreview, SkippedBonusPreview, PoisonPreview };
	const FWacomCardViewData SkippedConditionalView =
		UWacomCardPresentationBuilder::BuildCardViewData(
			ConditionalCard.Get(), SkippedConditionalContext);
	TestEqual(TEXT("Skipped conditional damage does not create another badge"),
		SkippedConditionalView.EffectBadges.Num(), 2);
	TestEqual(TEXT("Skipped bonus keeps authoritative base damage"),
		SkippedConditionalView.EffectBadges[0].Value, 4);
	TestFalse(TEXT("One applicable contribution keeps the aggregate active"),
		SkippedConditionalView.EffectBadges[0].bPreviewSkipped);
	TestFalse(TEXT("Unchanged aggregate does not start a numeric preview"),
		SkippedConditionalView.EffectBadges[0].bHasPreviewValue);

	FWacomCardPresentationRuntimeContext AppliedConditionalContext = SkippedConditionalContext;
	AppliedConditionalContext.EffectPreviews[1].bSkip = false;
	AppliedConditionalContext.EffectPreviews[1].bHasMagnitude = true;
	AppliedConditionalContext.EffectPreviews[1].Magnitude = 5;
	const FWacomCardViewData AppliedConditionalView =
		UWacomCardPresentationBuilder::BuildCardViewData(
			ConditionalCard.Get(), AppliedConditionalContext);
	TestEqual(TEXT("Applicable conditional damage previews the semantic total"),
		AppliedConditionalView.EffectBadges[0].PreviewValue, 9);
	TestTrue(TEXT("Applicable conditional total uses reversible numeric preview"),
		AppliedConditionalView.EffectBadges[0].bHasPreviewValue);
	TestEqual(TEXT("Poison remains a separate semantic badge"),
		AppliedConditionalView.EffectBadges[1].Kind,
		EWacomCardViewEffectBadgeKind::Poison);

	return true;
}
