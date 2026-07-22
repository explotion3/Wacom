// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/NavigationReply.h"
#include "WacomBackpackWorkspaceTypes.h"

enum class EWacomBackpackWorkspaceNavigationTargetKind : uint8
{
	None,
	Card,
	Flux,
	Pile,
	Delete
};

struct WACOMAPP_API FWacomBackpackWorkspaceNavigationTarget
{
	EWacomBackpackWorkspaceNavigationTargetKind Kind =
		EWacomBackpackWorkspaceNavigationTargetKind::None;
	FGuid InstanceId;
	FWacomBackpackZoneKey Zone;
	FVector2D Center = FVector2D::ZeroVector;
	int32 LayerRank = 0;
	bool bActionable = true;

	bool HasSameIdentity(const FWacomBackpackWorkspaceNavigationTarget& Other) const;
};

/** Stable virtual focus. Slate focus remains on the Workspace SObjectWidget. */
class WACOMAPP_API FWacomBackpackWorkspaceNavigationController
{
public:
	void ReconcileTargets(TConstArrayView<FWacomBackpackWorkspaceNavigationTarget> InTargets);
	bool Move(EUINavigation Direction);
	void ActivateSemanticFocus() { bSemanticFocusActive = true; }
	void NotifyPointerInput() { bSemanticFocusActive = false; }
	bool IsSemanticFocusActive() const { return bSemanticFocusActive; }
	void Clear();

	const FWacomBackpackWorkspaceNavigationTarget* GetFocusedTarget() const;
	bool IsCardFocused(FGuid InstanceId) const;

private:
	TArray<FWacomBackpackWorkspaceNavigationTarget> Targets;
	int32 FocusedIndex = INDEX_NONE;
	bool bSemanticFocusActive = false;
};
