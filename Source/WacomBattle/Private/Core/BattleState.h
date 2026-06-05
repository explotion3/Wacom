// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/WacomEnums.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Session/BattleResultPacket.h"

class UEnemyDefinition;
class UCharacterDefinition;

/**
 * 玩家运行时状态。
 *
 * 所有和"玩家本体"相关的数值、状态、累计计数都集中在这里。
 * 卡牌容器和敌人部位不放在这里（见 FCardContainers / FEnemyState）。
 */
struct FPlayerState
{
	int32 CurrentHp = 0;
	int32 MaxHp     = 0;
	int32 Shield    = 0;

	TObjectPtr<const UCharacterDefinition> CharacterDef = nullptr;

	/** 玩家持有的状态（当前支持 Poison，未来可扩展 Slow/Twilight/Freeze）。 */
	FGameplayTagContainer Statuses;
	TMap<FGameplayTag, int32> StatusStacks;

	/**
	 * 本场战斗内累计打出的 Companion 关键字卡牌数。
	 * 用于 Passive.Trigger.OnCompanionCount（拂晓飞蛾）。
	 * 任一 OnCompanionCount 被动触发后清零。
	 */
	int32 CompanionPlayedCount = 0;
};

/**
 * 卡牌容器。一场战斗内所有卡 + 七个定位容器。
 *
 * AllCards 持有所有卡的权威实例。DrawPile/Hand/... 只存 InstanceId，
 * 真实的 FRuntimeCardInstance::Location 字段和容器归属保持一致。
 *
 * **不变量**：AllCards 整场战斗只追加不删除（只改元素的 Location 字段），
 * 所以 CardIndexById 的索引一经建立就永远有效。若未来引入"战斗内销毁卡"，
 * 需要把被销毁的卡标记而非物理移除，或同步更新索引。
 */
struct FCardContainers
{
	TArray<FRuntimeCardInstance> AllCards;  // 一场战斗内所有卡的权威实例
	TMap<FGuid, int32> CardIndexById;       // InstanceId → AllCards 索引，O(1) 查找

	TArray<FGuid> DrawPile;                 // 抽牌堆
	TArray<FGuid> Hand;                     // 手牌队列（从左到右）
	TArray<FGuid> PlayedPile;               // 本回合使用牌堆
	TArray<FGuid> DiscardPile;              // 弃牌堆
	TArray<FGuid> ExhaustPile;              // 消耗牌堆
	TArray<FGuid> Limbo;                    // 本回合离手但不入任何区域的左右手

	/** 左手/右手运行时实例 ID。整场战斗不变。 */
	FGuid LeftHandInstanceId;
	FGuid RightHandInstanceId;
};

/**
 * 敌人运行时状态。
 *
 * **不变量**：Parts 整场战斗只追加不删除（部位破坏仅置 bDestroyed 标记）。
 */
struct FEnemyState
{
	TObjectPtr<const UEnemyDefinition> Definition = nullptr;
	TArray<FRuntimeEnemyPart> Parts;        // 按部位顺序
	TMap<FGuid, int32> PartIndexById;       // InstanceId → Parts 索引，O(1) 查找
};

/**
 * 战斗真相容器。
 *
 * 仅限 WacomBattle/Private 引用，外部模块编译期不可见。对外入口是
 * UBattleSession + FBattleSnapshot + FBattleEvent。
 *
 * 设计为非反射 struct：
 * - 不需要暴露给蓝图或序列化。
 * - 嵌套持有 FRuntimeCardInstance 的 TArray 时，非反射下仍可正常复制。
 * - TObjectPtr 在非反射容器里需要在 GC 侧手动追踪；BattleState 由 UBattleSession
 *   持有，所有对象引用通过 Session 间接追踪。
 *
 * 字段分组：
 * - Meta（阶段、回合、随机源、版本号）
 * - Player（FPlayerState）
 * - Cards  （FCardContainers）
 * - Enemy  （FEnemyState）
 *
 * 后续若有存档/网络需求，再升级为 USTRUCT 或切换到 UObject 容器。
 */
struct FBattleState
{
	// ---- Meta ----
	EBattlePhase Phase = EBattlePhase::None;
	int32 TurnNumber = 0;
	int32 CurrentWaitValue = 2;
	EBattleOutcome Outcome = EBattleOutcome::Undetermined;

	/** 随机源。测试可注入 seed 复现。 */
	FRandomStream Rng;

	/** 每次状态变更递增，用于 Snapshot Version 字段。 */
	int32 StateVersion = 0;

	// ---- 战内 → 战外回传 flag ----
	// 这些 flag 由 BattleSession::BuildResultPacket 读取后封装到 FBattleResultPacket
	// 传给 RunSession 处理压力 / 经验等战外结算。
	//
	/** 阈值比例。BattleSession::Initialize 从 InitParams 灌入。 */
	float HighHpThreshold = 0.5f;
	float LowHpThreshold  = 0.2f;

	/** 战内首次跨过 CurrentHp/MaxHp < HighHpThreshold（默认 0.5）时置 true。 */
	bool bCrossedHighHpThreshold = false;

	/** 战内首次跨过 CurrentHp/MaxHp < LowHpThreshold（默认 0.2）时置 true。 */
	bool bCrossedLowHpThreshold = false;

	/**
	 * 同归于尽：玩家 CurrentHp = 0 与敌方部位全死同时发生。
	 * 战斗结果仍判 Victory，战外加 +10% 伤口压力。
	 */
	bool bMutualDestruction = false;

	/**
	 * 战内累计的部位击倒经验记账。
	 *
	 * 部位破坏时（伤害命中或中毒结算导致 bDestroyed=true 的瞬间）追加一条。
	 * BattleSession::BuildResultPacket 拷给 packet.KnockdownExpGains。
	 *
	 * 每个部位只记账一次：触发点都在 `bDestroyed false → true` 边沿。
	 * 部位定义未填 ExperienceReward 或值为 0 时仍记一条 ExpAmount=0
	 * （让 Run 层有完整的"被破坏部位列表"，未来挂副作用更方便）。
	 */
	TArray<FKnockdownExpGain> PendingKnockdownExpGains;

	/**
	 * 待玩家三选一的击倒事件队列。
	 *
	 * 部位 bDestroyed false→true 边沿同时 push 一条到此队列。
	 * BattleSession 命令处理后检查队列：
	 *   - 非空 → Phase = PendingKnockdownChoice，发 KnockdownChoiceRequested 事件
	 *   - 空 → 正常推进
	 *
	 * 玩家提交 KnockdownChoice 命令后从头部 dequeue + 记账到 PendingKnockdownChoices。
	 *
	 * **预先破坏的部位**（来自 RunState.BattleProgress 的持久化破坏态）
	 * 在 Initialize 时设 bDestroyed = true，但**不入此队列**——避免重复弹 dialog。
	 */
	struct FPendingKnockdownEvent
	{
		FGuid PartInstanceId;
		FName PartId = NAME_None;
		bool bLeftHandAvailable = true;
		bool bRightHandAvailable = true;
	};
	TArray<FPendingKnockdownEvent> PendingKnockdownEvents;

	/**
	 * 玩家在击倒事件中的选择累计列表。
	 *
	 * 每次玩家选完一项 push 一条。
	 * BuildResultPacket 拷给 packet.KnockdownChoices。
	 */
	TArray<FKnockdownChoice> PendingKnockdownChoices;

	/**
	 * 战斗中获得、战后归入 Run 的卡牌。
	 *
	 * 第一版由击倒事件 Aid / Destroy 的部位奖励卡写入。
	 */
	TArray<FBattleGainedCard> PendingGainedCards;

	/**
	 * 本场战斗中所有被破坏的部位 ID。
	 *
	 * 撤离时由 Run 层用 packet.DestroyedPartIds 写入 RunState.BattleProgress；
	 * 胜利时清理 BattleProgress。
	 *
	 * **包含**预先破坏的部位（持久化的）+ 本场新破坏的部位——这样撤离时
	 * 写入的列表是"截至当前所有破坏过的部位"，覆盖式更新 BattleProgress。
	 */
	TArray<FName> DestroyedPartIds;

	// ---- 分组字段 ----
	FPlayerState    Player;
	FCardContainers Cards;
	FEnemyState     Enemy;

	/**
	 * 玩家 HP 变更后（**减少**或维持）调用，按当前 CurrentHp/MaxHp 比例
	 * 检查是否首次跨过 High / Low 阈值，触发后置 flag 永久 true（不会回退）。
	 *
	 * 调用约定：所有扣血路径在写入 CurrentHp 之后调用一次。
	 * 治疗（HP 增加）也可以调，逻辑等价于"现在是否已经在阈值下"，
	 * 但因为 flag 已设置就不会再设，对治疗实质 no-op。
	 */
	FORCEINLINE void CheckHpThresholdsCrossed()
	{
		if (Player.MaxHp <= 0) { return; }
		const float Ratio = static_cast<float>(Player.CurrentHp) / static_cast<float>(Player.MaxHp);
		if (!bCrossedHighHpThreshold && Ratio < HighHpThreshold)
		{
			bCrossedHighHpThreshold = true;
		}
		if (!bCrossedLowHpThreshold && Ratio < LowHpThreshold)
		{
			bCrossedLowHpThreshold = true;
		}
	}

	/**
	 * 部位被破坏的统一处理（伤害 / 中毒共用路径）。
	 *
	 * 调用约定：CurrentHp <= 0 且 bDestroyed 即将从 false 变 true 时调用一次。
	 * 调用方应已经把 Part->bDestroyed = true、CurrentInitiative = 0 设好。
	 *
	 * 本函数：
	 *   1. 发 EnemyPartHpEmptied 事件
	 *   2. 记 KnockdownExpGain 经验
	 *   3. 加入 DestroyedPartIds（撤离时持久化用）
	 *   4. push PendingKnockdownEvent 队列（等玩家三选一）
	 *
	 * 不调用本函数的场景：
	 *   - Initialize 时按 PreDestroyedPartIds 设的预先破坏部位（已经处理过经验和选择，不重复）
	 *
	 * @param Part           被破坏的部位（已置 bDestroyed=true / CurrentHp=0）
	 * @param Events         事件总线
	 * @param InflictedByCardId 触发本次破坏的卡牌实例 ID（Effect.Damage 命中时传 Ctx.SourceInstanceId；
	 *                          中毒结算等没有"卡牌来源"的路径传空）。保留参数用于事件来源追踪扩展；
	 *                          击倒事件的左右手分支不再依赖手牌区锚点是否存在。
	 */
	void RecordPartDestroyed(struct FRuntimeEnemyPart& Part, struct FBattleEventBus& Events,
		const FGuid& InflictedByCardId = FGuid());
};
