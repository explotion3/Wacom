// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWidget;
class UWacomRunMapNodeWidget;
class UWacomRunMapScreen;

/** WacomTests-private access wrapper；生产 API 不扩散测试 getter。 */
struct FWacomRunMapScreenTestAccess
{
	static void BuildAndConstruct(UWacomRunMapScreen& Screen);
	static void Destruct(UWacomRunMapScreen& Screen);
	static int32 GetNodeWidgetCount(const UWacomRunMapScreen& Screen);
	static UWacomRunMapNodeWidget* FindNodeWidget(
		const UWacomRunMapScreen& Screen,
		FName FloorId,
		FName NodeId);
	static UWidget* GetDesiredFocusTarget(const UWacomRunMapScreen& Screen);
	static bool IsTravelButtonEnabled(const UWacomRunMapScreen& Screen);
	static FReply PressKey(UWacomRunMapScreen& Screen, const FKey& Key);
	static bool HasRequiredBindings(const UWacomRunMapScreen& Screen);
	static UClass* GetNodeWidgetClass(const UWacomRunMapScreen& Screen);
};
