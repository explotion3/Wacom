// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UI/ExplorationHUDInputTestAccess.h"
#include "UI/Foundation/WacomExplorationHUD.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomExplorationHUDInputSpec
{
	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomExplorationHUDFocusLifecycleTest,
	"Wacom.UI.ExplorationHUD.GameViewportFocusLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomExplorationHUDFocusLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomExplorationHUDInputSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	TStrongObjectPtr<UWacomExplorationHUD> HUD(NewObject<UWacomExplorationHUD>(World));
	HUD->ActivateWidget();
	TestTrue(TEXT("Exploration HUD activates"), HUD->IsActivated());
	TestTrue(TEXT("Activation defers game viewport focus until CommonUI input config settles"),
		FWacomExplorationHUDInputTestAccess::IsGameViewportFocusPending(*HUD));

	HUD->DeactivateWidget();
	TestFalse(TEXT("Deactivation cancels a stale viewport focus request"),
		FWacomExplorationHUDInputTestAccess::IsGameViewportFocusPending(*HUD));
	return true;
}

#endif
