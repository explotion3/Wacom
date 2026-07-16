// Copyright Wacom. All Rights Reserved.

#include "Save/RunSaveGameSerializer.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Deck/RunDeckRules.h"
#include "RunState.h"
#include "WacomSaveGame.h"

#include "Kismet/GameplayStatics.h"
#include "UObject/SoftObjectPath.h"

namespace
{
	bool ShouldStarterCardStartInBattleDeck(const UCardDefinition* Card)
	{
		// 原型内容规则：暮色引虫灯默认进入备战区，但仍作为 A 类容器贡献通量容量。
		// 后续若类似规则增多，应抽成 Card/Character 数据字段，而不是继续扩硬编码列表。
		return Card && Card->CardId == FName(TEXT("MuseiYinchongdeng"));
	}

	bool RestoreCardInstanceList(const TArray<FCardInstanceSaveEntry>& Source,
	                              TArray<FCardInstance>& Dest,
	                              TSet<FGuid>& SeenInstanceIds,
	                              const TCHAR* ZoneName,
	                              FString& OutErr)
	{
		Dest.Reset();
		Dest.Reserve(Source.Num());
		for (const FCardInstanceSaveEntry& Entry : Source)
		{
			if (!Entry.InstanceId.IsValid())
			{
				OutErr = FString::Printf(
					TEXT("zone=%s entry InstanceId 为 zero GUID"), ZoneName);
				return false;
			}
			bool bAlreadyInSet = false;
			SeenInstanceIds.Add(Entry.InstanceId, &bAlreadyInSet);
			if (bAlreadyInSet)
			{
				OutErr = FString::Printf(
					TEXT("zone=%s 中 InstanceId %s 与其他 zone 重复"),
					ZoneName, *Entry.InstanceId.ToString());
				return false;
			}
			UCardDefinition* Def = Cast<UCardDefinition>(Entry.DefinitionAssetPath.TryLoad());
			if (!Def)
			{
				OutErr = FString::Printf(
					TEXT("zone=%s InstanceId=%s DefinitionAssetPath 加载失败: %s"),
					ZoneName, *Entry.InstanceId.ToString(),
					*Entry.DefinitionAssetPath.ToString());
				return false;
			}
			FCardInstance Inst;
			Inst.InstanceId = Entry.InstanceId;
			Inst.Definition = Def;
			Inst.bBattleEnabledInSpecialZone = Entry.bBattleEnabledInSpecialZone;
			Dest.Add(MoveTemp(Inst));
		}
		return true;
	}

	bool OwnerInBackpackOrBattleDeck(const FRunState& State, FGuid OwnerInstanceId)
	{
		for (const FCardInstance& Inst : State.Backpack)
		{
			if (Inst.InstanceId == OwnerInstanceId)
			{
				return true;
			}
		}
		for (const FCardInstance& Inst : State.BattleDeck)
		{
			if (Inst.InstanceId == OwnerInstanceId)
			{
				return true;
			}
		}
		return false;
	}
}

UWacomSaveGame* FRunSaveGameSerializer::BuildSaveGameFromRunState(const FRunState& State)
{
	UWacomSaveGame* Save = Cast<UWacomSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UWacomSaveGame::StaticClass()));
	if (!Save)
	{
		return nullptr;
	}

	Save->SaveVersion = UWacomSaveGame::CurrentSaveVersion;
	Save->SavedAtUtc = FDateTime::UtcNow();
	Save->ClientBuildId = FString();

	Save->CharacterAssetPath = State.Character
		? FSoftObjectPath(State.Character)
		: FSoftObjectPath();

	Save->BattleSeed = State.BattleSeed;
	Save->bRunActive = State.bRunActive;

	Save->DestroyedTriggerIds = State.DestroyedTriggerIds.Array();
	Save->GrantedCredentialIds = State.GrantedCredentialIds.Array();
	Save->GrantedCredentialIds.Sort(FNameLexicalLess());
	Save->PlayerTransform = State.PlayerTransform;
	Save->bHasPlayerTransform = State.bHasPlayerTransform;

	Save->Backpack.Reset();
	Save->BattleDeck.Reset();
	Save->BurdenZone.Reset();
	Save->SpecialZones.Reset();

	TSet<FGuid> SeenInstanceIds;
	{
		int32 SpecialZoneCardTotal = 0;
		for (const FSpecialZone& SpecialZone : State.SpecialZones)
		{
			SpecialZoneCardTotal += SpecialZone.Cards.Num();
		}
		SeenInstanceIds.Reserve(
			State.Backpack.Num() + State.BattleDeck.Num()
			+ State.BurdenZone.Num() + SpecialZoneCardTotal);
	}

	auto WriteInstanceList = [&SeenInstanceIds](const TArray<FCardInstance>& Source,
	                                             TArray<FCardInstanceSaveEntry>& Dest,
	                                             const TCHAR* ZoneName)
	{
		Dest.Reserve(Source.Num());
		for (const FCardInstance& Inst : Source)
		{
			if (!Inst.InstanceId.IsValid())
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunSession] BuildSaveGameFromRunState: %s 中存在 zero GUID InstanceId 的 instance（Definition=%s），跳过"),
					ZoneName, *GetNameSafe(Inst.Definition));
				continue;
			}

			bool bAlreadyInSet = false;
			SeenInstanceIds.Add(Inst.InstanceId, &bAlreadyInSet);
			if (bAlreadyInSet)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunSession] BuildSaveGameFromRunState: %s 中 InstanceId %s 与其他 zone 重复，跳过"),
					ZoneName, *Inst.InstanceId.ToString());
				continue;
			}

			FCardInstanceSaveEntry Entry;
			Entry.InstanceId = Inst.InstanceId;
			Entry.DefinitionAssetPath = Inst.Definition
				? FSoftObjectPath(Inst.Definition)
				: FSoftObjectPath();
			Entry.bBattleEnabledInSpecialZone = Inst.bBattleEnabledInSpecialZone;
			Dest.Add(MoveTemp(Entry));
		}
	};

	WriteInstanceList(State.Backpack, Save->Backpack, TEXT("Backpack"));
	WriteInstanceList(State.BattleDeck, Save->BattleDeck, TEXT("BattleDeck"));
	WriteInstanceList(State.BurdenZone, Save->BurdenZone, TEXT("BurdenZone"));

	Save->SpecialZones.Reserve(State.SpecialZones.Num());
	for (const FSpecialZone& SpecialZone : State.SpecialZones)
	{
		if (!SpecialZone.OwnerInstanceId.IsValid())
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunSession] BuildSaveGameFromRunState: SpecialZone OwnerInstanceId 为 zero GUID（Cards=%d），跳过整条"),
				SpecialZone.Cards.Num());
			continue;
		}

		if (!OwnerInBackpackOrBattleDeck(State, SpecialZone.OwnerInstanceId))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunSession] BuildSaveGameFromRunState: SpecialZone OwnerInstanceId %s 在 Backpack/BattleDeck 中找不到 owner instance，跳过整条"),
				*SpecialZone.OwnerInstanceId.ToString());
			continue;
		}

		FSpecialZoneSaveEntry Entry;
		Entry.OwnerInstanceId = SpecialZone.OwnerInstanceId;
		const FString ZoneNameStr = FString::Printf(TEXT("SpecialZone[%s]"), *SpecialZone.OwnerInstanceId.ToString());
		WriteInstanceList(SpecialZone.Cards, Entry.Cards, *ZoneNameStr);
		Save->SpecialZones.Add(MoveTemp(Entry));
	}

	return Save;
}

bool FRunSaveGameSerializer::TryApplySaveGameToRunState(UWacomSaveGame* SaveGame, FRunState& InOutState)
{
	if (!SaveGame)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] ApplySaveGameToRunState: SaveGame 为空"));
		return false;
	}

	if (SaveGame->SaveVersion > UWacomSaveGame::CurrentSaveVersion)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] 存档版本 %d 高于当前 %d，拒绝读档"),
			SaveGame->SaveVersion, UWacomSaveGame::CurrentSaveVersion);
		return false;
	}

	if (!UWacomSaveGame::MigrateIfNeeded(SaveGame))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] 存档迁移失败（源版本 %d → 目标 %d），拒绝读档"),
			SaveGame->SaveVersion, UWacomSaveGame::CurrentSaveVersion);
		return false;
	}

	TSet<FName> GrantedCredentialIds;
	GrantedCredentialIds.Reserve(SaveGame->GrantedCredentialIds.Num());
	for (const FName CredentialId : SaveGame->GrantedCredentialIds)
	{
		if (CredentialId.IsNone())
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: GrantedCredentialIds 包含 None"));
			return false;
		}

		bool bAlreadyGranted = false;
		GrantedCredentialIds.Add(CredentialId, &bAlreadyGranted);
		if (bAlreadyGranted)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: GrantedCredentialIds 重复 ID %s"),
				*CredentialId.ToString());
			return false;
		}
	}

	UCharacterDefinition* LoadedChar = Cast<UCharacterDefinition>(
		SaveGame->CharacterAssetPath.TryLoad());
	if (!LoadedChar)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] CharacterAssetPath 加载失败: %s"),
			*SaveGame->CharacterAssetPath.ToString());
		return false;
	}

	FRunState TempState;
	TempState.Character = LoadedChar;
	TempState.BattleSeed = SaveGame->BattleSeed;
	TempState.bRunActive = SaveGame->bRunActive;
	TempState.GrantedCredentialIds = MoveTemp(GrantedCredentialIds);
	TempState.PlayerTransform = SaveGame->PlayerTransform;
	TempState.bHasPlayerTransform = SaveGame->bHasPlayerTransform;

	TempState.DestroyedTriggerIds.Reset();
	for (const FName& Id : SaveGame->DestroyedTriggerIds)
	{
		if (!Id.IsNone())
		{
			TempState.DestroyedTriggerIds.Add(Id);
		}
	}

	const bool bAllInstanceArraysEmpty =
		   SaveGame->Backpack.Num() == 0
		&& SaveGame->BattleDeck.Num() == 0
		&& SaveGame->BurdenZone.Num() == 0
		&& SaveGame->SpecialZones.Num() == 0;

	if (bAllInstanceArraysEmpty)
	{
		for (const TObjectPtr<UCardDefinition>& Card : LoadedChar->StarterDeck)
		{
			if (!Card)
			{
				continue;
			}

			FCardInstance Inst;
			Inst.Definition = Card;
			Inst.InstanceId = FGuid::NewGuid();
			ensureMsgf(Inst.InstanceId.IsValid(),
				TEXT("[RunSession] ApplySaveGameToRunState (StarterDeck rebuild): FGuid::NewGuid() 生成 zero GUID"));

			if (ShouldStarterCardStartInBattleDeck(Card))
			{
				TempState.BattleDeck.Add(Inst);
			}
			else if (FRunDeckRules::IsContainerCard(Card))
			{
				TempState.Backpack.Add(Inst);
			}
			else
			{
				TempState.BattleDeck.Add(Inst);
			}

			if (FRunDeckRules::IsTypeBContainerCard(Inst.Definition) && Inst.InstanceId.IsValid())
			{
				FSpecialZone NewEntry;
				NewEntry.OwnerInstanceId = Inst.InstanceId;
				TempState.SpecialZones.Add(MoveTemp(NewEntry));
			}
		}

		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] ApplySaveGameToRunState: SaveVersion=%d 四数组全空，按 Character=%s StarterDeck 重建（Backpack=%d, BattleDeck=%d, SpecialZones=%d）"),
			SaveGame->SaveVersion, *GetNameSafe(LoadedChar),
			TempState.Backpack.Num(), TempState.BattleDeck.Num(), TempState.SpecialZones.Num());
	}
	else
	{
		TSet<FGuid> SeenInstanceIds;
		{
			int32 SpecialZoneCardTotal = 0;
			for (const FSpecialZoneSaveEntry& SpecialZone : SaveGame->SpecialZones)
			{
				SpecialZoneCardTotal += SpecialZone.Cards.Num();
			}
			SeenInstanceIds.Reserve(
				SaveGame->Backpack.Num() + SaveGame->BattleDeck.Num()
				+ SaveGame->BurdenZone.Num() + SpecialZoneCardTotal);
		}

		FString ErrMsg;
		if (!RestoreCardInstanceList(SaveGame->Backpack, TempState.Backpack, SeenInstanceIds, TEXT("Backpack"), ErrMsg))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: %s"), *ErrMsg);
			return false;
		}
		if (!RestoreCardInstanceList(SaveGame->BattleDeck, TempState.BattleDeck, SeenInstanceIds, TEXT("BattleDeck"), ErrMsg))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: %s"), *ErrMsg);
			return false;
		}
		if (!RestoreCardInstanceList(SaveGame->BurdenZone, TempState.BurdenZone, SeenInstanceIds, TEXT("BurdenZone"), ErrMsg))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: %s"), *ErrMsg);
			return false;
		}

		TSet<FGuid> SeenSpecialZoneOwners;
		SeenSpecialZoneOwners.Reserve(SaveGame->SpecialZones.Num());
		TempState.SpecialZones.Reserve(SaveGame->SpecialZones.Num());

		for (const FSpecialZoneSaveEntry& SpecialZoneEntry : SaveGame->SpecialZones)
		{
			if (!SpecialZoneEntry.OwnerInstanceId.IsValid())
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: SpecialZone OwnerInstanceId 为 zero GUID"));
				return false;
			}

			bool bAlreadyOwner = false;
			SeenSpecialZoneOwners.Add(SpecialZoneEntry.OwnerInstanceId, &bAlreadyOwner);
			if (bAlreadyOwner)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: SpecialZone OwnerInstanceId %s 在 SaveGame.SpecialZones 中重复"),
					*SpecialZoneEntry.OwnerInstanceId.ToString());
				return false;
			}

			if (!OwnerInBackpackOrBattleDeck(TempState, SpecialZoneEntry.OwnerInstanceId))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: SpecialZone OwnerInstanceId %s 在 Backpack/BattleDeck 中找不到 owner instance"),
					*SpecialZoneEntry.OwnerInstanceId.ToString());
				return false;
			}

			FSpecialZone Restored;
			Restored.OwnerInstanceId = SpecialZoneEntry.OwnerInstanceId;
			const FString ZoneNameStr = FString::Printf(TEXT("SpecialZone[%s]"), *SpecialZoneEntry.OwnerInstanceId.ToString());
			if (!RestoreCardInstanceList(SpecialZoneEntry.Cards, Restored.Cards, SeenInstanceIds, *ZoneNameStr, ErrMsg))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunSession] ApplySaveGameToRunState 拒绝加载: %s"), *ErrMsg);
				return false;
			}

			TempState.SpecialZones.Add(MoveTemp(Restored));
		}

		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] ApplySaveGameToRunState: SaveVersion=%d 按 SaveEntry 还原（Backpack=%d, BattleDeck=%d, BurdenZone=%d, SpecialZones=%d）"),
			SaveGame->SaveVersion,
			TempState.Backpack.Num(), TempState.BattleDeck.Num(),
			TempState.BurdenZone.Num(), TempState.SpecialZones.Num());
	}

	InOutState = MoveTemp(TempState);
	return true;
}
