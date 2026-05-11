// Copyright Wacom. All Rights Reserved.

#include "RunSession.h"
#include "WacomSaveGame.h"

#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Session/BattleSession.h"

#include "Kismet/GameplayStatics.h"
#include "UObject/SoftObjectPath.h"

// ================ 生命周期 ================

bool URunSession::Initialize(UCharacterDefinition* InCharacter)
{
	RunState = FRunState{};
	RunState.Character   = InCharacter;
	RunState.BattleSeed  = 0;
	RunState.bRunActive  = true;
	RunState.DefeatedEnemies.Reset();
	RunState.DestroyedTriggerIds.Reset();
	RunState.PlayerTransform   = FTransform::Identity;
	RunState.bHasPlayerTransform = false;

	if (!InCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] Initialize: Character 为空"));
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("[RunSession] Initialized with Character=%s"),
		*GetNameSafe(InCharacter));
	return true;
}

void URunSession::ResetRunState()
{
	UCharacterDefinition* KeepChar = RunState.Character;
	Initialize(KeepChar);
}

// ================ 战斗联动 ================

bool URunSession::BuildInitParamsForBattle(UEnemyDefinition* EnemyDef, FBattleInitParams& OutParams) const
{
	if (!RunState.Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] BuildInitParamsForBattle: RunState.Character 为空"));
		return false;
	}
	if (!EnemyDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] BuildInitParamsForBattle: EnemyDef 为空"));
		return false;
	}

	OutParams.Character  = RunState.Character;
	OutParams.Enemy      = EnemyDef;
	OutParams.RandomSeed = RunState.BattleSeed;
	return true;
}

void URunSession::OnBattleFinished(EBattleOutcome Outcome, UEnemyDefinition* EnemyDef)
{
	switch (Outcome)
	{
	case EBattleOutcome::Victory:
		if (EnemyDef)
		{
			RunState.DefeatedEnemies.AddUnique(EnemyDef);
		}
		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] Battle victory against %s (%d total defeated)"),
			*GetNameSafe(EnemyDef),
			RunState.DefeatedEnemies.Num());
		break;

	case EBattleOutcome::Defeat:
		RunState.bRunActive = false;
		UE_LOG(LogTemp, Display, TEXT("[RunSession] Battle defeat, run ended"));
		break;

	case EBattleOutcome::Undetermined:
	default:
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] OnBattleFinished with Undetermined outcome, ignored"));
		break;
	}
}

// ================ 场景状态 ================

void URunSession::MarkTriggerDestroyed(FName PersistentId)
{
	if (PersistentId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] MarkTriggerDestroyed 收到 NAME_None，忽略"));
		return;
	}
	RunState.DestroyedTriggerIds.Add(PersistentId);
}

bool URunSession::IsTriggerDestroyed(FName PersistentId) const
{
	if (PersistentId.IsNone()) { return false; }
	return RunState.DestroyedTriggerIds.Contains(PersistentId);
}

void URunSession::SetPlayerTransform(const FTransform& InTransform)
{
	RunState.PlayerTransform   = InTransform;
	RunState.bHasPlayerTransform = true;
}

// ================ 存档 / 读档 ================

UWacomSaveGame* URunSession::BuildSaveGameFromRunState() const
{
	UWacomSaveGame* Save = Cast<UWacomSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UWacomSaveGame::StaticClass()));
	if (!Save) { return nullptr; }

	Save->SaveVersion    = UWacomSaveGame::CurrentSaveVersion;
	Save->SavedAtUtc     = FDateTime::UtcNow();
	Save->ClientBuildId  = FString();  // 第一版留空，未来可接 FApp::GetBuildVersion()

	Save->CharacterAssetPath = RunState.Character
		? FSoftObjectPath(RunState.Character)
		: FSoftObjectPath();

	Save->BattleSeed = RunState.BattleSeed;
	Save->bRunActive = RunState.bRunActive;

	Save->DefeatedEnemyAssetPaths.Reset();
	Save->DefeatedEnemyAssetPaths.Reserve(RunState.DefeatedEnemies.Num());
	for (UEnemyDefinition* E : RunState.DefeatedEnemies)
	{
		if (E) { Save->DefeatedEnemyAssetPaths.Add(FSoftObjectPath(E)); }
	}

	Save->DestroyedTriggerIds = RunState.DestroyedTriggerIds.Array();

	Save->PlayerTransform     = RunState.PlayerTransform;
	Save->bHasPlayerTransform = RunState.bHasPlayerTransform;

	return Save;
}

bool URunSession::ApplySaveGameToRunState(UWacomSaveGame* SaveGame)
{
	if (!SaveGame)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] ApplySaveGameToRunState: SaveGame 为空"));
		return false;
	}

	// 版本检查：新版本拒绝
	if (SaveGame->SaveVersion > UWacomSaveGame::CurrentSaveVersion)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] 存档版本 %d 高于当前 %d，拒绝读档"),
			SaveGame->SaveVersion, UWacomSaveGame::CurrentSaveVersion);
		return false;
	}

	// 旧版本走迁移链。迁移失败拒绝读档。
	if (!UWacomSaveGame::MigrateIfNeeded(SaveGame))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] 存档迁移失败（源版本 %d → 目标 %d），拒绝读档"),
			SaveGame->SaveVersion, UWacomSaveGame::CurrentSaveVersion);
		return false;
	}

	// Character 资产必须加载成功；失败说明 Character 被删了，整个档作废。
	UCharacterDefinition* LoadedChar = Cast<UCharacterDefinition>(
		SaveGame->CharacterAssetPath.TryLoad());
	if (!LoadedChar)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] CharacterAssetPath 加载失败: %s"),
			*SaveGame->CharacterAssetPath.ToString());
		return false;
	}

	// 一切 OK，开始改 RunState。
	RunState = FRunState{};
	RunState.Character   = LoadedChar;
	RunState.BattleSeed  = SaveGame->BattleSeed;
	RunState.bRunActive  = SaveGame->bRunActive;

	RunState.DefeatedEnemies.Reset();
	RunState.DefeatedEnemies.Reserve(SaveGame->DefeatedEnemyAssetPaths.Num());
	for (const FSoftObjectPath& Path : SaveGame->DefeatedEnemyAssetPaths)
	{
		if (UEnemyDefinition* E = Cast<UEnemyDefinition>(Path.TryLoad()))
		{
			RunState.DefeatedEnemies.Add(E);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RunSession] DefeatedEnemy 加载失败，跳过: %s"),
				*Path.ToString());
		}
	}

	RunState.DestroyedTriggerIds.Reset();
	for (const FName& Id : SaveGame->DestroyedTriggerIds)
	{
		if (!Id.IsNone()) { RunState.DestroyedTriggerIds.Add(Id); }
	}

	RunState.PlayerTransform     = SaveGame->PlayerTransform;
	RunState.bHasPlayerTransform = SaveGame->bHasPlayerTransform;

	return true;
}

bool URunSession::SaveToSlot(const FString& SlotName) const
{
	UWacomSaveGame* Save = BuildSaveGameFromRunState();
	if (!Save)
	{
		UE_LOG(LogTemp, Error, TEXT("[RunSession] SaveToSlot(%s) 构造 SaveGame 失败"), *SlotName);
		return false;
	}

	const bool bOk = UGameplayStatics::SaveGameToSlot(Save, SlotName, /*UserIndex*/0);
	if (!bOk)
	{
		UE_LOG(LogTemp, Error, TEXT("[RunSession] SaveToSlot(%s) 写入磁盘失败"), *SlotName);
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("[RunSession] SaveToSlot(%s) OK，版本 %d"),
		*SlotName, Save->SaveVersion);
	return true;
}

bool URunSession::LoadFromSlot(const FString& SlotName)
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, /*UserIndex*/0))
	{
		UE_LOG(LogTemp, Display, TEXT("[RunSession] LoadFromSlot(%s): 存档不存在"), *SlotName);
		return false;
	}

	USaveGame* Base = UGameplayStatics::LoadGameFromSlot(SlotName, /*UserIndex*/0);
	UWacomSaveGame* Save = Cast<UWacomSaveGame>(Base);
	if (!Save)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] LoadFromSlot(%s): SaveGame 类型不匹配"), *SlotName);
		return false;
	}

	if (!ApplySaveGameToRunState(Save))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] LoadFromSlot(%s): 应用到 RunState 失败"), *SlotName);
		return false;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[RunSession] LoadFromSlot(%s) OK: Character=%s, Defeated=%d, Triggers=%d, HasPlayerTransform=%d"),
		*SlotName,
		*GetNameSafe(RunState.Character),
		RunState.DefeatedEnemies.Num(),
		RunState.DestroyedTriggerIds.Num(),
		RunState.bHasPlayerTransform ? 1 : 0);
	return true;
}

bool URunSession::HasSaveInSlot(const FString& SlotName) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, /*UserIndex*/0);
}
