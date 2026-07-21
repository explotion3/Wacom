// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

class UPileCountView;
class UTextBlock;

struct FWacomPileCountViewTestAccess
{
	static UPileCountView* CreateWidget();
	static void Tick(UPileCountView& Widget, float DeltaSeconds);
	static void Destruct(UPileCountView& Widget);
	static UTextBlock* GetCountText(const UPileCountView& Widget);
	static FReply PressMouseButton(UPileCountView& Widget, const FKey& Key);
	static FReply PressKey(UPileCountView& Widget, const FKey& Key);
};

#endif
