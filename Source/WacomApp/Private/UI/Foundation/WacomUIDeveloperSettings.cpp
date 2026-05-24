// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomUIDeveloperSettings.h"

#include "UI/Foundation/WacomActivatableWidget.h"
#include "UI/Foundation/WacomAppToastWidget.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"

#include "UObject/SoftObjectPath.h"

#define LOCTEXT_NAMESPACE "WacomUIDeveloperSettings"

namespace
{
	const FString UIWidgetTagPrefix(TEXT("UI.Widget."));

	bool IsUIWidgetTag(const FGameplayTag& WidgetTag)
	{
		return WidgetTag.IsValid() && WidgetTag.ToString().StartsWith(UIWidgetTagPrefix);
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

		ValidateSoftClass(
			Entry.WidgetClass,
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
