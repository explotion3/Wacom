// Copyright Wacom. All Rights Reserved.

#include "WacomSaveGame.h"

bool UWacomSaveGame::MigrateIfNeeded(UWacomSaveGame* SaveGame)
{
	if (!SaveGame) { return false; }

	// 已经是最新版本：无需迁移。
	if (SaveGame->SaveVersion == CurrentSaveVersion) { return true; }

	// 新版本存档被旧版本客户端读取：拒绝。
	if (SaveGame->SaveVersion > CurrentSaveVersion)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomSaveGame] 存档版本 %d 高于当前 %d，无法迁移"),
			SaveGame->SaveVersion, CurrentSaveVersion);
		return false;
	}

	// 旧版本走升级链。每个 case 处理"从这个版本升到下一个版本"。
	// 约束：
	//   - switch case 用 [[fallthrough]] 一路升到当前
	//   - 已发布的 case 分支不可修改，只能继续往上加 case
	//   - 永远不要删 case
	UE_LOG(LogTemp, Display,
		TEXT("[WacomSaveGame] 存档从 v%d 迁移到 v%d"),
		SaveGame->SaveVersion, CurrentSaveVersion);

	switch (SaveGame->SaveVersion)
	{
	case 0:
		// v0 → v1：S1 正式版，没有 v0 存档实际存在，这里只是为未来迁移链打样。
		// 约定示例：给 v1 才加入的字段填默认值。
		// 本项目 v1 的所有字段默认构造即可，无需额外处理。
		SaveGame->SaveVersion = 1;
		[[fallthrough]];

	case 1:
		// v1 → v2：Stage 4.5.0 引入 instance 列表（Backpack / BattleDeck / BurdenZone / SpecialZones）。
		// R7.3：在不修改任何 v0/v1 已存在字段原值的前提下，把新增的所有 instance 列表与
		//       SpecialZones 列表初始化为空 TArray，并把 SaveVersion 写为 2。
		// Empty() 是幂等的：USTRUCT 默认构造时这些 TArray 已经是空，这里显式 Empty()
		// 仅是把"语义"标注清楚——v1 旧档没有这些字段，迁到 v2 后语义就是"空容器"。
		// 后续 ApplySaveGameToRunState 见到四数组全空即走 R7.4 StarterDeck 重建路径。
		SaveGame->Backpack.Empty();
		SaveGame->BattleDeck.Empty();
		SaveGame->BurdenZone.Empty();
		SaveGame->SpecialZones.Empty();
		SaveGame->SaveVersion = 2;
		[[fallthrough]];

	// case 2:
	//     // v2 → v3：未来加新字段时在这里追加。
	//     SaveGame->SaveVersion = 3;
	//     [[fallthrough]];

	// case CurrentSaveVersion - 1: // 实际会是具体数字
	//     SaveGame->SaveVersion = CurrentSaveVersion;
	//     break;

	default:
		break;
	}

	const bool bOk = (SaveGame->SaveVersion == CurrentSaveVersion);
	if (!bOk)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomSaveGame] 迁移后版本仍为 %d，期望 %d，迁移链可能断了"),
			SaveGame->SaveVersion, CurrentSaveVersion);
	}
	return bOk;
}
