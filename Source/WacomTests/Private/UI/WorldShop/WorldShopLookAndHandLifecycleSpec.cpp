// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "RunSession.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "UI/RunFirstPersonCardLayerSpecReceiver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopHandWorldActivitySuppressionSpec,
	"Wacom.UI.WorldShop.LookAndHandLifecycle.WorldActivitySuppressionKeepsIdentityAndHints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopHandWorldActivitySuppressionSpec::RunTest(
	const FString& Parameters)
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
	TStrongObjectPtr<UWacomFirstPersonCardLayerWidget> Layer(
		NewObject<UWacomFirstPersonCardLayerWidget>());
	const TSharedRef<SWidget> LayerSlateWidget = Layer->TakeWidget();
	FWacomFirstPersonCardLayerTestAccess::SetViewportSizeOverride(
		*Layer,
		FVector2D(1920.0f, 1080.0f));
	Anchor->SetCardLayerWidgetForTest(Layer.Get());
	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	Source->AnchorForTest = Anchor.Get();
	Source->BindRunSession(Run.Get());
	Source->SetRunFirstPersonCardLayerActive(true);

	const int32 EntryCount = Anchor->GetRuntimeCardLayerEntries().Num();
	const int32 WriteCount = Source->WriteCount;
	const int32 HintCount = Source->LastWrittenTransitionHints.Num();
	const FName SourceId = Anchor->GetRuntimeCardLayerSourceId();
	TArray<FGuid> CardInstanceIds;
	for (const FWacomFirstPersonCardLayerEntry& Entry :
		Anchor->GetRuntimeCardLayerEntries())
	{
		CardInstanceIds.Add(Entry.CardInstanceId);
	}
	TestTrue(TEXT("default hand is interactive"), Anchor->IsFirstPersonCardLayerInteractionEnabled());

	Source->SetRunFirstPersonCardLayerWorldActivitySuppressed(true);
	TestEqual(TEXT("visible entries remain"), Anchor->GetRuntimeCardLayerEntries().Num(), EntryCount);
	TestEqual(TEXT("runtime source identity remains"), Anchor->GetRuntimeCardLayerSourceId(), SourceId);
	for (int32 Index = 0; Index < CardInstanceIds.Num(); ++Index)
	{
		TestEqual(
			*FString::Printf(TEXT("card instance %d remains"), Index),
			Anchor->GetRuntimeCardLayerEntries()[Index].CardInstanceId,
			CardInstanceIds[Index]);
	}
	TestTrue(TEXT("same card layer widget remains"),
		Anchor->GetAutomationTestViewForTest().CardLayerWidget == Layer.Get());
	TestEqual(TEXT("no presentation frame is rewritten on enter"), Source->WriteCount, WriteCount);
	TestEqual(TEXT("no new transition hint is produced"),
		Source->LastWrittenTransitionHints.Num(), HintCount);
	TestFalse(TEXT("hand interaction is frozen"), Anchor->IsFirstPersonCardLayerInteractionEnabled());
	TestTrue(TEXT("debug reports world activity gate"),
		Source->GetRunFirstPersonCardSourceDebugView().bWorldActivitySuppressed);

	FWacomFirstPersonCardLayerTestAccess::TickWorldActivitySuppression(
		*Layer,
		0.09f);
	const FVector2D HalfwayTranslation =
		FWacomFirstPersonCardLayerTestAccess::
			WorldActivitySuppressionRenderTranslation(*Layer);
	const float HalfwayOpacity =
		FWacomFirstPersonCardLayerTestAccess::
			WorldActivitySuppressionRenderOpacity(*Layer);
	TestTrue(TEXT("halfway suppression alpha follows smooth midpoint"),
		FMath::IsNearlyEqual(
			Layer->GetWorldActivitySuppressionAlpha(),
			0.5f,
			0.001f));
	TestTrue(TEXT("halfway hand moves down by 0.21 viewport height"),
		FMath::IsNearlyEqual(
			HalfwayTranslation.Y,
			1080.0f * 0.21f,
			0.1f));
	TestTrue(TEXT("halfway hand fades to half opacity"),
		FMath::IsNearlyEqual(
			HalfwayOpacity,
			0.5f,
			0.001f));
	FWacomFirstPersonCardLayerTestAccess::TickWorldActivitySuppression(
		*Layer,
		0.09f);
	TestTrue(TEXT("suppression settles fully hidden"),
		FMath::IsNearlyEqual(
			Layer->GetWorldActivitySuppressionAlpha(),
			1.0f));

	Source->SetRunFirstPersonCardLayerWorldActivitySuppressed(false);
	TestEqual(TEXT("visible entries still remain"), Anchor->GetRuntimeCardLayerEntries().Num(), EntryCount);
	TestEqual(TEXT("no presentation frame is rewritten on exit"), Source->WriteCount, WriteCount);
	TestTrue(TEXT("hand interaction is restored"), Anchor->IsFirstPersonCardLayerInteractionEnabled());
	FWacomFirstPersonCardLayerTestAccess::TickWorldActivitySuppression(
		*Layer,
		0.18f);
	TestTrue(TEXT("same layer restores to original position"),
		FWacomFirstPersonCardLayerTestAccess::
			WorldActivitySuppressionRenderTranslation(*Layer).IsNearlyZero());
	TestTrue(TEXT("same layer restores full opacity"),
		FMath::IsNearlyEqual(
			FWacomFirstPersonCardLayerTestAccess::
				WorldActivitySuppressionRenderOpacity(*Layer),
			1.0f));
	TestEqual(TEXT("restore does not create enter hints"),
		Source->LastWrittenTransitionHints.Num(), HintCount);

	Source->SetRunFirstPersonCardLayerWorldActivitySuppressed(true);
	FWacomFirstPersonCardLayerTestAccess::TickWorldActivitySuppression(
		*Layer,
		0.09f);
	Source->SetRunFirstPersonCardLayerWorldActivitySuppressed(
		false,
		/*bAnimate*/ false);
	TestTrue(TEXT("shutdown-style restore immediately resets suppression"),
		FMath::IsNearlyZero(
			Layer->GetWorldActivitySuppressionAlpha()));
	TestTrue(TEXT("shutdown-style restore immediately resets visual position"),
		FWacomFirstPersonCardLayerTestAccess::
			WorldActivitySuppressionRenderTranslation(*Layer).IsNearlyZero());
	TestTrue(TEXT("shutdown-style restore immediately resets opacity"),
		FMath::IsNearlyEqual(
			FWacomFirstPersonCardLayerTestAccess::
				WorldActivitySuppressionRenderOpacity(*Layer),
			1.0f));
	return true;
}
