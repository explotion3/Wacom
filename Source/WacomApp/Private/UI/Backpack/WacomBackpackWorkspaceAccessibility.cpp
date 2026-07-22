// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceAccessibility.h"

EWacomBackpackWorkspaceCardSemanticIcon
FWacomBackpackWorkspaceAccessibility::ResolveCardSemanticIcon(
	bool bSelected,
	bool bValidDrop,
	bool bRejectedDrop)
{
	if (bRejectedDrop)
	{
		return EWacomBackpackWorkspaceCardSemanticIcon::RejectedDrop;
	}
	if (bValidDrop)
	{
		return EWacomBackpackWorkspaceCardSemanticIcon::ValidDrop;
	}
	return bSelected
		? EWacomBackpackWorkspaceCardSemanticIcon::Selected
		: EWacomBackpackWorkspaceCardSemanticIcon::None;
}
