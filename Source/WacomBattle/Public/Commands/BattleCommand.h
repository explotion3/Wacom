// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomEnums.h"
#include "BattleCommand.generated.h"

/**
 * 战斗命令类型。
 *
 * 战斗命令类型。
 */
UENUM(BlueprintType)
enum class EBattleCommandType : uint8
{
	None     UMETA(DisplayName = "None"),
	PlayCard UMETA(DisplayName = "PlayCard"),
	Wait     UMETA(DisplayName = "Wait"),
	EndTurn  UMETA(DisplayName = "EndTurn"),
	KnockdownChoice UMETA(DisplayName = "KnockdownChoice"),
};

/**
 * 战斗命令。
 *
 * 玩家操作进入战斗内核前统一转换为命令。UI、测试入口、脚本都只提交命令，
 * 不直接修改 BattleState。
 *
 * 使用扁平字段的变体结构（不同 Type 用不同字段）。
 * 后续命令集膨胀到 6 个以上再考虑 polymorphic USTRUCT。
 *
 * 字段约定：
 * - Type == PlayCard：CardInstanceId 必填；TargetPartInstanceId / TargetCardInstanceId 按卡牌 TargetMode 选择其一填写。
 * - Type == Wait / EndTurn：不读取任何目标字段。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleCommand
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Wacom|Battle|Command")
	EBattleCommandType Type = EBattleCommandType::None;

	/** 要打出的卡牌运行时实例 ID。仅 PlayCard 使用。 */
	UPROPERTY(BlueprintReadWrite, Category = "Wacom|Battle|Command")
	FGuid CardInstanceId;

	/** 目标敌方部位的运行时实例 ID。按卡牌 TargetMode 决定是否必填。 */
	UPROPERTY(BlueprintReadWrite, Category = "Wacom|Battle|Command")
	FGuid TargetPartInstanceId;

	/** 目标手牌的运行时实例 ID。仅 TargetMode == HandCard 的 PlayCard 使用。 */
	UPROPERTY(BlueprintReadWrite, Category = "Wacom|Battle|Command")
	FGuid TargetCardInstanceId;

	/** 击倒事件玩家选择。仅 KnockdownChoice 使用。 */
	UPROPERTY(BlueprintReadWrite, Category = "Wacom|Battle|Command")
	EKnockdownChoice KnockdownChoiceValue = EKnockdownChoice::None;

	FBattleCommand() = default;

	static FBattleCommand MakePlayCard(const FGuid& InCardInstanceId, const FGuid& InTargetPartInstanceId = FGuid())
	{
		FBattleCommand Cmd;
		Cmd.Type = EBattleCommandType::PlayCard;
		Cmd.CardInstanceId = InCardInstanceId;
		Cmd.TargetPartInstanceId = InTargetPartInstanceId;
		return Cmd;
	}

	static FBattleCommand MakePlayCardOnHandCard(const FGuid& InCardInstanceId, const FGuid& InTargetCardInstanceId)
	{
		FBattleCommand Cmd;
		Cmd.Type = EBattleCommandType::PlayCard;
		Cmd.CardInstanceId = InCardInstanceId;
		Cmd.TargetCardInstanceId = InTargetCardInstanceId;
		return Cmd;
	}

	static FBattleCommand MakeWait()
	{
		FBattleCommand Cmd;
		Cmd.Type = EBattleCommandType::Wait;
		return Cmd;
	}

	static FBattleCommand MakeEndTurn()
	{
		FBattleCommand Cmd;
		Cmd.Type = EBattleCommandType::EndTurn;
		return Cmd;
	}

	static FBattleCommand MakeKnockdownChoice(EKnockdownChoice InChoice)
	{
		FBattleCommand Cmd;
		Cmd.Type = EBattleCommandType::KnockdownChoice;
		Cmd.KnockdownChoiceValue = InChoice;
		return Cmd;
	}
};
