// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../WacomApp/Private/UI/Battle/WacomBattleStatusPresentationCatalogProvider.h"
#include "../../../WacomApp/Private/UI/Battle/WacomBattleStatusTooltipPresentation.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopeExit.h"
#include "Statuses/BattleStatusRuleConstants.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleCombatActivityStyle.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UI/Battle/WacomBattleStatusPresentationCatalog.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace WacomBattleStatusPresentationCatalogSpec
{
	void InstallTransientIconResources(
		UWacomBattleStatusPresentationCatalog& Catalog)
	{
		UTexture2D* Texture = NewObject<UTexture2D>(
			&Catalog,
			TEXT("TransientStatusIcon"),
			RF_Transient);
		Catalog.FallbackIconBrush.SetResourceObject(Texture);
		Catalog.FallbackIconBrush.SetImageSize(FVector2f(32.0f, 32.0f));
		for (FWacomBattleStatusPresentationEntry& Entry : Catalog.Entries)
		{
			if (Entry.StatusTag != WacomTags::Status_Shield)
			{
				Entry.IconBrush.SetResourceObject(Texture);
				Entry.IconBrush.SetImageSize(FVector2f(32.0f, 32.0f));
			}
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleStatusPresentationCatalogResolutionSpec,
	"Wacom.UI.Battle.StatusPresentationCatalog.ResolvesCanonicalAliasesAndFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleStatusPresentationCatalogResolutionSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleStatusPresentationCatalog> Catalog(
		NewObject<UWacomBattleStatusPresentationCatalog>());

	const FWacomBattleStatusPresentationEntry* Poison =
		Catalog->FindEntry(WacomTags::Status_Poison);
	if (!TestNotNull(TEXT("Poison entry exists"), Poison))
	{
		return false;
	}
	TestTrue(TEXT("Effect alias resolves to canonical poison entry"),
		Catalog->FindEntry(WacomTags::Effect_ApplyStatus_Poison) == Poison);
	TestEqual(TEXT("Poison display name"), Catalog->ResolveDisplayName(
		WacomTags::Status_Poison).ToString(), FString(TEXT("中毒")));
	TestEqual(TEXT("Legacy event formatter delegates aliases to Catalog"),
		UWacomBattleEventPresentationBuilder::FormatStatusName(
			WacomTags::Effect_ApplyStatus_Poison),
		FString(TEXT("中毒")));
	TestEqual(TEXT("Poison sorts before stunned"),
		Catalog->ResolveSortPriority(WacomTags::Status_Poison) <
			Catalog->ResolveSortPriority(WacomTags::Status_Stunned),
		true);

	const FGameplayTag Unknown =
		FGameplayTag::RequestGameplayTag(TEXT("Status.AutomationUnknown"), false);
	TestEqual(TEXT("Invalid unknown name uses generic fallback"),
		Catalog->ResolveDisplayName(FGameplayTag()).ToString(),
		FString(TEXT("状态")));
	if (Unknown.IsValid())
	{
		TestEqual(TEXT("Known gameplay tag without an entry stays readable"),
			Catalog->ResolveDisplayName(Unknown).ToString(),
			Unknown.ToString());
		TestTrue(TEXT("Unknown status uses fallback icon"),
			Catalog->ResolveIconBrush(Unknown) == &Catalog->FallbackIconBrush);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleStatusPresentationCatalogCombatActivityIconSpec,
	"Wacom.UI.Battle.StatusPresentationCatalog.CombatActivityUsesCatalogBeforeLegacyTagMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleStatusPresentationCatalogCombatActivityIconSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCombatActivityStyle> Style(
		NewObject<UWacomBattleCombatActivityStyle>());
	UTexture2D* LegacyTexture = NewObject<UTexture2D>(
		Style.Get(),
		TEXT("LegacyPoisonIcon"),
		RF_Transient);
	FWacomBattleCombatActivityTagIconEntry LegacyEntry;
	LegacyEntry.Tag = WacomTags::Status_Poison;
	LegacyEntry.IconBrush.SetResourceObject(LegacyTexture);
	LegacyEntry.IconBrush.SetImageSize(FVector2f(32.0f, 32.0f));
	Style->TagIcons.Add(LegacyEntry);

	FWacomBattleCombatActivityRowView Row;
	Row.RowKind = EWacomBattleCombatActivityRowKind::Result;
	Row.IconTag = WacomTags::Status_Poison;
	const FSlateBrush Resolved = Style->ResolveActivityIconBrush(Row);
	TestTrue(TEXT("Legacy status TagIcons entry is not used"),
		Resolved.GetResourceObject() != LegacyTexture);
	TestTrue(TEXT("Catalog fallback brush remains renderable"),
		UWacomBattleStatusPresentationCatalog::IsIconBrushRenderable(Resolved));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleStatusPresentationCatalogHostRulesSpec,
	"Wacom.UI.Battle.StatusPresentationCatalog.HostRulesUseBattleConstants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleStatusPresentationCatalogHostRulesSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleStatusIconView PlayerView;
	PlayerView.StatusTag = WacomTags::Status_Poison;
	PlayerView.InspectionHost = EWacomBattleStatusInspectionHost::Player;
	FWacomBattleStatusTooltipPresentationBuilder::PopulateRuleText(PlayerView);
	TestTrue(TEXT("Poison damage comes from shared Battle constant"),
		PlayerView.CoreEffectText.ToString().Contains(FString::FromInt(
			WacomBattleStatusRuleConstants::PoisonDamagePerStack)));
	TestTrue(TEXT("Player poison removal ratio comes from shared Battle constant"),
		PlayerView.StackPolicyText.ToString().Contains(FString::FromInt(
			FMath::RoundToInt(
				WacomBattleStatusRuleConstants::PlayerHealPoisonRemovalRatio
					* 100.0f))));

	FWacomBattleStatusIconView EnemyView;
	EnemyView.StatusTag = WacomTags::Status_Slow;
	EnemyView.InspectionHost =
		EWacomBattleStatusInspectionHost::EnemyPart;
	FWacomBattleStatusTooltipPresentationBuilder::PopulateRuleText(EnemyView);
	TestTrue(TEXT("Enemy slow uses enemy-host semantics"),
		EnemyView.CoreEffectText.ToString().Contains(TEXT("当前意图")));

	FWacomBattleStatusIconView UnknownHostView;
	UnknownHostView.StatusTag = WacomTags::Status_Slow;
	UnknownHostView.InspectionHost =
		EWacomBattleStatusInspectionHost::Unknown;
	FWacomBattleStatusTooltipPresentationBuilder::PopulateRuleText(
		UnknownHostView);
	TestTrue(TEXT("Unknown host does not pretend to use player rules"),
		UnknownHostView.CoreEffectText.ToString().Contains(TEXT("暂无详细规则")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleStatusPresentationCatalogProviderFallbackSpec,
	"Wacom.UI.Battle.StatusPresentationCatalog.MissingConfigUsesReadableCdoFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleStatusPresentationCatalogProviderFallbackSpec::RunTest(
	const FString& /*Parameters*/)
{
	UWacomUIDeveloperSettings* Settings =
		GetMutableDefault<UWacomUIDeveloperSettings>();
	const TSoftObjectPtr<UWacomBattleStatusPresentationCatalog> SavedCatalog =
		Settings->BattleStatusPresentationCatalog;
	ON_SCOPE_EXIT
	{
		Settings->BattleStatusPresentationCatalog = SavedCatalog;
		WacomBattleStatusPresentationCatalogProvider::ClearCachedCatalogForTests();
	};

	Settings->BattleStatusPresentationCatalog.Reset();
	WacomBattleStatusPresentationCatalogProvider::ClearCachedCatalogForTests();
	const UWacomBattleStatusPresentationCatalog& Fallback =
		WacomBattleStatusPresentationCatalogProvider::GetCatalog();
	TestTrue(TEXT("Provider returns Catalog CDO when configuration is absent"),
		&Fallback == GetDefault<UWacomBattleStatusPresentationCatalog>());
	TestEqual(TEXT("CDO fallback stays readable"),
		Fallback.ResolveDisplayName(WacomTags::Status_Freeze).ToString(),
		FString(TEXT("冻结")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleStatusPresentationCatalogValidationSpec,
	"Wacom.UI.Battle.StatusPresentationCatalog.ValidationRejectsAliasAndTemplateErrors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleStatusPresentationCatalogValidationSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleStatusPresentationCatalogSpec;

	TStrongObjectPtr<UWacomBattleStatusPresentationCatalog> Catalog(
		NewObject<UWacomBattleStatusPresentationCatalog>());
	InstallTransientIconResources(*Catalog);

	FDataValidationContext ValidContext;
	TestEqual(TEXT("Complete transient catalog validates"),
		Catalog->IsDataValid(ValidContext),
		EDataValidationResult::Valid);

	Catalog->Entries[1].LookupAliases.Add(
		WacomTags::Effect_ApplyStatus_Poison);
	Catalog->Entries[0].PlayerRules.CoreEffectText =
		FText::FromString(TEXT("非法 {UnsupportedStatusValue}"));
	FDataValidationContext InvalidContext;
	TestEqual(TEXT("Duplicate alias or unsupported parameter is rejected"),
		Catalog->IsDataValid(InvalidContext),
		EDataValidationResult::Invalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleStatusPresentationCatalogOwnsLegacyWidgetFieldsSpec,
	"Wacom.UI.Battle.StatusPresentationCatalog.LegacyWidgetPresentationFieldsAreRemoved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleStatusPresentationCatalogOwnsLegacyWidgetFieldsSpec::RunTest(
	const FString& /*Parameters*/)
{
	const FName IconLegacyProperties[] = {
		TEXT("PreviewDisplayName"),
		TEXT("PreviewIconBrush"),
	};
	for (const FName PropertyName : IconLegacyProperties)
	{
		TestNull(
			*FString::Printf(TEXT("Status icon no longer owns %s"),
				*PropertyName.ToString()),
			FindFProperty<FProperty>(
				UWacomBattleStatusIconWidget::StaticClass(),
				PropertyName));
	}

	const FName ListLegacyProperties[] = {
		TEXT("PoisonIconBrush"),
		TEXT("SlowIconBrush"),
		TEXT("FreezeIconBrush"),
		TEXT("TwilightIconBrush"),
		TEXT("StunnedIconBrush"),
		TEXT("FallbackStatusIconBrush"),
	};
	for (const FName PropertyName : ListLegacyProperties)
	{
		TestNull(
			*FString::Printf(TEXT("Status list no longer owns %s"),
				*PropertyName.ToString()),
			FindFProperty<FProperty>(
				UWacomBattleStatusIconListWidget::StaticClass(),
				PropertyName));
	}
	return true;
}
