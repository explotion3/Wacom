// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "UI/Backpack/BackpackWorkspaceModelTestAccess.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceLayoutContractSpec,
	"Wacom.UI.Backpack.Workspace.LayoutContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceLayoutContractSpec::RunTest(const FString& Parameters)
{
	const FGuid OwnerId(11, 22, 33, 44);
	TestFalse(
		TEXT("Non-special zone normalizes owner identity"),
		FWacomBackpackWorkspaceModelTestAccess::NormalizeZoneOwner(
			EZoneKind::Backpack,
			OwnerId).IsValid());
	TestEqual(
		TEXT("Special zone preserves owner identity"),
		FWacomBackpackWorkspaceModelTestAccess::NormalizeZoneOwner(
			EZoneKind::SpecialZone,
			OwnerId),
		OwnerId);

	const FVector2D WorkspaceSize(1280.0f, 720.0f);
	const FVector2D CardSize(220.0f, 320.0f);
	const TArray<FWacomBackpackWorkspaceResolvedLayoutTestView> DefaultLayouts =
		FWacomBackpackWorkspaceModelTestAccess::BuildDefaultLayout(
			6,
			WorkspaceSize,
			CardSize,
			FVector2D(36.0f, 44.0f),
			FVector2D(56.0f, 56.0f));
	TestEqual(TEXT("Default layout returns one transform per card"), DefaultLayouts.Num(), 6);
	for (int32 Index = 0; Index < DefaultLayouts.Num(); ++Index)
	{
		const FWacomBackpackWorkspaceResolvedLayoutTestView& Layout = DefaultLayouts[Index];
		TestEqual(TEXT("Default layer rank follows stable card order"), Layout.LayerRank, Index);
		TestTrue(TEXT("Default card center remains inside workspace X"), Layout.CardCenter.X >= 0.0f && Layout.CardCenter.X <= WorkspaceSize.X);
		TestTrue(TEXT("Default card center remains inside workspace Y"), Layout.CardCenter.Y >= 0.0f && Layout.CardCenter.Y <= WorkspaceSize.Y);
	}

	const FVector2D DenseWorkspaceSize(1164.0f, 693.0f);
	const TArray<FWacomBackpackWorkspaceResolvedLayoutTestView> DenseLayouts =
		FWacomBackpackWorkspaceModelTestAccess::BuildDefaultLayout(
			15,
			DenseWorkspaceSize,
			CardSize,
			FVector2D(36.0f, 44.0f),
			FVector2D(56.0f, 56.0f));
	TestEqual(TEXT("Dense layout returns every card"), DenseLayouts.Num(), 15);
	for (const FWacomBackpackWorkspaceResolvedLayoutTestView& Layout : DenseLayouts)
	{
		TestTrue(TEXT("Dense layout keeps the full card inside horizontally"),
			Layout.CardCenter.X >= CardSize.X * 0.5f - KINDA_SMALL_NUMBER
			&& Layout.CardCenter.X <= DenseWorkspaceSize.X - CardSize.X * 0.5f + KINDA_SMALL_NUMBER);
		TestTrue(TEXT("Dense layout keeps the full card inside vertically"),
			Layout.CardCenter.Y >= CardSize.Y * 0.5f - KINDA_SMALL_NUMBER
			&& Layout.CardCenter.Y <= DenseWorkspaceSize.Y - CardSize.Y * 0.5f + KINDA_SMALL_NUMBER);
	}
	if (DenseLayouts.Num() == 15)
	{
		TestTrue(TEXT("Dense rows remain individually identifiable after vertical compression"),
			DenseLayouts[0].CardCenter.Y < DenseLayouts[4].CardCenter.Y
			&& DenseLayouts[4].CardCenter.Y < DenseLayouts[8].CardCenter.Y
			&& DenseLayouts[8].CardCenter.Y < DenseLayouts[12].CardCenter.Y);
	}

	const FWacomBackpackWorkspaceResolvedLayoutTestView ManualLayout =
		FWacomBackpackWorkspaceModelTestAccess::ResolveManualLayout(
			FVector2D(-2.0f, 3.0f),
			-17.0f,
			9,
			WorkspaceSize,
			CardSize,
			0.3f);
	const float MinimumX = (0.3f - 0.5f) * CardSize.X;
	const float MaximumY = WorkspaceSize.Y - (0.3f - 0.5f) * CardSize.Y;
	TestTrue(TEXT("Manual layout keeps at least 30 percent visible on left"), ManualLayout.CardCenter.X >= MinimumX - KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Manual layout keeps at least 30 percent visible on bottom"), ManualLayout.CardCenter.Y <= MaximumY + KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Manual layout preserves angle"), ManualLayout.AngleDegrees, -17.0f);
	TestEqual(TEXT("Manual layout preserves layer rank"), ManualLayout.LayerRank, 9);

	TestEqual(
		TEXT("Arrange All clears every manual layout entry"),
		FWacomBackpackWorkspaceModelTestAccess::ArrangeAllAndCountRemainingManualLayouts(5),
		0);
	return true;
}

#endif
