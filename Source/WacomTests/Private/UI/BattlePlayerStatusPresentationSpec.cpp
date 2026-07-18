// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/PlayerStatusBarTestAccess.h"

namespace WacomBattlePlayerStatusPresentationSpec
{
	TStrongObjectPtr<UPlayerStatusBar> MakeStatusBar(TSharedPtr<SWidget>& OutSlateWidget)
	{
		TStrongObjectPtr<UPlayerStatusBar> Widget(NewObject<UPlayerStatusBar>());
		OutSlateWidget = Widget->TakeWidget();
		return Widget;
	}

	FBattleSnapshot MakeSnapshot(
		const int32 Hp,
		const int32 MaxHp,
		const int32 Shield)
	{
		FBattleSnapshot Snapshot;
		Snapshot.Player.CurrentHp = Hp;
		Snapshot.Player.MaxHp = MaxHp;
		Snapshot.Player.Shield = Shield;
		return Snapshot;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePlayerStatusPreviewSpec,
	"Wacom.UI.Battle.PlayerStatus.PreviewSegmentsAndAuthoritativeValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePlayerStatusPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattlePlayerStatusPresentationSpec;
	TSharedPtr<SWidget> SlateWidget;
	TStrongObjectPtr<UPlayerStatusBar> Widget = MakeStatusBar(SlateWidget);
	Widget->RefreshFromSnapshot(MakeSnapshot(50, 100, 5));

	FPlayerSnapshot Preview = MakeSnapshot(70, 100, 9).Player;
	Widget->SetActionPreview(Preview);
	FWacomPlayerStatusBarAutomationTestView View =
		FWacomPlayerStatusBarTestAccess::View(*Widget);
	TestTrue(TEXT("Preview is active"), View.bHasActionPreview);
	TestEqual(TEXT("Authoritative HP remains 50 percent"), View.CurrentHpPercent, 0.5f);
	TestEqual(TEXT("Projected HP is 70 percent"), View.PreviewHpPercent, 0.7f);
	TestEqual(TEXT("Healing uses gain preview mode"), View.PreviewMode, 1);
	TestEqual(TEXT("Projected HP text value"), View.DisplayHp, 70);
	TestEqual(TEXT("Projected shield value"), View.DisplayShield, 9);

	Preview.CurrentHp = 35;
	Preview.Shield = 0;
	Widget->SetActionPreview(Preview);
	View = FWacomPlayerStatusBarTestAccess::View(*Widget);
	TestEqual(TEXT("Damage uses loss preview mode"), View.PreviewMode, 2);
	TestTrue(TEXT("Authoritative shield keeps fixed shield area visible"), View.bShieldVisible);

	Widget->ClearActionPreview();
	View = FWacomPlayerStatusBarTestAccess::View(*Widget);
	TestFalse(TEXT("Preview clears"), View.bHasActionPreview);
	TestEqual(TEXT("Authoritative HP returns"), View.DisplayHp, 50);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePlayerStatusDamageTrailSpec,
	"Wacom.UI.Battle.PlayerStatus.DamageTrailHoldsThenRecovers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePlayerStatusDamageTrailSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattlePlayerStatusPresentationSpec;
	TSharedPtr<SWidget> SlateWidget;
	TStrongObjectPtr<UPlayerStatusBar> Widget = MakeStatusBar(SlateWidget);
	const FBattleSnapshot Previous = MakeSnapshot(80, 100, 0);
	const FBattleSnapshot Current = MakeSnapshot(50, 100, 0);
	Widget->RefreshFromSnapshot(Current);
	Widget->PlayEnemyActionImpactFeedback(Previous.Player, Current.Player);

	FWacomPlayerStatusBarAutomationTestView View =
		FWacomPlayerStatusBarTestAccess::View(*Widget);
	TestEqual(TEXT("Damage trail starts at previous HP"), View.DamageTrailPercent, 0.8f);
	TestTrue(TEXT("Damage feedback is active"), View.bPlaybackActive);

	FWacomPlayerStatusBarTestAccess::Tick(*Widget, 0.04f);
	View = FWacomPlayerStatusBarTestAccess::View(*Widget);
	TestEqual(TEXT("Trail remains during hold"), View.DamageTrailPercent, 0.8f);

	FWacomPlayerStatusBarTestAccess::Tick(*Widget, 0.20f);
	View = FWacomPlayerStatusBarTestAccess::View(*Widget);
	TestTrue(TEXT("Trail recovers between previous and current HP"),
		View.DamageTrailPercent < 0.8f && View.DamageTrailPercent > 0.5f);

	FWacomPlayerStatusBarTestAccess::Tick(*Widget, 0.30f);
	View = FWacomPlayerStatusBarTestAccess::View(*Widget);
	TestEqual(TEXT("Trail reaches authoritative HP"), View.DamageTrailPercent, 0.5f);
	TestFalse(TEXT("Damage playback completes"), View.bPlaybackActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePlayerStatusShieldBreakSpec,
	"Wacom.UI.Battle.PlayerStatus.ShieldBreakHidesAfterFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePlayerStatusShieldBreakSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattlePlayerStatusPresentationSpec;
	TSharedPtr<SWidget> SlateWidget;
	TStrongObjectPtr<UPlayerStatusBar> Widget = MakeStatusBar(SlateWidget);
	const FBattleSnapshot Previous = MakeSnapshot(60, 100, 8);
	const FBattleSnapshot Current = MakeSnapshot(60, 100, 0);
	Widget->RefreshFromSnapshot(Current);
	Widget->PlayEnemyActionImpactFeedback(Previous.Player, Current.Player);

	FWacomPlayerStatusBarAutomationTestView View =
		FWacomPlayerStatusBarTestAccess::View(*Widget);
	TestTrue(TEXT("Broken shield remains visible during feedback"), View.bShieldVisible);
	TestTrue(TEXT("Shield feedback starts"), View.bPlaybackActive);

	FWacomPlayerStatusBarTestAccess::Tick(*Widget, 0.20f);
	View = FWacomPlayerStatusBarTestAccess::View(*Widget);
	TestFalse(TEXT("Shield hides after break feedback"), View.bShieldVisible);
	TestFalse(TEXT("Shield playback completes"), View.bPlaybackActive);
	TestEqual(TEXT("Shield transform restores"), View.ShieldScale, 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePlayerStatusAccessibilitySpec,
	"Wacom.UI.Battle.PlayerStatus.LowHealthAndSimplifiedMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePlayerStatusAccessibilitySpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattlePlayerStatusPresentationSpec;
	TSharedPtr<SWidget> SlateWidget;
	TStrongObjectPtr<UPlayerStatusBar> Widget = MakeStatusBar(SlateWidget);
	Widget->RefreshFromSnapshot(MakeSnapshot(24, 100, 3));
	FWacomPlayerStatusBarAutomationTestView View =
		FWacomPlayerStatusBarTestAccess::View(*Widget);
	TestTrue(TEXT("Below 25 percent enters low-health color"), View.LowHealthAmount > 0.0f);

	Widget->RefreshFromSnapshot(MakeSnapshot(30, 100, 3));
	View = FWacomPlayerStatusBarTestAccess::View(*Widget);
	TestEqual(TEXT("Above threshold exits low-health color"), View.LowHealthAmount, 0.0f);

	FWacomPlayerStatusBarTestAccess::SetReducedMotion(*Widget, true);
	const FBattleSnapshot Previous = MakeSnapshot(30, 100, 3);
	const FBattleSnapshot Current = MakeSnapshot(20, 100, 0);
	Widget->RefreshFromSnapshot(Current);
	Widget->PlayEnemyActionImpactFeedback(Previous.Player, Current.Player);
	View = FWacomPlayerStatusBarTestAccess::View(*Widget);
	TestTrue(TEXT("Simplified Motion state is reported"), View.bReducedMotion);
	TestFalse(TEXT("Simplified Motion disables delayed playback"), View.bPlaybackActive);
	TestEqual(TEXT("Simplified Motion keeps shield scale authored"), View.ShieldScale, 1.0f);
	return true;
}
