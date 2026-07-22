// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "Types/WacomEnums.h"
#include "WacomKnockdownChoiceDialogTypes.generated.h"

/** 击倒选择单项的 UI-only 展示数据。它不承载规则状态，也不提交战斗命令。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomKnockdownChoiceOptionViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice")
	EKnockdownChoice Choice = EKnockdownChoice::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice")
	FText BranchLabel;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice")
	FText ChoiceLabel;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice", meta = (MultiLine = true))
	FText DescriptionText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice")
	bool bAvailable = false;

	/** 已翻译为玩家文案的禁用原因；WBP 不解析规则层 FName。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice")
	FText DisabledReasonText;

	/** 规则 View 声明该分支有奖励卡。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice")
	bool bHasRewardCard = false;

	/** 已成功从 Snapshot 定义构造完整卡面；为 false 时使用 RewardFallbackText。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice")
	bool bHasRewardCardView = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice")
	FText RewardFallbackText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice")
	FWacomCardViewData RewardCardViewData;
};

/** 一个待处理击倒事件的完整 UI-only Modal 展示数据。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomKnockdownChoiceDialogViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice")
	FText TitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice")
	FText PartNameText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice")
	FWacomKnockdownChoiceOptionViewData AidOption;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice")
	FWacomKnockdownChoiceOptionViewData WithdrawOption;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Knockdown Choice")
	FWacomKnockdownChoiceOptionViewData DestroyOption;
};
