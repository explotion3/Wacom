// Copyright Wacom. All Rights Reserved.

#include "WacomSaveGame.h"

bool FRunCompletionSummarySaveEntry::IsValid() const
{
	return !JourneyId.IsNone()
		&& !TerminalFloorId.IsNone()
		&& !TerminalNodeId.IsNone()
		&& CompletionDay > 0
		&& EnteredFloorCount > 0
		&& TotalFloorCount > 0
		&& EnteredFloorCount <= TotalFloorCount
		&& ResolvedNodeCount > 0
		&& TotalNodeCount > 0
		&& ResolvedNodeCount <= TotalNodeCount
		&& FinalPressure >= 0;
}

namespace
{
	constexpr TCHAR LegacyVenomProofWhitePath[] =
		TEXT("/Game/Wacom/Data/Cards/Debug/ShopUpgrade/DA_Card_TestShopUpgrade_VenomProof_White.DA_Card_TestShopUpgrade_VenomProof_White");
	constexpr TCHAR LegacyVenomProofBluePath[] =
		TEXT("/Game/Wacom/Data/Cards/Debug/ShopUpgrade/DA_Card_TestShopUpgrade_VenomProof_Blue.DA_Card_TestShopUpgrade_VenomProof_Blue");

	bool IsRemovedLegacyUpgradeCard(const FCardInstanceSaveEntry& Entry)
	{
		const FString Path = Entry.DefinitionAssetPath.ToString();
		return Path == LegacyVenomProofWhitePath || Path == LegacyVenomProofBluePath;
	}

	void MigrateCardEntriesToV6(TArray<FCardInstanceSaveEntry>& Entries)
	{
		Entries.RemoveAllSwap(IsRemovedLegacyUpgradeCard, EAllowShrinking::No);
		for (FCardInstanceSaveEntry& Entry : Entries)
		{
			Entry.UpgradeTier = EWacomCardUpgradeTier::White;
			Entry.PersistentModifiers = FWacomCardPersistentModifierState();
		}
	}

	void MigrateCardInstancesToV6(UWacomSaveGame& SaveGame)
	{
		MigrateCardEntriesToV6(SaveGame.Backpack);
		MigrateCardEntriesToV6(SaveGame.BattleDeck);
		MigrateCardEntriesToV6(SaveGame.BurdenZone);
		for (FSpecialZoneSaveEntry& Zone : SaveGame.SpecialZones)
		{
			MigrateCardEntriesToV6(Zone.Cards);
		}
	}

	bool HasValidV5OutcomeSchema(const UWacomSaveGame& SaveGame)
	{
		switch (SaveGame.Outcome)
		{
		case ERunOutcome::InProgress:
		case ERunOutcome::Failed:
			return !SaveGame.bHasCompletionSummary;

		case ERunOutcome::Succeeded:
			return SaveGame.bHasCompletionSummary && SaveGame.CompletionSummary.IsValid();

		default:
			return false;
		}
	}
}

bool UWacomSaveGame::MigrateIfNeeded(UWacomSaveGame* SaveGame)
{
	if (!SaveGame) { return false; }

	// 已经是最新版本：仍需校验 Outcome/摘要组合，不能让非法终态进入 apply。
	if (SaveGame->SaveVersion == CurrentSaveVersion)
	{
		const bool bValid = HasValidV5OutcomeSchema(*SaveGame);
		if (!bValid)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WacomSaveGame] Outcome/CompletionSummary 组合非法"));
		}
		return bValid;
	}

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
		// v0 -> v1：没有 v0 存档实际存在，这里只是为未来迁移链打样。
		// 约定示例：给 v1 才加入的字段填默认值。
		// 本项目 v1 的所有字段默认构造即可，无需额外处理。
		SaveGame->SaveVersion = 1;
		[[fallthrough]];

	case 1:
		// v1 -> v2：引入 instance 列表（Backpack / BattleDeck / BurdenZone / SpecialZones）。
		// 迁移时不修改旧字段，只把新增列表清空；ApplySaveGameToRunState 会根据
		// Character.StarterDeck 重建旧档缺失的 instance 数据。
		SaveGame->Backpack.Empty();
		SaveGame->BattleDeck.Empty();
		SaveGame->BurdenZone.Empty();
		SaveGame->SpecialZones.Empty();
		SaveGame->SaveVersion = 2;
		[[fallthrough]];

	case 2:
		// v2 -> v3：移除 DefeatedEnemyAssetPaths。旧 Trigger 完成投影不再恢复。
		SaveGame->SaveVersion = 3;
		[[fallthrough]];

	case 3:
		// v3 -> v4：引入独立 Run Credential 集合。
		// 旧档明确迁移为空集合；不从可能已被移除的同名实体卡牌反推任务凭证。
		SaveGame->GrantedCredentialIds.Empty();
		SaveGame->SaveVersion = 4;
		[[fallthrough]];

	case 4:
		// v4 -> v5：旧活动位只作为迁移来源；旧档没有成功摘要。
		SaveGame->Outcome = SaveGame->bRunActive
			? ERunOutcome::InProgress
			: ERunOutcome::Failed;
		SaveGame->bHasCompletionSummary = false;
		SaveGame->CompletionSummary = FRunCompletionSummarySaveEntry();
		SaveGame->SaveVersion = 5;
		[[fallthrough]];

	case 5:
		// v5 -> v6：单 Definition 四阶强化。旧链式 Debug 毒牙不属于正式内容，
		// 从所有物理区移除；其余旧实例显式初始化为 White 且没有持久修正。
		MigrateCardInstancesToV6(*SaveGame);
		SaveGame->SaveVersion = 6;
		break;

	default:
		break;
	}

	const bool bOk = SaveGame->SaveVersion == CurrentSaveVersion
		&& HasValidV5OutcomeSchema(*SaveGame);
	if (!bOk)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomSaveGame] 迁移后版本仍为 %d，期望 %d，迁移链可能断了"),
			SaveGame->SaveVersion, CurrentSaveVersion);
	}
	return bOk;
}
