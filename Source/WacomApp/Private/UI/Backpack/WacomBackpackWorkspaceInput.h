// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * App-private input result shared by the Workspace runtime controllers.
 *
 * Controllers describe semantic ownership only. The UMG adapter remains the
 * sole place that translates this result into Slate FReply operations.
 */
enum class EWacomBackpackWorkspaceInputReply : uint8
{
	Unhandled,
	Handled,
	CaptureAndFocus,
	ReleaseCapture
};

[[nodiscard]] constexpr bool IsWacomBackpackInputHandled(
	const EWacomBackpackWorkspaceInputReply Reply)
{
	return Reply != EWacomBackpackWorkspaceInputReply::Unhandled;
}

[[nodiscard]] constexpr bool DoesWacomBackpackInputCapturePointer(
	const EWacomBackpackWorkspaceInputReply Reply)
{
	return Reply == EWacomBackpackWorkspaceInputReply::CaptureAndFocus;
}

[[nodiscard]] constexpr bool DoesWacomBackpackInputReleasePointer(
	const EWacomBackpackWorkspaceInputReply Reply)
{
	return Reply == EWacomBackpackWorkspaceInputReply::ReleaseCapture;
}
