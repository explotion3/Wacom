// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomEnums.h"
#include "BattleResultPacket.generated.h"

class UCardDefinition;

/**
 * 单个被破坏部位给予玩家的经验值记账（GDD §3.3）。
 *
 * 战内 BattleState 在部位破坏路径累积，BattleSession::BuildResultPacket
 * 拷贝到 packet 给 Run 层结算。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FKnockdownExpGain
{
	GENERATED_BODY()

	/** 来源部位 ID（来自 UEnemyPartDefinition::PartId）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FName PartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	int32 ExpAmount = 0;
};

/**
 * 单个击倒事件玩家选择记账（GDD §6 / §3.3）。
 *
 * 玩家在击倒事件中选了"援助 / 破坏 / 撤离"。Run 层用此触发对应分支
 * （第一阶段先记日志，节点事件 Stage 9 时接入实际效果）。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FKnockdownChoice
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FName PartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	EKnockdownChoice Choice = EKnockdownChoice::None;
};

/**
 * 战斗中获得、战后需要归入 Run 的卡牌记账。
 *
 * 第一版来源是击倒事件 Aid / Destroy 的部位奖励卡。战斗内创建 runtime card，
 * 战斗结束后 Run 层按 Definition 生成新的持久 FCardInstance。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleGainedCard
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TObjectPtr<UCardDefinition> Definition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	FName SourcePartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	EKnockdownChoice SourceChoice = EKnockdownChoice::None;
};

/**
 * 战斗结束时打包给 Run 层结算的"战后包"。
 *
 * 对齐 Game_Design.md §9.2 战内 → 战外回传表。
 *
 * 当前字段：
 *   - Outcome：胜负判定
 *   - bCrossedHighHpThreshold：战内首次跨过 CurrentHp/MaxHp < HighHpThreshold（默认 0.5）
 *   - bCrossedLowHpThreshold：战内首次跨过 CurrentHp/MaxHp < LowHpThreshold（默认 0.2）
 *   - bMutualDestruction：玩家 HP=0 与敌方全死同时发生
 *   - bWithdrawn：玩家通过击倒事件选择"撤离"结束战斗（Outcome=Victory，但 Run 层不计敌人为已击败）
 *   - KnockdownExpGains：战内被破坏部位的经验奖励列表（Stage 3）
 *   - KnockdownChoices：玩家在击倒事件中的选择列表（Stage 7）
 *   - GainedCards：战斗中获得、战后归入 Run 的卡牌列表
 *   - DestroyedPartIds：本场战斗中被破坏的部位 ID 列表（Stage 7，撤离时持久化）
 *
 * 由 UBattleSession::BuildResultPacket() 构造，
 * 由 URunSession::OnBattleFinished(Packet, EnemyDef) 消费。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleResultPacket
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	EBattleOutcome Outcome = EBattleOutcome::Undetermined;

	/** 战内首次跨过 HighHpThreshold（默认 0.5）。Run 层 +1% 伤口。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	bool bCrossedHighHpThreshold = false;

	/** 战内首次跨过 LowHpThreshold（默认 0.2）。Run 层 +5% 伤口。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	bool bCrossedLowHpThreshold = false;

	/**
	 * 同归于尽。Run 层 +10% 伤口。
	 * Outcome 仍为 Victory（GDD §9.2），不触发战外失败。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	bool bMutualDestruction = false;

	/**
	 * 撤离（GDD §6 / §10.5）。Outcome=Victory 但 Run 层不计敌人为已击败、
	 * 战斗节点不变"已完成"。下次进入同一战斗节点仍触发战斗，
	 * 但已破坏的部位（见 DestroyedPartIds）维持破坏态。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	bool bWithdrawn = false;

	/**
	 * 战内被破坏部位的经验奖励（GDD §3.3）。
	 *
	 * 部位破坏时由战内路径（伤害 / 中毒）记账。
	 * Run 层在 Outcome=Victory（含同归于尽 / 撤离）时结算；Defeat 不结算。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TArray<FKnockdownExpGain> KnockdownExpGains;

	/**
	 * 玩家在击倒事件中的选择列表（GDD §6）。
	 *
	 * 部位被击倒时弹出三选一面板，玩家每次选择记一条。
	 * Run 层第一阶段记日志；节点事件 Stage 9 接入时按 Choice 分支处理。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TArray<FKnockdownChoice> KnockdownChoices;

	/**
	 * 战斗中获得、战斗结束后归入 Run 的卡牌。
	 *
	 * 第一版仅 Aid / Destroy 击倒奖励会写入；Defeat 时 Run 层不结算。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TArray<FBattleGainedCard> GainedCards;

	/**
	 * 本场战斗中所有被破坏的部位 ID（GDD §10.5）。
	 *
	 * 撤离时由 Run 层写入 RunState.BattleProgress，下次进入同一 Trigger
	 * 时持久化破坏状态。胜利时 Run 层会清理对应 Trigger 的进度。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Result")
	TArray<FName> DestroyedPartIds;
};
