// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Backpack/WacomDeckCardWidget.h"

/** Workspace 非颜色语义反馈的 production-private 归约规则。 */
class WACOMAPP_API FWacomBackpackWorkspaceAccessibility
{
public:
	static EWacomBackpackWorkspaceCardSemanticIcon ResolveCardSemanticIcon(
		bool bSelected,
		bool bValidDrop,
		bool bRejectedDrop);
};
