// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleFloatingCombatTextStyle.h"

#define LOCTEXT_NAMESPACE "WacomBattleFloatingCombatTextStyle"

bool UWacomBattleFloatingCombatTextStyle::ValidateStyle(TArray<FText>& OutErrors) const
{
	if (FadeInSeconds < 0.0f || ReadableHoldSeconds < 0.0f || FadeOutSeconds < 0.0f)
	{
		OutErrors.Add(LOCTEXT("InvalidTiming", "飘字淡入、可读保持和淡出时间不能为负数。"));
	}
	if (SameTargetStaggerSeconds < 0.0f)
	{
		OutErrors.Add(LOCTEXT("InvalidStagger", "同目标飘字错峰时间不能为负数。"));
	}
	if (EntrySize.X <= 0.0f || EntrySize.Y <= 0.0f)
	{
		OutErrors.Add(LOCTEXT("InvalidEntrySize", "飘字 EntrySize 必须为正数。"));
	}
	if (MaxConcurrentPerTarget <= 0)
	{
		OutErrors.Add(LOCTEXT("InvalidConcurrency", "同目标并发飘字数量必须至少为 1。"));
	}
	if (ViewportSafePaddingPixels < 0.0f)
	{
		OutErrors.Add(LOCTEXT("InvalidSafePadding", "Viewport 安全留白不能为负数。"));
	}
	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
