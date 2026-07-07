// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BattleHUDTestHarness.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/BattleCommandBarWidget.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCommandBarAppliesViewDataSpec,
	"Wacom.UI.Battle.CommandBar.AppliesViewData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCommandBarAppliesViewDataSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCommandBarTestProbe> CommandBar(
		NewObject<UWacomBattleCommandBarTestProbe>());
	CommandBar->TakeWidget();

	FWacomBattleCommandBarViewData ViewData;
	ViewData.WaitValueText = FText::FromString(TEXT("Wait Value: 3"));
	ViewData.PendingCommandText = FText::FromString(TEXT("等待排队中"));

	FWacomBattleCommandButtonView WaitView;
	WaitView.CommandId = EWacomBattleCommandId::Wait;
	WaitView.DisplayText = FText::FromString(TEXT("Wait"));
	WaitView.bEnabled = true;
	WaitView.SortOrder = 20;
	ViewData.Commands.Add(WaitView);

	FWacomBattleCommandButtonView EndTurnView;
	EndTurnView.CommandId = EWacomBattleCommandId::EndTurn;
	EndTurnView.DisplayText = FText::FromString(TEXT("End Turn"));
	EndTurnView.bEnabled = false;
	EndTurnView.bPending = true;
	EndTurnView.SortOrder = 10;
	ViewData.Commands.Add(EndTurnView);

	UTexture2D* WaitIcon = NewObject<UTexture2D>(CommandBar.Get());
	UTexture2D* EndTurnIcon = NewObject<UTexture2D>(CommandBar.Get());
	CommandBar->WaitIconBrush.SetResourceObject(WaitIcon);
	CommandBar->EndTurnIconBrush.SetResourceObject(EndTurnIcon);

	CommandBar->SetCommandBarViewData(ViewData);

	TestEqual(TEXT("Wait value text is cached"), CommandBar->GetWaitValueTextForTest().ToString(), FString(TEXT("Wait Value: 3")));
	TestEqual(TEXT("Pending text is cached"), CommandBar->GetPendingCommandTextForTest().ToString(), FString(TEXT("等待排队中")));
	TestEqual(TEXT("Visible command buttons are generated"), CommandBar->GetGeneratedCommandButtons().Num(), 2);
	TestTrue(TEXT("Wait command enabled"), CommandBar->IsWaitCommandEnabledForTest());
	TestFalse(TEXT("EndTurn command disabled"), CommandBar->IsEndTurnCommandEnabledForTest());

	FWacomBattleCommandButtonView FoundEndTurnView;
	TestTrue(TEXT("EndTurn view can be queried"),
		CommandBar->FindCommandButtonView(EWacomBattleCommandId::EndTurn, FoundEndTurnView));
	TestTrue(TEXT("EndTurn view keeps pending marker"), FoundEndTurnView.bPending);
	TestTrue(TEXT("EndTurn authoring icon is applied"), FoundEndTurnView.bHasIconBrush);
	TestTrue(TEXT("EndTurn authoring icon resource is preserved"),
		FoundEndTurnView.IconBrush.GetResourceObject() == EndTurnIcon);

	FWacomBattleCommandButtonView FoundWaitView;
	TestTrue(TEXT("Wait view can be queried"),
		CommandBar->FindCommandButtonView(EWacomBattleCommandId::Wait, FoundWaitView));
	TestTrue(TEXT("Wait authoring icon is applied"), FoundWaitView.bHasIconBrush);
	TestTrue(TEXT("Wait authoring icon resource is preserved"),
		FoundWaitView.IconBrush.GetResourceObject() == WaitIcon);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCommandBarRoutesRequestsThroughHUDSpec,
	"Wacom.UI.Battle.CommandBar.RoutesRequestsThroughHUD",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCommandBarRoutesRequestsThroughHUDSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(nullptr);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UWacomBattleCommandBarTestProbe* CommandBar = Harness->AttachCommandBar();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("CommandBar"), CommandBar))
	{
		return false;
	}

	const int32 WaitValueBefore = Session->BuildSnapshot().CurrentWaitValue;
	TestTrue(TEXT("Wait command starts enabled"), CommandBar->IsWaitCommandEnabledForTest());

	CommandBar->RequestCommand(EWacomBattleCommandId::Wait);

	TestEqual(TEXT("Wait request routes through HUD"),
		Session->BuildSnapshot().CurrentWaitValue,
		WaitValueBefore + 1);
	TestTrue(TEXT("Wait command remains enabled after command refresh"),
		CommandBar->IsWaitCommandEnabledForTest());

	HUD->SetBattleInputReady(false);
	const int32 WaitValueBeforeBlockedRequest = Session->BuildSnapshot().CurrentWaitValue;
	TestFalse(TEXT("Battle input gate disables command bar wait"),
		CommandBar->IsWaitCommandEnabledForTest());

	CommandBar->RequestCommand(EWacomBattleCommandId::Wait);

	TestEqual(TEXT("Disabled command request is ignored"),
		Session->BuildSnapshot().CurrentWaitValue,
		WaitValueBeforeBlockedRequest);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCommandBarUsesAuthoredButtonsSpec,
	"Wacom.UI.Battle.CommandBar.UsesAuthoredButtons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCommandBarUsesAuthoredButtonsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleCommandBarTestProbe> CommandBar(
		NewObject<UWacomBattleCommandBarTestProbe>());
	CommandBar->TakeWidget();
	CommandBar->CreateAuthoredCommandButtonsForTest();

	FWacomBattleCommandBarViewData ViewData;
	FWacomBattleCommandButtonView WaitView;
	WaitView.CommandId = EWacomBattleCommandId::Wait;
	WaitView.DisplayText = FText::FromString(TEXT("等待"));
	WaitView.bEnabled = true;
	WaitView.SortOrder = 10;
	ViewData.Commands.Add(WaitView);

	FWacomBattleCommandButtonView EndTurnView;
	EndTurnView.CommandId = EWacomBattleCommandId::EndTurn;
	EndTurnView.DisplayText = FText::FromString(TEXT("结束"));
	EndTurnView.bEnabled = false;
	EndTurnView.bPending = true;
	EndTurnView.SortOrder = 20;
	ViewData.Commands.Add(EndTurnView);

	CommandBar->SetCommandBarViewData(ViewData);

	TestTrue(TEXT("Authored button mode is active"), CommandBar->UsesAuthoredCommandButtons());
	TestEqual(TEXT("No dynamic buttons are generated when authored buttons exist"),
		CommandBar->GetGeneratedCommandButtons().Num(),
		0);
	TestNotNull(TEXT("Authored wait button exists"), CommandBar->GetAuthoredWaitButtonForTest());
	TestNotNull(TEXT("Authored end turn button exists"), CommandBar->GetAuthoredEndTurnButtonForTest());
	if (!CommandBar->GetAuthoredWaitButtonForTest() || !CommandBar->GetAuthoredEndTurnButtonForTest())
	{
		return false;
	}

	TestEqual(TEXT("Wait view is applied to authored wait button"),
		CommandBar->GetAuthoredWaitButtonForTest()->GetCommandView().DisplayText.ToString(),
		FString(TEXT("等待")));
	TestTrue(TEXT("Authored wait button is enabled"),
		CommandBar->GetAuthoredWaitButtonForTest()->GetCommandView().bEnabled);
	TestTrue(TEXT("EndTurn pending view is applied to authored end turn button"),
		CommandBar->GetAuthoredEndTurnButtonForTest()->GetCommandView().bPending);
	TestFalse(TEXT("Authored end turn button is disabled"),
		CommandBar->GetAuthoredEndTurnButtonForTest()->GetCommandView().bEnabled);

	return true;
}
