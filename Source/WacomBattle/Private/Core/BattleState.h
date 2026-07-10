// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Runtime/BattleEnemyKeys.h"
#include "Types/WacomEnums.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Session/BattleResultPacket.h"
#include "Enemies/IntentEffect.h"

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

	/** 玩家持有的 stack status 权威状态；只保存正层数。 */
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

/** Encounter 内的一个敌人槽运行时投影。 */
struct FEnemySlotState
{
	FName EncounterId = TEXT("Encounter");
	FName EnemySlotId = TEXT("Enemy");
	TObjectPtr<const UEnemyDefinition> Definition = nullptr;
	TArray<FGuid> PartInstanceIds;
};

/**
 * 敌人运行时状态。
 *
 * **不变量**：Parts 整场战斗只追加不删除（部位破坏仅置 bDestroyed 标记）。
 */
struct FEnemyState
{
	FName EncounterId = TEXT("Encounter");

	TArray<FEnemySlotState> EnemySlots;
	TArray<FRuntimeEnemyPart> Parts;        // 按部位顺序
	TMap<FGuid, int32> PartIndexById;       // InstanceId → Parts 索引，O(1) 查找
	TMap<FBattleEnemyPartKey, int32> PartIndexByKey; // Stable key → Parts 索引，O(1) 查找
};

/** 尚未在下个玩家回合物化到具体卡牌的控制效果。 */
struct FPendingHandAffliction
{
	FGameplayTag Status;
	int32 StacksPerCard = 0;
	FHandAfflictionDelivery Delivery;
	FGuid SourceInstanceId;
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
		FBattlePartSlotIdentity Identity;
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

	TArray<FBattlePartSlotIdentity> DestroyedParts;

	// ---- 分组字段 ----
	FPlayerState    Player;
	FCardContainers Cards;
	FEnemyState     Enemy;

	/** 玩家侧 Slow / Freeze / Twilight 的唯一待生效真相。 */
	TArray<FPendingHandAffliction> PendingHandAfflictions;
};
