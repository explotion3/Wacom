// Copyright Wacom. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "WacomEnemyUIToolset.generated.h"

class UWidget;
class UWidgetBlueprint;

/**
 * Wacom-owned MCP bridge for UMG authoring operations that are not exposed by
 * the engine UMG toolset yet. The tool mutates in-memory assets only; the MCP
 * writer lease and the caller remain responsible for compiling and saving.
 */
UCLASS(BlueprintType)
class UWacomEnemyUIToolset final : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Adds the required left/right open and close slide animations to the exact
	 * enemy inspection WBP. Existing named animations are validated and never
	 * overwritten. The function refuses every other package and widget tree.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wacom|Enemy UI")
	static bool EnsureInspectionPanelAnimations(
		UWidgetBlueprint* WidgetBlueprint,
		UWidget* LeftPanel,
		UWidget* RightPanel);

	/**
	 * Stamps the versioned contract description on one of the six exact enemy
	 * segmented-vitals or inspection WBPs. The expected marker is derived from
	 * the package and parent class; arbitrary packages and marker text are not
	 * accepted.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wacom|Enemy UI")
	static bool EnsureSegmentedUIContractMarker(
		UWidgetBlueprint* WidgetBlueprint);
};
