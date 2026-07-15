// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../WacomApp/Private/Components/WacomBattleEnemyPartTargetPreviewPlayback.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyTargetPreviewPlaybackSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetPreview.PlaybackEnterHoldExitAndValidity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyTargetPreviewPlaybackSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleEnemyPartTargetPreviewPlayback Playback;
	TestTrue(TEXT("valid preview begins"), Playback.Begin(
		EWacomBattleEnemyPartTargetPreviewKind::Valid,
		0.18f,
		0.10f,
		0.90f,
		false));
	TestTrue(TEXT("preview active"), Playback.GetView().bActive);
	TestEqual(TEXT("valid kind"), Playback.GetView().Kind,
		EWacomBattleEnemyPartTargetPreviewKind::Valid);
	TestEqual(TEXT("enter starts from zero"), Playback.GetView().Amount, 0.0f);

	FWacomBattleEnemyPartTargetPreviewPlaybackView View = Playback.Tick(0.09f);
	TestTrue(TEXT("enter amount advances"), View.Amount > 0.0f && View.Amount < 1.0f);
	View = Playback.Tick(0.09f);
	TestEqual(TEXT("enter reaches hold"), View.Phase,
		EWacomBattleEnemyPartTargetPreviewPhase::Holding);
	TestEqual(TEXT("hold amount is one"), View.Amount, 1.0f);

	View = Playback.Tick(0.225f);
	TestTrue(TEXT("valid hold produces weak pulse phase"), View.Pulse > 0.9f);
	TestFalse(TEXT("same valid target does not restart"), Playback.Begin(
		EWacomBattleEnemyPartTargetPreviewKind::Valid,
		0.18f,
		0.10f,
		0.90f,
		false));
	TestEqual(TEXT("same target remains holding"), Playback.GetView().Phase,
		EWacomBattleEnemyPartTargetPreviewPhase::Holding);

	TestTrue(TEXT("switching to invalid restarts preview"), Playback.Begin(
		EWacomBattleEnemyPartTargetPreviewKind::Invalid,
		0.18f,
		0.10f,
		0.90f,
		false));
	View = Playback.Tick(0.18f);
	View = Playback.Tick(0.25f);
	TestEqual(TEXT("invalid preview never breathes"), View.Pulse, 0.0f);

	Playback.BeginExit();
	View = Playback.Tick(0.05f);
	TestTrue(TEXT("exit fades without instant clear"), View.bActive && View.Amount > 0.0f);
	View = Playback.Tick(0.05f);
	TestFalse(TEXT("exit completes"), View.bActive);
	TestEqual(TEXT("exit clears amount"), View.Amount, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyTargetPreviewReducedMotionSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetPreview.ReducedMotionUsesStaticSemanticFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyTargetPreviewReducedMotionSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleEnemyPartTargetPreviewPlayback Playback;
	Playback.Begin(
		EWacomBattleEnemyPartTargetPreviewKind::Valid,
		0.18f,
		0.10f,
		0.90f,
		true);
	const FWacomBattleEnemyPartTargetPreviewPlaybackView View = Playback.Tick(0.45f);
	TestTrue(TEXT("reduced preview remains active"), View.bActive);
	TestEqual(TEXT("reduced preview is immediately fully visible"), View.Amount, 1.0f);
	TestEqual(TEXT("reduced preview has no pulse"), View.Pulse, 0.0f);
	TestEqual(TEXT("reduced preview holds without enter motion"), View.Phase,
		EWacomBattleEnemyPartTargetPreviewPhase::Holding);
	return true;
}
