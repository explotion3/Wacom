// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Runtime/BattleEnemyKeys.h"
#include "Types/WacomEnums.h"
#include "BattleEvent.generated.h"

class UCardDefinition;

/**
 * 战斗事件类型。
 *
 * 事件是结算过程的记录流，给 UI 播表现、给日志写历史、给测试验证用。
 * 事件本身不作为战斗真相；断线重连或存档恢复都以 BattleSnapshot 为准。
 */
UENUM(BlueprintType)
enum class EBattleEventType : uint8
{
	None                  UMETA(DisplayName = "None"),
	BattleStarted         UMETA(DisplayName = "BattleStarted"),
	TurnStarted           UMETA(DisplayName = "TurnStarted"),
	CardsDrawn            UMETA(DisplayName = "CardsDrawn"),
	CardsRetained         UMETA(DisplayName = "CardsRetained"),
	HandZoneChanged       UMETA(DisplayName = "HandZoneChanged"),      // 区域重新判定/腾挪后触发
	CardPlayed            UMETA(DisplayName = "CardPlayed"),
	InitiativeHit         UMETA(DisplayName = "InitiativeHit"),        // 先机命中（完美释放窗口成立）
	ResistanceResolved    UMETA(DisplayName = "ResistanceResolved"),
	PerfectReleaseResolved UMETA(DisplayName = "PerfectReleaseResolved"),
	DamageDealt           UMETA(DisplayName = "DamageDealt"),
	StatusApplied         UMETA(DisplayName = "StatusApplied"),
	CardStatusChanged     UMETA(DisplayName = "CardStatusChanged"),
	EnemyInitiativeChanged UMETA(DisplayName = "EnemyInitiativeChanged"),
	InitiativePushed      UMETA(DisplayName = "InitiativePushed"),     // 打牌推进尝试摘要；逐部位实际变化见 EnemyInitiativeChanged
	WaitPerformed         UMETA(DisplayName = "WaitPerformed"),
	EnemyPartActed        UMETA(DisplayName = "EnemyPartActed"),
	EnemyIntentSelected   UMETA(DisplayName = "EnemyIntentSelected"),
	EnemyPhaseChanged     UMETA(DisplayName = "EnemyPhaseChanged"),
	EnemyPartHpEmptied    UMETA(DisplayName = "EnemyPartHpEmptied"),   // 部位被破坏
	EnemyKnockdown        UMETA(DisplayName = "EnemyKnockdown"),       // 击倒事件记录
	KnockdownChoiceRequested UMETA(DisplayName = "KnockdownChoiceRequested"), // 等待玩家三选一
	KnockdownChoiceMade   UMETA(DisplayName = "KnockdownChoiceMade"),  // 玩家选完一项
	TurnEnded             UMETA(DisplayName = "TurnEnded"),
	PassiveTriggered      UMETA(DisplayName = "PassiveTriggered"),    // 被动触发通知
	HandLimitDiscarded    UMETA(DisplayName = "HandLimitDiscarded"),  // 普通手牌上限导致弃牌
	CardDiscarded         UMETA(DisplayName = "CardDiscarded"),       // 卡牌因弃牌规则从手牌进入弃牌堆
	CardExhausted         UMETA(DisplayName = "CardExhausted"),       // 卡牌因消耗规则从手牌进入消耗牌堆
	CardGained            UMETA(DisplayName = "CardGained"),          // 战斗中获得一张新卡
	BattleEnded           UMETA(DisplayName = "BattleEnded"),
	// Append-only presentation fact. Keep existing serialized enum values stable.
	CardPlayDestinationResolved UMETA(DisplayName = "CardPlayDestinationResolved"),
	DiscardPileReshuffledIntoDraw UMETA(DisplayName = "DiscardPileReshuffledIntoDraw"),
	CardRuntimeCostChanged UMETA(DisplayName = "CardRuntimeCostChanged"),
};

/**
 * 普通手牌上限弃牌来源。抽牌路径当前会在移动前按容量截断；TurnStart / EffectDraw
 * 保留为兼容来源，当前活跃路径主要是被动回手和战斗内获得卡牌。
 */
UENUM(BlueprintType)
enum class EHandLimitDiscardSource : uint8
{
	None                    UMETA(DisplayName = "None"),
	TurnStart               UMETA(DisplayName = "TurnStart"),
	EffectDraw              UMETA(DisplayName = "EffectDraw"),
	PassiveOnCompanionCount UMETA(DisplayName = "PassiveOnCompanionCount"),
};

/**
 * 手牌卡移动原因。用于 CardDiscarded / CardExhausted 区分规则来源。
 */
UENUM(BlueprintType)
enum class EHandCardZoneMoveReason : uint8
{
	None      UMETA(DisplayName = "None"),
	Effect    UMETA(DisplayName = "Effect"),
	HandLimit UMETA(DisplayName = "HandLimit"),
	TurnEnd   UMETA(DisplayName = "TurnEnd"),
};

/**
 * 战斗事件。
 *
 * 使用扁平字段变体：不同事件类型填不同字段，未使用的留默认值。
 *
 * 字段使用约定（非穷举）：
 * - CardsDrawn          ：CardInstanceIds = 本批真实抽到 / 移入手牌的普通卡实例，Count = CardInstanceIds.Num()
 * - DiscardPileReshuffledIntoDraw：CardInstanceIds = 本批从弃牌堆洗回抽牌堆的卡，Count = CardInstanceIds.Num()
 * - CardsRetained       ：CardInstanceIds = 本次回合结束明确保留的普通手牌，Count = CardInstanceIds.Num()
 * - CardPlayed          ：CardInstanceId、ActorEnemyPartKey = 目标部位稳定 key
 * - CardPlayDestinationResolved：CardInstanceId、CardDestination = 打出结算后的最终区域
 * - InitiativeHit       ：ActorEnemyPartKey = 被命中部位、Amount = 本次 RuntimeCost
 * - ResistanceResolved ：ActorEnemyPartKey = 比较部位、Amount = 玩家最高单段伤害、Count = 敌方最高单段伤害、bSuccess = 严格大于是否成立、成功时 Tag=Status.Stunned
 * - DamageDealt         ：ActorEnemyPartKey = 受伤害部位、Amount = 实际扣血量；全盾吸收为 0，overkill 只记剩余 HP，玩家目标时 key 为空
 * - StatusApplied       ：ActorEnemyPartKey、Tag = Status.*、Amount = 层数；玩家目标时 key 为空
 * - CardStatusChanged   ：CardInstanceId = 目标卡，Tag = Status.*、Amount = 本次 delta、Count = 变更后层数
 * - CardRuntimeCostChanged：CardInstanceId = 目标卡、ActorInstanceId = 来源卡、Tag = 来源效果、Amount = RuntimeCostModifier delta、Count = 变更后实际 RuntimeCost
 * - EnemyInitiativeChanged：ActorEnemyPartKey、Tag = 原因、Amount = 实际 delta、Count = 变更后倒计时
 * - InitiativePushed    ：Amount = 本次尝试推进量（RuntimeCost）；冻结可能使部分部位实际变化为 0
 * - WaitPerformed       ：Amount = 本次等待值
 * - EnemyPartActed      ：ActorEnemyPartKey = 行动部位、IntentId = 本次执行的意图
 * - EnemyIntentSelected ：ActorEnemyPartKey = 行动部位、IntentId / IntentSetId / EnemyPhaseId = 新选中意图上下文
 * - EnemyPhaseChanged   ：ActorEnemyPartKey = 触发部位、EnemyPhaseId = 新 phase
 * - EnemyPartHpEmptied  ：ActorEnemyPartKey = 被破坏部位
 * - HandLimitDiscarded  ：CardInstanceId = 被弃掉的卡；ActorInstanceId = 触发源卡（若存在）
 * - CardDiscarded       ：CardInstanceId = 被弃掉的卡；ActorInstanceId = 触发源卡；Tag = 效果 tag
 * - CardExhausted       ：CardInstanceId = 被消耗的卡；ActorInstanceId = 触发源卡；Tag = 效果 tag
 * - CardGained          ：CardInstanceId = 战斗内新卡实例；ActorEnemyPartKey = 来源部位；CardDefinition = 新卡定义；Count = EKnockdownChoice
 * - BattleEnded         ：Count = 1 表示胜利、0 表示失败（后续换专用字段）
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	EBattleEventType Type = EBattleEventType::None;

	/** 事件序号，单场战斗内唯一递增。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	int32 Sequence = 0;

	/** 与事件相关的主体实例 ID（打出的卡、行动的部位、受伤单位等）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	FGuid ActorInstanceId;

	/** 与事件相关的敌方部位稳定 key。玩家目标或非敌方事件时为空。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	FBattleEnemyPartKey ActorEnemyPartKey;

	/** 若事件涉及一张卡，这是该卡的运行时实例 ID。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	FGuid CardInstanceId;

	/** 批量卡牌事件涉及的运行时实例 ID。CardsDrawn / CardsRetained / CardDiscarded 按规则顺序记录真实普通手牌；CardDiscarded 的同批逐张事件共享完整批次列表。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	TArray<FGuid> CardInstanceIds;

	/** 通用标签字段。按 Type 语义使用（状态 tag、意图 id、效果 tag 等）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	FGameplayTag Tag;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	FName IntentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	FName IntentSetId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	FName EnemyPhaseId = NAME_None;

	/** 通用数值字段：伤害、层数、等待值、RuntimeCost 等。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	int32 Amount = 0;

	/** 通用计数字段：抽牌数、连击次数等。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	int32 Count = 0;

	/** 通用成功事实；当前由 ResistanceResolved 显式使用。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	bool bSuccess = false;

	/** Deck step 完成后的抽牌堆数量；仅 CardsDrawn / DiscardPileReshuffledIntoDraw 使用。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	int32 DrawPileCountAfter = INDEX_NONE;

	/** Deck step 或正式弃牌批次完成后的弃牌堆数量；CardsDrawn / DiscardPileReshuffledIntoDraw / CardDiscarded 使用。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	int32 DiscardPileCountAfter = INDEX_NONE;

	/** 普通手牌上限弃牌的来源，仅 HandLimitDiscarded 使用。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	EHandLimitDiscardSource HandLimitDiscardSource = EHandLimitDiscardSource::None;

	/** 手牌卡移动来源，仅 CardDiscarded / CardExhausted 使用。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	EHandCardZoneMoveReason HandCardZoneMoveReason = EHandCardZoneMoveReason::None;

	/** 同一正式弃牌迁移批次的稳定首个 CardDiscarded Sequence；仅 CardDiscarded 使用，用于把逐张事件还原成一次有序表现。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	int32 HandCardZoneMoveBatchSequence = INDEX_NONE;

	/** 打出的源卡最终所在区域，仅 CardPlayDestinationResolved 使用。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	ECardLocation CardDestination = ECardLocation::Unknown;

	/** 事件涉及的新卡定义。第一版仅 CardGained 使用。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;
};
