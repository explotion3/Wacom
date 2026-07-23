// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RunOutcomeTypes.h"
#include "UObject/SoftObjectPath.h"
#include "Misc/DateTime.h"
#include "WacomSaveGame.generated.h"

/**
 * 单张卡 instance 的磁盘条目（SaveGame v2 起）。
 *
 * 与内存中的 FCardInstance 对应：
 *   FCardInstance.InstanceId                    → FCardInstanceSaveEntry.InstanceId
 *   FCardInstance.Definition (TObjectPtr<...>)  → FCardInstanceSaveEntry.DefinitionAssetPath (FSoftObjectPath)
 *   FCardInstance.bBattleEnabledInSpecialZone   → FCardInstanceSaveEntry.bBattleEnabledInSpecialZone
 *
 * 反射门槛：仅 SaveGame 序列化需要 USTRUCT 反射，蓝图不直接访问，因此不带 BlueprintType。
 */
USTRUCT()
struct WACOMRUN_API FCardInstanceSaveEntry
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FGuid InstanceId;

	UPROPERTY(SaveGame)
	FSoftObjectPath DefinitionAssetPath;

	UPROPERTY(SaveGame)
	bool bBattleEnabledInSpecialZone = false;
};

/**
 * 单个 SpecialZone 的磁盘条目（SaveGame v2 起）。
 *
 * 与内存中的 FSpecialZone 对应：
 *   FSpecialZone.OwnerInstanceId  → FSpecialZoneSaveEntry.OwnerInstanceId
 *   FSpecialZone.Cards            → FSpecialZoneSaveEntry.Cards (TArray<FCardInstanceSaveEntry>)
 *
 * 反射门槛同 FCardInstanceSaveEntry，不带 BlueprintType。
 */
USTRUCT()
struct WACOMRUN_API FSpecialZoneSaveEntry
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FGuid OwnerInstanceId;

	UPROPERTY(SaveGame)
	TArray<FCardInstanceSaveEntry> Cards;
};

/**
 * Journey 成功摘要的稳定磁盘结构（SaveGame v5 起）。
 *
 * 与 FRunCompletionSummary 保持逐字段转换，但不直接序列化 runtime struct，避免后续
 * 运行时展示字段扩展无意改变历史存档 schema。
 */
USTRUCT()
struct WACOMRUN_API FRunCompletionSummarySaveEntry
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FName JourneyId;

	UPROPERTY(SaveGame)
	FName TerminalFloorId;

	UPROPERTY(SaveGame)
	FName TerminalNodeId;

	UPROPERTY(SaveGame)
	int32 CompletionDay = 0;

	UPROPERTY(SaveGame)
	int32 EnteredFloorCount = 0;

	UPROPERTY(SaveGame)
	int32 TotalFloorCount = 0;

	UPROPERTY(SaveGame)
	int32 ResolvedNodeCount = 0;

	UPROPERTY(SaveGame)
	int32 TotalNodeCount = 0;

	UPROPERTY(SaveGame)
	int32 FinalPressure = 0;

	bool IsValid() const;
};

/**
 * Wacom 项目的 SaveGame 磁盘数据。
 *
 * 和 FRunState 的区别：
 *   - FRunState 是内存运行时数据（TObjectPtr 直接引用）
 *   - UWacomSaveGame 是磁盘格式（FSoftObjectPath 按路径加载）
 *
 * 两者通过 URunSession::SaveToSlot / LoadFromSlot 做字段拷贝。
 *
 * 版本机制：
 *   - 每次结构变更 CurrentSaveVersion++
 *   - Load 时比较 SaveVersion，旧版走迁移，新版拒绝
 *   - 存档边界见 Docs/WacomRun.md 与 Docs/TechDebt.md
 */
UCLASS()
class WACOMRUN_API UWacomSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/**
	 * 当前代码支持的最新版本。
	 *
	 * 改结构时 ++。加字段 / 改字段语义 / 删字段都要升版本。
	 * 同时在 MigrateIfNeeded 里追加一条 case 处理从前一版到当前的迁移。
	 *
	 * 版本演进：
	 *   v0 → v1: 保留位（首版默认结构，初始迁移占位）
	 *   v1 → v2: 引入 Backpack/BattleDeck/BurdenZone/SpecialZones instance 列表
	 *   v2 → v3: 移除 DefeatedEnemyAssetPaths（旧 Trigger 完成投影不再受支持）
	 *   v3 → v4: 引入独立于实体卡牌的 Run Credential 集合
	 *   v4 → v5: 引入 Run Outcome 与独立 Journey 成功摘要
	 */
	static constexpr int32 CurrentSaveVersion = 5;

	/**
	 * 防止有人未同步修改 MigrateIfNeeded 迁移链就升 / 降版本号。
	 * 这里的硬编码值必须与上面的 CurrentSaveVersion 保持一致；改版本号时要么:
	 *   - 同步把这里的硬编码值与 MigrateIfNeeded 的 case 链一起改；
	 *   - 要么不改（编译失败提醒下一位作者去看 MigrateIfNeeded）。
	 */
	static_assert(CurrentSaveVersion == 5,
		"CurrentSaveVersion 升级必须同步更新 MigrateIfNeeded 的 case 链与本断言。");

	/**
	 * 把 SaveGame 从更老的版本逐步迁移到 CurrentSaveVersion。
	 *
	 * 调用前：SaveGame->SaveVersion 可能 < CurrentSaveVersion。
	 * 调用后：若返回 true，SaveGame->SaveVersion == CurrentSaveVersion，字段都已填好。
	 *
	 * 约定：
	 * - `switch` case 用 fallthrough，从入档版本开始逐步往上推
	 * - 已发布的 case 分支不可修改，只能往上继续加 case
	 * - 返回 false 表示无法迁移（版本号 > Current、未知路径）
	 */
	static bool MigrateIfNeeded(UWacomSaveGame* SaveGame);

	/** 本存档文件写入时的版本号。 */
	UPROPERTY(SaveGame)
	int32 SaveVersion = CurrentSaveVersion;

	/** 写入时间戳（UTC）。调试 / 显示用。 */
	UPROPERTY(SaveGame)
	FDateTime SavedAtUtc = FDateTime::MinValue();

	/** 可选 build 标识，方便 QA 复现。 */
	UPROPERTY(SaveGame)
	FString ClientBuildId;

	// ---- Run 数据 ----

	/** 当前角色资产路径。 */
	UPROPERTY(SaveGame)
	FSoftObjectPath CharacterAssetPath;

	UPROPERTY(SaveGame)
	int32 BattleSeed = 0;

	/** v4 及更早存档的迁移来源；v5 runtime/apply 不再读取此字段。 */
	UPROPERTY(SaveGame)
	bool bRunActive = true;

	/** v5 权威 Run 结果。 */
	UPROPERTY(SaveGame)
	ERunOutcome Outcome = ERunOutcome::InProgress;

	/** v5 成功摘要存在位；只有 Outcome=Succeeded 时允许为 true。 */
	UPROPERTY(SaveGame)
	bool bHasCompletionSummary = false;

	/** v5 最近一次 Journey 成功摘要。 */
	UPROPERTY(SaveGame)
	FRunCompletionSummarySaveEntry CompletionSummary;

	// ---- 场景数据 ----

	/**
	 * 已获得的稳定 Run Credential ID。
	 * 内存中由 FRunState.GrantedCredentialIds 以 TSet 持有；写盘前按 FName 词法序排序。
	 * 所有条目必须非 None 且唯一，非法存档会被原子拒绝，不从同名实体卡牌推断。
	 */
	UPROPERTY(SaveGame)
	TArray<FName> GrantedCredentialIds;

	UPROPERTY(SaveGame)
	FTransform PlayerTransform = FTransform::Identity;

	UPROPERTY(SaveGame)
	bool bHasPlayerTransform = false;

	// ---- v2 instance 列表 ----
	//
	// 与 FRunState 内存字段的对应关系：
	//   FRunState.Backpack       (TArray<FCardInstance>)  → Backpack       (TArray<FCardInstanceSaveEntry>)
	//   FRunState.BattleDeck     (TArray<FCardInstance>)  → BattleDeck     (TArray<FCardInstanceSaveEntry>)
	//   FRunState.BurdenZone     (TArray<FCardInstance>)  → BurdenZone     (TArray<FCardInstanceSaveEntry>)
	//   FRunState.SpecialZones   (TArray<FSpecialZone>)   → SpecialZones   (TArray<FSpecialZoneSaveEntry>)
	//
	// 写入约束：
	//   - 三个 instance 列表合并去重后所有 InstanceId 必须全局唯一、非 zero GUID。
	//   - BuildSaveGameFromRunState 把 BurdenZone / SpecialZones 一并填充实际数据。
	//
	// 读取约束：
	//   - 四数组全空 → ApplySaveGameToRunState 走 StarterDeck 重建路径。
	//   - 任一非空 → 按 SaveEntry 还原；失败任一校验则拒绝加载并保持 RunState 不变。

	UPROPERTY(SaveGame)
	TArray<FCardInstanceSaveEntry> Backpack;

	UPROPERTY(SaveGame)
	TArray<FCardInstanceSaveEntry> BattleDeck;

	UPROPERTY(SaveGame)
	TArray<FCardInstanceSaveEntry> BurdenZone;

	UPROPERTY(SaveGame)
	TArray<FSpecialZoneSaveEntry> SpecialZones;
};
