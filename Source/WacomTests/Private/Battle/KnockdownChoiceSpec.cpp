// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Session/BattleSession.h"
#include "Session/BattleResultPacket.h"
#include "Runtime/BattleEnemyKeys.h"
#include "Snapshots/BattleSnapshot.h"
#include "Snapshots/HandSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

#include "UObject/StrongObjectPtr.h"
#include "RunSession.h"
#include "RunState.h"

/**
 * 击倒事件三选一回归。
 *
 * 验证：
 *   - 部位 HP 归零后 Phase 切到 PendingKnockdownChoice
 *   - 撤离 → BattleEnd Outcome=Victory + bWithdrawn
 *   - 援助 → 战斗继续（多部位时仍非空时维持 Pending；单部位时进 BattleEnd）
 *   - 左右手事件不依赖左右手牌当前是否仍在手牌区
 *   - 最后一个存活部位击倒后 Withdraw 不可选
 *   - 一次行动多部位破坏 → 逐个弹 dialog
 *   - RunSession 撤离写 BattleProgress；真胜利清 BattleProgress
 *   - 第二次进同一战斗 PreDestroyedParts 应用为预破坏
 */

namespace
{
	UCharacterDefinition* MakeStandardChar(FWacomBattleFixture& Fx,
		UCardDefinition** OutKiller = nullptr, UCardDefinition** OutLight = nullptr)
	{
		UCardDefinition* LH = Fx.MakeNoopCard(0);
		UCardDefinition* RH = Fx.MakeNoopCard(0);
		UCardDefinition* Killer = Fx.MakeSimpleDamageCard(/*Cost*/0, /*Dmg*/100);
		UCardDefinition* Light  = Fx.MakeSimpleDamageCard(/*Cost*/0, /*Dmg*/1);
		if (OutKiller) { *OutKiller = Killer; }
		if (OutLight)  { *OutLight  = Light; }

		TArray<UCardDefinition*> Deck = { Killer, Light };
		for (int32 i = 0; i < 5; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }
		return Fx.MakeCharacter(LH, RH, Deck);
	}

	void ApplySingleEnemySlot(FBattleInitParams& Params, UEnemyDefinition* Enemy)
	{
		Params.EnemySlots.Reset();

		FBattleEnemySlotInit Slot;
		Slot.EnemySlotId = TEXT("Enemy");
		Slot.Enemy = Enemy;
		Params.EnemySlots.Add(Slot);
	}

	bool HandContainsDefinition(const FBattleSnapshot& Snap, const UCardDefinition* Definition)
	{
		for (const FHandCardSnapshot& Card : Snap.Hand.Cards)
		{
			if (Card.Definition == Definition)
			{
				return true;
			}
		}
		return false;
	}

	const FBattleEvent* FindCardGainedEvent(const TArray<FBattleEvent>& Events, const UCardDefinition* Definition)
	{
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type == EBattleEventType::CardGained && Event.CardDefinition == Definition)
			{
				return &Event;
			}
		}
		return nullptr;
	}

	const FBattlePresentationCheckpoint* FindCardGainedCheckpoint(
		const FBattlePresentationJournal& Journal)
	{
		return Journal.Checkpoints.FindByPredicate(
			[](const FBattlePresentationCheckpoint& Checkpoint)
			{
				return Checkpoint.Type
					== EBattlePresentationCheckpointType::CardGainedResolved;
			});
	}

	UCardDefinition* MakeKnockdownRewardLimitDrawCard(FWacomBattleFixture& Fx, int32 DrawCount)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(/*Cost*/0);

		FCardEffect Effect;
		Effect.EffectType = WacomTags::Effect_Draw;
		Effect.Magnitude = DrawCount;
		Effect.TargetZone = WacomTags::CardLocation_Draw;
		Card->Effects.Add(Effect);
		return Card;
	}

	FBattleCommand MakePlayOnPartInstance(
		const FBattleSnapshot& Snapshot,
		const FGuid& CardInstanceId,
		const FGuid& PartInstanceId)
	{
		return FWacomBattleFixture::MakePlayCardOnPartInstance(Snapshot, CardInstanceId, PartInstanceId);
	}
}

// ================ 部位破坏后切到 PendingKnockdownChoice ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoicePhaseSpec,
	"Wacom.Battle.Knockdown.PhaseSwitchesOnPartDestroyed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoicePhaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Killer = nullptr;
	UCharacterDefinition* Char = MakeStandardChar(Fx, &Killer);

	// 三部位敌人：Head/Body/Tail HP=50，Head 先打死会有击倒事件
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(50, 50, 50, 7, 7, 7);
	UBattleSession* S = Fx.CreateSession(Char, Enemy, /*Seed*/1);

	const FBattleSnapshot Snap0 = S->BuildSnapshot();
	const FGuid Head = FWacomBattleFixture::FindPartInstanceId(Snap0, 0);
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(Snap0, Killer->CardId);

	TestTrue(TEXT("Phase 起始 PlayerAction"), S->GetPhase() == EBattlePhase::PlayerAction);

	S->ResolveCommand(MakePlayOnPartInstance(Snap0, KillerId, Head));

	TestTrue(TEXT("部位破坏后 Phase 切到 PendingKnockdownChoice"),
		S->GetPhase() == EBattlePhase::PendingKnockdownChoice);

	return true;
}

// ================ 撤离结束战斗 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceWithdrawSpec,
	"Wacom.Battle.Knockdown.WithdrawEndsBattle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceWithdrawSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Killer = nullptr;
	UCharacterDefinition* Char = MakeStandardChar(Fx, &Killer);

	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(50, 50, 50, 7, 7, 7);
	UBattleSession* S = Fx.CreateSession(Char, Enemy, /*Seed*/1);

	const FBattleSnapshot Snap0 = S->BuildSnapshot();
	const FGuid Head = FWacomBattleFixture::FindPartInstanceId(Snap0, 0);
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(Snap0, Killer->CardId);

	S->ResolveCommand(MakePlayOnPartInstance(Snap0, KillerId, Head));
	S->ResolveCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Withdraw));

	TestTrue(TEXT("撤离后 Phase=BattleEnd"), S->GetPhase() == EBattlePhase::BattleEnd);
	TestTrue(TEXT("Outcome=Victory"),         S->BuildSnapshot().Outcome == EBattleOutcome::Victory);

	const FBattleResultPacket P = S->BuildResultPacket();
	TestTrue(TEXT("packet.bWithdrawn=true"),  P.bWithdrawn);
	TestEqual(TEXT("DestroyedPartKeys 1 项"), P.DestroyedPartKeys.Num(), 1);
	if (P.DestroyedPartKeys.Num() == 1)
	{
		TestEqual(TEXT("DestroyedPartKeys 记录 Head key"),
			P.DestroyedPartKeys[0],
			FWacomBattleFixture::FindPartKey(Snap0, 0));
	}
	TestEqual(TEXT("DestroyedParts projection 1 项"),  P.DestroyedParts.Num(), 1);
	TestEqual(TEXT("KnockdownChoices 1 项"),  P.KnockdownChoices.Num(), 1);

	return true;
}

// ================ 援助战斗继续（多部位） ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceAidContinuesSpec,
	"Wacom.Battle.Knockdown.AidContinuesBattleInMultiPartFight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceAidContinuesSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Killer = nullptr;
	UCharacterDefinition* Char = MakeStandardChar(Fx, &Killer);

	// 三部位敌人，Head 50 / Body 50 / Tail 50
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(50, 50, 50, 7, 7, 7);
	UBattleSession* S = Fx.CreateSession(Char, Enemy, /*Seed*/1);

	const FBattleSnapshot Snap0 = S->BuildSnapshot();
	const FGuid Head = FWacomBattleFixture::FindPartInstanceId(Snap0, 0);
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(Snap0, Killer->CardId);

	S->ResolveCommand(MakePlayOnPartInstance(Snap0, KillerId, Head));
	TestTrue(TEXT("Pending"), S->GetPhase() == EBattlePhase::PendingKnockdownChoice);

	const FKnockdownChoiceView View = S->BuildPendingKnockdownChoiceView();
	TestTrue(TEXT("View has pending knockdown choice"), View.bHasPendingChoice);
	TestEqual(TEXT("View points at destroyed head"), View.PartInstanceId, Head);
	TestTrue(TEXT("Aid available in multi-part fight"), View.AidOption.bAvailable);
	TestTrue(TEXT("Withdraw available while enemy still has living parts"), View.WithdrawOption.bAvailable);
	TestTrue(TEXT("Destroy available in multi-part fight"), View.DestroyOption.bAvailable);
	TestEqual(TEXT("Withdraw reason none"), View.WithdrawOption.DisabledReason, FName(TEXT("None")));

	S->ResolveCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid));
	TestTrue(TEXT("Aid 后回到 PlayerAction（敌人未全死）"),
		S->GetPhase() == EBattlePhase::PlayerAction);

	return true;
}

// ================ Aid 击倒奖励卡：战内入手 + 战后包记账 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceRewardCardAidSpec,
	"Wacom.Battle.Knockdown.RewardCardAid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceRewardCardAidSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Killer = nullptr;
	UCharacterDefinition* Char = MakeStandardChar(Fx, &Killer);
	UCardDefinition* RewardCard = Fx.MakeNoopCard(/*Cost*/0);

	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(50, 50, 50, 7, 7, 7);
	Enemy->Parts[0].PartDef->KnockdownRewardCard = RewardCard;
	UBattleSession* S = Fx.CreateSession(Char, Enemy, /*Seed*/1);

	const FBattleSnapshot Snap0 = S->BuildSnapshot();
	const FGuid Head = FWacomBattleFixture::FindPartInstanceId(Snap0, 0);
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(Snap0, Killer->CardId);

	S->ResolveCommand(MakePlayOnPartInstance(Snap0, KillerId, Head));
	const FBattleResolution AidStatus = S->ResolveCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid));
	TestTrue(TEXT("Aid resolves"), AidStatus.IsOk());

	const FBattleSnapshot SnapAfter = S->BuildSnapshot();
	TestTrue(TEXT("Reward card immediately appears in battle hand"),
		HandContainsDefinition(SnapAfter, RewardCard));

	const TArray<FBattleEvent>& Events = AidStatus.Events;
	const FBattleEvent* CardGained = FindCardGainedEvent(Events, RewardCard);
	TestNotNull(TEXT("CardGained event emitted"), CardGained);
	if (CardGained)
	{
		TestTrue(TEXT("CardGained has battle runtime card id"), CardGained->CardInstanceId.IsValid());
		TestEqual(TEXT("CardGained source part instance id"), CardGained->ActorInstanceId, Head);
		TestEqual(TEXT("CardGained source part key"), CardGained->ActorEnemyPartKey, FWacomBattleFixture::FindPartKey(Snap0, 0));
		TestEqual(TEXT("CardGained choice count is Aid"), CardGained->Count, static_cast<int32>(EKnockdownChoice::Aid));
	}
	const FBattlePresentationCheckpoint* GainedCheckpoint =
		FindCardGainedCheckpoint(AidStatus.PresentationJournal);
	if (TestNotNull(TEXT("CardGainedResolved checkpoint recorded"), GainedCheckpoint)
		&& CardGained)
	{
		TestTrue(TEXT("Checkpoint contains the gained runtime card"),
			GainedCheckpoint->CardInstanceIds.Contains(CardGained->CardInstanceId));
		TestTrue(TEXT("Checkpoint snapshot contains the gained runtime card"),
			GainedCheckpoint->Snapshot.Hand.Cards.ContainsByPredicate(
				[CardGained](const FHandCardSnapshot& Card)
				{
					return Card.InstanceId == CardGained->CardInstanceId;
				}));
		TestEqual(TEXT("Checkpoint begins at the CardGained event"),
			GainedCheckpoint->FirstEventSequence,
			CardGained->Sequence);
		TestEqual(TEXT("Checkpoint ends at the CardGained event"),
			GainedCheckpoint->LastEventSequence,
			CardGained->Sequence);
	}

	const FBattleResultPacket Packet = S->BuildResultPacket();
	TestEqual(TEXT("GainedCards has one reward"), Packet.GainedCards.Num(), 1);
	if (Packet.GainedCards.IsValidIndex(0))
	{
		TestEqual(TEXT("Reward definition recorded"), Packet.GainedCards[0].Definition.Get(), RewardCard);
		TestEqual(TEXT("Reward source part key recorded"), Packet.GainedCards[0].SourcePartKey, FWacomBattleFixture::FindPartKey(Snap0, 0));
		TestEqual(TEXT("Reward source choice recorded"), Packet.GainedCards[0].SourceChoice, EKnockdownChoice::Aid);
	}

	return true;
}

// ================ Destroy 击倒奖励卡：同一部位奖励配置 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceRewardCardDestroySpec,
	"Wacom.Battle.Knockdown.RewardCardDestroy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceRewardCardDestroySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Killer = nullptr;
	UCharacterDefinition* Char = MakeStandardChar(Fx, &Killer);
	UCardDefinition* RewardCard = Fx.MakeNoopCard(/*Cost*/0);

	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(50, 50, 50, 7, 7, 7);
	Enemy->Parts[0].PartDef->KnockdownRewardCard = RewardCard;
	UBattleSession* S = Fx.CreateSession(Char, Enemy, /*Seed*/1);

	const FBattleSnapshot Snap0 = S->BuildSnapshot();
	const FGuid Head = FWacomBattleFixture::FindPartInstanceId(Snap0, 0);
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(Snap0, Killer->CardId);

	S->ResolveCommand(MakePlayOnPartInstance(Snap0, KillerId, Head));
	const FBattleResolution DestroyStatus = S->ResolveCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Destroy));
	TestTrue(TEXT("Destroy resolves"), DestroyStatus.IsOk());

	const FBattleSnapshot SnapAfter = S->BuildSnapshot();
	TestTrue(TEXT("Reward card immediately appears in battle hand"),
		HandContainsDefinition(SnapAfter, RewardCard));

	const TArray<FBattleEvent>& Events = DestroyStatus.Events;
	const FBattleEvent* CardGained = FindCardGainedEvent(Events, RewardCard);
	TestNotNull(TEXT("CardGained event emitted"), CardGained);
	if (CardGained)
	{
		TestTrue(TEXT("CardGained has battle runtime card id"), CardGained->CardInstanceId.IsValid());
		TestEqual(TEXT("CardGained source part instance id"), CardGained->ActorInstanceId, Head);
		TestEqual(TEXT("CardGained source part key"), CardGained->ActorEnemyPartKey, FWacomBattleFixture::FindPartKey(Snap0, 0));
		TestEqual(TEXT("CardGained choice count is Destroy"), CardGained->Count, static_cast<int32>(EKnockdownChoice::Destroy));
	}

	const FBattleResultPacket Packet = S->BuildResultPacket();
	TestEqual(TEXT("GainedCards has one reward"), Packet.GainedCards.Num(), 1);
	if (Packet.GainedCards.IsValidIndex(0))
	{
		TestEqual(TEXT("Reward definition recorded"), Packet.GainedCards[0].Definition.Get(), RewardCard);
		TestEqual(TEXT("Reward source part key recorded"), Packet.GainedCards[0].SourcePartKey, FWacomBattleFixture::FindPartKey(Snap0, 0));
		TestEqual(TEXT("Reward source choice recorded"), Packet.GainedCards[0].SourceChoice, EKnockdownChoice::Destroy);
	}

	return true;
}

// ================ Withdraw 不触发奖励卡 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceRewardCardWithdrawIgnoredSpec,
	"Wacom.Battle.Knockdown.RewardCardWithdrawIgnored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceRewardCardWithdrawIgnoredSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Killer = nullptr;
	UCharacterDefinition* Char = MakeStandardChar(Fx, &Killer);
	UCardDefinition* RewardCard = Fx.MakeNoopCard(/*Cost*/0);

	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(50, 50, 50, 7, 7, 7);
	Enemy->Parts[0].PartDef->KnockdownRewardCard = RewardCard;
	UBattleSession* S = Fx.CreateSession(Char, Enemy, /*Seed*/1);

	const FBattleSnapshot Snap0 = S->BuildSnapshot();
	const FGuid Head = FWacomBattleFixture::FindPartInstanceId(Snap0, 0);
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(Snap0, Killer->CardId);

	S->ResolveCommand(MakePlayOnPartInstance(Snap0, KillerId, Head));
	const FBattleResolution WithdrawStatus = S->ResolveCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Withdraw));
	TestTrue(TEXT("Withdraw resolves"), WithdrawStatus.IsOk());

	const TArray<FBattleEvent>& Events = WithdrawStatus.Events;
	TestNull(TEXT("Withdraw does not emit CardGained"), FindCardGainedEvent(Events, RewardCard));

	const FBattleResultPacket Packet = S->BuildResultPacket();
	TestEqual(TEXT("Withdraw records no gained cards"), Packet.GainedCards.Num(), 0);

	return true;
}

// ================ 奖励卡入手后仍执行普通手牌上限 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceRewardCardRespectsHandLimitSpec,
	"Wacom.Battle.Knockdown.RewardCardRespectsHandLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceRewardCardRespectsHandLimitSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(/*Cost*/0);
	UCardDefinition* RH = Fx.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/100);
	UCardDefinition* DrawCard = MakeKnockdownRewardLimitDrawCard(Fx, /*DrawCount*/7);
	UCardDefinition* RewardCard = Fx.MakeNoopCard(/*Cost*/0);

	TArray<UCardDefinition*> Deck = { DrawCard };
	for (int32 Index = 0; Index < 20; ++Index)
	{
		Deck.Add(Fx.MakeNoopCard(/*Cost*/0));
	}
	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);

	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(50, 50, 50, 100, 100, 100);
	Enemy->Parts[0].PartDef->KnockdownRewardCard = RewardCard;

	bool bCovered = false;
	for (int32 Seed = 1; Seed <= 80 && !bCovered; ++Seed)
	{
		UBattleSession* S = Fx.CreateSession(Char, Enemy, Seed);
		FBattleSnapshot Snap = S->BuildSnapshot();
		const FGuid Head = FWacomBattleFixture::FindPartInstanceId(Snap, 0);
		const FGuid DrawCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snap, DrawCard->CardId);
		const FGuid KillerAnchorId = FWacomBattleFixture::FindHandInstanceByCardId(Snap, RH->CardId);
		if (!DrawCardId.IsValid() || !KillerAnchorId.IsValid())
		{
			continue;
		}

		TestTrue(TEXT("Play draw card to fill hand"),
			S->ResolveCommand(FBattleCommand::MakePlayCard(DrawCardId)).IsOk());
		Snap = S->BuildSnapshot();
		TestEqual(TEXT("Draw effect fills normal hand to limit"),
			Snap.Hand.NormalCardCount,
			Snap.Hand.NormalCardLimit);

		TestTrue(TEXT("Play right-hand killer anchor"),
			S->ResolveCommand(MakePlayOnPartInstance(Snap, KillerAnchorId, Head)).IsOk());
		const FBattleResolution AidResolution =
			S->ResolveCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid));
		TestTrue(TEXT("Choose Aid"), AidResolution.IsOk());

		const TArray<FBattleEvent>& Events = AidResolution.Events;
		Snap = S->BuildSnapshot();

		int32 LimitDiscardEvents = 0;
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type == EBattleEventType::HandLimitDiscarded)
			{
				++LimitDiscardEvents;
				TestTrue(TEXT("Limit discard has valid card id"), Event.CardInstanceId.IsValid());
			}
		}

		TestLessEqual(TEXT("Normal cards remain within hand limit"),
			Snap.Hand.NormalCardCount,
			Snap.Hand.NormalCardLimit);
		TestNotNull(TEXT("Reward still emits CardGained even if limit discard happens"),
			FindCardGainedEvent(Events, RewardCard));
		const FBattleEvent* GainedEvent = FindCardGainedEvent(Events, RewardCard);
		const FBattlePresentationCheckpoint* GainedCheckpoint =
			FindCardGainedCheckpoint(AidResolution.PresentationJournal);
		if (TestNotNull(TEXT("Full-hand reward still records the pre-discard gained checkpoint"),
			GainedCheckpoint)
			&& GainedEvent)
		{
			TestTrue(TEXT("Pre-discard checkpoint retains the gained card even if final hand does not"),
				GainedCheckpoint->Snapshot.Hand.Cards.ContainsByPredicate(
					[GainedEvent](const FHandCardSnapshot& Card)
					{
						return Card.InstanceId == GainedEvent->CardInstanceId;
					}));
		}
		TestTrue(TEXT("Reward grant over full hand emits HandLimitDiscarded"), LimitDiscardEvents > 0);
		bCovered = true;
	}

	TestTrue(TEXT("A seed put the killer card in opening hand"), bCovered);
	return true;
}

// ================ 左手已打出 → Aid 仍可选 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceAidAvailableWhenLeftHandPlayedSpec,
	"Wacom.Battle.Knockdown.AidAllowedWhenLeftHandPlayed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceAidAvailableWhenLeftHandPlayedSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(0);
	UCardDefinition* RH = Fx.MakeNoopCard(0);
	UCardDefinition* Killer = Fx.MakeSimpleDamageCard(0, 100);

	TArray<UCardDefinition*> Deck = { Killer };
	for (int32 i = 0; i < 5; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }
	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);

	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(50, 50, 50, 7, 7, 7);
	UBattleSession* S = Fx.CreateSession(Char, Enemy, /*Seed*/1);

	const FBattleSnapshot Snap0 = S->BuildSnapshot();
	const FGuid Head    = FWacomBattleFixture::FindPartInstanceId(Snap0, 0);
	const FGuid KillerId= FWacomBattleFixture::FindHandInstanceByCardId(Snap0, Killer->CardId);
	const FGuid LHId    = FWacomBattleFixture::FindHandInstanceByCardId(Snap0, LH->CardId);

	// 先打出左手（无目标）
	S->ResolveCommand(FBattleCommand::MakePlayCard(LHId));
	// 再打死 Head
	S->ResolveCommand(MakePlayOnPartInstance(S->BuildSnapshot(), KillerId, Head));

	TestTrue(TEXT("Pending"), S->GetPhase() == EBattlePhase::PendingKnockdownChoice);

	// Aid 是击倒事件分支，不依赖左手锚点当前是否在手牌区。
	const FBattleResolution AidStatus = S->ResolveCommand(
		FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid));
	TestTrue(TEXT("左手已打出，Aid 仍可选"), AidStatus.IsOk());

	TestTrue(TEXT("Aid 后回到 PlayerAction（敌人未全死）"),
		S->GetPhase() == EBattlePhase::PlayerAction);

	return true;
}

// ================ 用左/右手 anchor 直接打死部位 → 对应分支仍可选 ================
// 击倒事件左右手分支不再按手牌区锚点存在与否判断。

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceAnchorAsKillerStillAllowsChoiceSpec,
	"Wacom.Battle.Knockdown.AnchorAsKillerStillAllowsChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceAnchorAsKillerStillAllowsChoiceSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LH = Fx.MakeNoopCard(0);
	// 右手就是杀手卡：cost 0 + 100 伤害
	UCardDefinition* RH = Fx.MakeSimpleDamageCard(0, 100);

	TArray<UCardDefinition*> Deck;
	for (int32 i = 0; i < 5; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }
	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);

	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(50, 50, 50, 7, 7, 7);
	UBattleSession* S = Fx.CreateSession(Char, Enemy, /*Seed*/1);

	const FBattleSnapshot Snap0 = S->BuildSnapshot();
	const FGuid Head = FWacomBattleFixture::FindPartInstanceId(Snap0, 0);
	const FGuid RHId = FWacomBattleFixture::FindHandInstanceByCardId(Snap0, RH->CardId);

	// 用右手 anchor 直接打死 Head
	S->ResolveCommand(MakePlayOnPartInstance(Snap0, RHId, Head));
	TestTrue(TEXT("Pending"), S->GetPhase() == EBattlePhase::PendingKnockdownChoice);

	// Destroy 是击倒事件分支，不依赖右手锚点当前是否在手牌区。
	const FBattleResolution DestroyStatus = S->ResolveCommand(
		FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Destroy));
	TestTrue(TEXT("右手就是杀手，Destroy 仍可选"), DestroyStatus.IsOk());

	TestTrue(TEXT("Destroy 后回到 PlayerAction（敌人未全死）"),
		S->GetPhase() == EBattlePhase::PlayerAction);

	return true;
}

// ================ 最后一个存活部位击倒后 → Withdraw 不可选 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceWithdrawUnavailableOnLastLivingPartSpec,
	"Wacom.Battle.Knockdown.WithdrawRejectedWhenNoLivingPartRemains",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceWithdrawUnavailableOnLastLivingPartSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Killer = nullptr;
	UCharacterDefinition* Char = MakeStandardChar(Fx, &Killer);
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(50, 7, 0);
	UBattleSession* S = Fx.CreateSession(Char, Enemy, /*Seed*/1);

	const FBattleSnapshot Snap0 = S->BuildSnapshot();
	const FGuid Solo = FWacomBattleFixture::FindPartInstanceId(Snap0, 0);
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(Snap0, Killer->CardId);

	const FBattleResolution KnockdownResolution =
		S->ResolveCommand(MakePlayOnPartInstance(Snap0, KillerId, Solo));
	TestTrue(TEXT("Pending"), S->GetPhase() == EBattlePhase::PendingKnockdownChoice);

	const FKnockdownChoiceView View = S->BuildPendingKnockdownChoiceView();
	TestTrue(TEXT("View has pending final knockdown choice"), View.bHasPendingChoice);
	TestTrue(TEXT("Final-part Aid available"), View.AidOption.bAvailable);
	TestFalse(TEXT("Final-part Withdraw unavailable"), View.WithdrawOption.bAvailable);
	TestEqual(TEXT("Final-part Withdraw disabled reason"),
		View.WithdrawOption.DisabledReason,
		FName(TEXT("NoLivingEnemyPart")));
	TestTrue(TEXT("Final-part Destroy available"), View.DestroyOption.bAvailable);

	{
		const TArray<FBattleEvent>& Events = KnockdownResolution.Events;
		int32 LastRequestMask = INDEX_NONE;
		for (const FBattleEvent& E : Events)
		{
			if (E.Type == EBattleEventType::KnockdownChoiceRequested)
			{
				LastRequestMask = E.Count;
			}
		}
		TestEqual(TEXT("Last-part request exposes Aid+Destroy but not Withdraw"), LastRequestMask, 3);
	}

	const FBattleResolution WithdrawStatus = S->ResolveCommand(
		FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Withdraw));
	TestFalse(TEXT("No living part remains, Withdraw rejected"), WithdrawStatus.IsOk());
	TestTrue(TEXT("Still pending after rejected Withdraw"),
		S->GetPhase() == EBattlePhase::PendingKnockdownChoice);

	const FBattleResolution AidStatus = S->ResolveCommand(
		FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid));
	TestTrue(TEXT("Aid still resolves final knockdown"), AidStatus.IsOk());
	TestTrue(TEXT("Final Aid ends battle as victory"), S->GetPhase() == EBattlePhase::BattleEnd);

	const FBattleResultPacket Packet = S->BuildResultPacket();
	TestFalse(TEXT("Final Aid is not withdrawal"), Packet.bWithdrawn);
	TestTrue(TEXT("Outcome=Victory"), Packet.Outcome == EBattleOutcome::Victory);

	return true;
}

// ================ 撤离持久化 + 重入恢复 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceWithdrawPersistsProgressSpec,
	"Wacom.Battle.Knockdown.WithdrawPersistsThenReentryRestoresDestroyed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceWithdrawPersistsProgressSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Killer = nullptr;
	UCharacterDefinition* Char = MakeStandardChar(Fx, &Killer);

	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(50, 50, 50, 7, 7, 7);

	// 用 RunSession 接收战斗结果，模拟完整闭环
	FWacomRunExplorationFixture Exploration;
	UWacomFloorMapDefinition* Floor =
		Exploration.MakeLinearFloor(TEXT("Knockdown.Progress.Floor"), 1);
	Floor->Nodes[0].NodeType = EWacomMapNodeType::Encounter;
	URunSession* Run = Exploration.CreateInitializedSession(
		Char,
		Exploration.MakeJourney({ Floor }, TEXT("Knockdown.Progress.Journey"))).Session;
	FWacomRunSessionTestAccess::GetMutableRunState(*Run).BattleSeed = 1;
	const FRunExplorationResolution EncounterBegin =
		Run->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	if (!TestTrue(TEXT("Encounter begins"),
		EncounterBegin.IsOk() && EncounterBegin.NodeActivityTicket.IsSet()))
	{
		return false;
	}

	// 第一场战斗：撤离前击倒 Head
	{
		FBattleInitParams Params;
		const FName TriggerId(TEXT("TestTrigger"));
		const bool bBuildOk = Run->BuildInitParamsForBattle(TriggerId, Params);
		TestTrue(TEXT("Initial BuildInitParams"), bBuildOk);
		TestEqual(TEXT("Initial EncounterId uses TriggerPersistentId"), Params.EncounterId, TriggerId);
		TestEqual(TEXT("Initial PreDestroyedParts empty"), Params.PreDestroyedParts.Num(), 0);
		if (!bBuildOk)
		{
			return false;
		}
		ApplySingleEnemySlot(Params, Enemy);

		TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
		const FBattleInitializationResult InitStatus = Session->Initialize(Params);
		TestTrue(TEXT("Initial battle initializes"), InitStatus.IsOk());
		if (!InitStatus.IsOk())
		{
			return false;
		}

		const FBattleSnapshot Snap0 = Session->BuildSnapshot();
		const FGuid Head = FWacomBattleFixture::FindPartInstanceId(Snap0, 0);
		const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(Snap0, Killer->CardId);

		TestTrue(TEXT("Killer card is in opening hand"), KillerId.IsValid());
		TestTrue(TEXT("Play killer"), Session->ResolveCommand(MakePlayOnPartInstance(Snap0, KillerId, Head)).IsOk());
		TestTrue(TEXT("Phase pending knockdown after head destroyed"),
			Session->GetPhase() == EBattlePhase::PendingKnockdownChoice);
		TestTrue(TEXT("Choose withdraw"), Session->ResolveCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Withdraw)).IsOk());

		const FBattleResultPacket Packet = Session->BuildResultPacket();
		TestTrue(TEXT("Packet outcome is Victory after withdraw"),
			Packet.Outcome == EBattleOutcome::Victory);
		TestTrue(TEXT("Packet is withdrawn"), Packet.bWithdrawn);
		TestTrue(TEXT("Withdraw settlement succeeds"),
			Run->SettleEncounterNodeActivity(
				EncounterBegin.NodeActivityTicket.GetValue(), Packet).IsOk());

		// BattleProgress 应该有 TestTrigger 的进度
		const FBattleProgressSnapshot* Progress =
			Run->GetRunState().BattleProgress.Find(
				FWacomMapNodeHandle{ TEXT("Knockdown.Progress.Floor"), TEXT("Node.01") });
		TestNotNull(TEXT("BattleProgress 含 TestTrigger"), Progress);
		if (!Progress)
		{
			return false;
		}
		TestEqual(TEXT("DestroyedPartKeys 1 项"),
			Progress->DestroyedPartKeys.Num(), 1);
		if (Progress->DestroyedPartKeys.Num() == 1)
		{
			TestEqual(TEXT("DestroyedPartKeys records Head key"),
				Progress->DestroyedPartKeys[0],
				FBattleEnemyPartKey::Make(TriggerId, TEXT("Enemy"), TEXT("Test.Part.Head")));
		}
		TestEqual(TEXT("DestroyedParts projection is not duplicated into Run progress"),
			Progress->DestroyedParts.Num(), 0);
	}

	// 第二场战斗（重入同一 Trigger）：BuildInitParamsForBattle 应灌入 PreDestroyedParts
	{
		FBattleInitParams Params;
		const bool bOk = Run->BuildInitParamsForBattle(FName(TEXT("TestTrigger")), Params);
		TestTrue(TEXT("BuildInitParams"), bOk);
		TestEqual(TEXT("EncounterId uses TriggerPersistentId"), Params.EncounterId, FName(TEXT("TestTrigger")));
		TestEqual(TEXT("PreDestroyedParts 1 项"), Params.PreDestroyedParts.Num(), 1);
		if (Params.PreDestroyedParts.Num() == 1)
		{
			TestEqual(TEXT("PreDestroyedParts encounter"), Params.PreDestroyedParts[0].GetEffectiveEncounterId(), FName(TEXT("TestTrigger")));
			TestEqual(TEXT("PreDestroyedParts enemy slot"), Params.PreDestroyedParts[0].GetEffectiveEnemySlotId(), FName(TEXT("Enemy")));
			TestEqual(TEXT("PreDestroyedParts part slot"), Params.PreDestroyedParts[0].GetEffectivePartSlotId(), FName(TEXT("Test.Part.Head")));
		}
	}

	return true;
}

// ================ 真胜利清 BattleProgress ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceVictoryClearsProgressSpec,
	"Wacom.Battle.Knockdown.VictoryClearsBattleProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceVictoryClearsProgressSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Killer = nullptr;
	UCharacterDefinition* Char = MakeStandardChar(Fx, &Killer);
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(50, 7, 0);

	FWacomRunExplorationFixture Exploration;
	UWacomFloorMapDefinition* Floor =
		Exploration.MakeLinearFloor(TEXT("Knockdown.Victory.Floor"), 1);
	Floor->Nodes[0].NodeType = EWacomMapNodeType::Encounter;
	URunSession* Run = Exploration.CreateInitializedSession(
		Char,
		Exploration.MakeJourney({ Floor }, TEXT("Knockdown.Victory.Journey"))).Session;
	const FRunExplorationResolution EncounterBegin =
		Run->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	if (!TestTrue(TEXT("Encounter begins"),
		EncounterBegin.IsOk() && EncounterBegin.NodeActivityTicket.IsSet()))
	{
		return false;
	}

	// 先模拟一次撤离写入 BattleProgress
	{
		FBattleProgressSnapshot FakeProgress;
FakeProgress.DestroyedParts.Add(FBattlePartSlotIdentity::Make(
	TEXT("TestTrigger"),
	TEXT("Enemy"),
	TEXT("Test.Part.Solo")));
		FRunState& RunState = FWacomRunSessionTestAccess::GetMutableRunState(*Run);
		RunState.BattleProgress.Add(
			FWacomMapNodeHandle{ TEXT("Knockdown.Victory.Floor"), TEXT("Node.01") },
			FakeProgress);
	}

	// 真胜利
	UBattleSession* S = Fx.CreateSession(Char, Enemy, /*Seed*/1);
	const FBattleSnapshot Snap0 = S->BuildSnapshot();
	const FGuid Solo = FWacomBattleFixture::FindPartInstanceId(Snap0, 0);
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(Snap0, Killer->CardId);
	S->ResolveCommand(MakePlayOnPartInstance(Snap0, KillerId, Solo));
	S->ResolveCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid));

	const FBattleResultPacket Packet = S->BuildResultPacket();
	TestFalse(TEXT("非撤离"), Packet.bWithdrawn);

	TestTrue(TEXT("Victory settlement succeeds"),
		Run->SettleEncounterNodeActivity(
			EncounterBegin.NodeActivityTicket.GetValue(), Packet).IsOk());
	TestFalse(TEXT("真胜利后 BattleProgress 清理"),
		Run->GetRunState().BattleProgress.Contains(
			FWacomMapNodeHandle{ TEXT("Knockdown.Victory.Floor"), TEXT("Node.01") }));

	return true;
}

// ================ 多部位同时破坏 → 逐个发 KnockdownChoiceRequested ================
// 回归用：一张 AllEnemyParts 伤害卡一次性打死 N 个部位时，
// 必须连续发出 N 条 KnockdownChoiceRequested 让 UI 顺序 push N 个 dialog。

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceMultiPartSequenceSpec,
	"Wacom.Battle.Knockdown.MultiPartDestroyedTriggersSequentialRequests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceMultiPartSequenceSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	// 一张 AllEnemyParts 高伤卡一次打死三个部位
	UCardDefinition* Aoe = Fx.MakeNoopCard(/*Cost*/0);
	{
		FCardEffect Eff;
		Eff.EffectType = WacomTags::Effect_Damage;
		Eff.Magnitude  = 100;
		Eff.Target     = WacomTags::Target_AllEnemyParts;
		Aoe->Effects.Add(Eff);
	}

	UCardDefinition* LH = Fx.MakeNoopCard(0);
	UCardDefinition* RH = Fx.MakeNoopCard(0);
	TArray<UCardDefinition*> Deck = { Aoe };
	for (int32 i = 0; i < 5; ++i) { Deck.Add(Fx.MakeNoopCard(0)); }
	UCharacterDefinition* Char = Fx.MakeCharacter(LH, RH, Deck);

	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(50, 50, 50, 7, 7, 7);
	UBattleSession* S = Fx.CreateSession(Char, Enemy, /*Seed*/1);

	const FBattleSnapshot Snap0 = S->BuildSnapshot();
	const FGuid AoeId = FWacomBattleFixture::FindHandInstanceByCardId(Snap0, Aoe->CardId);

	// 打出 AoE 卡，目标随便给 part0（AllEnemyParts 不依赖 TargetInstanceId）
	const FGuid Part0 = FWacomBattleFixture::FindPartInstanceId(Snap0, 0);
	const FBattleResolution AoeResolution =
		S->ResolveCommand(MakePlayOnPartInstance(Snap0, AoeId, Part0));

	// 出牌后应进入 PendingKnockdownChoice 阶段
	TestTrue(TEXT("AoE 后 Phase=PendingKnockdownChoice"),
		S->GetPhase() == EBattlePhase::PendingKnockdownChoice);

	// 这一波事件里应有 1 条 KnockdownChoiceRequested（首次入队时由 Session 末尾发）
	{
		const TArray<FBattleEvent>& Events = AoeResolution.Events;
		int32 RequestCount = 0;
		for (const FBattleEvent& E : Events)
		{
			if (E.Type == EBattleEventType::KnockdownChoiceRequested) { ++RequestCount; }
		}
		TestEqual(TEXT("首条 Request 已发"), RequestCount, 1);
	}

	// 第一次 Aid → 队列还剩 2 条 → Resolver 必须再发一条 Request
	const FBattleResolution FirstAidResolution =
		S->ResolveCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid));
	TestTrue(TEXT("仍 Pending"), S->GetPhase() == EBattlePhase::PendingKnockdownChoice);
	{
		const TArray<FBattleEvent>& Events = FirstAidResolution.Events;
		int32 RequestCount = 0;
		for (const FBattleEvent& E : Events)
		{
			if (E.Type == EBattleEventType::KnockdownChoiceRequested) { ++RequestCount; }
		}
		TestEqual(TEXT("Aid 后再发一条 Request"), RequestCount, 1);
	}

	// 第二次 Aid → 队列还剩 1 条
	const FBattleResolution SecondAidResolution =
		S->ResolveCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid));
	TestTrue(TEXT("仍 Pending"), S->GetPhase() == EBattlePhase::PendingKnockdownChoice);
	{
		const TArray<FBattleEvent>& Events = SecondAidResolution.Events;
		int32 RequestCount = 0;
		for (const FBattleEvent& E : Events)
		{
			if (E.Type == EBattleEventType::KnockdownChoiceRequested) { ++RequestCount; }
		}
		TestEqual(TEXT("第二次 Aid 后再发一条 Request"), RequestCount, 1);
	}

	// 第三次 Aid → 队列空 → 触发 CheckAndApplyBattleEnd → 三部位全死 → Victory
	S->ResolveCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Aid));
	TestTrue(TEXT("第三次 Aid 后 BattleEnd"), S->GetPhase() == EBattlePhase::BattleEnd);

	const FBattleResultPacket P = S->BuildResultPacket();
	TestTrue(TEXT("Outcome=Victory"), P.Outcome == EBattleOutcome::Victory);
	TestFalse(TEXT("非撤离"),         P.bWithdrawn);
	TestEqual(TEXT("KnockdownChoices 3 项"), P.KnockdownChoices.Num(), 3);
	TestEqual(TEXT("DestroyedPartKeys 3 项"), P.DestroyedPartKeys.Num(), 3);
	TestEqual(TEXT("DestroyedParts projection 3 项"), P.DestroyedParts.Num(), 3);

	return true;
}
