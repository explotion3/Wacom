// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyPartAnimationStyle.h"

#define LOCTEXT_NAMESPACE "WacomBattleEnemyPartAnimationStyle"

const FWacomBattleEnemyPartAnimationClip*
UWacomBattleEnemyPartAnimationStyle::ResolveActionClip(FName IntentId) const
{
	if (!IntentId.IsNone())
	{
		if (const FWacomBattleEnemyPartAnimationClip* ExplicitClip =
			ActionClipsByIntentId.Find(IntentId))
		{
			return ExplicitClip->IsRuntimeUsable() ? ExplicitClip : nullptr;
		}
	}

	return DefaultActionClip.IsRuntimeUsable() ? &DefaultActionClip : nullptr;
}

#if WITH_EDITOR
EDataValidationResult UWacomBattleEnemyPartAnimationStyle::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (TargetVisualLayerId.IsNone())
	{
		Context.AddError(LOCTEXT(
			"MissingTargetVisualLayerId",
			"Enemy Part 动画配置错误：TargetVisualLayerId 不能为空。"));
		Result = EDataValidationResult::Invalid;
	}

	auto RejectClip = [&Context, &Result](
		const FText& Label,
		const FWacomBattleEnemyPartAnimationClip& Clip,
		bool bRequired)
	{
		if (!Clip.Flipbook && !bRequired)
		{
			return;
		}
		if (!Clip.Flipbook)
		{
			Context.AddError(FText::Format(
				LOCTEXT("MissingClipFlipbook", "Enemy Part 动画配置错误：{0} 缺少 Flipbook。"),
				Label));
			Result = EDataValidationResult::Invalid;
		}
		if (!FMath::IsFinite(Clip.PlayRate) || Clip.PlayRate <= 0.0f)
		{
			Context.AddError(FText::Format(
				LOCTEXT("InvalidClipPlayRate", "Enemy Part 动画配置错误：{0} 的 PlayRate 必须为有限正数，当前为 {1}。"),
				Label,
				FText::AsNumber(Clip.PlayRate)));
			Result = EDataValidationResult::Invalid;
		}
	};

	RejectClip(LOCTEXT("DefaultActionLabel", "DefaultActionClip"), DefaultActionClip, false);
	for (const TPair<FName, FWacomBattleEnemyPartAnimationClip>& Pair : ActionClipsByIntentId)
	{
		if (Pair.Key.IsNone())
		{
			Context.AddError(LOCTEXT(
				"EmptyIntentId",
				"Enemy Part 动画配置错误：ActionClipsByIntentId 含有空 IntentId。"));
			Result = EDataValidationResult::Invalid;
		}
		RejectClip(
			FText::Format(LOCTEXT("IntentClipLabel", "Intent {0}"), FText::FromName(Pair.Key)),
			Pair.Value,
			true);
	}

	return Result == EDataValidationResult::Invalid
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif

#undef LOCTEXT_NAMESPACE
