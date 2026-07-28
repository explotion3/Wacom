// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Misc/AutomationTest.h"

#include "RunSession.h"
#include "RunState.h"
#include "WacomSaveGame.h"

#include "Characters/CharacterDefinition.h"
#include "Cards/CardDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Tags/WacomGameplayTags.h"

#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/SoftObjectPath.h"

/**
 * Save/Load 往返测试。
 *
 * 覆盖：
 *   - 字段往返无损（Character / BattleSeed / PlayerTransform / Outcome）
 *   - HasSaveInSlot 前后返回值
 *   - 手动构造 SaveVersion > Current 的 SaveGame，ApplySaveGameToRunState 返回 false
 *   - ApplySaveGameToRunState(nullptr) 返回 false
 *   - LoadFromSlot(不存在的 slot) 返回 false
 *
 * 不走磁盘 SaveGameToSlot / LoadGameFromSlot 的 IO 部分单独用一个 slot 名走一遍，
 * 测完清理文件。
 */

namespace
{
	const FString TestSlot = TEXT("Wacom_SaveRoundtripTest");
	const FString NotifyTestSlot = TEXT("Wacom_SaveLoadNotifyTest");

	void ClearTestSlot()
	{
		if (UGameplayStatics::DoesSaveGameExist(TestSlot, 0))
		{
			UGameplayStatics::DeleteGameInSlot(TestSlot, 0);
		}
		if (UGameplayStatics::DoesSaveGameExist(NotifyTestSlot, 0))
		{
			UGameplayStatics::DeleteGameInSlot(NotifyTestSlot, 0);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSaveGameRoundtripSpec,
	"Wacom.Run.Save.Roundtrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSaveGameRoundtripSpec::RunTest(const FString& /*Parameters*/)
{
	ClearTestSlot();

	FWacomBattleFixture Fx;

	// 构造一个最小角色。
	UCardDefinition* LH = Fx.MakeNoopCard(1);
	UCardDefinition* RH = Fx.MakeNoopCard(1);
	TArray<UCardDefinition*> Deck = { Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) };
	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);

	// --- Session A：填数据，Save ---
	TStrongObjectPtr<URunSession> A(NewObject<URunSession>());
	InitializeRunSessionForTest(*A, Char).IsOk();

	FRunState& SA = FWacomRunSessionTestAccess::GetMutableRunState(*A.Get());
	SA.BattleSeed = 4242;
	SA.Outcome = ERunOutcome::InProgress;
	SA.PlayerTransform     = FTransform(FRotator(0, 90, 0), FVector(100, 200, 300));
	SA.bHasPlayerTransform = true;

	// ---- 结构层测试：BuildSaveGameFromRunState + ApplySaveGameToRunState ----

	UWacomSaveGame* Sg = A->BuildSaveGameFromRunState();
	TestNotNull(TEXT("BuildSaveGameFromRunState non-null"), Sg);
	if (!Sg) { return false; }

	TestEqual(TEXT("Save schema is version 6"), UWacomSaveGame::CurrentSaveVersion, 6);
	TestEqual(TEXT("SaveVersion == Current"), Sg->SaveVersion, UWacomSaveGame::CurrentSaveVersion);
	TestEqual(TEXT("BattleSeed passthrough"), Sg->BattleSeed, 4242);
	TestEqual(TEXT("Outcome passthrough"), Sg->Outcome, ERunOutcome::InProgress);
	TestFalse(TEXT("In-progress save has no completion summary"), Sg->bHasCompletionSummary);
	TestTrue(TEXT("bHasPlayerTransform passthrough"), Sg->bHasPlayerTransform);

	// Apply 回一个新 Session B（Transient 对象 FSoftObjectPath::TryLoad 走内存查找，能找到）
	TStrongObjectPtr<URunSession> B(NewObject<URunSession>());
	InitializeRunSessionForTest(*B, Char).IsOk();  // 先给 Character，不影响后续 Apply 会覆盖

	const bool bApplied = B->ApplySaveGameToRunState(Sg);
	TestTrue(TEXT("ApplySaveGameToRunState ok"), bApplied);

	const FRunState& SB = B->GetRunState();
	TestEqual(TEXT("Character roundtrip"), SB.Character.Get(), Char);
	TestEqual(TEXT("BattleSeed roundtrip"), SB.BattleSeed, 4242);
	TestEqual(TEXT("Outcome roundtrip"), SB.Outcome, ERunOutcome::InProgress);
	TestTrue(TEXT("bHasPlayerTransform roundtrip"), SB.bHasPlayerTransform);
	TestTrue(TEXT("PlayerTransform location roundtrip"),
		SB.PlayerTransform.GetLocation().Equals(FVector(100, 200, 300)));
	TestTrue(TEXT("PlayerTransform rotation roundtrip"),
		SB.PlayerTransform.Rotator().Equals(FRotator(0, 90, 0)));

	// Schema 5 仍未持久化正式探索图状态。读档只恢复旧 Run/卡牌与 Credential 字段；
	// Journey/Floor/Node 必须由未来独立存档切片定义新 schema 后才能恢复。
	TestNull(TEXT("Schema 5 does not restore Journey"),
		SB.ExplorationState.JourneyDefinition.Get());
	TestTrue(TEXT("Schema 5 does not restore FloorId"),
		SB.ExplorationState.CurrentFloorId.IsNone());
	TestTrue(TEXT("Schema 5 does not restore NodeId"),
		SB.ExplorationState.CurrentNodeId.IsNone());
	TestEqual(TEXT("Schema 5 does not restore floor progress"),
		SB.ExplorationState.FloorProgress.Num(), 0);
	TestEqual(TEXT("Schema 5 does not restore exploration version"),
		SB.ExplorationState.ExplorationStateVersion, 0);

	// ---- 版本拒绝：构造一个 SaveVersion > Current 的 SaveGame ----
	{
		UWacomSaveGame* FutureSg = A->BuildSaveGameFromRunState();
		FutureSg->SaveVersion = UWacomSaveGame::CurrentSaveVersion + 1;

		TStrongObjectPtr<URunSession> C(NewObject<URunSession>());
		InitializeRunSessionForTest(*C, Char).IsOk();
		const bool bOk = C->ApplySaveGameToRunState(FutureSg);
		TestFalse(TEXT("Future SaveVersion rejected"), bOk);
	}

	// ---- 迁移链：构造一个 SaveVersion < Current 的 SaveGame ----
	// v0 目前被视作合法起点（S4 迁移链的第一站），应该被升级到 Current 并可读。
	{
		UWacomSaveGame* OldSg = A->BuildSaveGameFromRunState();
		OldSg->SaveVersion = 0;

		// 直接测迁移函数
		const bool bMigrated = UWacomSaveGame::MigrateIfNeeded(OldSg);
		TestTrue(TEXT("v0 migrates to Current"), bMigrated);
		TestEqual(TEXT("SaveVersion after migration == Current"),
			OldSg->SaveVersion, UWacomSaveGame::CurrentSaveVersion);

		// 端到端：ApplySaveGameToRunState 也应该对 v0 成功
		UWacomSaveGame* OldSg2 = A->BuildSaveGameFromRunState();
		OldSg2->SaveVersion = 0;

		TStrongObjectPtr<URunSession> MigSession(NewObject<URunSession>());
		InitializeRunSessionForTest(*MigSession, Char).IsOk();
		const bool bAppliedOld = MigSession->ApplySaveGameToRunState(OldSg2);
		TestTrue(TEXT("ApplySaveGameToRunState accepts v0 via migration"), bAppliedOld);
		TestEqual(TEXT("Migrated run state BattleSeed"),
			MigSession->GetRunState().BattleSeed, 4242);
	}

	// ---- 空指针拒绝 ----
	{
		TStrongObjectPtr<URunSession> D(NewObject<URunSession>());
		const bool bOk = D->ApplySaveGameToRunState(nullptr);
		TestFalse(TEXT("Null SaveGame rejected"), bOk);
	}

	// ---- 磁盘 IO：SaveToSlot + LoadFromSlot ----
	// 注意：SoftObjectPath 对 transient 对象可能无法 TryLoad；我们这次测试绕过。
	// 只验证 HasSaveInSlot + LoadFromSlot 不存在时的回退。

	// 不存在的 slot
	TestFalse(TEXT("Unknown slot HasSaveInSlot"), A->HasSaveInSlot(TEXT("Wacom_NoSuchSlot_XYZ")));
	{
		TStrongObjectPtr<URunSession> E(NewObject<URunSession>());
		const bool bOk = E->LoadFromSlot(TEXT("Wacom_NoSuchSlot_XYZ"));
		TestFalse(TEXT("LoadFromSlot of missing slot returns false"), bOk);
	}

	// 不写磁盘 IO 往返测试（避免 transient 对象路径的副作用）：我们单独验证
	// HasSaveInSlot 在 Save 前为 false、Save 后为 true，然后清理。
	// 即使 Load 会失败（因为角色路径是 transient），Save 本身和 HasSaveInSlot 语义仍可验证。
	ClearTestSlot();
	TestFalse(TEXT("Before SaveToSlot, HasSaveInSlot=false"), A->HasSaveInSlot(TestSlot));
	const bool bSaved = A->SaveToSlot(TestSlot);
	TestTrue(TEXT("SaveToSlot returns true"), bSaved);
	TestTrue(TEXT("After SaveToSlot, HasSaveInSlot=true"), A->HasSaveInSlot(TestSlot));
	ClearTestSlot();
	TestFalse(TEXT("After delete, HasSaveInSlot=false"), A->HasSaveInSlot(TestSlot));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSaveGameLoadNotifiesRunStateChangedSpec,
	"Wacom.Run.Save.LoadFromSlotNotifiesRunStateChanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSaveGameLoadNotifiesRunStateChangedSpec::RunTest(const FString& /*Parameters*/)
{
	ClearTestSlot();

	UCharacterDefinition* Character = LoadObject<UCharacterDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Characters/DA_Character_BugGirl.DA_Character_BugGirl"));
	TestNotNull(TEXT("Default character asset loads for save notification test"), Character);
	if (!Character)
	{
		return false;
	}

	UWacomSaveGame* Save = NewObject<UWacomSaveGame>();
	Save->SaveVersion = UWacomSaveGame::CurrentSaveVersion;
	Save->CharacterAssetPath = FSoftObjectPath(Character);
	Save->BattleSeed = 9901;
	Save->Outcome = ERunOutcome::InProgress;
	Save->bHasPlayerTransform = true;
	Save->PlayerTransform = FTransform(
		FRotator(0.0, 45.0, 0.0),
		FVector(11.0, 22.0, 33.0));

	TestTrue(TEXT("Notification save writes to slot"),
		UGameplayStatics::SaveGameToSlot(Save, NotifyTestSlot, 0));

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	int32 NotifyCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&NotifyCount]()
	{
		++NotifyCount;
	});
	TestTrue(TEXT("LoadFromSlot succeeds"), Run->LoadFromSlot(NotifyTestSlot));
	TestEqual(TEXT("LoadFromSlot broadcasts RunStateChanged once"), NotifyCount, 1);
	TestEqual(TEXT("Loaded save applied to RunState"),
		Run->GetRunState().BattleSeed,
		9901);

	ClearTestSlot();
	return true;
}


// =====================================================================================
// Stage 4.5.0 task 4.7：SaveGame 升档与拒绝（SMOKE / EXAMPLE / EDGE_CASE）
//
// 覆盖 backpack-special-zone-stage-4-5 spec：
//   - R7.1：UWacomSaveGame::CurrentSaveVersion 编译期为 5（SMOKE 静态断言）
//   - R7.3 / R7.8a：v0 → v5 与 v1 → v5 迁移后新字段全部为空容器 + SaveVersion==5
//   - R7.7 / R7.8d：SaveVersion = 6 → MigrateIfNeeded false 且 SaveVersion 不被改写
//   - R7.4：v5 + 四数组全空 + 当前 Character.StarterDeck → 按 StarterDeck 重建路径
//
// SMOKE 静态断言放在文件作用域：UWacomSaveGame.h 自身已有 static_assert，本文件再放一份
// 是为了在测试模块编译时也复检（spec task 4.7 明确要求）。两处任一失配都会触发编译错。
// =====================================================================================

static_assert(UWacomSaveGame::CurrentSaveVersion == 6,
	"SaveGame 必须保持 v6；"
	"若改 CurrentSaveVersion，请同步更新 MigrateIfNeeded 迁移链与本文件断言。");

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSaveGameMigrateAndRejectSpec,
	"Wacom.Run.Save.MigrateAndReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSaveGameMigrateAndRejectSpec::RunTest(const FString& /*Parameters*/)
{
	// ---- R7.3 / R7.8a：v0 → v5 迁移 ----
	// 即便外部改档把 v0 档塞了脏数据，MigrateIfNeeded 也必须按 R7.3 把四个新数组清空。
	{
		TStrongObjectPtr<UWacomSaveGame> Sg(NewObject<UWacomSaveGame>());
		Sg->SaveVersion = 0;

		FCardInstanceSaveEntry JunkCard;
		JunkCard.InstanceId = FGuid::NewGuid();
		Sg->Backpack.Add(JunkCard);
		Sg->BattleDeck.Add(JunkCard);
		Sg->BurdenZone.Add(JunkCard);

		FSpecialZoneSaveEntry JunkSz;
		JunkSz.OwnerInstanceId = FGuid::NewGuid();
		Sg->SpecialZones.Add(JunkSz);

		const bool bMigrated = UWacomSaveGame::MigrateIfNeeded(Sg.Get());
		TestTrue(TEXT("v0 → v6 迁移成功"), bMigrated);
		TestEqual(TEXT("v0 → SaveVersion == 6"), Sg->SaveVersion, 6);
		TestEqual(TEXT("v0 → Backpack 清空"), Sg->Backpack.Num(), 0);
		TestEqual(TEXT("v0 → BattleDeck 清空"), Sg->BattleDeck.Num(), 0);
		TestEqual(TEXT("v0 → BurdenZone 清空"), Sg->BurdenZone.Num(), 0);
		TestEqual(TEXT("v0 → SpecialZones 清空"), Sg->SpecialZones.Num(), 0);
	}

	// ---- R7.3 / R7.8a：v1 → v6 迁移 ----
	{
		TStrongObjectPtr<UWacomSaveGame> Sg(NewObject<UWacomSaveGame>());
		Sg->SaveVersion = 1;

		FCardInstanceSaveEntry JunkCard;
		JunkCard.InstanceId = FGuid::NewGuid();
		Sg->Backpack.Add(JunkCard);
		Sg->BattleDeck.Add(JunkCard);

		const bool bMigrated = UWacomSaveGame::MigrateIfNeeded(Sg.Get());
		TestTrue(TEXT("v1 → v6 迁移成功"), bMigrated);
		TestEqual(TEXT("v1 → SaveVersion == 6"), Sg->SaveVersion, 6);
		TestEqual(TEXT("v1 → Backpack 清空"), Sg->Backpack.Num(), 0);
		TestEqual(TEXT("v1 → BattleDeck 清空"), Sg->BattleDeck.Num(), 0);
		TestEqual(TEXT("v1 → BurdenZone 清空"), Sg->BurdenZone.Num(), 0);
		TestEqual(TEXT("v1 → SpecialZones 清空"), Sg->SpecialZones.Num(), 0);
	}

	// ---- R7.7 / R7.8d：SaveVersion = 7（来自更新版本客户端）→ MigrateIfNeeded false ----
	// 此外 SaveVersion 不应被改写（保持 7，便于上层日志诊断）。
	{
		TStrongObjectPtr<UWacomSaveGame> Sg(NewObject<UWacomSaveGame>());
		Sg->SaveVersion = 7;

		const bool bMigrated = UWacomSaveGame::MigrateIfNeeded(Sg.Get());
		TestFalse(TEXT("v7（未来版本）拒绝迁移"), bMigrated);
		TestEqual(TEXT("v7 SaveVersion 不被改写"), Sg->SaveVersion, 7);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSaveGameStarterDeckRebuildSpec,
	"Wacom.Run.Save.StarterDeckRebuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSaveGameStarterDeckRebuildSpec::RunTest(const FString& /*Parameters*/)
{
	// ---- R7.4：v5 + 四数组全空 + 当前 Character.StarterDeck → 按 StarterDeck 重建 ----
	//
	// StarterDeck 含一张容器卡（Capacity > 0，进 Backpack）+ 两张普通卡（进 BattleDeck），
	// 验证：
	//   1) ApplySaveGameToRunState 返回 true
	//   2) Backpack/BattleDeck 数量 + Definition 与 StarterDeck 分流一致
	//   3) 每张 instance 的 InstanceId 非 zero GUID 且全表唯一
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(1);
	UCardDefinition* RH = Fx.MakeNoopCard(1);

	UCardDefinition* Container = Fx.MakeNoopCard(0);
	Container->Physique.Capacity = 5;  // 容器卡 → Initialize / 重建路径都进 Backpack

	UCardDefinition* Normal1 = Fx.MakeNoopCard(0);
	UCardDefinition* Normal2 = Fx.MakeNoopCard(0);

	TArray<UCardDefinition*> Deck = { Container, Normal1, Normal2 };
	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);

	// 手动构造 v5 + 四数组全空 + 指向当前角色的 SaveGame，
	// 模拟"v0/v1 迁移后"或"全新档"两种共同走 R7.4 重建路径的情形。
	TStrongObjectPtr<UWacomSaveGame> Sg(NewObject<UWacomSaveGame>());
	Sg->SaveVersion = UWacomSaveGame::CurrentSaveVersion;  // == 6
	Sg->CharacterAssetPath = FSoftObjectPath(Char);
	Sg->BattleSeed = 7;
	Sg->Outcome = ERunOutcome::InProgress;
	// Backpack / BattleDeck / BurdenZone / SpecialZones 保持默认空数组

	TStrongObjectPtr<URunSession> Session(NewObject<URunSession>());
	// 不预先 Initialize：Apply 内部会通过 SaveGame.CharacterAssetPath 加载 Character
	// 并按 StarterDeck 重建（覆盖 R7.4 主路径）。
	const bool bApplied = Session->ApplySaveGameToRunState(Sg.Get());
	TestTrue(TEXT("v5 + 四数组全空 → ApplySaveGameToRunState 成功（R7.4）"), bApplied);
	if (!bApplied) { return false; }

	const FRunState& State = Session->GetRunState();

	TestEqual(TEXT("Character roundtrip"), State.Character.Get(), Char);
	TestEqual(TEXT("BattleSeed roundtrip"), State.BattleSeed, 7);
	TestEqual(TEXT("Outcome roundtrip"), State.Outcome, ERunOutcome::InProgress);

	// 分流：容器卡进 Backpack，普通卡进 BattleDeck（与 Initialize 同源逻辑 R1.3 一致）
	TestEqual(TEXT("Backpack 重建后含 1 张容器卡"), State.Backpack.Num(), 1);
	TestEqual(TEXT("BattleDeck 重建后含 2 张普通卡"), State.BattleDeck.Num(), 2);

	if (State.Backpack.Num() == 1)
	{
		TestEqual(TEXT("Backpack[0].Definition == Container"),
			State.Backpack[0].Definition.Get(), Container);
	}
	if (State.BattleDeck.Num() == 2)
	{
		TestEqual(TEXT("BattleDeck[0].Definition == Normal1"),
			State.BattleDeck[0].Definition.Get(), Normal1);
		TestEqual(TEXT("BattleDeck[1].Definition == Normal2"),
			State.BattleDeck[1].Definition.Get(), Normal2);
	}

	// InstanceId 全部合法 + 跨 zone 全表唯一（R7.2 不变量）
	TSet<FGuid> SeenIds;
	for (const FCardInstance& Inst : State.Backpack)
	{
		TestTrue(TEXT("Backpack instance.InstanceId 非 zero GUID"),
			Inst.InstanceId.IsValid());
		bool bAlreadyInSet = false;
		SeenIds.Add(Inst.InstanceId, &bAlreadyInSet);
		TestFalse(TEXT("Backpack instance.InstanceId 全表唯一"), bAlreadyInSet);
	}
	for (const FCardInstance& Inst : State.BattleDeck)
	{
		TestTrue(TEXT("BattleDeck instance.InstanceId 非 zero GUID"),
			Inst.InstanceId.IsValid());
		bool bAlreadyInSet = false;
		SeenIds.Add(Inst.InstanceId, &bAlreadyInSet);
		TestFalse(TEXT("BattleDeck instance.InstanceId 全表唯一"), bAlreadyInSet);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunSaveGameV2InstanceZoneRoundtripSpec,
	"Wacom.Run.Save.V2InstanceZoneRoundtrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunSaveGameV2InstanceZoneRoundtripSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, Property 16: SaveGame v2 round-trip 完整保留 instance 归属
	// Validates: Requirements 7.2, 7.5
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA = Fx.MakeNoopCard(0);
	TypeA->Physique.Capacity = 8;
	TypeA->Keywords.AddTag(WacomTags::Card_Keyword_BagProvider);
	TypeA->Rarity = WacomTags::Card_Rarity_White;

	UCardDefinition* TypeB = Fx.MakeNoopCard(0);
	TypeB->Physique.Capacity = 3;
	TypeB->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_WeaponDamagePlus3;
	TypeB->Rarity = WacomTags::Card_Rarity_White;

	UCardDefinition* BattleNative = Fx.MakeNoopCard(0);
	UCardDefinition* BackpackOnly = Fx.MakeNoopCard(0);
	UCardDefinition* SpecialStored = Fx.MakeNoopCard(0);
	UCardDefinition* BurdenStored = Fx.MakeNoopCard(0);

	UCharacterDefinition* Char = Fx.MakeCharacter(
		Fx.MakeNoopCard(1), Fx.MakeNoopCard(1),
		{ TypeA, TypeB, BattleNative });

	TStrongObjectPtr<URunSession> Source(NewObject<URunSession>());
	TestTrue(TEXT("Initialize source"), InitializeRunSessionForTest(*Source, Char).IsOk());

	const FGuid BOwnerId = Source->GetRunState().SpecialZones[0].OwnerInstanceId;
	Source->AcquireCardToRun(BackpackOnly);
	Source->AcquireCardToRun(SpecialStored);
	const FGuid SpecialId = Source->GetBackpack().Last().InstanceId;
	TestTrue(TEXT("Move stored card to SpecialZone"), Source->MoveInstance(SpecialId, EZoneKind::SpecialZone, BOwnerId));
	TestTrue(TEXT("Enable stored card"), Source->SetSpecialZoneCardBattleEnabled(SpecialId, true));
	TestTrue(TEXT("Move B owner to BattleDeck"), Source->MoveInstance(BOwnerId, EZoneKind::BattleDeck, FGuid()));

	FCardInstance BurdenInst;
	BurdenInst.InstanceId = FGuid::NewGuid();
	BurdenInst.Definition = BurdenStored;
	BurdenInst.bBattleEnabledInSpecialZone = true; // round-trip 应原样保存，虽 BurdenZone 不使用该 flag
	FWacomRunSessionTestAccess::GetMutableRunState(*Source.Get()).BurdenZone.Add(BurdenInst);

	auto SnapshotInstanceMap = [](const FRunState& State)
	{
		TMap<FGuid, FString> Out;
		for (const FCardInstance& Inst : State.Backpack)
		{
			Out.Add(Inst.InstanceId, FString::Printf(TEXT("Backpack|%s|%d"),
				*GetNameSafe(Inst.Definition.Get()),
				Inst.bBattleEnabledInSpecialZone ? 1 : 0));
		}
		for (const FCardInstance& Inst : State.BattleDeck)
		{
			Out.Add(Inst.InstanceId, FString::Printf(TEXT("BattleDeck|%s|%d"),
				*GetNameSafe(Inst.Definition.Get()),
				Inst.bBattleEnabledInSpecialZone ? 1 : 0));
		}
		for (const FCardInstance& Inst : State.BurdenZone)
		{
			Out.Add(Inst.InstanceId, FString::Printf(TEXT("BurdenZone|%s|%d"),
				*GetNameSafe(Inst.Definition.Get()),
				Inst.bBattleEnabledInSpecialZone ? 1 : 0));
		}
		for (const FSpecialZone& SZ : State.SpecialZones)
		{
			for (const FCardInstance& Inst : SZ.Cards)
			{
				Out.Add(Inst.InstanceId, FString::Printf(TEXT("SpecialZone:%s|%s|%d"),
					*SZ.OwnerInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
					*GetNameSafe(Inst.Definition.Get()),
					Inst.bBattleEnabledInSpecialZone ? 1 : 0));
			}
		}
		return Out;
	};

	const TMap<FGuid, FString> BeforeMap = SnapshotInstanceMap(Source->GetRunState());
	TestTrue(TEXT("Before map has instances"), BeforeMap.Num() > 0);

	UWacomSaveGame* Save = Source->BuildSaveGameFromRunState();
	TestNotNull(TEXT("Save non-null"), Save);
	if (!Save) { return false; }

	TestEqual(TEXT("Save Backpack count"), Save->Backpack.Num(), Source->GetRunState().Backpack.Num());
	TestEqual(TEXT("Save BattleDeck count"), Save->BattleDeck.Num(), Source->GetRunState().BattleDeck.Num());
	TestEqual(TEXT("Save BurdenZone count"), Save->BurdenZone.Num(), Source->GetRunState().BurdenZone.Num());
	TestEqual(TEXT("Save SpecialZones count"), Save->SpecialZones.Num(), Source->GetRunState().SpecialZones.Num());

	TSet<FGuid> SaveIds;
	auto CheckSaveEntries = [this, &SaveIds](const TArray<FCardInstanceSaveEntry>& Entries, const TCHAR* Label) -> bool
	{
		for (const FCardInstanceSaveEntry& Entry : Entries)
		{
			if (!Entry.InstanceId.IsValid())
			{
				AddError(FString::Printf(TEXT("%s has zero InstanceId"), Label));
				return false;
			}
			bool bAlready = false;
			SaveIds.Add(Entry.InstanceId, &bAlready);
			if (bAlready)
			{
				AddError(FString::Printf(TEXT("%s duplicate InstanceId %s"),
					Label,
					*Entry.InstanceId.ToString(EGuidFormats::DigitsWithHyphens)));
				return false;
			}
		}
		return true;
	};

	if (!CheckSaveEntries(Save->Backpack, TEXT("Backpack"))) { return false; }
	if (!CheckSaveEntries(Save->BattleDeck, TEXT("BattleDeck"))) { return false; }
	if (!CheckSaveEntries(Save->BurdenZone, TEXT("BurdenZone"))) { return false; }
	for (const FSpecialZoneSaveEntry& SZ : Save->SpecialZones)
	{
		TestTrue(TEXT("SpecialZone owner valid"), SZ.OwnerInstanceId.IsValid());
		if (!CheckSaveEntries(SZ.Cards, TEXT("SpecialZone"))) { return false; }
	}

	TStrongObjectPtr<URunSession> Loaded(NewObject<URunSession>());
	TestTrue(TEXT("Apply v2 save"), Loaded->ApplySaveGameToRunState(Save));

	const TMap<FGuid, FString> AfterMap = SnapshotInstanceMap(Loaded->GetRunState());
	TestEqual(TEXT("Instance count preserved"), AfterMap.Num(), BeforeMap.Num());
	for (const TPair<FGuid, FString>& Pair : BeforeMap)
	{
		const FString* AfterValue = AfterMap.Find(Pair.Key);
		if (!AfterValue)
		{
			AddError(FString::Printf(TEXT("Missing InstanceId after load: %s"),
				*Pair.Key.ToString(EGuidFormats::DigitsWithHyphens)));
			return false;
		}
		TestEqual(TEXT("Instance zone/definition/flag preserved"), *AfterValue, Pair.Value);
	}

	return true;
}
