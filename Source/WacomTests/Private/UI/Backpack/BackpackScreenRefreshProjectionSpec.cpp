// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Fixtures/WacomRunExplorationFixture.h"

#include "../BackpackScreenTestAccess.h"
#include "BackpackScreenSpecFixture.h"

#include "Blueprint/WidgetTree.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Card/WacomCardEffectBadgeWidget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardDetailPlainTextRenderer.h"
#include "UI/Card/WacomCardDetailSectionWidget.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomCardView.h"
#include "UI/CardViewTestAccess.h"
#include "UI/CardViewSpecReceiver.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/Image.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackCardDetailController.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"

#include "UObject/StrongObjectPtr.h"

using namespace WacomBackpackScreenSpecFixture;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenRefreshReusesCardWidgetsSpec,
	"Wacom.UI.Backpack.BackpackScreenRefreshReusesCardWidgetsForEquivalentSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenRefreshReusesCardWidgetsSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Capacity = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Capacity"), 4);
	UCardDefinition* BattleCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Battle"));
	UCardDefinition* FluxCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Flux"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { Capacity, BattleCard });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());
	Run->AcquireCardToRun(FluxCard);

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* InitialBattle = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0);
	UWacomDeckCardWidget* InitialFlux = FWacomBackpackScreenTestAccess::FluxContentCard(*Screen, 0);
	TestNotNull(TEXT("Initial battle widget"), InitialBattle);
	TestNotNull(TEXT("Initial flux widget"), InitialFlux);

	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestEqual(TEXT("Equivalent refresh reuses battle widget"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0), InitialBattle);
	TestEqual(TEXT("Equivalent refresh reuses flux widget"), FWacomBackpackScreenTestAccess::FluxContentCard(*Screen, 0), InitialFlux);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenRefreshDirtyGateSkipsEquivalentListReconcileSpec,
	"Wacom.UI.Backpack.BackpackScreenRefreshDirtyGateSkipsEquivalentListReconcile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenRefreshDirtyGateSkipsEquivalentListReconcileSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Capacity = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Capacity"), 4);
	UCardDefinition* BattleCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Battle"));
	UCardDefinition* FluxCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Flux"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { Capacity, BattleCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());
	Run->AcquireCardToRun(FluxCard);

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* InitialBattle = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0);
	UWacomDeckCardWidget* InitialFlux = FWacomBackpackScreenTestAccess::FluxContentCard(*Screen, 0);
	TestNotNull(TEXT("Initial battle widget"), InitialBattle);
	TestNotNull(TEXT("Initial flux widget"), InitialFlux);
	const int32 BaselineApplyCount = FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen);
	const int32 BaselineSkipCount = FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen);
	const int32 BaselineSnapshotBuildCount = FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen);
	const int32 BaselineSnapshotSkipCount = FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(*Screen);
	TestTrue(TEXT("Initial refresh applies list reconcile"), BaselineApplyCount >= 1);
	TestTrue(TEXT("Initial refresh builds storage snapshot"), BaselineSnapshotBuildCount >= 1);

	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestEqual(TEXT("Equivalent revision refresh skips snapshot"), FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(*Screen), BaselineSnapshotSkipCount + 1);
	TestEqual(TEXT("Equivalent revision refresh does not build snapshot"), FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen), BaselineSnapshotBuildCount);
	TestEqual(TEXT("Equivalent revision refresh does not reach signature skip"), FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen), BaselineSkipCount);
	TestEqual(TEXT("Equivalent refresh does not apply again"), FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen), BaselineApplyCount);
	TestEqual(TEXT("Skipped refresh keeps battle widget"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0), InitialBattle);
	TestEqual(TEXT("Skipped refresh keeps flux widget"), FWacomBackpackScreenTestAccess::FluxContentCard(*Screen, 0), InitialFlux);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenRefreshCreatesAndRemovesOnlyChangedCardWidgetsSpec,
	"Wacom.UI.Backpack.BackpackScreenRefreshCreatesAndRemovesOnlyChangedCardWidgets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenRefreshCreatesAndRemovesOnlyChangedCardWidgetsSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Capacity = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Capacity"), 4);
	UCardDefinition* BattleCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Battle"));
	UCardDefinition* NewFluxCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.NewFlux"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { Capacity, BattleCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* InitialBattle = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0);
	UWacomDeckCardWidget* InitialFlux = FWacomBackpackScreenTestAccess::FluxContentCard(*Screen, 0);
	TestNotNull(TEXT("Initial battle widget"), InitialBattle);
	TestNotNull(TEXT("Initial capacity card is visible as flux content"), InitialFlux);

	Run->AcquireCardToRun(NewFluxCard);
	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestEqual(TEXT("Existing battle widget stays reused"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0), InitialBattle);
	TestEqual(TEXT("Existing flux widget stays reused"), FWacomBackpackScreenTestAccess::FluxContentCard(*Screen, 0), InitialFlux);
	TestNotNull(TEXT("New flux widget created"), FWacomBackpackScreenTestAccess::FluxContentCard(*Screen, 1));

	const FGuid BattleInstanceId = Run->GetBattleDeck().IsValidIndex(0)
		? Run->GetBattleDeck()[0].InstanceId
		: FGuid();
	TestTrue(TEXT("Battle instance valid"), BattleInstanceId.IsValid());
	TestTrue(TEXT("Move battle card to backpack"), Run->MoveInstance(BattleInstanceId, EZoneKind::Backpack, FGuid()));
	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestTrue(TEXT("Moved battle widget no longer remains in battle list"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0) != InitialBattle);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenRefreshDirtyGateRefreshesMovedCardsSpec,
	"Wacom.UI.Backpack.BackpackScreenRefreshDirtyGateRefreshesMovedCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenRefreshDirtyGateRefreshesMovedCardsSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Capacity = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Move.Capacity"), 4);
	UCardDefinition* BattleCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Move.Battle"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { Capacity, BattleCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* InitialBattle = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0);
	TestNotNull(TEXT("Initial battle widget"), InitialBattle);
	const int32 BaselineApplyCount = FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen);
	const int32 BaselineSkipCount = FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen);
	const int32 BaselineSnapshotBuildCount = FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen);
	TestTrue(TEXT("Initial refresh applies list reconcile"), BaselineApplyCount >= 1);
	TestTrue(TEXT("Initial refresh builds storage snapshot"), BaselineSnapshotBuildCount >= 1);

	const FGuid BattleInstanceId = Run->GetBattleDeck().IsValidIndex(0)
		? Run->GetBattleDeck()[0].InstanceId
		: FGuid();
	TestTrue(TEXT("Move battle card to backpack"), Run->MoveInstance(BattleInstanceId, EZoneKind::Backpack, FGuid()));
	FWacomBackpackScreenTestAccess::Refresh(*Screen);

	TestEqual(TEXT("Moved card refresh builds new storage snapshot"), FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen), BaselineSnapshotBuildCount + 1);
	TestEqual(TEXT("Moved card refresh applies list reconcile"), FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen), BaselineApplyCount + 1);
	TestEqual(TEXT("Moved card refresh does not skip"), FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen), BaselineSkipCount);
	TestTrue(TEXT("Moved battle widget no longer remains in battle list"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0) != InitialBattle);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenProjectedDuplicateCardsDoNotShareWidgetSpec,
	"Wacom.UI.Backpack.BackpackScreenProjectedAndPhysicalDuplicateCardsDoNotShareWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenProjectedDuplicateCardsDoNotShareWidgetSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* TypeB = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.TypeB"), 3, true);
	UCardDefinition* Content = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Content"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { TypeB, Content });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());

	const FGuid OwnerId = Run->GetBackpack().IsValidIndex(0) ? Run->GetBackpack()[0].InstanceId : FGuid();
	const FGuid ContentId = Run->GetBattleDeck().IsValidIndex(0) ? Run->GetBattleDeck()[0].InstanceId : FGuid();
	TestTrue(TEXT("Owner id valid"), OwnerId.IsValid());
	TestTrue(TEXT("Content id valid"), ContentId.IsValid());
	TestTrue(TEXT("Move content to special"), Run->MoveInstance(ContentId, EZoneKind::SpecialZone, OwnerId));
	TestTrue(TEXT("Move owner to battle"), Run->MoveInstance(OwnerId, EZoneKind::BattleDeck, FGuid()));
	TestTrue(TEXT("Enable content projection"), Run->SetSpecialZoneCardBattleEnabled(ContentId, true));

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* PhysicalOwnerWidget = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0);
	UWacomDeckCardWidget* ProjectedContentWidget = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 1);
	UWacomDeckCardWidget* SpecialContentWidget = FWacomBackpackScreenTestAccess::SpecialContentCard(*Screen, OwnerId, 0);
	TestNotNull(TEXT("Physical owner widget"), PhysicalOwnerWidget);
	TestNotNull(TEXT("Projected content widget"), ProjectedContentWidget);
	TestNotNull(TEXT("Special content widget"), SpecialContentWidget);
	TestNotEqual(TEXT("Projected card does not share widget with special content"), ProjectedContentWidget, SpecialContentWidget);
	TestEqual(TEXT("Projected widget is marked projected"),
		ProjectedContentWidget ? ProjectedContentWidget->GetBackpackListReuseRole() : EWacomBackpackDeckCardListReuseRole::PhysicalList,
		EWacomBackpackDeckCardListReuseRole::BattleDeckProjected);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenRefreshDirtyGateRefreshesProjectedAndSpecialZoneStateSpec,
	"Wacom.UI.Backpack.BackpackScreenRefreshDirtyGateRefreshesProjectedAndSpecialZoneState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenRefreshDirtyGateRefreshesProjectedAndSpecialZoneStateSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* TypeB = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.TypeB"), 3, true);
	UCardDefinition* Content = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Content"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { TypeB, Content });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());

	const FGuid OwnerId = Run->GetBackpack().IsValidIndex(0) ? Run->GetBackpack()[0].InstanceId : FGuid();
	const FGuid ContentId = Run->GetBattleDeck().IsValidIndex(0) ? Run->GetBattleDeck()[0].InstanceId : FGuid();
	TestTrue(TEXT("Move content to special"), Run->MoveInstance(ContentId, EZoneKind::SpecialZone, OwnerId));
	TestTrue(TEXT("Move owner to battle"), Run->MoveInstance(OwnerId, EZoneKind::BattleDeck, FGuid()));

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	const int32 BaselineApplyCount = FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen);
	const int32 BaselineSkipCount = FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen);
	const int32 BaselineSnapshotBuildCount = FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen);
	TestTrue(TEXT("Initial refresh applies list reconcile"), BaselineApplyCount >= 1);
	TestNull(TEXT("No projected content before enable"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 1));
	UWacomDeckCardWidget* InitialSpecialContent = FWacomBackpackScreenTestAccess::SpecialContentCard(*Screen, OwnerId, 0);
	TestNotNull(TEXT("Initial special content"), InitialSpecialContent);

	TestTrue(TEXT("Enable content projection"), Run->SetSpecialZoneCardBattleEnabled(ContentId, true));
	FWacomBackpackScreenTestAccess::Refresh(*Screen);

	TestEqual(TEXT("Projection change builds new storage snapshot"), FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen), BaselineSnapshotBuildCount + 1);

	TestEqual(TEXT("Projection change applies list reconcile"), FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen), BaselineApplyCount + 1);
	TestEqual(TEXT("Projection change does not skip"), FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen), BaselineSkipCount);
	TestNotNull(TEXT("Projected content appears"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 1));
	TestEqual(TEXT("Special content widget reused while projection state refreshes"),
		FWacomBackpackScreenTestAccess::SpecialContentCard(*Screen, OwnerId, 0),
		InitialSpecialContent);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenReusableCardWidgetsResetStateSpec,
	"Wacom.UI.Backpack.BackpackScreenReusableCardWidgetsResetProjectedBadgeAndToggleState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenReusableCardWidgetsResetStateSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* TypeB = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.TypeB"), 3, true);
	UCardDefinition* Content = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Content"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { TypeB, Content });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());

	const FGuid OwnerId = Run->GetBackpack().IsValidIndex(0) ? Run->GetBackpack()[0].InstanceId : FGuid();
	const FGuid ContentId = Run->GetBattleDeck().IsValidIndex(0) ? Run->GetBattleDeck()[0].InstanceId : FGuid();
	TestTrue(TEXT("Move content to special"), Run->MoveInstance(ContentId, EZoneKind::SpecialZone, OwnerId));
	TestTrue(TEXT("Move owner to battle"), Run->MoveInstance(OwnerId, EZoneKind::BattleDeck, FGuid()));
	TestTrue(TEXT("Enable content projection"), Run->SetSpecialZoneCardBattleEnabled(ContentId, true));

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* ProjectedWidget = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 1);
	TestNotNull(TEXT("Projected widget exists"), ProjectedWidget);
	if (!ProjectedWidget)
	{
		return false;
	}
	TestTrue(TEXT("Projected badge visible before disabling"), ProjectedWidget->IsProjectedFromBadgeVisible());

	TestTrue(TEXT("Disable content projection"), Run->SetSpecialZoneCardBattleEnabled(ContentId, false));
	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestNull(TEXT("Projected widget removed from battle list"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 1));

	UWacomDeckCardWidget* SpecialContentWidget = FWacomBackpackScreenTestAccess::SpecialContentCard(*Screen, OwnerId, 0);
	TestNotNull(TEXT("Special content widget remains"), SpecialContentWidget);
	if (SpecialContentWidget)
	{
		TestFalse(TEXT("Special content has no projected badge after refresh"), SpecialContentWidget->IsProjectedFromBadgeVisible());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackSpecialZoneRefreshReusesWidgetsSpec,
	"Wacom.UI.Backpack.BackpackWorkspaceRefreshReusesSpecialOwnerAndContentWidgets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackSpecialZoneRefreshReusesWidgetsSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* TypeB = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.TypeB"), 3, true);
	UCardDefinition* Content = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Content"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { TypeB, Content });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());

	const FGuid OwnerId = Run->GetBackpack().IsValidIndex(0) ? Run->GetBackpack()[0].InstanceId : FGuid();
	const FGuid ContentId = Run->GetBattleDeck().IsValidIndex(0) ? Run->GetBattleDeck()[0].InstanceId : FGuid();
	TestTrue(TEXT("Move content to special"), Run->MoveInstance(ContentId, EZoneKind::SpecialZone, OwnerId));

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* InitialOwner = FWacomBackpackScreenTestAccess::SpecialOwnerCard(*Screen, OwnerId);
	UWacomDeckCardWidget* InitialContent = FWacomBackpackScreenTestAccess::SpecialContentCard(*Screen, OwnerId, 0);
	TestNotNull(TEXT("Initial owner"), InitialOwner);
	TestNotNull(TEXT("Initial content"), InitialContent);

	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestEqual(TEXT("Owner card widget reused"), FWacomBackpackScreenTestAccess::SpecialOwnerCard(*Screen, OwnerId), InitialOwner);
	TestEqual(TEXT("Content card widget reused"), FWacomBackpackScreenTestAccess::SpecialContentCard(*Screen, OwnerId, 0), InitialContent);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenRemovesHoveredSourceAndHidesDetailSpec,
	"Wacom.UI.Backpack.BackpackScreenRemovesHoveredSourceAndHidesDetail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenRemovesHoveredSourceAndHidesDetailSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Capacity = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Capacity"), 4);
	UCardDefinition* BattleCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Battle"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { Capacity, BattleCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	UWacomDeckCardWidget* BattleWidget = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0);
	TestNotNull(TEXT("Battle widget"), BattleWidget);
	if (!BattleWidget)
	{
		return false;
	}
	TestTrue(TEXT("Show detail for battle widget"), FWacomBackpackScreenTestAccess::ShowDetailForCardWidget(*Screen, BattleWidget));
	TestTrue(TEXT("Detail visible before remove"), FWacomBackpackScreenTestAccess::IsDetailVisible(*Screen));
	TestTrue(TEXT("Stable Workspace geometry change is accepted while detail is visible"),
		FWacomBackpackScreenTestAccess::ApplyStableWorkspaceGeometry(
			*Screen, FVector2D(1600.0f, 900.0f)));
	TestTrue(TEXT("Same-source detail remains visible after geometry-triggered reconcile"),
		FWacomBackpackScreenTestAccess::IsDetailVisible(*Screen));
	TestEqual(TEXT("Geometry-triggered reposition keeps the same detail payload"),
		FWacomBackpackScreenTestAccess::DetailNameText(*Screen).ToString(),
		BattleCard->DisplayName.ToString());

	const FGuid BattleId = Run->GetBattleDeck().IsValidIndex(0) ? Run->GetBattleDeck()[0].InstanceId : FGuid();
	TestTrue(TEXT("Move battle card to backpack"), Run->MoveInstance(BattleId, EZoneKind::Backpack, FGuid()));
	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestFalse(TEXT("Detail hidden after hovered source removed"), FWacomBackpackScreenTestAccess::IsDetailVisible(*Screen));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackScreenRefreshDirtyGateResetsAfterMissingRunOrWidgetRebuildSpec,
	"Wacom.UI.Backpack.BackpackScreenRefreshDirtyGateResetsAfterMissingRunOrWidgetRebuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackScreenRefreshDirtyGateResetsAfterMissingRunOrWidgetRebuildSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Capacity = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Reset.Capacity"), 4);
	UCardDefinition* BattleCard = MakeBackpackUiCardForTest(Outer, TEXT("Backpack.UI.Dirty.Reset.Battle"));
	UCharacterDefinition* Character = MakeBackpackUiCharacterForTest(Outer, { Capacity, BattleCard });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());

	TStrongObjectPtr<UWacomBackpackScreen> Screen(MakeBackpackUiScreenForTest(GetTransientPackage(), Run.Get()));
	const int32 BaselineApplyCount = FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen);
	const int32 BaselineSkipCount = FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen);
	const int32 BaselineSnapshotBuildCount = FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen);
	const int32 BaselineSnapshotSkipCount = FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(*Screen);
	TestTrue(TEXT("Initial refresh applies list reconcile"), BaselineApplyCount >= 1);
	UWacomDeckCardWidget* InitialBattle = FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0);
	TestNotNull(TEXT("Initial battle widget"), InitialBattle);

	FWacomBackpackScreenTestAccess::SetRunSession(*Screen, nullptr);
	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestNull(TEXT("Missing run clears battle list"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0));

	FWacomBackpackScreenTestAccess::SetRunSession(*Screen, Run.Get());
	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestEqual(TEXT("Restored run builds snapshot after dirty gate reset"), FWacomBackpackScreenTestAccess::SnapshotBuildCount(*Screen), BaselineSnapshotBuildCount + 1);
	TestEqual(TEXT("Restored run applies after dirty gate reset"), FWacomBackpackScreenTestAccess::RefreshApplyCount(*Screen), BaselineApplyCount + 1);
	TestNotNull(TEXT("Restored run rebuilds battle list"), FWacomBackpackScreenTestAccess::BattleDeckCard(*Screen, 0));

	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestEqual(TEXT("Equivalent restored refresh skips snapshot"), FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(*Screen), BaselineSnapshotSkipCount + 1);
	TestEqual(TEXT("Equivalent restored refresh does not reach signature skip"), FWacomBackpackScreenTestAccess::RefreshSkipCount(*Screen), BaselineSkipCount);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackSpecialZoneBattleEnabledBadgeSpec,
	"Wacom.UI.Backpack.SpecialZoneBattleEnabledBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackSpecialZoneBattleEnabledBadgeSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.7/R6.10: SpecialZone cards expose an identifiable battle-enabled badge.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();
	Inst.bBattleEnabledInSpecialZone = true;

	Widget->SetCard(Inst, EZoneKind::SpecialZone, FGuid::NewGuid());
	Widget->TakeWidget();

	TestTrue(TEXT("BattleEnabledBadge visible for selected SpecialZone card"), Widget->IsBattleEnabledBadgeVisible());

	Inst.bBattleEnabledInSpecialZone = false;
	Widget->SetCard(Inst, EZoneKind::SpecialZone, FGuid::NewGuid());
	TestFalse(TEXT("BattleEnabledBadge collapsed for unselected SpecialZone card"), Widget->IsBattleEnabledBadgeVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackProjectedFromBadgeSpec,
	"Wacom.UI.Backpack.ProjectedFromBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackProjectedFromBadgeSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.10: BattleDeck projection keeps a visible source-owner badge.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	Widget->TakeWidget();

	TestFalse(TEXT("ProjectedFromBadge hidden by default"), Widget->IsProjectedFromBadgeVisible());

	const FText SourceText = FText::FromString(TEXT("来自 蛛茧绒囊"));
	Widget->SetProjectedFromBadgeText(SourceText);


	TestTrue(TEXT("ProjectedFromBadge visible when text is set"), Widget->IsProjectedFromBadgeVisible());
	TestEqual(TEXT("ProjectedFromBadge text preserved"), Widget->GetProjectedFromBadgeText().ToString(), SourceText.ToString());

	Widget->SetProjectedFromBadgeText(FText::GetEmpty());
	TestFalse(TEXT("ProjectedFromBadge collapsed when text is cleared"), Widget->IsProjectedFromBadgeVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackProjectedFromBadgePresenterSpec,
	"Wacom.UI.Backpack.ProjectedFromBadgePresenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackProjectedFromBadgePresenterSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> OwnerCard(NewObject<UCardDefinition>());
	OwnerCard->DisplayName = FText::FromString(TEXT("蛛茧绒囊"));

	FRunBackpackStorageSnapshot Snapshot;
	FRunSpecialStorageView SpecialView;
	SpecialView.OwnerCard.Instance.InstanceId = FGuid::NewGuid();
	SpecialView.OwnerCard.Instance.Definition = OwnerCard.Get();
	Snapshot.SpecialZones.Add(SpecialView);

	FRunStorageCardView ProjectedCard;
	ProjectedCard.ZoneOwnerInstanceId = SpecialView.OwnerCard.Instance.InstanceId;

	const FText BadgeText =
		FWacomBackpackWorkspaceSceneBuilder::BuildBattleDeckProjectedBadge(
			ProjectedCard, Snapshot);
	TestEqual(TEXT("Projected badge text uses special zone owner name"), BadgeText.ToString(), TEXT("来自 蛛茧绒囊"));

	ProjectedCard.ZoneOwnerInstanceId = FGuid::NewGuid();
	TestTrue(
		TEXT("Projected badge text is empty when owner cannot be found"),
		FWacomBackpackWorkspaceSceneBuilder::BuildBattleDeckProjectedBadge(
			ProjectedCard, Snapshot).IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackBattleEnabledToggleRequestSpec,
	"Wacom.UI.Backpack.BattleEnabledToggleRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackBattleEnabledToggleRequestSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.11: right-click toggle path emits one request for the card instance.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();

	FRunStorageCardView View;
	View.Instance = Inst;
	View.PhysicalZone = EZoneKind::SpecialZone;
	View.ZoneOwnerInstanceId = FGuid::NewGuid();
	Widget->SetStorageCardView(View);

	int32 ToggleCount = 0;
	FGuid LastToggledId;
	Widget->OnBattleEnabledToggleRequestedNative.AddLambda(
		[&ToggleCount, &LastToggledId](FGuid InstanceId)
		{
			++ToggleCount;
			LastToggledId = InstanceId;
		});

	TestFalse(TEXT("Toggle disabled by default"), Widget->RequestBattleEnabledToggle());
	TestEqual(TEXT("No request emitted while disabled"), ToggleCount, 0);

	View.bCanToggleBattleEnabledInSpecialZone = true;
	Widget->SetStorageCardView(View);
	TestTrue(TEXT("Toggle request accepted when enabled"), Widget->RequestBattleEnabledToggle());
	TestEqual(TEXT("One toggle request emitted"), ToggleCount, 1);
	TestEqual(TEXT("Toggle request carries instance id"), LastToggledId, Inst.InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackBurdenZoneTitleAndCardOrderSpec,
	"Wacom.UI.Backpack.BurdenCardOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackBurdenZoneTitleAndCardOrderSpec::RunTest(const FString& /*Parameters*/)
{
	TArray<FCardInstance> BurdenCards;
	TArray<TStrongObjectPtr<UCardDefinition>> CardDefinitions;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
		FCardInstance Inst;
		Inst.InstanceId = FGuid::NewGuid();
		Inst.Definition = Card.Get();

		CardDefinitions.Add(MoveTemp(Card));
		BurdenCards.Add(Inst);
	}

	for (int32 Index = 0; Index < BurdenCards.Num(); ++Index)
	{
		TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
		Widget->SetCard(BurdenCards[Index], EZoneKind::BurdenZone, FGuid::NewGuid());

		TestEqual(TEXT("Burden card order preserves instance id"), Widget->GetCardInstanceId(), BurdenCards[Index].InstanceId);
		TestEqual(TEXT("Burden card order preserves definition"), Widget->GetCard(), BurdenCards[Index].Definition.Get());
		TestTrue(TEXT("Burden card source zone is BurdenZone"), Widget->GetFromZone() == EZoneKind::BurdenZone);
		TestFalse(TEXT("Burden card owner id normalized to invalid"), Widget->GetFromZoneOwnerInstanceId().IsValid());
	}

	return true;
}
