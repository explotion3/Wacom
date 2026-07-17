// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"

#define LOCTEXT_NAMESPACE "WacomBattleEnemyIntentPresentationStyle"

const FSlateBrush* UWacomBattleEnemyIntentPresentationStyle::ResolveIntentIcon(
	const FName IntentId) const
{
	if (!IntentId.IsNone())
	{
		for (const FWacomBattleEnemyIntentIconEntry& Entry : IntentIcons)
		{
			if (Entry.IntentId == IntentId && IsIconBrushUsable(Entry.IconBrush))
			{
				return &Entry.IconBrush;
			}
		}
	}

	return IsIconBrushUsable(FallbackIconBrush) ? &FallbackIconBrush : nullptr;
}

bool UWacomBattleEnemyIntentPresentationStyle::IsIconBrushUsable(
	const FSlateBrush& Brush)
{
	return Brush.GetResourceObject() != nullptr
		&& Brush.ImageSize.X > 0.0f
		&& Brush.ImageSize.Y > 0.0f;
}

#if WITH_EDITOR
EDataValidationResult UWacomBattleEnemyIntentPresentationStyle::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!IsIconBrushUsable(FallbackIconBrush))
	{
		Context.AddError(LOCTEXT(
			"InvalidFallbackIcon",
			"Enemy Intent UI Style 配置错误：FallbackIconBrush 必须引用有效资源并具有正数 ImageSize。"));
		Result = EDataValidationResult::Invalid;
	}

	TSet<FName> SeenIntentIds;
	for (int32 Index = 0; Index < IntentIcons.Num(); ++Index)
	{
		const FWacomBattleEnemyIntentIconEntry& Entry = IntentIcons[Index];
		if (Entry.IntentId.IsNone())
		{
			Context.AddError(FText::Format(
				LOCTEXT("EmptyIntentId", "Enemy Intent UI Style 配置错误：IntentIcons[{0}].IntentId 不能为空。"),
				FText::AsNumber(Index)));
			Result = EDataValidationResult::Invalid;
		}
		else if (SeenIntentIds.Contains(Entry.IntentId))
		{
			Context.AddError(FText::Format(
				LOCTEXT("DuplicateIntentId", "Enemy Intent UI Style 配置错误：IntentId {0} 重复。"),
				FText::FromName(Entry.IntentId)));
			Result = EDataValidationResult::Invalid;
		}
		else
		{
			SeenIntentIds.Add(Entry.IntentId);
		}

		if (!IsIconBrushUsable(Entry.IconBrush))
		{
			Context.AddError(FText::Format(
				LOCTEXT("InvalidIntentIcon", "Enemy Intent UI Style 配置错误：IntentIcons[{0}].IconBrush 必须引用有效资源并具有正数 ImageSize。"),
				FText::AsNumber(Index)));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result == EDataValidationResult::Invalid
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif

#undef LOCTEXT_NAMESPACE
