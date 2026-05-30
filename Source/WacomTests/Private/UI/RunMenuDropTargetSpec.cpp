// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "UI/Run/WacomRunMenuDropTargetWidget.h"
#include "UI/WacomShopRunEventTestProbes.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuZoneDropTargetBuildsHandleSpec,
	"Wacom.UI.RunMenuDropTarget.ZoneDropTargetBuildsInteractionHandle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuZoneDropTargetBuildsHandleSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunMenuDropTargetWidget> Target(
		NewObject<UWacomRunMenuDropTargetWidget>());
	Target->ZoneId = TEXT("RunEvent.Pay.Fang");
	Target->StableTargetId = TEXT("Event.Fang.Zone");

	const FWacomInteractionTargetHandle Handle =
		Target->BuildZoneTargetHandle(FVector2D(120.0f, 240.0f));

	TestTrue(TEXT("Zone handle is valid"), Handle.IsValid());
	TestEqual(TEXT("Handle kind is Zone"),
		Handle.TargetKind,
		EWacomInteractionTargetKind::Zone);
	TestEqual(TEXT("Zone id preserved"),
		Handle.ZoneId,
		FName(TEXT("RunEvent.Pay.Fang")));
	TestEqual(TEXT("Stable id preserved"),
		Handle.StableTargetId,
		FName(TEXT("Event.Fang.Zone")));
	TestEqual(TEXT("Screen position preserved"),
		Handle.ScreenPosition,
		FVector2D(120.0f, 240.0f));
	TestTrue(TEXT("Source object is widget"), Handle.SourceObject.Get() == Target.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuInvisibleDisabledDropTargetSpec,
	"Wacom.UI.RunMenuDropTarget.InvisibleOrDisabledDropTargetDoesNotProbe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuInvisibleDisabledDropTargetSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunMenuDropTargetWidget> Target(
		NewObject<UWacomRunMenuDropTargetWidget>());
	Target->ZoneId = TEXT("RunEvent.Disabled.Zone");

	TestTrue(TEXT("Default target can probe"), Target->CanProbeRunMenuDropTarget());

	Target->SetIsEnabled(false);
	TestFalse(TEXT("Disabled target cannot probe"), Target->CanProbeRunMenuDropTarget());

	Target->SetIsEnabled(true);
	Target->SetVisibility(ESlateVisibility::Collapsed);
	TestFalse(TEXT("Collapsed target cannot probe"), Target->CanProbeRunMenuDropTarget());

	Target->SetVisibility(ESlateVisibility::Visible);
	Target->bEnableRunMenuDropProbe = false;
	TestFalse(TEXT("Probe flag disables target"), Target->CanProbeRunMenuDropTarget());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuTopmostDropTargetSpec,
	"Wacom.UI.RunMenuDropTarget.TopmostMenuDropTargetWinsProbe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuTopmostDropTargetSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Bottom(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));
	TStrongObjectPtr<UWacomRunMenuDropTargetWidgetProbe> Top(
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(PC.Get()));

	Bottom->ZoneId = TEXT("Zone.Bottom");
	Bottom->bProbeHitForTest = true;
	Top->ZoneId = TEXT("Zone.Top");
	Top->bProbeHitForTest = true;

	PC->RegisterRunMenuDropTargetForTest(Bottom.Get());
	PC->RegisterRunMenuDropTargetForTest(Top.Get());

	FWacomInteractionTargetHandle Handle;
	TestTrue(TEXT("Probe finds a target"),
		PC->ProbeRunMenuDropTargetAtWidgetPositionForTest(FVector2D(32.0f, 64.0f), Handle));
	TestEqual(TEXT("Last registered target wins as topmost"),
		Handle.ZoneId,
		FName(TEXT("Zone.Top")));
	TestEqual(TEXT("Probe position passed through"),
		Top->GetLastWidgetPositionForTest(),
		FVector2D(32.0f, 64.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuDropTargetPreviewStateSpec,
	"Wacom.UI.RunMenuDropTarget.DropTargetPreviewStateSetsAndClears",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuDropTargetPreviewStateSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunMenuDropTargetWidget> Target(
		NewObject<UWacomRunMenuDropTargetWidget>());
	Target->ZoneId = TEXT("Zone.Preview");
	Target->ProbePreviewScale = 1.1f;

	Target->TakeWidget();
	Target->SetRunMenuDropPreviewState(EWacomRunMenuDropTargetPreviewState::Probe);
	TestEqual(TEXT("Preview state is set"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Probe);
	TestEqual(TEXT("Fallback preview scales widget"),
		Target->GetRenderTransform().Scale,
		FVector2D(1.1f, 1.1f));

	Target->SetRunMenuDropPreviewState(EWacomRunMenuDropTargetPreviewState::SubmitReady);
	TestEqual(TEXT("Submit-ready state is set"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::SubmitReady);
	TestEqual(TEXT("Submit-ready preview scales widget"),
		Target->GetRenderTransform().Scale,
		FVector2D(1.1f, 1.1f));

	Target->SetRunMenuDropPreviewState(EWacomRunMenuDropTargetPreviewState::Submitted);
	TestEqual(TEXT("Submitted state is set"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Submitted);

	Target->ClearRunMenuDropPreviewState();
	TestEqual(TEXT("Preview state clears"),
		Target->GetRunMenuDropPreviewState(),
		EWacomRunMenuDropTargetPreviewState::Normal);
	TestEqual(TEXT("Fallback preview scale restores"),
		Target->GetRenderTransform().Scale,
		FVector2D(1.0f, 1.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunMenuDropTargetDebugSummarySpec,
	"Wacom.UI.RunMenuDropTarget.MenuDropTargetDebugSummaryReportsZoneAndPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunMenuDropTargetDebugSummarySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunMenuDropTargetWidget> Target(
		NewObject<UWacomRunMenuDropTargetWidget>());
	Target->ZoneId = TEXT("Zone.Debug");
	Target->StableTargetId = TEXT("Stable.Debug");
	Target->SetRunMenuDropPreviewState(EWacomRunMenuDropTargetPreviewState::Probe);

	const FString Summary = Target->GetRunMenuDropTargetDebugSummary();
	TestTrue(TEXT("Summary includes zone id"), Summary.Contains(TEXT("Zone.Debug")));
	TestTrue(TEXT("Summary includes stable id"), Summary.Contains(TEXT("Stable.Debug")));
	TestTrue(TEXT("Summary reports preview active"), Summary.Contains(TEXT("PreviewActive=true")));

	return true;
}
