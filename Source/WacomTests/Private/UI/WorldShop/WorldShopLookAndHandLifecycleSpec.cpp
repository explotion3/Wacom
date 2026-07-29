// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "RunSession.h"
#include "UI/Card/WacomFirstPersonCardLayerSourceIds.h"
#include "UI/RunFirstPersonCardLayerSpecReceiver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopHandWorldActivitySuppressionSpec,
	"Wacom.UI.WorldShop.LookAndHandLifecycle.WorldActivitySuppressionClearsAndRebuildsLatestSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopHandWorldActivitySuppressionSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Card = Fixture.MakeNoopCard(0);
	Card->CardId = TEXT("WorldShop.Hand.Visible");
	UCardDefinition* RemainingCard = Fixture.MakeNoopCard(0);
	RemainingCard->CardId = TEXT("WorldShop.Hand.Remaining");
	UCardDefinition* PurchasedCard = Fixture.MakeNoopCard(0);
	PurchasedCard->CardId = TEXT("WorldShop.Hand.Purchased");
	UCardDefinition* Pack = Fixture.MakeNoopCard(0);
	Pack->CardId = TEXT("WorldShop.Hand.Capacity");
	Pack->Physique.Capacity = 4;
	UCharacterDefinition* Character =
		Fixture.MakeCharacter(nullptr, nullptr, { Card, RemainingCard, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());
	TStrongObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor(
		NewObject<UWacomFirstPersonCardAnchorComponent>());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	const int32 EntryCount = Anchor->GetRuntimeCardLayerEntries().Num();
	const int32 BackpackCountBefore = Run->GetBackpack().Num();
	const uint64 StorageRevisionBefore = Run->GetBackpackStorageSnapshotRevision();
	TArray<FGuid> CardInstanceIds;
	for (const FWacomFirstPersonCardLayerEntry& Entry :
		Anchor->GetRuntimeCardLayerEntries())
	{
		CardInstanceIds.Add(Entry.CardInstanceId);
	}
	TestTrue(TEXT("fixture starts with multiple visible Run hand entries"), EntryCount > 1);
	TestTrue(TEXT("default hand is interactive"), Anchor->IsFirstPersonCardLayerInteractionEnabled());

	const int32 WritesBeforeSuppression = Source->WriteCount;
	Source->SetRunFirstPersonCardLayerWorldActivitySuppressed(true);
	TestEqual(TEXT("world activity writes one suppressed presentation frame"),
		Source->WriteCount,
		WritesBeforeSuppression + 1);
	TestTrue(TEXT("suppressed presentation retains runtime ownership"),
		Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("suppressed presentation clears all visible entries"),
		Anchor->GetRuntimeCardLayerEntries().Num(),
		0);
	TestEqual(TEXT("suppressed presentation uses the reserved empty source"),
		Anchor->GetRuntimeCardLayerSourceId(),
		WacomFirstPersonCardLayerSourceIds::RunMenuSuppressed());
	TestEqual(TEXT("suppressed frame has the expected commit mode"),
		Source->LastWrittenCommitMode,
		EWacomFirstPersonCardLayerFrameCommitMode::Suppressed);
	TestEqual(TEXT("suppressed frame has no enter hints"),
		Source->LastWrittenTransitionHints.Num(),
		0);
	TestFalse(TEXT("hand interaction is frozen"), Anchor->IsFirstPersonCardLayerInteractionEnabled());
	TestTrue(TEXT("debug reports world activity gate"),
		Source->GetRunFirstPersonCardSourceDebugView().bWorldActivitySuppressed);
	TestEqual(TEXT("debug records world activity suppression"),
		Source->GetRunFirstPersonCardSourceDebugView().LastRefreshResult,
		FName(TEXT("SuppressedByWorldActivity")));
	TestEqual(TEXT("suppression itself does not modify Backpack"),
		Run->GetBackpack().Num(),
		BackpackCountBefore);
	TestEqual(TEXT("suppression itself does not modify storage revision"),
		Run->GetBackpackStorageSnapshotRevision(),
		StorageRevisionBefore);

	const int32 WritesWhileSuppressed = Source->WriteCount;
	Source->SetRunFirstPersonCardLayerWorldActivitySuppressed(true);
	TestEqual(TEXT("repeated suppression is idempotent"),
		Source->WriteCount,
		WritesWhileSuppressed);

	Run->AcquireCardToRun(PurchasedCard);
	TestTrue(TEXT("shop-time acquisition changes the authoritative storage revision"),
		Run->GetBackpackStorageSnapshotRevision() > StorageRevisionBefore);
	TestTrue(TEXT("fixture can remove one formerly visible card while suppressed"),
		Run->DestroyCardByInstance(CardInstanceIds[0]));
	TestEqual(TEXT("Run state changes cannot repopulate the suppressed hand"),
		Anchor->GetRuntimeCardLayerEntries().Num(),
		0);
	TestEqual(TEXT("Run state changes keep the suppressed source"),
		Anchor->GetRuntimeCardLayerSourceId(),
		WacomFirstPersonCardLayerSourceIds::RunMenuSuppressed());

	Source->SetRunFirstPersonCardLayerWorldActivitySuppressed(
		false,
		/*bAnimate*/ true);
	TestFalse(TEXT("debug clears world activity gate"),
		Source->GetRunFirstPersonCardSourceDebugView().bWorldActivitySuppressed);
	TestEqual(TEXT("release restores the default Run source"),
		Anchor->GetRuntimeCardLayerSourceId(),
		Source->RunFirstPersonCardLayerSourceId);
	TestEqual(TEXT("release rebuilds from the latest Run snapshot"),
		Anchor->GetRuntimeCardLayerEntries().Num(),
		EntryCount - 1);
	for (int32 Index = 1; Index < CardInstanceIds.Num(); ++Index)
	{
		TestEqual(
			*FString::Printf(TEXT("remaining card instance %d matches"), Index - 1),
			Anchor->GetRuntimeCardLayerEntries()[Index - 1].CardInstanceId,
			CardInstanceIds[Index]);
	}
	TestEqual(TEXT("every rebuilt card receives a Run hand entry hint"),
		Source->LastWrittenTransitionHints.Num(),
		EntryCount - 1);
	for (const FWacomFirstPersonCardLayerTransitionHint& Hint :
		Source->LastWrittenTransitionHints)
	{
		TestEqual(TEXT("rebuilt hand uses RunHandEntered"),
			Hint.TransitionKind,
			EWacomFirstPersonCardSlotTransitionKind::RunHandEntered);
		TestEqual(TEXT("rebuilt hand exposes the full sequence count"),
			Hint.SequenceCount,
			EntryCount - 1);
	}
	TestTrue(TEXT("default hand interaction is restored"),
		Anchor->IsFirstPersonCardLayerInteractionEnabled());
	return true;
}
