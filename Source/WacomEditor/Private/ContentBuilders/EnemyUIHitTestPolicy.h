// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

class UWidgetBlueprint;

namespace Wacom::ContentBuilder::EnemyUIHitTestPolicy
{
	/**
	 * Validates every formal Enemy UI interaction route, including the complete
	 * parent chain that Slate uses for hit testing.
	 */
	bool ValidateInteractiveRoutes(const UWidgetBlueprint& Blueprint);

	/**
	 * Converts only HitTestInvisible widgets on known interaction routes to the
	 * child-preserving visibility required by Slate. Hidden/collapsed routes are
	 * rejected instead of being made visible implicitly.
	 */
	bool NormalizeInteractiveRoutes(
		UWidgetBlueprint& Blueprint,
		int32& OutChangedWidgetCount);
}
