// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardDetailTheme.h"
#include "UI/Card/WacomCardExplanationLexicon.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"

namespace
{
	UWacomCardExplanationLexicon* LoadConfiguredLexicon()
	{
		const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
		if (Settings && !Settings->CardExplanationLexicon.IsNull())
		{
			return Settings->CardExplanationLexicon.LoadSynchronous();
		}
		return nullptr;
	}

	UWacomCardDetailTheme* LoadConfiguredTheme()
	{
		const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
		if (Settings && !Settings->CardDetailTheme.IsNull())
		{
			return Settings->CardDetailTheme.LoadSynchronous();
		}
		return nullptr;
	}

	bool TestEffectTemplate(
		FAutomationTestBase& Test,
		const UWacomCardExplanationLexicon& Lexicon,
		const FGameplayTag& EffectTag)
	{
		FWacomCardExplanationTemplateEntry Entry;
		const bool bFound = Lexicon.FindEffectTemplate(EffectTag, Entry);
		Test.TestTrue(
			FString::Printf(TEXT("Lexicon has effect template for %s"), *EffectTag.ToString()),
			bFound);
		Test.TestFalse(
			FString::Printf(TEXT("Effect template for %s is not empty"), *EffectTag.ToString()),
			bFound ? Entry.Template.IsEmpty() : true);
		return bFound && !Entry.Template.IsEmpty();
	}

	bool TestPassiveTriggerTemplate(
		FAutomationTestBase& Test,
		const UWacomCardExplanationLexicon& Lexicon,
		const FGameplayTag& TriggerTag)
	{
		FWacomCardExplanationTemplateEntry Entry;
		const bool bFound = Lexicon.FindPassiveTriggerTemplate(TriggerTag, Entry);
		Test.TestTrue(
			FString::Printf(TEXT("Lexicon has passive trigger template for %s"), *TriggerTag.ToString()),
			bFound);
		Test.TestFalse(
			FString::Printf(TEXT("Passive trigger template for %s is not empty"), *TriggerTag.ToString()),
			bFound ? Entry.Template.IsEmpty() : true);
		return bFound && !Entry.Template.IsEmpty();
	}

	bool TestMagnitudeSourceTemplate(
		FAutomationTestBase& Test,
		const UWacomCardExplanationLexicon& Lexicon,
		const FGameplayTag& SourceTag)
	{
		FWacomCardExplanationTemplateEntry Entry;
		const bool bFound = Lexicon.FindMagnitudeSourceTemplate(SourceTag, Entry);
		Test.TestTrue(
			FString::Printf(TEXT("Lexicon has magnitude source template for %s"), *SourceTag.ToString()),
			bFound);
		Test.TestFalse(
			FString::Printf(TEXT("Magnitude source template for %s is not empty"), *SourceTag.ToString()),
			bFound ? Entry.Template.IsEmpty() : true);
		return bFound && !Entry.Template.IsEmpty();
	}

	bool TestTagDisplayName(
		FAutomationTestBase& Test,
		const UWacomCardExplanationLexicon& Lexicon,
		const FGameplayTag& Tag)
	{
		FText DisplayName;
		const bool bFound = Lexicon.FindTagDisplayName(Tag, DisplayName);
		Test.TestTrue(
			FString::Printf(TEXT("Lexicon has display name for %s"), *Tag.ToString()),
			bFound);
		Test.TestFalse(
			FString::Printf(TEXT("Display name for %s is not empty"), *Tag.ToString()),
			bFound ? DisplayName.IsEmpty() : true);
		return bFound && !DisplayName.IsEmpty();
	}

	bool TestNamedText(
		FAutomationTestBase& Test,
		const UWacomCardExplanationLexicon& Lexicon,
		const FName Key)
	{
		FText Text;
		const bool bFound = Lexicon.FindNamedText(Key, Text);
		Test.TestTrue(
			FString::Printf(TEXT("Lexicon has named text %s"), *Key.ToString()),
			bFound);
		Test.TestFalse(
			FString::Printf(TEXT("Named text %s is not empty"), *Key.ToString()),
			bFound ? Text.IsEmpty() : true);
		return bFound && !Text.IsEmpty();
	}

	bool TestStyleRow(
		FAutomationTestBase& Test,
		const UDataTable& StyleSet,
		const FName RowName)
	{
		const bool bFound = StyleSet.GetRowMap().Contains(RowName);
		Test.TestTrue(
			FString::Printf(TEXT("Card detail RichText style set has row %s"), *RowName.ToString()),
			bFound);
		return bFound;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardDetailDefaultContentAssetsSpec,
	"Wacom.UI.CardDetail.Assets.DefaultContent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardDetailDefaultContentAssetsSpec::RunTest(const FString& /*Parameters*/)
{
	const UWacomCardExplanationLexicon* Lexicon = LoadConfiguredLexicon();
	if (!TestNotNull(TEXT("Configured CardExplanationLexicon loads"), Lexicon))
	{
		return false;
	}

	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_Damage);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_Heal);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Status_Shield);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_Draw);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_Discard);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_ExhaustSelf);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_GainKeyword);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_RemoveStatus);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_ModifyInitiative);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_ApplyStatus_Poison);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_ApplyStatus_Slow);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_ApplyStatus_Freeze);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_ApplyStatus_Twilight);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_Shuffle_Random);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_Shuffle_FromBothToOther);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_Shuffle_ToRandomZone);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_Card_AddCost);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_Card_ReduceCost);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_Card_DiscardSelected);
	TestEffectTemplate(*this, *Lexicon, WacomTags::Effect_Card_ExhaustSelected);

	TestPassiveTriggerTemplate(*this, *Lexicon, WacomTags::Passive_Trigger_AfterPlayed);
	TestPassiveTriggerTemplate(*this, *Lexicon, WacomTags::Passive_Trigger_OnCompanionCount);
	TestPassiveTriggerTemplate(*this, *Lexicon, WacomTags::Passive_Trigger_OnTwilightTriggered);
	TestPassiveTriggerTemplate(*this, *Lexicon, WacomTags::Passive_Trigger_OnTurnStart);
	TestPassiveTriggerTemplate(*this, *Lexicon, WacomTags::Passive_Trigger_OnTurnEnd);
	TestPassiveTriggerTemplate(*this, *Lexicon, WacomTags::Passive_Trigger_OnDraw);
	TestPassiveTriggerTemplate(*this, *Lexicon, WacomTags::Passive_Trigger_OnDiscard);

	FWacomCardExplanationTemplateEntry PassiveOutcomeEntry;
	TestTrue(TEXT("Lexicon has companion-count passive outcome template"),
		Lexicon->FindPassiveOutcomeTemplate(WacomTags::Passive_Trigger_OnCompanionCount, PassiveOutcomeEntry));
	TestFalse(TEXT("Companion-count passive outcome template is not empty"),
		PassiveOutcomeEntry.Template.IsEmpty());

	TestMagnitudeSourceTemplate(*this, *Lexicon, WacomTags::Magnitude_Source_RuntimeCost);
	TestMagnitudeSourceTemplate(*this, *Lexicon, WacomTags::Magnitude_Source_TargetStatusStacks);
	TestMagnitudeSourceTemplate(*this, *Lexicon, WacomTags::Magnitude_Source_HandCount);

	TestTagDisplayName(*this, *Lexicon, WacomTags::HandZone_Left);
	TestTagDisplayName(*this, *Lexicon, WacomTags::HandZone_Both);
	TestTagDisplayName(*this, *Lexicon, WacomTags::HandZone_Right);
	TestTagDisplayName(*this, *Lexicon, WacomTags::Status_Poison);
	TestTagDisplayName(*this, *Lexicon, WacomTags::Status_Slow);
	FText SlowDisplayName;
	TestTrue(TEXT("Lexicon resolves Slow display name"),
		Lexicon->FindTagDisplayName(WacomTags::Status_Slow, SlowDisplayName));
	TestEqual(TEXT("Slow uses the canonical Chinese display name"),
		SlowDisplayName.ToString(), FString(TEXT("减速")));
	TestTagDisplayName(*this, *Lexicon, WacomTags::Status_Freeze);
	TestTagDisplayName(*this, *Lexicon, WacomTags::Status_Twilight);
	TestTagDisplayName(*this, *Lexicon, WacomTags::Status_Stunned);
	TestTagDisplayName(*this, *Lexicon, WacomTags::Status_Shield);

	TestNamedText(*this, *Lexicon, WacomCardExplanationLexiconKeys::SectionDescriptionTitle);
	TestNamedText(*this, *Lexicon, WacomCardExplanationLexiconKeys::SectionPassiveTitle);
	TestNamedText(*this, *Lexicon, WacomCardExplanationLexiconKeys::DetailSkipPrefix);
	TestNamedText(*this, *Lexicon, WacomCardExplanationLexiconKeys::ConditionSelfInZone);
	TestNamedText(*this, *Lexicon, WacomCardExplanationLexiconKeys::ConditionTargetHasStatus);
	TestNamedText(*this, *Lexicon, WacomCardExplanationLexiconKeys::ModifierConditional);

	const UWacomCardDetailTheme* Theme = LoadConfiguredTheme();
	if (!TestNotNull(TEXT("Configured CardDetailTheme loads"), Theme))
	{
		return false;
	}

	TestNotNull(TEXT("Card detail theme has BodyTextStyleSet"), Theme->BodyTextStyleSet.Get());
	if (Theme->BodyTextStyleSet)
	{
		TestStyleRow(*this, *Theme->BodyTextStyleSet, TEXT("Default"));
		TestStyleRow(*this, *Theme->BodyTextStyleSet, TEXT("Value"));
		TestStyleRow(*this, *Theme->BodyTextStyleSet, TEXT("ValueBuffed"));
		TestStyleRow(*this, *Theme->BodyTextStyleSet, TEXT("ValueNerfed"));
		TestStyleRow(*this, *Theme->BodyTextStyleSet, TEXT("Status"));
		TestStyleRow(*this, *Theme->BodyTextStyleSet, TEXT("Keyword"));
		TestStyleRow(*this, *Theme->BodyTextStyleSet, TEXT("Muted"));
	}

	TestNotNull(TEXT("Card detail theme resolves Damage icon brush or fallback"),
		Theme->ResolveIconBrush(EWacomCardDetailIcon::Damage));
	TestNotNull(TEXT("Card detail theme resolves Heal icon brush or fallback"),
		Theme->ResolveIconBrush(EWacomCardDetailIcon::Heal));
	TestNotNull(TEXT("Card detail theme resolves Shield icon brush or fallback"),
		Theme->ResolveIconBrush(EWacomCardDetailIcon::Shield));
	TestNotNull(TEXT("Card detail theme resolves Poison status brush or fallback"),
		Theme->ResolveStatusBrush(WacomTags::Status_Poison));
	TestNotNull(TEXT("Card detail theme resolves Slow status brush or fallback"),
		Theme->ResolveStatusBrush(WacomTags::Status_Slow));
	TestNotNull(TEXT("Card detail theme resolves Freeze status brush or fallback"),
		Theme->ResolveStatusBrush(WacomTags::Status_Freeze));
	TestNotNull(TEXT("Card detail theme resolves Twilight status brush or fallback"),
		Theme->ResolveStatusBrush(WacomTags::Status_Twilight));
	TestNotNull(TEXT("Card detail theme resolves Stunned status brush or fallback"),
		Theme->ResolveStatusBrush(WacomTags::Status_Stunned));
	TestTrue(TEXT("Card detail inline icon render offset is finite"),
		FMath::IsFinite(Theme->InlineIconRenderOffset.X) &&
		FMath::IsFinite(Theme->InlineIconRenderOffset.Y));
	TestTrue(TEXT("Card detail inline icon render offset is in a reasonable visual range"),
		FMath::Abs(Theme->InlineIconRenderOffset.X) <= 16.0 &&
		FMath::Abs(Theme->InlineIconRenderOffset.Y) <= 16.0);

	return true;
}
