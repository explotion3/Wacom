// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "RunSession.h"
#include "UI/RunFirstPersonCardLayerSpecReceiver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopHandInteractionOnlySuppressionSpec,
	"Wacom.UI.WorldShop.LookAndHandLifecycle.InteractionOnlySuppressionKeepsEntriesAndHints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopHandInteractionOnlySuppressionSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Card = Fixture.MakeNoopCard(0);
	Card->CardId = TEXT("WorldShop.Hand.Visible");
	UCardDefinition* Pack = Fixture.MakeNoopCard(0);
	Pack->CardId = TEXT("WorldShop.Hand.Capacity");
	Pack->Physique.Capacity = 2;
	UCharacterDefinition* Character = Fixture.MakeCharacter(nullptr, nullptr, { Card, Pack });

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
	const int32 WriteCount = Source->WriteCount;
	const int32 HintCount = Source->LastWrittenTransitionHints.Num();
	TestTrue(TEXT("default hand is interactive"), Anchor->IsFirstPersonCardLayerInteractionEnabled());

	Source->SetRunFirstPersonCardLayerInteractionSuppressedByWorldShop(true);
	TestEqual(TEXT("visible entries remain"), Anchor->GetRuntimeCardLayerEntries().Num(), EntryCount);
	TestEqual(TEXT("no presentation frame is rewritten on enter"), Source->WriteCount, WriteCount);
	TestEqual(TEXT("no new transition hint is produced"),
		Source->LastWrittenTransitionHints.Num(), HintCount);
	TestFalse(TEXT("hand interaction is frozen"), Anchor->IsFirstPersonCardLayerInteractionEnabled());
	TestTrue(TEXT("debug reports world shop gate"),
		Source->GetRunFirstPersonCardSourceDebugView().bInteractionSuppressedByWorldShop);

	Source->SetRunFirstPersonCardLayerInteractionSuppressedByWorldShop(false);
	TestEqual(TEXT("visible entries still remain"), Anchor->GetRuntimeCardLayerEntries().Num(), EntryCount);
	TestEqual(TEXT("no presentation frame is rewritten on exit"), Source->WriteCount, WriteCount);
	TestTrue(TEXT("hand interaction is restored"), Anchor->IsFirstPersonCardLayerInteractionEnabled());
	return true;
}
