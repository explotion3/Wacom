// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS && WITH_EDITOR

#include "HAL/IConsoleManager.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackPerformanceCaptureTimeline.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackSalePerformanceCaptureTimeline.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackPerformanceCaptureTimelineSpec,
	"Wacom.UI.Backpack.PerformanceCapture.TimelineAndCommands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackPerformanceCaptureTimelineSpec::RunTest(
	const FString& Parameters)
{
	using FTimeline = FWacomBackpackPerformanceCaptureTimeline;
	using EPhase = EWacomBackpackPerformanceCapturePhase;

	TestEqual(
		TEXT("The capture starts in warmup"),
		FTimeline::ResolvePhase(0.0),
		EPhase::Warmup);
	TestEqual(
		TEXT("Warmup lasts exactly ten seconds"),
		FTimeline::ResolvePhase(9.999),
		EPhase::Warmup);
	TestEqual(
		TEXT("The closed baseline starts at ten seconds"),
		FTimeline::ResolvePhase(10.0),
		EPhase::Closed);
	TestEqual(
		TEXT("The closed baseline lasts exactly sixty seconds"),
		FTimeline::ResolvePhase(69.999),
		EPhase::Closed);
	TestEqual(
		TEXT("Opening stabilization follows the closed baseline"),
		FTimeline::ResolvePhase(70.0),
		EPhase::Opening);
	TestEqual(
		TEXT("Opening stabilization lasts exactly three seconds"),
		FTimeline::ResolvePhase(72.999),
		EPhase::Opening);
	TestEqual(
		TEXT("The open idle baseline starts after stabilization"),
		FTimeline::ResolvePhase(73.0),
		EPhase::Idle);
	TestEqual(
		TEXT("The open idle baseline lasts exactly sixty seconds"),
		FTimeline::ResolvePhase(132.999),
		EPhase::Idle);
	TestEqual(
		TEXT("The interaction baseline follows idle"),
		FTimeline::ResolvePhase(133.0),
		EPhase::Interaction);
	TestEqual(
		TEXT("The interaction baseline lasts exactly sixty seconds"),
		FTimeline::ResolvePhase(192.999),
		EPhase::Interaction);
	TestEqual(
		TEXT("Trace finalization is outside the interaction region"),
		FTimeline::ResolvePhase(193.0),
		EPhase::Finalizing);
	TestEqual(
		TEXT("The capture completes after the unmeasured finalization guard"),
		FTimeline::ResolvePhase(193.25),
		EPhase::Complete);

	TestTrue(
		TEXT("Opening stabilization is intentionally outside measured regions"),
		!FTimeline::IsMeasuredPhase(EPhase::Opening));
	TestTrue(
		TEXT("Warmup is recorded as a measured region"),
		FTimeline::IsMeasuredPhase(EPhase::Warmup));
	TestTrue(
		TEXT("Trace finalization is intentionally outside measured regions"),
		!FTimeline::IsMeasuredPhase(EPhase::Finalizing));
	TestEqual(
		TEXT("The phase chain terminates without wrapping"),
		FTimeline::NextPhase(EPhase::Complete),
		EPhase::Complete);

	using FSaleTimeline = FWacomBackpackSalePerformanceCaptureTimeline;
	TestEqual(
		TEXT("Sale capture warmup lasts five seconds"),
		FSaleTimeline::ResolvePhase(4.999),
		EPhase::Warmup);
	TestEqual(
		TEXT("Sale capture closed baseline starts at five seconds"),
		FSaleTimeline::ResolvePhase(5.0),
		EPhase::Closed);
	TestEqual(
		TEXT("Sale capture opens the workspace after ten seconds"),
		FSaleTimeline::ResolvePhase(10.0),
		EPhase::Opening);
	TestEqual(
		TEXT("Sale capture idles after three seconds of geometry stabilization"),
		FSaleTimeline::ResolvePhase(13.0),
		EPhase::Idle);
	TestEqual(
		TEXT("Sale capture submits the transaction at interaction start"),
		FSaleTimeline::ResolvePhase(16.0),
		EPhase::Interaction);
	TestEqual(
		TEXT("Sale capture keeps Trace finalization outside its five-second interaction region"),
		FSaleTimeline::ResolvePhase(21.0),
		EPhase::Finalizing);
	TestEqual(
		TEXT("Sale capture completes after the finalization guard"),
		FSaleTimeline::ResolvePhase(21.25),
		EPhase::Complete);

	IConsoleManager& ConsoleManager = IConsoleManager::Get();
	TestNotNull(
		TEXT("The automated presentation capture command is registered"),
		ConsoleManager.FindConsoleObject(
			TEXT("Wacom.Backpack.CapturePresentationBaseline")));
	TestNotNull(
		TEXT("The automated sale-departure capture command is registered"),
		ConsoleManager.FindConsoleObject(
			TEXT("Wacom.Backpack.CaptureSaleDepartureBaseline")));
	TestNotNull(
		TEXT("The capture cancellation command is registered"),
		ConsoleManager.FindConsoleObject(
			TEXT("Wacom.Backpack.CancelPerformanceCapture")));
	return true;
}

#endif
