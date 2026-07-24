// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../WacomApp/Private/UI/Battle/WacomBattleFloatingCombatTextSynchronizer.h"
#include "Events/BattleEvent.h"
#include "Tags/WacomGameplayTags.h"

namespace WacomBattleFloatingCombatTextSpec
{
	FBattleEvent MakeDamage(
		const int32 Sequence,
		const int32 HpLoss,
		const int32 ShieldAbsorbed,
		const EBattleDamageKind Kind = EBattleDamageKind::Direct,
		const bool bCritical = false,
		const FBattleEnemyPartKey& PartKey = FBattleEnemyPartKey())
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::DamageDealt;
		Event.Sequence = Sequence;
		Event.Amount = HpLoss;
		Event.ActorEnemyPartKey = PartKey;
		Event.DamageResolution.RequestedDamage = HpLoss + ShieldAbsorbed;
		Event.DamageResolution.ShieldBefore = ShieldAbsorbed;
		Event.DamageResolution.ShieldAbsorbed = ShieldAbsorbed;
		Event.DamageResolution.ShieldAfter = 0;
		Event.DamageResolution.Kind = Kind;
		Event.DamageResolution.bCritical = bCritical;
		return Event;
	}

	FWacomBattlePresentationProgress MakeProgress(
		const uint64 TransactionId,
		const EWacomBattlePresentationProgressKind Kind)
	{
		FWacomBattlePresentationProgress Progress;
		Progress.PresentationTransactionId = TransactionId;
		Progress.Kind = Kind;
		return Progress;
	}

	const TArray<FWacomBattleFloatingCombatTextRow>& FirstRows(
		const TArray<FWacomBattleFloatingCombatTextEmission>& Emissions)
	{
		static const TArray<FWacomBattleFloatingCombatTextRow> Empty;
		return Emissions.IsEmpty() ? Empty : Emissions[0].Rows;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleFloatingCombatTextProjectionSpec,
	"Wacom.UI.Battle.FloatingCombatText.Projection.BuildsExactStructuredRows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleFloatingCombatTextProjectionSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleFloatingCombatTextSpec;

	const FBattleEnemyPartKey PartKey =
		FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("Snake"), TEXT("Tail"));
	TArray<FBattleEvent> Events;
	Events.Add(MakeDamage(11, 3, 4, EBattleDamageKind::Direct, false, PartKey));
	Events.Add(MakeDamage(12, 0, 7));
	FBattleEvent Poison =
		MakeDamage(13, 8, 0, EBattleDamageKind::Periodic, false, PartKey);
	Poison.Tag = WacomTags::Status_Poison;
	Events.Add(Poison);
	Events.Add(MakeDamage(14, 9, 0, EBattleDamageKind::Direct, true, PartKey));
	FBattleEvent Shield;
	Shield.Type = EBattleEventType::ShieldChanged;
	Shield.Sequence = 15;
	Shield.Amount = 5;
	Shield.Count = 9;
	Shield.ActorEnemyPartKey = PartKey;
	Events.Add(Shield);

	FWacomBattleFloatingCombatTextSynchronizer Synchronizer;
	Synchronizer.Stage(7, Events);
	FWacomBattlePresentationProgress Progress =
		MakeProgress(7, EWacomBattlePresentationProgressKind::PhaseEventsReached);
	Progress.EventSequences = { 11, 12, 13, 14, 15 };
	bool bFlushed = false;
	const TArray<FWacomBattleFloatingCombatTextEmission> Emissions =
		Synchronizer.ApplyProgress(Progress, bFlushed);
	const TArray<FWacomBattleFloatingCombatTextRow>& Rows = FirstRows(Emissions);

	TestFalse(TEXT("Phase release is not a fallback flush"), bFlushed);
	TestEqual(TEXT("Each shield/HP channel is preserved without -0"), Rows.Num(), 6);
	if (Rows.Num() == 6)
	{
		TestEqual(TEXT("Partial hit releases shield first"),
			Rows[0].Kind, EWacomBattleFloatingCombatTextKind::ShieldAbsorbed);
		TestEqual(TEXT("Partial hit shield amount"), Rows[0].Amount, 4);
		TestEqual(TEXT("Partial hit releases HP second"),
			Rows[1].Kind, EWacomBattleFloatingCombatTextKind::HpDamage);
		TestEqual(TEXT("Partial hit HP amount"), Rows[1].Amount, 3);
		TestEqual(TEXT("Full absorption emits only a shield channel"),
			Rows[2].Kind, EWacomBattleFloatingCombatTextKind::ShieldAbsorbed);
		TestEqual(TEXT("Periodic damage retains its explicit kind"),
			Rows[3].Kind, EWacomBattleFloatingCombatTextKind::PeriodicDamage);
		TestTrue(TEXT("Periodic damage retains its status icon tag"),
			Rows[3].IconTag == WacomTags::Status_Poison.GetTag());
		TestEqual(TEXT("Critical fact activates dormant critical presentation"),
			Rows[4].Kind, EWacomBattleFloatingCombatTextKind::CriticalDamage);
		TestEqual(TEXT("Typed shield mutation emits signed shield row"),
			Rows[5].Kind, EWacomBattleFloatingCombatTextKind::ShieldChanged);
		TestEqual(TEXT("Shield gain amount remains signed"), Rows[5].Amount, 5);
		TestEqual(TEXT("Enemy channels retain exact part identity"),
			Rows[0].Target.EnemyPartKey, PartKey);
		TestEqual(TEXT("Player channel remains player-owned"),
			Rows[2].Target.Kind, EWacomBattleFloatingCombatTextTargetKind::Player);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleFloatingCombatTextSynchronizationSpec,
	"Wacom.UI.Battle.FloatingCombatText.Synchronization.ReleasesExactlyOnceAtSemanticProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleFloatingCombatTextSynchronizationSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleFloatingCombatTextSpec;

	FWacomBattleFloatingCombatTextSynchronizer Synchronizer;
	Synchronizer.Stage(21, { MakeDamage(41, 2, 0), MakeDamage(42, 3, 0) });

	FWacomBattlePresentationProgress Phase =
		MakeProgress(21, EWacomBattlePresentationProgressKind::PhaseEventsReached);
	Phase.EventSequences = { 41 };
	bool bFlushed = false;
	TArray<FWacomBattleFloatingCombatTextEmission> Emissions =
		Synchronizer.ApplyProgress(Phase, bFlushed);
	TestEqual(TEXT("Phase releases its exact event once"), FirstRows(Emissions).Num(), 1);

	Emissions = Synchronizer.ApplyProgress(Phase, bFlushed);
	TestTrue(TEXT("Duplicate progress is idempotent"), Emissions.IsEmpty());

	FWacomBattlePresentationProgress Impact =
		MakeProgress(21, EWacomBattlePresentationProgressKind::EnemyActionImpact);
	Impact.FirstEventSequence = 42;
	Impact.LastEventSequence = 42;
	Emissions = Synchronizer.ApplyProgress(Impact, bFlushed);
	TestEqual(TEXT("Enemy impact releases its journal range"), FirstRows(Emissions).Num(), 1);
	TestEqual(TEXT("Impact row keeps event sequence"),
		FirstRows(Emissions)[0].EventSequence, 42);

	FWacomBattlePresentationProgress Complete =
		MakeProgress(21, EWacomBattlePresentationProgressKind::PlanCompleted);
	Emissions = Synchronizer.ApplyProgress(Complete, bFlushed);
	TestFalse(TEXT("Fully matched completion has no remainder"), bFlushed);
	TestTrue(TEXT("Fully matched completion emits nothing twice"), Emissions.IsEmpty());

	Synchronizer.Stage(22, { MakeDamage(51, 4, 0), MakeDamage(52, 5, 0) });
	Complete.PresentationTransactionId = 22;
	Emissions = Synchronizer.ApplyProgress(Complete, bFlushed);
	TestTrue(TEXT("Normal completion flushes unmatched results"), bFlushed);
	TestEqual(TEXT("Flush keeps all unmatched rows in order"),
		FirstRows(Emissions).Num(), 2);

	Synchronizer.Stage(23, { MakeDamage(61, 6, 0) });
	FWacomBattlePresentationProgress Cancel =
		MakeProgress(23, EWacomBattlePresentationProgressKind::PlanCancelled);
	Cancel.CancelPolicy = EWacomBattlePresentationCancelPolicy::DiscardPending;
	Emissions = Synchronizer.ApplyProgress(Cancel, bFlushed);
	TestTrue(TEXT("Session-style cancellation discards pending rows"), Emissions.IsEmpty());
	Complete.PresentationTransactionId = 23;
	Emissions = Synchronizer.ApplyProgress(Complete, bFlushed);
	TestTrue(TEXT("Discarded rows cannot leak into a later completion"), Emissions.IsEmpty());
	return true;
}
