// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Session/BattleSession.h"  // FBattleInitParams
#include "RunStateTypes.h"
#include "RunState.generated.h"

class UCharacterDefinition;
class UCardDefinition;
class UEnemyDefinition;

/**
 * 单个战斗节点（Trigger）的进度快照（GDD §10.5 撤离重入）。
 *
 * 撤离时 Run 层用 packet.DestroyedPartIds 写入 RunState.BattleProgress；
 * 下次进入同一 Trigger 时，BuildInitParamsForBattle 把 DestroyedPartIds
 * 灌进 BattleInitParams.PreDestroyedPartIds，BattleSession 应用为
 * Part.bDestroyed = true（不发经验、不入击倒队列）。
 *
 * 战斗胜利时清理对应 Trigger 的进度。
 *
 * **第一阶段只持久化"破坏部位列表"**。如果将来需要存中间血量、
 * 部位状态层数等，扩字段。
 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FBattleProgressSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Battle")
	TArray<FName> DestroyedPartIds;
};

/** 调用方传入的一条商店商品配置。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunShopOfferInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Shop")
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Shop", meta = (ClampMin = "0", UIMin = "0", ToolTip = "商品价格，单位为金币。0 表示免费商品；负数输入会被跳过。"))
	int32 Price = 0;
};

/** Run 内保存的一条商店商品状态。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunShopOffer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	FGuid OfferId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	int32 Price = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	bool bPurchased = false;
};

/** 单个商店节点在当前 Run 内的库存状态。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunShopState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	TArray<FRunShopOffer> Offers;
};

/** 当前商店 UI/测试可读取的只读快照。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunShopSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	FName ShopId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	bool bIsActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	bool bHasPurchaseThisVisit = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	TArray<FRunShopOffer> Offers;
};

/**
 * 一次冒险（Run）的持久状态。
 *
 * Stage 1.1 起，本结构覆盖 Game_Design.md §3 / §8 / §10 / §11 描述的全部战外字段：
 *   - §3.1 / §3.4：手指数 + HpPerFinger（战外只持有手指；战内 HP 每场独立）
 *   - §3.2：八种压力（FPressureValues）+ 阈值常量
 *   - §3.3：经验值 + 技能池
 *   - §8：当前日期 / 时段 / 节点剩余数 + 五时段初始节点常量
 *   - §10 地图 Run 状态字段在 Stage 8 接入（这里暂未引入 MapNodeStates）
 *   - §11：背包 + 备战卡组（容量上限）
 *
 * 行为约束：
 *   - 本结构是数据容器，不做业务逻辑；业务逻辑放 URunSession
 *   - URunSession 只读字段对外提供方法（GetXxx / AddPressure / RemoveFinger / ConsumeNode 等）
 *
 * 注意：FRunState 是内存数据层，不直接序列化到磁盘（存档系统 Stage 0.1 已停）。
 * 磁盘格式见 UWacomSaveGame，两者之间做字段拷贝。新字段第一阶段不进 SaveGame，
 * 等存档恢复时再统一升版本同步。
 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunState
{
	GENERATED_BODY()

	// ---- §3.1 / §3.4：玩家本体（手指 → HP 上限）----

	/**
	 * 战外手指数量。
	 *
	 * 战内本体 HP 上限 = FingerCount × HpPerFinger。
	 * 战外失败：FingerCount = 0。
	 * 残疾压力同步：每缺 1 指 +5%（业务逻辑由 URunSession::RemoveFinger 处理）。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|HP")
	int32 FingerCount = 10;

	/** 每根手指对应的 HP（数值常量化，按角色配置）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|HP")
	int32 HpPerFinger = 2;

	// ---- §3.2：压力系统 ----

	/** 八种压力值。详见 FPressureValues。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	FPressureValues Pressure;

	/**
	 * 战内伤口阈值 1（CurrentHp/MaxHp 跨过时 +1% 伤口压力）。
	 * GDD §3.2 / §9.2，第一版默认 0.5。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	float HighHpThreshold = 0.5f;

	/**
	 * 战内伤口阈值 2（CurrentHp/MaxHp 跨过时 +5% 伤口压力）。
	 * GDD §3.2 / §9.2，第一版默认 0.8。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	float LowHpThreshold = 0.2f;

	// ---- §3.3：经验值与技能池 ----

	/** 累计经验值。满 ExperienceCapacity 自动入账并清零。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Skill")
	int32 ExperienceCurrent = 0;

	/** 经验值上限。第一版默认 10。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Skill")
	int32 ExperienceCapacity = 10;

	/**
	 * 已获得的技能（按获得顺序）。
	 * 第一阶段只占位，技能效果未实现。每次满经验入账一个 SkillSlot.Placeholder。
	 *
	 * 用 TArray 而不是 FGameplayTagContainer：
	 *   - Container 是 Set 语义，重复 AddTag 不增加 Num
	 *   - 占位阶段需要"重复累加"才能反映已得技能数
	 *   - 未来真技能上线（每种技能唯一）后可以平移到 GetUniqueSkillTags 投影
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Skill")
	TArray<FGameplayTag> AcquiredSkills;

	// ---- §3.2 压力辅助计数 ----

	/**
	 * 累计偷窃次数。OnTheftCommitted 调用时 ++ 后用于公式 n*(n+1)/2 + 1。
	 * GDD §3.2 劣迹增量公式（b 语义）。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	int32 TheftCount = 0;

	// ---- §11.7 / 经济：金币 ----

	/**
	 * 玩家持有的金币数量。
	 *
	 * 来源（GDD §11.7）：
	 *   - 删牌区：拖卡到删牌区置换金币（白=1 / 蓝=2，第一阶段占位）
	 *   - 商店出售（未来 Stage 9）
	 *
	 * 用途（未来）：商店购买 / 节点事件支付等。
	 *
	 * 第一阶段：Stage 0.1 存档暂停，Run 结束自动清零。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Economy")
	int32 Gold = 0;

	/** 当前正在访问的商店节点 ID。NAME_None 表示没有打开商店。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	FName ActiveShopId = NAME_None;

	/** 当前商店访问内是否买过至少一件商品。关闭商店时据此消耗 1 节点。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	bool bShopVisitHasPurchase = false;

	// ---- §8：时间与昼夜 ----

	/** 当前 Run 的天数（从 1 开始）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 CurrentDayNumber = 1;

	/** 当前时段。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	ETimePhase CurrentTimePhase = ETimePhase::Morning;

	/** 当前时段剩余可用节点数（资源点）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 RemainingNodeCount = 2;

	/** 五时段初始节点数。可被技能 / 事件改。GDD §8.2 默认值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 InitialNodeCount_Morning = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 InitialNodeCount_Day = 6;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 InitialNodeCount_Dusk = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 InitialNodeCount_Night = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 InitialNodeCount_Sunrise = 1;

	// ---- §11：背包与备战卡组 ----

	/**
	 * 背包：永久卡牌池（GDD §11.1）。
	 * 战斗结束所有战内卡回背包；事件 / 掉落进背包。
	 *
	 * 内容容量公式见 GDD §11.4，由 URunSession::GetFluxCapacity() 动态计算
	 * （遍历所有 A 类容器卡 max(Capacity - 1, 0) 求和）。
	 *
	 * Stage 4.5.0：元素从 `TObjectPtr<UCardDefinition>` 升级为 `FCardInstance`。
	 * 升级动机：同款 Definition 的多张卡需被独立放进不同 zone（备战 / 通量 / 各 SpecialZone /
	 * 负重），单纯按 Definition 指针无法区分。Instance 引入后，每张卡有全局唯一 InstanceId。
	 * 4.5.0 阶段先做字段级类型升级；InstanceId 生成在 4.5.0 任务 2.2 / 2.3 接入。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck")
	TArray<FCardInstance> Backpack;

	/**
	 * 备战卡组：从玩家拥有卡牌中选出，战斗实际读取的卡组（GDD §11.2 / §11.6）。
	 * 不含左右手卡（左右手卡始终通过 Character 字段独立加载）。
	 * 容量公式见 GDD §11.4，由 URunSession::GetBattleDeckCapacity() 动态计算。
	 *
	 * Stage 4.5.0：元素类型升级到 `FCardInstance`，理由同 Backpack。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck")
	TArray<FCardInstance> BattleDeck;

	/**
	 * 负重区（GDD §11.5 / Stage 4.5.1 引入）。
	 *
	 * 当通量 / 备战 / SpecialZone 都装不下时，溢出 instance 进入此处。
	 * 4.5.1 起从纯压力计算量升级为独立数据数组，与其它三区共同维持
	 * "InstanceId 互斥四选一"全局不变量（design.md §Data Models）。
	 *
	 * 写入时机（design.md §Components and Interfaces #6）：
	 *   - URunSession::RecomputeBurden 步骤 ① 把超容卡从对应区末尾摘出追加到本数组末尾；
	 *   - DestroyCardFromBackpack 中 B 主卡销毁分支：Backpack 满时把 SpecialZone 内含卡退到此（R2.4）。
	 *
	 * 读出时机：
	 *   - RecomputeBurden 步骤 ② 按"通量 → 备战 → SpecialZones"优先序回填头部 instance（R2.14）；
	 *   - 步骤 ③ 按 `n*(n+1)/2` 公式写 Burden 通道压力（R9.1，n = 本数组 Num()）。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck")
	TArray<FCardInstance> BurdenZone;

	/**
	 * 每张玩家拥有的 B 类容器卡 instance 各自开辟的特殊存放区集合（Stage 4.5.1 引入）。
	 *
	 * 不变量（design.md §Data Models 全局不变量）：
	 *   - 每条 entry 的 OwnerInstanceId 必须等于 Backpack ∪ BattleDeck 中某张 B 主卡 instance 的 InstanceId；
	 *   - 同一 OwnerInstanceId 在本数组中至多一条（R2.2）；
	 *   - SZ.Cards 与 Backpack / BattleDeck / BurdenZone 及其他 SpecialZone.Cards 互斥不重叠。
	 *
	 * 维护时机（design.md §Components and Interfaces #4 / #6）：
	 *   - B 主卡 instance 进入 Backpack/BattleDeck 时由 URunSession 幂等追加空 entry（R2.3）；
	 *   - B 主卡 instance 永久销毁时按 R2.4 退回内含卡后移除该 entry；
	 *   - 跨 Backpack ↔ BattleDeck 移动时 entry 跟随主卡保留（R5.1，OwnerInstanceId 不变）。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Deck")
	TArray<FSpecialZone> SpecialZones;

	// ---- 既有字段（R5 / S1 骨架）----

	/** 玩家选择的角色。第一阶段固定为 BugGirl。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run")
	TObjectPtr<UCharacterDefinition> Character = nullptr;

	/**
	 * 战斗随机种子。
	 * 0 表示每场战斗独立随机；非 0 时用于复现。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run")
	int32 BattleSeed = 0;

	/** 已击败的敌人 Definition 列表。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	TArray<TObjectPtr<UEnemyDefinition>> DefeatedEnemies;

	/**
	 * 当前 Run 是否仍在进行。
	 * 战内失败：bRunActive = false。
	 * 战外失败（压力满 / 手指掉光）：bRunActive = false。
	 * URunSession::IsRunFailed() 提供综合判定。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	bool bRunActive = true;

	/** 已被永久销毁的场景触发器 ID 列表。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	TSet<FName> DestroyedTriggerIds;

	/**
	 * 战斗节点进度（GDD §10.5 撤离重入）。
	 *
	 * Key = ABattleTriggerActor.PersistentId。Value = 该 Trigger 上次撤离时的破坏状态。
	 * 撤离写入 / 胜利清理 / 失败保留（Run 都结束了无意义保留与否）。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	TMap<FName, FBattleProgressSnapshot> BattleProgress;

	/** 商店节点库存状态。Key = 场景商店/节点 PersistentId；当前只在 Run 内存态保留，不接 SaveGame。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Shop")
	TMap<FName, FRunShopState> ShopStates;

	/** 玩家在探索地图的 Transform。仅当 bHasPlayerTransform == true 时有效。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	FTransform PlayerTransform = FTransform::Identity;

	/** PlayerTransform 是否有效；新开 Run 时为 false。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	bool bHasPlayerTransform = false;
};
