// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Resolution/BattleCardActionPreview.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomBattleCriticalSpec
{
	FCardEffect MakeEffect(
		const FGameplayTag& Type,
		const int32 Magnitude)
	{
		FCardEffect Effect;
		Effect.EffectType = Type;
		Effect.Magnitude = Magnitude;
		Effect.Target = WacomTags::Target_SingleEnemyPart;
		return Effect;
	}

	TStrongObjectPtr<UCardDefinition> MakeCriticalCard()
	{
		TStrongObjectPtr<UCardDefinition> Card(
			NewObject<UCardDefinition>());
		Card->CardId = TEXT("Card.Test.Critical");
		Card->DisplayName = FText::FromString(TEXT("暴击测试"));
		Card->TargetMode = ECardTargetMode::SingleEnemyPart;
		Card->Rarity = WacomTags::Card_Rarity_White;
		for (int32 Tier = 0; Tier < WacomCardUpgrade::TierCount; ++Tier)
		{
			FWacomCardTierProfile& Profile =
				Card->TierProfiles.AddDefaulted_GetRef();
			Profile.Description = FText::FromString(TEXT("暴击测试"));
			Profile.BaseCost = 5;
			Profile.BaseCriticalChancePercent = 100;
			Profile.Effects = {
				MakeEffect(WacomTags::Effect_Damage, 3),
				MakeEffect(WacomTags::Effect_ApplyStatus_Poison, 2),
				MakeEffect(WacomTags::Effect_ApplyStatus_Slow, 1),
			};
		}
		return Card;
	}

	UBattleSession* CreateSession(
		FWacomBattleFixture& Fixture,
		UCardDefinition* Card)
	{
		TArray<UCardDefinition*> Deck = { Card };
		while (Deck.Num() < 5)
		{
			Deck.Add(Fixture.MakeNoopCard(0));
		}
		return Fixture.CreateSession(
			Fixture.MakeCharacter(
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Deck),
			Fixture.MakeSinglePartEnemyWithIntentDamage(
				/*Hp=*/100,
				/*Initiative=*/5,
				/*Damage=*/5),
			123);
	}

	const FBattleEvent* FindEffectFact(
		const TArray<FBattleEvent>& Events,
		const FGameplayTag& EffectType)
	{
		return Events.FindByPredicate([EffectType](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::EffectResolved
				&& Event.EffectResolution.EffectType.MatchesTagExact(
					EffectType);
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCriticalResolutionAndPreviewSpec,
	"Wacom.Battle.Critical.ResolutionPreviewAndResistanceLedger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCriticalResolutionAndPreviewSpec::RunTest(const FString&)
{
	using namespace WacomBattleCriticalSpec;
	FWacomBattleFixture Fixture;
	TStrongObjectPtr<UCardDefinition> Card = MakeCriticalCard();
	UBattleSession* Session = CreateSession(Fixture, Card.Get());
	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(
		Before, Card->CardId);
	const FEnemyPartSnapshot* Target =
		FWacomBattleFixture::GetEnemyPartSnapshot(Before, 0);
	if (!TestTrue(TEXT("Critical card and target exist"),
		CardId.IsValid() && Target))
	{
		return false;
	}

	const FWacomInteractionTargetHandle TargetHandle =
		FWacomInteractionTargetHandle::ForWorldTarget(
			Target->InstanceId,
			nullptr,
			FVector::ZeroVector,
			FVector2D::ZeroVector,
			FGameplayTag(),
			NAME_None,
			Target->EncounterId,
			Target->EnemySlotId,
			Target->PartSlotId);
	const FBattleCardActionPreview Preview =
		Session->BuildCardActionPreview(CardId, TargetHandle);
	TestTrue(TEXT("Preview is available"), Preview.bHasPreview);
	TestEqual(TEXT("Preview exposes an eligible resistance comparison"),
		Preview.ResistancePreviews.Num(), 1);
	if (Preview.ResistancePreviews.Num() == 1)
	{
		TestEqual(TEXT("Preview uses non-critical player peak"),
			Preview.ResistancePreviews[0].PlayerPeakSingleHitDamage, 3);
		TestFalse(TEXT("Preview does not reveal the future critical"),
			Preview.ResistancePreviews[0].bWillStun);
	}
	const FBattleCardActionPreviewEnemyPartState* PreviewPart =
		Preview.ProjectedEnemyParts.FindByPredicate(
			[Target](const FBattleCardActionPreviewEnemyPartState& State)
			{
				return State.Snapshot.InstanceId == Target->InstanceId;
			});
	if (TestNotNull(TEXT("Preview target projection exists"), PreviewPart))
	{
		TestEqual(TEXT("Preview applies base damage and base Poison tick"),
			PreviewPart->Snapshot.CurrentHp, 81);
	}

	const FBattleResolution Resolution = Session->ResolveCommand(
		FWacomBattleFixture::MakePlayCardOnPart(
			Before, CardId, 0));
	TestTrue(TEXT("Formal play succeeds"), Resolution.IsOk());
	const FEnemyPartSnapshot* AfterPart =
		FWacomBattleFixture::GetEnemyPartSnapshot(
			Session->BuildSnapshot(), 0);
	if (!TestNotNull(TEXT("Resolved target exists"), AfterPart))
	{
		return false;
	}
	TestEqual(TEXT("Critical doubles direct damage and Poison application"),
		AfterPart->CurrentHp, 62);

	const FBattleEvent* Damage =
		FindEffectFact(Resolution.Events, WacomTags::Effect_Damage);
	const FBattleEvent* Poison =
		FindEffectFact(
			Resolution.Events, WacomTags::Effect_ApplyStatus_Poison);
	const FBattleEvent* Slow =
		FindEffectFact(
			Resolution.Events, WacomTags::Effect_ApplyStatus_Slow);
	if (!TestTrue(TEXT("All effect facts are emitted"),
		Damage && Poison && Slow))
	{
		return false;
	}
	TestTrue(TEXT("Damage fact records critical"), Damage->EffectResolution.bCritical);
	TestEqual(TEXT("Damage fact stores pre-critical value"),
		Damage->EffectResolution.PreCriticalMagnitude, 3);
	TestEqual(TEXT("Damage fact stores doubled value"),
		Damage->EffectResolution.ResolvedMagnitude, 6);
	TestTrue(TEXT("Poison fact records critical"), Poison->EffectResolution.bCritical);
	TestEqual(TEXT("Poison fact stores doubled stacks"),
		Poison->EffectResolution.ResolvedMagnitude, 4);
	TestFalse(TEXT("Slow fact never crits"), Slow->EffectResolution.bCritical);
	TestEqual(TEXT("Slow remains one stack"),
		Slow->EffectResolution.ResolvedMagnitude, 1);

	const FBattleEvent* PoisonTick = Resolution.Events.FindByPredicate(
		[](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::DamageDealt
				&& Event.Tag.MatchesTagExact(WacomTags::Status_Poison);
		});
	if (!TestNotNull(TEXT("Critical Poison application produces its normal tick"),
		PoisonTick))
	{
		return false;
	}
	TestEqual(TEXT("Doubled Poison layers drive the tick"),
		PoisonTick->DamageResolution.RequestedDamage, 32);
	TestFalse(TEXT("Periodic Poison tick never rolls critical again"),
		PoisonTick->DamageResolution.bCritical);

	const FBattleEvent* DamageEvent = Resolution.Events.FindByPredicate(
		[](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::DamageDealt;
		});
	TestTrue(TEXT("Damage event mirrors critical fact"),
		DamageEvent && DamageEvent->DamageResolution.bCritical);
	const FBattleEvent* Resistance = Resolution.Events.FindByPredicate(
		[](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::ResistanceResolved;
		});
	if (!TestNotNull(TEXT("Formal resistance comparison exists"), Resistance))
	{
		return false;
	}
	TestEqual(TEXT("Resistance reuses critical damage peak"),
		Resistance->Amount, 6);
	TestEqual(TEXT("Enemy attack peak remains five"),
		Resistance->Count, 5);
	TestTrue(TEXT("Critical turns formal resistance into success"),
		Resistance->bSuccess);
	return true;
}
