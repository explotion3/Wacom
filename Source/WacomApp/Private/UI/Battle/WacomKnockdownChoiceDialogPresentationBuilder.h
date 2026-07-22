// Copyright Wacom. All Rights Reserved.

#pragma once

#include "UI/Battle/WacomKnockdownChoiceDialogTypes.h"

struct FBattleSnapshot;
struct FKnockdownChoiceView;

/** App-private projection from the Battle knockdown contract to passive UI data. */
class WACOMAPP_API FWacomKnockdownChoiceDialogPresentationBuilder
{
public:
	static FWacomKnockdownChoiceDialogViewData Build(
		const FKnockdownChoiceView& ChoiceView,
		const FBattleSnapshot& Snapshot);
};
