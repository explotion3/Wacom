// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/WacomCursorLookDriverComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopCursorLookProfileSpec,
	"Wacom.UI.WorldShop.CursorLookProfile.LiveRunCopyAndSanitize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopCursorLookProfileSpec::RunTest(const FString& Parameters)
{
	UWacomRunPathTraversalComponent* RunPath = NewObject<UWacomRunPathTraversalComponent>();
	RunPath->YawClampDegrees = 17.0f;
	RunPath->PitchClampDegrees = 9.0f;
	RunPath->LookYawScale = 1.25f;
	RunPath->LookPitchScale = 0.75f;
	RunPath->LookInterpSpeed = 14.0f;
	const FWacomCursorLookProfile Profile = RunPath->GetLiveCursorLookProfile();
	TestEqual(TEXT("live yaw copied"), Profile.YawClampDegrees, 17.0f);
	TestEqual(TEXT("live pitch copied"), Profile.PitchClampDegrees, 9.0f);
	TestEqual(TEXT("live interpolation copied"), Profile.LookInterpSpeed, 14.0f);

	Profile.Sanitized();
	TestEqual(TEXT("copy does not mutate authored yaw"), RunPath->YawClampDegrees, 17.0f);

	FWacomCursorLookProfile Invalid;
	Invalid.YawClampDegrees = -5.0f;
	Invalid.PitchClampDegrees = -3.0f;
	Invalid.LookInterpSpeed = -2.0f;
	const FWacomCursorLookProfile Sanitized = Invalid.Sanitized();
	TestEqual(TEXT("yaw becomes positive"), Sanitized.YawClampDegrees, 5.0f);
	TestEqual(TEXT("pitch becomes positive"), Sanitized.PitchClampDegrees, 3.0f);
	TestEqual(TEXT("interpolation cannot be negative"), Sanitized.LookInterpSpeed, 0.0f);
	return true;
}
