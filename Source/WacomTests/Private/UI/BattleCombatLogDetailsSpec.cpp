// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "CommonInputBaseTypes.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Enemies/EnemyDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "UI/Battle/BattleCombatActivityRowWidget.h"
#include "UI/Battle/BattleCombatLogTurnDividerWidget.h"
#include "UI/Battle/WacomBattleCombatActivityStyle.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattleCombatLogDetailsScreen.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UObject/StrongObjectPtr.h"
#include "WidgetBlueprint.h"

namespace WacomBattleCombatLogDetailsSpec
{
	FWacomInitializedBattleSession CreateSession(FWacomBattleFixture& Fixture)
	{
		UCharacterDefinition* Character = Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0)
			});
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(20, 10, 0);
		return Fixture.CreateInitializedSession(Character, Enemy, 1);
	}

	FWacomBattleCombatLogTurnSectionView MakeCompletedSection()
	{
		FWacomBattleCombatLogTurnSectionView Section;
		Section.TurnNumber = 2;
		Section.bCompleted = true;
		FWacomBattleCombatActivityGroupView& Group = Section.Groups.AddDefaulted_GetRef();
		Group.TurnNumber = 2;
		Group.RootAction.RowKind = EWacomBattleCombatActivityRowKind::RootAction;
		Group.RootAction.MessageText = FText::FromString(TEXT("泛滥"));
		Group.RootAction.IconKey = TEXT("Player");
		for (int32 Index = 0; Index < 2; ++Index)
		{
			FWacomBattleCombatActivityRowView& Result = Group.ResultRows.AddDefaulted_GetRef();
			Result.RowKind = EWacomBattleCombatActivityRowKind::Result;
			Result.MessageText = FText::FromString(FString::Printf(TEXT("结果%d"), Index + 1));
			Result.EventSequence = Index + 1;
		}
		return Section;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogDetailsHistorySpec,
	"Wacom.UI.Battle.CombatLogDetails.History.TurnSectionsFollowResolvedCommands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogDetailsHistorySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	const FWacomInitializedBattleSession Initialized =
		WacomBattleCombatLogDetailsSpec::CreateSession(Fixture);
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetInjectedBattleSession(Initialized.Session);
	HUD->SetBattleInputReadyForTest(true);
	HUD->SetUIStateForTest(EBattleUIState::Idle);

	const FBattleSnapshot BeforeWait = Initialized.Session->BuildSnapshot();
	const FBattleResolution WaitResolution =
		Initialized.Session->ResolveCommand(FBattleCommand::MakeWait());
	TestTrue(TEXT("Wait resolves before history projection"), WaitResolution.IsOk());
	HUD->ApplyCommandResolutionForTest(
		UWacomBattleCombatLogBuilder::BuildWaitCommandContext(BeforeWait),
		BeforeWait,
		WaitResolution);

	TArray<FWacomBattleCombatLogTurnSectionView> History =
		HUD->GetBattleCombatLogDetailsHistoryForTest();
	TestEqual(TEXT("First resolved command creates turn one"), History.Num(), 1);
	if (History.IsEmpty())
	{
		return false;
	}
	TestEqual(TEXT("First section is turn one"), History[0].TurnNumber, 1);
	TestFalse(TEXT("Wait does not complete the turn"), History[0].bCompleted);
	TestEqual(TEXT("Wait is retained as one root action"), History[0].Groups.Num(), 1);

	const FBattleSnapshot BeforeEndTurn = Initialized.Session->BuildSnapshot();
	const FBattleResolution EndTurnResolution =
		Initialized.Session->ResolveCommand(FBattleCommand::MakeEndTurn());
	TestTrue(TEXT("EndTurn resolves before history projection"), EndTurnResolution.IsOk());
	HUD->ApplyCommandResolutionForTest(
		UWacomBattleCombatLogBuilder::BuildEndTurnCommandContext(BeforeEndTurn),
		BeforeEndTurn,
		EndTurnResolution);

	History = HUD->GetBattleCombatLogDetailsHistoryForTest();
	TestEqual(TEXT("EndTurn closes turn one and opens turn two"), History.Num(), 2);
	if (History.Num() == 2)
	{
		TestTrue(TEXT("Turn one is marked complete"), History[0].bCompleted);
		TestTrue(TEXT("Enemy action is retained in turn one"), History[0].Groups.Num() >= 2);
		TestEqual(TEXT("Next empty section is turn two"), History[1].TurnNumber, 2);
		TestFalse(TEXT("New turn starts incomplete"), History[1].bCompleted);
		TestTrue(TEXT("New turn starts without fabricated actions"), History[1].Groups.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogDetailsScreenSpec,
	"Wacom.UI.Battle.CombatLogDetails.Widget.ConciseAndDetailedRows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogDetailsScreenSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCombatLogDetailsScreenTest> Screen(
		NewObject<UWacomBattleCombatLogDetailsScreenTest>());
	TSharedRef<SWidget> SlateWidget = Screen->TakeWidget();
	const TArray<FWacomBattleCombatLogTurnSectionView> History = {
		WacomBattleCombatLogDetailsSpec::MakeCompletedSection()
	};

	Screen->SetCombatLogContext(History, false);
	TestFalse(TEXT("Details starts in concise mode"), Screen->IsShowingDetails());
	TestEqual(TEXT("Concise mode renders start, root and end"), Screen->GetRenderedEntryCount(), 3);

	int32 ModeChangeCount = 0;
	Screen->OnDetailsModeChangedNative().AddLambda(
		[&ModeChangeCount](bool bShowDetails)
		{
			if (bShowDetails)
			{
				++ModeChangeCount;
			}
		});
	Screen->SetDetailsCheckedForTest(true);
	TestTrue(TEXT("Toggle enables detailed mode"), Screen->IsShowingDetails());
	TestEqual(TEXT("Detailed mode expands every result row"), Screen->GetRenderedEntryCount(), 5);
	TestEqual(TEXT("Details preference changes exactly once"), ModeChangeCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogDetailsInputSpec,
	"Wacom.UI.Battle.CombatLogDetails.Input.AllNoCaptureAndCloseRoutes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogDetailsInputSpec::RunTest(const FString& /*Parameters*/)
{
	auto VerifyCloseRoute = [this](const TCHAR* Label, TFunction<void(UWacomBattleCombatLogDetailsScreenTest&)> Invoke)
	{
		TStrongObjectPtr<UWacomBattleCombatLogDetailsScreenTest> Screen(
			NewObject<UWacomBattleCombatLogDetailsScreenTest>());
		TSharedRef<SWidget> SlateWidget = Screen->TakeWidget();
		int32 CloseCount = 0;
		Screen->OnSecondaryPanelClosedNative().AddLambda([&CloseCount]() { ++CloseCount; });
		Invoke(*Screen);
		TestEqual(FString::Printf(TEXT("%s closes once"), Label), CloseCount, 1);
		Screen->RequestClose();
		TestEqual(FString::Printf(TEXT("%s remains idempotent"), Label), CloseCount, 1);
	};

	TStrongObjectPtr<UWacomBattleCombatLogDetailsScreenTest> ConfigScreen(
		NewObject<UWacomBattleCombatLogDetailsScreenTest>());
	TSharedRef<SWidget> ConfigSlateWidget = ConfigScreen->TakeWidget();
	const TOptional<FUIInputConfig> InputConfig = ConfigScreen->GetDesiredInputConfigForTest();
	TestTrue(TEXT("Secondary panel supplies an input config"), InputConfig.IsSet());
	if (InputConfig.IsSet())
	{
		TestEqual(TEXT("Secondary panel keeps game and UI input"),
			static_cast<int32>(InputConfig->GetInputMode()),
			static_cast<int32>(ECommonInputMode::All));
		TestEqual(TEXT("Secondary panel does not capture the mouse"),
			static_cast<int32>(InputConfig->GetMouseCaptureMode()),
			static_cast<int32>(EMouseCaptureMode::NoCapture));
	}
	int32 LeftClickCloseCount = 0;
	ConfigScreen->OnSecondaryPanelClosedNative().AddLambda(
		[&LeftClickCloseCount]() { ++LeftClickCloseCount; });
	ConfigScreen->PressMouseButtonForTest(EKeys::LeftMouseButton);
	TestEqual(TEXT("Panel-internal left click does not close through the base screen"), LeftClickCloseCount, 0);

	VerifyCloseRoute(TEXT("Escape"), [](UWacomBattleCombatLogDetailsScreenTest& Screen)
	{
		Screen.PressKeyForTest(EKeys::Escape);
	});
	VerifyCloseRoute(TEXT("Gamepad B"), [](UWacomBattleCombatLogDetailsScreenTest& Screen)
	{
		Screen.PressKeyForTest(EKeys::Gamepad_FaceButton_Right);
	});
	VerifyCloseRoute(TEXT("Right mouse"), [](UWacomBattleCombatLogDetailsScreenTest& Screen)
	{
		Screen.PressMouseButtonForTest(EKeys::RightMouseButton);
	});
	VerifyCloseRoute(TEXT("Backdrop"), [](UWacomBattleCombatLogDetailsScreenTest& Screen)
	{
		Screen.ClickBackdropForTest();
	});
	VerifyCloseRoute(TEXT("Close button"), [](UWacomBattleCombatLogDetailsScreenTest& Screen)
	{
		Screen.ClickCloseForTest();
	});
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogDetailsCommandGateSpec,
	"Wacom.UI.Battle.CombatLogDetails.Input.CommandGateLeavesPresentationIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogDetailsCommandGateSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	const FWacomInitializedBattleSession Initialized =
		WacomBattleCombatLogDetailsSpec::CreateSession(Fixture);
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetInjectedBattleSession(Initialized.Session);
	HUD->SetBattleInputReadyForTest(true);
	HUD->SetUIStateForTest(EBattleUIState::Idle);

	TestTrue(TEXT("Baseline accepts player commands"), HUD->CanSubmitPlayerActionCommand());
	HUD->SetSecondaryPanelOpenForTest(true);
	TestTrue(TEXT("Runtime reports the active secondary panel"), HUD->IsSecondaryPanelOpenForTest());
	TestFalse(TEXT("Secondary panel blocks Battle commands"), HUD->CanSubmitPlayerActionCommand());
	TestFalse(TEXT("Secondary panel disables first-person hand interaction"),
		HUD->ShouldEnableFirstPersonBattleHandInteractionForTest());
	TestFalse(TEXT("Opening the secondary panel does not fabricate presentation work"),
		HUD->IsPresentationPlanActiveForTest());

	HUD->SetSecondaryPanelOpenForTest(false);
	TestFalse(TEXT("Closing clears the secondary panel gate"), HUD->IsSecondaryPanelOpenForTest());
	TestTrue(TEXT("Closing restores player command submission"), HUD->CanSubmitPlayerActionCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogDetailsFormalAssetSpec,
	"Wacom.UI.Battle.CombatLogDetails.Assets.FormalBuilderContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogDetailsFormalAssetSpec::RunTest(const FString& /*Parameters*/)
{
	UWidgetBlueprint* DetailsBlueprint = Cast<UWidgetBlueprint>(StaticLoadObject(
		UWidgetBlueprint::StaticClass(), nullptr,
		TEXT("/Game/Wacom/UI/Battle/CombatLog/WBP_BattleCombatLogDetailsScreen.WBP_BattleCombatLogDetailsScreen")));
	UWidgetBlueprint* DividerBlueprint = Cast<UWidgetBlueprint>(StaticLoadObject(
		UWidgetBlueprint::StaticClass(), nullptr,
		TEXT("/Game/Wacom/UI/Battle/CombatLog/WBP_BattleCombatLogTurnDivider.WBP_BattleCombatLogTurnDivider")));
	TestNotNull(TEXT("Formal details Screen exists"), DetailsBlueprint);
	TestNotNull(TEXT("Formal turn divider exists"), DividerBlueprint);
	if (!DetailsBlueprint || !DividerBlueprint)
	{
		return false;
	}

	TestTrue(TEXT("Details parent contract"), DetailsBlueprint->ParentClass
		&& DetailsBlueprint->ParentClass->IsChildOf(UWacomBattleCombatLogDetailsScreen::StaticClass()));
	TestTrue(TEXT("Divider parent contract"), DividerBlueprint->ParentClass
		&& DividerBlueprint->ParentClass->IsChildOf(UBattleCombatLogTurnDividerWidget::StaticClass()));
	TestNotNull(TEXT("Details backdrop binding"), DetailsBlueprint->WidgetTree->FindWidget(TEXT("BackdropButton")));
	TestNotNull(TEXT("Details panel binding"), DetailsBlueprint->WidgetTree->FindWidget(TEXT("PanelRoot")));
	TestNotNull(TEXT("Details close binding"), DetailsBlueprint->WidgetTree->FindWidget(TEXT("CloseButton")));
	TestNotNull(TEXT("Details toggle binding"), DetailsBlueprint->WidgetTree->FindWidget(TEXT("DetailsToggle")));
	TestNotNull(TEXT("Details history list binding"), DetailsBlueprint->WidgetTree->FindWidget(TEXT("HistoryList")));

	const UButton* Backdrop = Cast<UButton>(DetailsBlueprint->WidgetTree->FindWidget(TEXT("BackdropButton")));
	const UButton* Close = Cast<UButton>(DetailsBlueprint->WidgetTree->FindWidget(TEXT("CloseButton")));
	if (Backdrop) { TestFalse(TEXT("Backdrop is not focusable"), Backdrop->GetIsFocusable()); }
	if (Close) { TestFalse(TEXT("Close button is not focusable"), Close->GetIsFocusable()); }

	const UWacomBattleCombatLogDetailsScreen* Defaults = DetailsBlueprint->GeneratedClass
		? Cast<UWacomBattleCombatLogDetailsScreen>(DetailsBlueprint->GeneratedClass->GetDefaultObject())
		: nullptr;
	TestNotNull(TEXT("Details generated defaults"), Defaults);
	if (Defaults)
	{
		TestNotNull(TEXT("Details shares the combat activity Style"), Defaults->GetActivityStyle());
		TestNotNull(TEXT("Details uses the formal activity Row class"), Defaults->GetActivityRowWidgetClass());
		TestTrue(TEXT("Details uses the formal divider class"),
			Defaults->GetTurnDividerWidgetClass() == DividerBlueprint->GeneratedClass);
	}
	return true;
}
