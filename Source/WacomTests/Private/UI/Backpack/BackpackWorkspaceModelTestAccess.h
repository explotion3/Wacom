// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"

#if WITH_AUTOMATION_TESTS

struct FWacomBackpackWorkspaceResolvedLayoutTestView
{
	FVector2D CardCenter = FVector2D::ZeroVector;
	float AngleDegrees = 0.0f;
	int32 LayerRank = 0;
};

struct FWacomBackpackWorkspaceStateLifecycleTestView
{
	bool bSameRunPreservedLayout = false;
	bool bRemovedCardLayoutPruned = false;
	bool bNewCardHasNoManualLayout = false;
	bool bNewRunClearedLayouts = false;
	bool bActiveZonePreservedForSameRun = false;
	bool bActiveZoneResetForNewRun = false;
};

struct FWacomBackpackWorkspaceModelTestAccess
{
	static FGuid NormalizeZoneOwner(EZoneKind Zone, FGuid OwnerInstanceId);
	static TArray<FWacomBackpackWorkspaceResolvedLayoutTestView> BuildDefaultLayout(
		int32 CardCount,
		FVector2D WorkspaceSize,
		FVector2D CardSize,
		FVector2D Spacing,
		FVector2D Padding);
	static FWacomBackpackWorkspaceResolvedLayoutTestView ResolveManualLayout(
		FVector2D NormalizedPosition,
		float AngleDegrees,
		int32 LayerRank,
		FVector2D WorkspaceSize,
		FVector2D CardSize,
		float MinimumVisibleFraction);
	static int32 ArrangeAllAndCountRemainingManualLayouts(int32 LayoutCount);
	static FWacomBackpackWorkspaceStateLifecycleTestView RunStateLifecycleScenario();
};

#endif
