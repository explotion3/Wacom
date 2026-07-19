// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomUIDeveloperSettings.h"

#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyInspectionWidget.h"
#include "UI/Battle/WacomBattleCombatLogDetailsScreen.h"
#include "UI/Card/WacomCardDetailTheme.h"
#include "UI/Card/WacomCardExplanationLexicon.h"
#include "UI/Events/WacomRunEventScreen.h"
#include "UI/Foundation/WacomActivatableWidget.h"
#include "UI/Foundation/WacomAppToastWidget.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/Menus/WacomPauseMenuScreen.h"
#include "UI/Settings/WacomSettingsScreen.h"
#include "UI/Shop/WacomShopScreen.h"
#include "UI/Map/WacomRunMapScreen.h"
#include "Tags/WacomGameplayTags.h"

#include "UObject/SoftObjectPath.h"

#define LOCTEXT_NAMESPACE "WacomUIDeveloperSettings"

namespace
{
	const FString UIWidgetTagPrefix(TEXT("UI.Widget."));

	bool IsUIWidgetTag(const FGameplayTag& WidgetTag)
	{
		return WidgetTag.IsValid() && WidgetTag.ToString().StartsWith(UIWidgetTagPrefix);
	}

	UClass* GetExpectedWidgetClassParent(const FGameplayTag& WidgetTag)
	{
		if (WidgetTag == WacomUITags::UI_Widget_BackpackScreen.GetTag())
		{
			return UWacomBackpackScreen::StaticClass();
		}
		if (WidgetTag == WacomUITags::UI_Widget_ShopScreen.GetTag())
		{
			return UWacomShopScreen::StaticClass();
		}
		if (WidgetTag == WacomUITags::UI_Widget_RunEventScreen.GetTag())
		{
			return UWacomRunEventScreen::StaticClass();
		}
		if (WidgetTag == WacomUITags::UI_Widget_PauseMenuScreen.GetTag())
		{
			return UWacomPauseMenuScreen::StaticClass();
		}
		if (WidgetTag == WacomUITags::UI_Widget_SettingsScreen.GetTag())
		{
			return UWacomSettingsScreen::StaticClass();
		}
		if (WidgetTag == WacomUITags::UI_Widget_BattleCombatLogDetailsScreen.GetTag())
		{
			return UWacomBattleCombatLogDetailsScreen::StaticClass();
		}
		if (WidgetTag == WacomTags::UI_Widget_RunMapScreen.GetTag())
		{
			return UWacomRunMapScreen::StaticClass();
		}
		return UWacomActivatableWidget::StaticClass();
	}

	template<typename ExpectedT>
	void ValidateSoftObject(
		const TSoftObjectPtr<ExpectedT>& SoftObject,
		const FText& FieldLabel,
		TArray<FText>& OutErrors)
	{
		if (SoftObject.IsNull())
		{
			return;
		}

		UObject* LoadedObject = SoftObject.ToSoftObjectPath().TryLoad();
		if (!LoadedObject)
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("SoftObjectLoadFailed", "{0} 加载失败：{1}"),
				FieldLabel,
				FText::FromString(SoftObject.ToString())));
			return;
		}

		if (!LoadedObject->IsA(ExpectedT::StaticClass()))
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("SoftObjectWrongClass", "{0} 必须是 {1}，当前为 {2}"),
				FieldLabel,
				FText::FromString(ExpectedT::StaticClass()->GetName()),
				FText::FromString(LoadedObject->GetClass()->GetName())));
		}
	}

	template<typename ExpectedT>
	void ValidateSoftClass(
		const TSoftClassPtr<ExpectedT>& SoftClass,
		const FText& FieldLabel,
		TArray<FText>& OutErrors)
	{
		if (SoftClass.IsNull())
		{
			return;
		}

		UObject* LoadedObject = SoftClass.ToSoftObjectPath().TryLoad();
		UClass* LoadedClass = Cast<UClass>(LoadedObject);
		if (!LoadedClass)
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("SoftClassLoadFailed", "{0} 加载失败或不是 UClass：{1}"),
				FieldLabel,
				FText::FromString(SoftClass.ToString())));
			return;
		}

		if (!LoadedClass->IsChildOf(ExpectedT::StaticClass()))
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("SoftClassWrongParent", "{0} 必须继承 {1}，当前为 {2}"),
				FieldLabel,
				FText::FromString(ExpectedT::StaticClass()->GetName()),
				FText::FromString(LoadedClass->GetName())));
		}
	}

	void ValidateSoftWidgetClass(
		const TSoftClassPtr<UWacomActivatableWidget>& SoftClass,
		UClass* ExpectedParentClass,
		const FText& FieldLabel,
		TArray<FText>& OutErrors)
	{
		if (SoftClass.IsNull())
		{
			return;
		}

		if (!ExpectedParentClass)
		{
			ExpectedParentClass = UWacomActivatableWidget::StaticClass();
		}

		UObject* LoadedObject = SoftClass.ToSoftObjectPath().TryLoad();
		UClass* LoadedClass = Cast<UClass>(LoadedObject);
		if (!LoadedClass)
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("SoftWidgetClassLoadFailed", "{0} 加载失败或不是 UClass：{1}"),
				FieldLabel,
				FText::FromString(SoftClass.ToString())));
			return;
		}

		if (!LoadedClass->IsChildOf(ExpectedParentClass))
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("SoftWidgetClassWrongParent", "{0} 必须继承 {1}，当前为 {2}"),
				FieldLabel,
				FText::FromString(ExpectedParentClass->GetName()),
				FText::FromString(LoadedClass->GetName())));
		}
	}
}

bool UWacomUIDeveloperSettings::ValidateSettings(TArray<FText>& OutErrors) const
{
	ValidateSoftClass(
		PrimaryLayoutClass,
		LOCTEXT("PrimaryLayoutClassLabel", "PrimaryLayoutClass"),
		OutErrors);

	ValidateSoftClass(
		AppToastWidgetClass,
		LOCTEXT("AppToastWidgetClassLabel", "AppToastWidgetClass"),
		OutErrors);
	if (DefaultBattleEnemyPanelWidgetClass.IsNull())
	{
		OutErrors.Add(LOCTEXT(
			"DefaultBattleEnemyPanelWidgetClassNull",
			"DefaultBattleEnemyPanelWidgetClass 不能为空；Scene Enemy Host 需要正式聚合面板 WBP。"));
	}
	else
	{
		ValidateSoftClass(
			DefaultBattleEnemyPanelWidgetClass,
			LOCTEXT("DefaultBattleEnemyPanelWidgetClassLabel", "DefaultBattleEnemyPanelWidgetClass"),
			OutErrors);
		if (UClass* PanelClass = DefaultBattleEnemyPanelWidgetClass.LoadSynchronous();
			PanelClass && PanelClass->HasAnyClassFlags(
				CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("DefaultBattleEnemyPanelWidgetClassNotConstructible",
					"DefaultBattleEnemyPanelWidgetClass 必须是可实例化的正式 WBP，当前类 {0} 为 abstract、deprecated 或已被新版本替代。"),
				FText::FromString(PanelClass->GetPathName())));
		}
	}
	if (DefaultBattleEnemySinglePartPanelWidgetClass.IsNull())
	{
		OutErrors.Add(LOCTEXT(
			"DefaultBattleEnemySinglePartPanelWidgetClassNull",
			"DefaultBattleEnemySinglePartPanelWidgetClass 不能为空；单部位 Scene Enemy Host 需要正式紧凑面板 WBP。"));
	}
	else
	{
		ValidateSoftClass(
			DefaultBattleEnemySinglePartPanelWidgetClass,
			LOCTEXT("DefaultBattleEnemySinglePartPanelWidgetClassLabel", "DefaultBattleEnemySinglePartPanelWidgetClass"),
			OutErrors);
		if (UClass* PanelClass = DefaultBattleEnemySinglePartPanelWidgetClass.LoadSynchronous();
			PanelClass && PanelClass->HasAnyClassFlags(
				CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("DefaultBattleEnemySinglePartPanelWidgetClassNotConstructible",
					"DefaultBattleEnemySinglePartPanelWidgetClass 必须是可实例化的正式 WBP，当前类 {0} 为 abstract、deprecated 或已被新版本替代。"),
				FText::FromString(PanelClass->GetPathName())));
		}
	}
	if (DefaultBattleEnemyInspectionWidgetClass.IsNull())
	{
		OutErrors.Add(LOCTEXT(
			"DefaultBattleEnemyInspectionWidgetClassNull",
			"DefaultBattleEnemyInspectionWidgetClass 不能为空；Scene Enemy 点击检查需要正式双侧详情 WBP。"));
	}
	else
	{
		ValidateSoftClass(
			DefaultBattleEnemyInspectionWidgetClass,
			LOCTEXT("DefaultBattleEnemyInspectionWidgetClassLabel", "DefaultBattleEnemyInspectionWidgetClass"),
			OutErrors);
		if (UClass* InspectionClass = DefaultBattleEnemyInspectionWidgetClass.LoadSynchronous();
			InspectionClass && InspectionClass->HasAnyClassFlags(
				CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("DefaultBattleEnemyInspectionWidgetClassNotConstructible",
					"DefaultBattleEnemyInspectionWidgetClass 必须是可实例化的正式 WBP，当前类 {0} 不可实例化。"),
				FText::FromString(InspectionClass->GetPathName())));
		}
	}
	ValidateSoftObject(
		CardExplanationLexicon,
		LOCTEXT("CardExplanationLexiconLabel", "CardExplanationLexicon"),
		OutErrors);
	ValidateSoftObject(
		CardDetailTheme,
		LOCTEXT("CardDetailThemeLabel", "CardDetailTheme"),
		OutErrors);

	TSet<FGameplayTag> SeenWidgetTags;
	for (int32 Index = 0; Index < WidgetClasses.Num(); ++Index)
	{
		const FWacomUIWidgetClassEntry& Entry = WidgetClasses[Index];
		const FText EntryLabel = FText::Format(
			LOCTEXT("WidgetClassesEntryLabel", "WidgetClasses[{0}]"),
			FText::AsNumber(Index));

		if (!Entry.WidgetTag.IsValid())
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("WidgetTagInvalid", "{0}.WidgetTag 无效"),
				EntryLabel));
		}
		else
		{
			if (!IsUIWidgetTag(Entry.WidgetTag))
			{
				OutErrors.Add(FText::Format(
					LOCTEXT("WidgetTagWrongNamespace", "{0}.WidgetTag 必须属于 UI.Widget.*，当前为 {1}"),
					EntryLabel,
					FText::FromString(Entry.WidgetTag.ToString())));
			}

			if (SeenWidgetTags.Contains(Entry.WidgetTag))
			{
				OutErrors.Add(FText::Format(
					LOCTEXT("WidgetTagDuplicate", "{0}.WidgetTag 重复：{1}"),
					EntryLabel,
					FText::FromString(Entry.WidgetTag.ToString())));
			}
			else
			{
				SeenWidgetTags.Add(Entry.WidgetTag);
			}
		}

		if (Entry.WidgetClass.IsNull())
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("WidgetClassNull", "{0}.WidgetClass 不能为空；需要 fallback 时请删除该 entry"),
				EntryLabel));
			continue;
		}

		ValidateSoftWidgetClass(
			Entry.WidgetClass,
			GetExpectedWidgetClassParent(Entry.WidgetTag),
			FText::Format(
				LOCTEXT("WidgetClassLabel", "{0}.WidgetClass"),
				EntryLabel),
			OutErrors);
	}

	return OutErrors.IsEmpty();
}

#if WITH_EDITOR
EDataValidationResult UWacomUIDeveloperSettings::IsDataValid(FDataValidationContext& Context) const
{
	TArray<FText> Errors;
	if (ValidateSettings(Errors))
	{
		return EDataValidationResult::Valid;
	}

	for (const FText& Error : Errors)
	{
		Context.AddError(Error);
	}

	return EDataValidationResult::Invalid;
}
#endif

#undef LOCTEXT_NAMESPACE
