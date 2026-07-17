// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyHostAnimationStyle.h"

#include "PaperFlipbook.h"

#define LOCTEXT_NAMESPACE "WacomBattleEnemyHostAnimationStyle"

const FWacomBattleEnemyHostAnimationClip*
UWacomBattleEnemyHostAnimationStyle::ResolveActionClip(FName IntentId) const
{
	if (!IntentId.IsNone())
	{
		if (const FWacomBattleEnemyHostAnimationClip* ExplicitClip =
			ActionClipsByIntentId.Find(IntentId))
		{
			return ExplicitClip->IsRuntimeUsable() ? ExplicitClip : nullptr;
		}
	}

	return DefaultActionClip.IsRuntimeUsable() ? &DefaultActionClip : nullptr;
}

const FWacomBattleEnemyHostAnimationClip*
UWacomBattleEnemyHostAnimationStyle::ResolveDestroyedClip() const
{
	return DestroyedClip.IsPlaybackUsable() ? &DestroyedClip : nullptr;
}

#if WITH_EDITOR
EDataValidationResult UWacomBattleEnemyHostAnimationStyle::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	auto RejectClip = [&Context, &Result](const FText& Label,
		const FWacomBattleEnemyHostAnimationClip& Clip,
		bool bRequired,
		bool bValidateImpact)
	{
		if (!Clip.Flipbook && !bRequired)
		{
			return;
		}
		if (!Clip.Flipbook)
		{
			Context.AddError(FText::Format(
				LOCTEXT("MissingClipFlipbook", "Enemy Host 动画配置错误：{0} 缺少 Flipbook。"),
				Label));
			Result = EDataValidationResult::Invalid;
		}
		if (!FMath::IsFinite(Clip.PlayRate) || Clip.PlayRate <= 0.0f)
		{
			Context.AddError(FText::Format(
				LOCTEXT("InvalidClipPlayRate", "Enemy Host 动画配置错误：{0} 的 PlayRate 必须为有限正数，当前为 {1}。"),
				Label,
				FText::AsNumber(Clip.PlayRate)));
			Result = EDataValidationResult::Invalid;
		}
		if (bValidateImpact
			&& (!FMath::IsFinite(Clip.ImpactNormalizedTime)
			|| Clip.ImpactNormalizedTime < 0.0f
			|| Clip.ImpactNormalizedTime > 1.0f))
		{
			Context.AddError(FText::Format(
				LOCTEXT("InvalidClipImpactTime", "Enemy Host 动画配置错误：{0} 的 ImpactNormalizedTime 必须位于 0–1，当前为 {1}。"),
				Label,
				FText::AsNumber(Clip.ImpactNormalizedTime)));
			Result = EDataValidationResult::Invalid;
		}
	};

	RejectClip(
		LOCTEXT("DefaultActionLabel", "DefaultActionClip"),
		DefaultActionClip,
		false,
		true);
	RejectClip(
		LOCTEXT("DestroyedLabel", "DestroyedClip"),
		DestroyedClip,
		false,
		false);
	for (const TPair<FName, FWacomBattleEnemyHostAnimationClip>& Pair : ActionClipsByIntentId)
	{
		if (Pair.Key.IsNone())
		{
			Context.AddError(LOCTEXT(
				"EmptyIntentId",
				"Enemy Host 动画配置错误：ActionClipsByIntentId 含有空 IntentId。"));
			Result = EDataValidationResult::Invalid;
		}
		RejectClip(
			FText::Format(LOCTEXT("IntentClipLabel", "Intent {0}"), FText::FromName(Pair.Key)),
			Pair.Value,
			true,
			true);
	}

	return Result == EDataValidationResult::Invalid
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif

#undef LOCTEXT_NAMESPACE
