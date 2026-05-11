// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UObject/SoftObjectPath.h"
#include "Misc/DateTime.h"
#include "WacomSaveGame.generated.h"

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
 *   - 详细参见 Docs/Save_System_Plan.md §5
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
	 */
	static constexpr int32 CurrentSaveVersion = 1;

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

	/** 已击败敌人资产路径列表。 */
	UPROPERTY(SaveGame)
	TArray<FSoftObjectPath> DefeatedEnemyAssetPaths;

	UPROPERTY(SaveGame)
	bool bRunActive = true;

	// ---- 场景数据 ----

	/**
	 * 已被永久销毁的触发器 ID 列表。
	 * SaveGame 里用 TArray 而不是 TSet，避免 FArchive 对 TSet 序列化的潜在兼容问题。
	 * 内存里在 FRunState 做 TSet<FName>，拷贝时转换。
	 */
	UPROPERTY(SaveGame)
	TArray<FName> DestroyedTriggerIds;

	UPROPERTY(SaveGame)
	FTransform PlayerTransform = FTransform::Identity;

	UPROPERTY(SaveGame)
	bool bHasPlayerTransform = false;
};
