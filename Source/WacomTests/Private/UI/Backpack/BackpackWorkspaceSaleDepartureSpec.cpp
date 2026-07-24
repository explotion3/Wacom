// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Fixtures/WacomRunExplorationFixture.h"

#if WITH_AUTOMATION_TESTS

#include "../BackpackScreenTestAccess.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Card/WacomFirstPersonCardPlayedDissolveStyle.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
struct FSaleDepartureFixture
{
	TStrongObjectPtr<URunSession> Run;
	TStrongObjectPtr<UWacomBackpackScreen> Screen;
	TArray<FGuid> InstanceIds;
};

TUniquePtr<FSaleDepartureFixture> BuildSaleFixture(
	UObject* Outer,
	int32 CardCount)
{
	UCardDefinition* Bag = NewObject<UCardDefinition>(Outer);
	Bag->CardId = FName(*FString::Printf(
		TEXT("WorkspaceSaleDeparture.Bag.%d"),
		CardCount));
	Bag->Physique.Capacity = CardCount + 4;
	UCardDefinition* DeleteProvider = NewObject<UCardDefinition>(Outer);
	DeleteProvider->CardId = FName(*FString::Printf(
		TEXT("WorkspaceSaleDeparture.Provider.%d"),
		CardCount));
	DeleteProvider->Keywords.AddTag(
		WacomTags::Card_Keyword_DeleteProvider);
	UCardDefinition* SaleCard = NewObject<UCardDefinition>(Outer);
	SaleCard->CardId = FName(*FString::Printf(
		TEXT("WorkspaceSaleDeparture.Card.%d"),
		CardCount));
	SaleCard->Rarity = WacomTags::Card_Rarity_White;
	UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
	Character->CharacterId = FName(*FString::Printf(
		TEXT("WorkspaceSaleDeparture.Character.%d"),
		CardCount));
	Character->StarterDeck.Add(Bag);
	Character->StarterDeck.Add(DeleteProvider);

	TUniquePtr<FSaleDepartureFixture> Fixture =
		MakeUnique<FSaleDepartureFixture>();
	Fixture->Run.Reset(NewObject<URunSession>(Outer));
	if (!InitializeRunSessionForTest(*Fixture->Run, Character).IsOk())
	{
		return nullptr;
	}
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		Fixture->Run->AcquireCardToRun(SaleCard);
	}
	const FRunBackpackStorageSnapshot Snapshot =
		Fixture->Run->BuildBackpackStorageSnapshot();
	for (const FRunStorageCardView& Card : Snapshot.Flux.ContentCards)
	{
		if (Card.Instance.Definition == SaleCard)
		{
			Fixture->InstanceIds.Add(Card.Instance.InstanceId);
		}
	}
	Fixture->Screen.Reset(
		FWacomBackpackScreenTestAccess::Create(Outer, Fixture->Run.Get()));
	return Fixture;
}

void AdvanceReadySale(
	UWacomBackpackScreen& Screen,
	int32 FrameCount,
	float DeltaSeconds = 0.1f)
{
	for (int32 Frame = 0; Frame < FrameCount; ++Frame)
	{
		FWacomBackpackScreenTestAccess::ForceWorkspaceSaleReadiness(Screen);
		FWacomBackpackScreenTestAccess::TickWorkspaceSaleDeparture(
			Screen,
			DeltaSeconds);
	}
}

void AdvanceReadySaleBySeconds(
	UWacomBackpackScreen& Screen,
	float TotalSeconds)
{
	float RemainingSeconds = FMath::Max(0.0f, TotalSeconds);
	while (RemainingSeconds > KINDA_SMALL_NUMBER)
	{
		const float StepSeconds = FMath::Min(0.025f, RemainingSeconds);
		FWacomBackpackScreenTestAccess::ForceWorkspaceSaleReadiness(Screen);
		FWacomBackpackScreenTestAccess::TickWorkspaceSaleDeparture(
			Screen,
			StepSeconds);
		RemainingSeconds -= StepSeconds;
	}
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceSaleDepartureSingleLaunchSpec,
	"Wacom.UI.Backpack.Workspace.SaleDeparture.RandomSingleCardLaunchAndMaterial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceSaleDepartureSingleLaunchSpec::RunTest(
	const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	for (const int32 CardCount : { 1, 4, 5, 18 })
	{
		TUniquePtr<FSaleDepartureFixture> Fixture =
			BuildSaleFixture(Outer, CardCount);
		if (!TestNotNull(
				*FString::Printf(TEXT("%d-card fixture initializes"), CardCount),
				Fixture.Get())
			|| !TestNotNull(
				*FString::Printf(TEXT("%d-card Screen exists"), CardCount),
				Fixture ? Fixture->Screen.Get() : nullptr))
		{
			continue;
		}
		TestEqual(
			*FString::Printf(TEXT("%d sale identities exist"), CardCount),
			Fixture->InstanceIds.Num(),
			CardCount);
		FWacomBackpackScreenTestAccess::SetWorkspaceSimplifiedMotion(
			*Fixture->Screen,
			false);
		TestTrue(
			*FString::Printf(TEXT("%d cards enter carry"), CardCount),
			FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(
				*Fixture->Screen,
				Fixture->InstanceIds));
		const FWacomBackpackWorkspaceAutomationTestView CarryView =
			FWacomBackpackScreenTestAccess::WorkspaceView(*Fixture->Screen);
		const FGuid CurrentId =
			CarryView.CarriedInstanceIds.IsValidIndex(
				CarryView.CurrentCarryIndex)
				? CarryView.CarriedInstanceIds[CarryView.CurrentCarryIndex]
				: FGuid();

		FWacomBackpackScreenTestAccess::SubmitWorkspaceDelete(
			*Fixture->Screen,
			Fixture->InstanceIds);
		FWacomBackpackWorkspaceAutomationTestView View =
			FWacomBackpackScreenTestAccess::WorkspaceView(*Fixture->Screen);
		const FRunBackpackStorageSnapshot SoldSnapshot =
			Fixture->Run->BuildBackpackStorageSnapshot();
		TestFalse(
			*FString::Printf(TEXT("%d rules commit immediately"), CardCount),
			SoldSnapshot.Flux.ContentCards.ContainsByPredicate(
				[&Fixture](const FRunStorageCardView& Card)
				{
					return Fixture->InstanceIds.Contains(
						Card.Instance.InstanceId);
				}));
		TestEqual(
			*FString::Printf(TEXT("%d visuals enter randomized queue"), CardCount),
			View.SaleDepartureQueuedCardCount,
			CardCount);
		TestEqual(
			*FString::Printf(TEXT("%d ghosts leave Registry"), CardCount),
			View.WorkspaceCardCount,
			CarryView.WorkspaceCardCount - CardCount);
		TestEqual(
			*FString::Printf(TEXT("%d visual poses remain in departure layer"), CardCount),
			View.SettlementCardCount,
			CardCount);
		TArray<FGuid> OriginalCarryOrder;
		OriginalCarryOrder.Add(CurrentId);
		for (const FGuid InstanceId : CarryView.CarriedInstanceIds)
		{
			if (InstanceId != CurrentId)
			{
				OriginalCarryOrder.Add(InstanceId);
			}
		}
		if (CardCount > 1)
		{
			TestNotEqual(
				*FString::Printf(TEXT("%d-card batch is visibly shuffled"), CardCount),
				View.SaleDeparturePendingInstanceIds,
				OriginalCarryOrder);
		}
		const TArray<FGuid> RandomizedOrder =
			View.SaleDeparturePendingInstanceIds;
		TestEqual(
			*FString::Printf(TEXT("%d cards receive seeds"), CardCount),
			View.SaleDepartureSeeds.Num(),
			CardCount);
		TSet<float> UniqueSeeds;
		for (const TPair<FGuid, float>& Pair : View.SaleDepartureSeeds)
		{
			UniqueSeeds.Add(Pair.Value);
		}
		TestEqual(
			*FString::Printf(TEXT("%d InstanceIds receive distinct seeds"), CardCount),
			UniqueSeeds.Num(),
			CardCount);

		FWacomBackpackScreenTestAccess::TickWorkspaceSaleDeparture(
			*Fixture->Screen,
			0.0f);
		View = FWacomBackpackScreenTestAccess::WorkspaceView(*Fixture->Screen);
		TestEqual(
			*FString::Printf(TEXT("%d first scheduler decision launches one card"), CardCount),
			View.SaleDepartureActiveCardCount,
			1);
		TestEqual(
			*FString::Printf(TEXT("%d first launch uses randomized identity"), CardCount),
			View.SaleDepartureActiveInstanceIds[0],
			RandomizedOrder[0]);
		TestEqual(
			*FString::Printf(TEXT("%d remaining cards wait for individual launch decisions"), CardCount),
			View.SaleDepartureQueuedCardCount,
			CardCount - 1);
		TestTrue(
			*FString::Printf(TEXT("%d never exceeds four realtime Retainers"), CardCount),
			View.RealtimeCardCount <= 4
				&& View.SaleDepartureMaximumRealtimeCardCount <= 4);
		for (const FWacomBackpackSaleCardSurfaceProbe& Probe :
			FWacomBackpackScreenTestAccess::WorkspaceSaleSurfaceProbes(
				*Fixture->Screen))
		{
			TestTrue(TEXT("Every active sold card owns an Exhaust dissolve view"),
				Probe.bPlayedDissolveActive);
			TestTrue(TEXT("Every active sold card targets the real Surface MID"),
				Probe.bUsingSurfaceEffectMaterial);
			TestTrue(TEXT("Every active sold card owns a realtime Retainer"),
				Probe.bRealtimePresentationEnabled);
			TestTrue(TEXT("Sale clears selected/current presentation before dissolve"),
				Probe.bSelectionPresentationCleared);
			TestTrue(TEXT("Sale collapses the WBP feedback outline before dissolve"),
				Probe.bFeedbackOverlayCollapsed);
			TestTrue(TEXT("Sale clears native focus and semantic markers before dissolve"),
				Probe.bAccessibilityPresentationCleared);
		}

		const int32 ExpectedLaunchesBeforeCompletion =
			FMath::Min(4, CardCount);
		for (int32 ExpectedLaunchCount = 1;
			ExpectedLaunchCount < ExpectedLaunchesBeforeCompletion;
			++ExpectedLaunchCount)
		{
			FWacomBackpackScreenTestAccess::ForceWorkspaceSaleReadiness(
				*Fixture->Screen);
			FWacomBackpackScreenTestAccess::TickWorkspaceSaleDeparture(
				*Fixture->Screen,
				0.0f);
			View = FWacomBackpackScreenTestAccess::WorkspaceView(
				*Fixture->Screen);
			const float LaunchDelay =
				View.SaleDepartureNextLaunchDelaySeconds;
			TestTrue(
				*FString::Printf(
					TEXT("%d launch interval %d is visibly staggered"),
					CardCount,
					ExpectedLaunchCount),
				LaunchDelay >= 0.09f - KINDA_SMALL_NUMBER
					&& LaunchDelay <= 0.12f + KINDA_SMALL_NUMBER);

			AdvanceReadySaleBySeconds(
				*Fixture->Screen,
				LaunchDelay - 0.005f);
			View = FWacomBackpackScreenTestAccess::WorkspaceView(
				*Fixture->Screen);
			TestEqual(
				TEXT("No second card launches before the random interval expires"),
				View.SaleDepartureActiveCardCount,
				ExpectedLaunchCount);

			AdvanceReadySaleBySeconds(*Fixture->Screen, 0.01f);
			View = FWacomBackpackScreenTestAccess::WorkspaceView(
				*Fixture->Screen);
			TestEqual(
				TEXT("Crossing one interval launches exactly one additional card"),
				View.SaleDepartureActiveCardCount,
				ExpectedLaunchCount + 1);
			TArray<FGuid> ExpectedActiveOrder = RandomizedOrder;
			ExpectedActiveOrder.SetNum(ExpectedLaunchCount + 1);
			TestEqual(
				TEXT("Single-card launches preserve the randomized queue order"),
				View.SaleDepartureActiveInstanceIds,
				ExpectedActiveOrder);
			TestEqual(
				TEXT("One scheduler event consumes exactly one queued identity"),
				View.SaleDepartureQueuedCardCount,
				CardCount - ExpectedLaunchCount - 1);
		}

		if (CardCount > 1)
		{
			const TArray<FWacomBackpackSaleCardSurfaceProbe> OverlapProbes =
				FWacomBackpackScreenTestAccess::WorkspaceSaleSurfaceProbes(
					*Fixture->Screen);
			TestTrue(
				TEXT("Individually launched cards overlap while earlier dissolves continue"),
				OverlapProbes.Num() > 1);
			TestTrue(
				TEXT("Earlier card is already dissolving when the next card starts"),
				OverlapProbes.ContainsByPredicate(
					[](const FWacomBackpackSaleCardSurfaceProbe& Probe)
					{
						return Probe.Amount > 0.0f
							&& Probe.Amount < 1.0f;
					}));
		}

		AdvanceReadySale(*Fixture->Screen, CardCount * 3 + 5);
		View = FWacomBackpackScreenTestAccess::WorkspaceView(*Fixture->Screen);
		TestEqual(
			*FString::Printf(TEXT("%d sale visuals all complete"), CardCount),
			View.SaleDepartureCompletedCardCount,
			CardCount);
		TestEqual(TEXT("Completed randomized queue has no queued cards"),
			View.SaleDepartureQueuedCardCount, 0);
		TestEqual(TEXT("Completed randomized queue has no active cards"),
			View.SaleDepartureActiveCardCount, 0);
		TestEqual(TEXT("Completed randomized queue leaves no ghost Widgets"),
			View.SettlementCardCount, 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceSaleDepartureAppendLifecycleSpec,
	"Wacom.UI.Backpack.Workspace.SaleDeparture.AppendReadinessLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceSaleDepartureAppendLifecycleSpec::RunTest(
	const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	TUniquePtr<FSaleDepartureFixture> Fixture = BuildSaleFixture(Outer, 8);
	if (!TestNotNull(TEXT("Append fixture initializes"), Fixture.Get())
		|| !TestNotNull(TEXT("Append Screen exists"),
			Fixture ? Fixture->Screen.Get() : nullptr))
	{
		return false;
	}
	FWacomBackpackScreenTestAccess::SetWorkspaceSimplifiedMotion(
		*Fixture->Screen,
		true);
	const TArray<FGuid> FirstBatch{
		Fixture->InstanceIds[0],
		Fixture->InstanceIds[1],
		Fixture->InstanceIds[2],
		Fixture->InstanceIds[3],
	};
	const TArray<FGuid> SecondBatch{
		Fixture->InstanceIds[4],
		Fixture->InstanceIds[5],
		Fixture->InstanceIds[6],
		Fixture->InstanceIds[7],
	};
	TestTrue(TEXT("First batch enters carry"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(
			*Fixture->Screen,
			FirstBatch));
	FWacomBackpackScreenTestAccess::SubmitWorkspaceDelete(
		*Fixture->Screen,
		FirstBatch);
	FWacomBackpackScreenTestAccess::TickWorkspaceSaleDeparture(
		*Fixture->Screen,
		0.0f);
	const uint64 SchedulerGeneration =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Fixture->Screen)
			.FrameSchedulerGeneration;
	FWacomBackpackScreenTestAccess::ForceWorkspaceSaleReadiness(
		*Fixture->Screen);
	FWacomBackpackScreenTestAccess::TickWorkspaceSaleDeparture(
		*Fixture->Screen,
		0.0f);
	const FWacomBackpackWorkspaceAutomationTestView FirstLaunchView =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Fixture->Screen);
	TestEqual(TEXT("Simplified queue initially launches one card"),
		FirstLaunchView.SaleDepartureActiveCardCount, 1);
	TestEqual(TEXT("Remaining first-batch cards stay pending"),
		FirstLaunchView.SaleDepartureQueuedCardCount, 3);
	TestTrue(TEXT("Simplified single-card interval is shortened but visible"),
		FirstLaunchView.SaleDepartureNextLaunchDelaySeconds
				>= 0.03f - KINDA_SMALL_NUMBER
			&& FirstLaunchView.SaleDepartureNextLaunchDelaySeconds
				<= 0.04f + KINDA_SMALL_NUMBER);

	TestTrue(TEXT("Input remains available while first card dissolves"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(
			*Fixture->Screen,
			SecondBatch));
	FWacomBackpackScreenTestAccess::SubmitWorkspaceDelete(
		*Fixture->Screen,
		SecondBatch);
	FWacomBackpackWorkspaceAutomationTestView View =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Fixture->Screen);
	TestEqual(TEXT("Existing single launch remains active"),
		View.SaleDepartureActiveCardCount, 1);
	TestEqual(TEXT("New atomic sale appends behind the pending first batch"),
		View.SaleDepartureQueuedCardCount, 7);
	TestEqual(TEXT("Repeated wake does not register a second Timer generation"),
		View.FrameSchedulerGeneration, SchedulerGeneration);
	TestTrue(TEXT("Simplified Motion still uses real material surfaces"),
		FWacomBackpackScreenTestAccess::WorkspaceSaleSurfaceProbes(
			*Fixture->Screen).ContainsByPredicate(
				[](const FWacomBackpackSaleCardSurfaceProbe& Probe)
				{
					return Probe.bPlayedDissolveActive
						&& Probe.bUsingSurfaceEffectMaterial;
				}));

	FWacomBackpackScreenTestAccess::DeactivateWorkspaceScreen(*Fixture->Screen);
	View = FWacomBackpackScreenTestAccess::WorkspaceView(*Fixture->Screen);
	TestEqual(TEXT("Deactivate clears active sale surfaces"),
		View.SaleDepartureActiveCardCount, 0);
	TestEqual(TEXT("Deactivate clears queued sale surfaces"),
		View.SaleDepartureQueuedCardCount, 0);
	TestEqual(TEXT("Deactivate removes all departure ghosts"),
		View.SettlementCardCount, 0);

	TUniquePtr<FSaleDepartureFixture> TimeoutFixture =
		BuildSaleFixture(Outer, 1);
	TestNotNull(TEXT("Readiness timeout fixture initializes"),
		TimeoutFixture.Get());
	if (TimeoutFixture && TimeoutFixture->Screen)
	{
		TestTrue(TEXT("Timeout card enters carry"),
			FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(
				*TimeoutFixture->Screen,
				TimeoutFixture->InstanceIds));
		FWacomBackpackScreenTestAccess::SubmitWorkspaceDelete(
			*TimeoutFixture->Screen,
			TimeoutFixture->InstanceIds);
		FWacomBackpackScreenTestAccess::TickWorkspaceSaleDeparture(
			*TimeoutFixture->Screen,
			0.0f);
		for (int32 Index = 0; Index < 8; ++Index)
		{
			FWacomBackpackScreenTestAccess::TickWorkspaceSaleDeparture(
				*TimeoutFixture->Screen,
				0.1f);
		}
		const FWacomBackpackWorkspaceAutomationTestView TimeoutView =
			FWacomBackpackScreenTestAccess::WorkspaceView(
				*TimeoutFixture->Screen);
		TestEqual(TEXT("Readiness timeout removes the ghost"),
			TimeoutView.SettlementCardCount, 0);
		const FRunBackpackStorageSnapshot TimeoutSnapshot =
			TimeoutFixture->Run->BuildBackpackStorageSnapshot();
		TestFalse(TEXT("Readiness timeout does not undo the sale rule"),
			TimeoutSnapshot.Flux.ContentCards.ContainsByPredicate(
				[&TimeoutFixture](const FRunStorageCardView& Card)
				{
					return TimeoutFixture->InstanceIds.Contains(
						Card.Instance.InstanceId);
				}));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceSaleDepartureAssetContractSpec,
	"Wacom.UI.Backpack.Workspace.SaleDeparture.AssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceSaleDepartureAssetContractSpec::RunTest(
	const FString& Parameters)
{
	const UWacomBackpackWorkspaceStyle* Defaults =
		GetDefault<UWacomBackpackWorkspaceStyle>();
	TestTrue(TEXT("Workspace defaults reference a sale dissolve Style"),
		!Defaults->SaleDissolveStyle.IsNull());
	UWacomFirstPersonCardPlayedDissolveStyle* Style =
		Defaults->SaleDissolveStyle.LoadSynchronous();
	if (TestNotNull(TEXT("Formal Ordered Dither Style resolves"), Style))
	{
		TestEqual(TEXT("Backpack sale reuses Exhausted Ordered Dither"),
			Style->Style.EffectKind,
			EWacomFirstPersonCardPlayedDissolveEffectKind::OrderedDither);
		TestNotNull(TEXT("Sale Style has a Retainer surface material"),
			Style->Style.SurfaceEffectMaterial.Get());
		TestNotNull(TEXT("Sale Style has its authored noise texture"),
			Style->Style.NoiseTexture.Get());
		TestTrue(TEXT("Full Motion duration remains 0.40 seconds"),
			FMath::IsNearlyEqual(Style->Style.DurationSeconds, 0.40f));
		TestTrue(TEXT("Full Motion confirm hold remains 0.05 seconds"),
			FMath::IsNearlyEqual(Style->Style.ConfirmHoldSeconds, 0.05f));
	}
	return true;
}

#endif
