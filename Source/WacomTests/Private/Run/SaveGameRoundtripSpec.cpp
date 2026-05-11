// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "RunSession.h"
#include "RunState.h"
#include "WacomSaveGame.h"

#include "Characters/CharacterDefinition.h"
#include "Cards/CardDefinition.h"
#include "Enemies/EnemyDefinition.h"

#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/SoftObjectPath.h"

/**
 * Save/Load 往返测试。
 *
 * 覆盖：
 *   - 字段往返无损（Character / BattleSeed / DefeatedEnemies / DestroyedTriggerIds / PlayerTransform / bRunActive）
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

	void ClearTestSlot()
	{
		if (UGameplayStatics::DoesSaveGameExist(TestSlot, 0))
		{
			UGameplayStatics::DeleteGameInSlot(TestSlot, 0);
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

	// 构造一个最小角色 + 两个敌人定义（SoftObjectPath 需要真 UObject 才能拿到路径；
	// Transient package 的对象可以，TryLoad 不会真加载但 == 比较能通过）
	UCardDefinition* LH = Fx.MakeNoopCard(1);
	UCardDefinition* RH = Fx.MakeNoopCard(1);
	TArray<UCardDefinition*> Deck = { Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) };
	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);

	UEnemyDefinition* Enemy1 = Fx.MakeSinglePartEnemy(10, 1, 0);
	UEnemyDefinition* Enemy2 = Fx.MakeSinglePartEnemy(20, 2, 0);

	// --- Session A：填数据，Save ---
	TStrongObjectPtr<URunSession> A(NewObject<URunSession>());
	A->Initialize(Char);

	FRunState& SA = A->GetMutableRunState();
	SA.BattleSeed = 4242;
	SA.bRunActive = true;
	SA.DefeatedEnemies.Add(Enemy1);
	SA.DefeatedEnemies.Add(Enemy2);
	SA.DestroyedTriggerIds.Add(TEXT("Trigger_A"));
	SA.DestroyedTriggerIds.Add(TEXT("Trigger_B"));
	SA.PlayerTransform     = FTransform(FRotator(0, 90, 0), FVector(100, 200, 300));
	SA.bHasPlayerTransform = true;

	// ---- 结构层测试：BuildSaveGameFromRunState + ApplySaveGameToRunState ----

	UWacomSaveGame* Sg = A->BuildSaveGameFromRunState();
	TestNotNull(TEXT("BuildSaveGameFromRunState non-null"), Sg);
	if (!Sg) { return false; }

	TestEqual(TEXT("SaveVersion == Current"), Sg->SaveVersion, UWacomSaveGame::CurrentSaveVersion);
	TestEqual(TEXT("BattleSeed passthrough"), Sg->BattleSeed, 4242);
	TestTrue(TEXT("bRunActive passthrough"), Sg->bRunActive);
	TestEqual(TEXT("DefeatedEnemyAssetPaths count"), Sg->DefeatedEnemyAssetPaths.Num(), 2);
	TestEqual(TEXT("DestroyedTriggerIds count"), Sg->DestroyedTriggerIds.Num(), 2);
	TestTrue(TEXT("bHasPlayerTransform passthrough"), Sg->bHasPlayerTransform);

	// Apply 回一个新 Session B（Transient 对象 FSoftObjectPath::TryLoad 走内存查找，能找到）
	TStrongObjectPtr<URunSession> B(NewObject<URunSession>());
	B->Initialize(Char);  // 先给 Character，不影响后续 Apply 会覆盖

	const bool bApplied = B->ApplySaveGameToRunState(Sg);
	TestTrue(TEXT("ApplySaveGameToRunState ok"), bApplied);

	const FRunState& SB = B->GetRunState();
	TestEqual(TEXT("Character roundtrip"), SB.Character.Get(), Char);
	TestEqual(TEXT("BattleSeed roundtrip"), SB.BattleSeed, 4242);
	TestTrue(TEXT("bRunActive roundtrip"), SB.bRunActive);
	TestEqual(TEXT("DefeatedEnemies count roundtrip"), SB.DefeatedEnemies.Num(), 2);
	TestTrue(TEXT("DestroyedTriggerIds has Trigger_A"), SB.DestroyedTriggerIds.Contains(TEXT("Trigger_A")));
	TestTrue(TEXT("DestroyedTriggerIds has Trigger_B"), SB.DestroyedTriggerIds.Contains(TEXT("Trigger_B")));
	TestEqual(TEXT("DestroyedTriggerIds count roundtrip"), SB.DestroyedTriggerIds.Num(), 2);
	TestTrue(TEXT("bHasPlayerTransform roundtrip"), SB.bHasPlayerTransform);
	TestTrue(TEXT("PlayerTransform location roundtrip"),
		SB.PlayerTransform.GetLocation().Equals(FVector(100, 200, 300)));
	TestTrue(TEXT("PlayerTransform rotation roundtrip"),
		SB.PlayerTransform.Rotator().Equals(FRotator(0, 90, 0)));

	// ---- 版本拒绝：构造一个 SaveVersion > Current 的 SaveGame ----
	{
		UWacomSaveGame* FutureSg = A->BuildSaveGameFromRunState();
		FutureSg->SaveVersion = UWacomSaveGame::CurrentSaveVersion + 1;

		TStrongObjectPtr<URunSession> C(NewObject<URunSession>());
		C->Initialize(Char);
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
		MigSession->Initialize(Char);
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
