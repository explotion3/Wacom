// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Events/BattleEvent.h"

namespace WacomBattleCardChangeFeedbackPolicy
{
	inline bool LicensesCostRewrite(const EBattleEventType Type)
	{
		return Type == EBattleEventType::CardRuntimeCostChanged
			|| Type == EBattleEventType::CardStatusChanged;
	}

	inline bool LicensesEffectBadgeRewrite(const EBattleEventType Type)
	{
		return LicensesCostRewrite(Type)
			|| Type == EBattleEventType::CardEffectMagnitudeChanged;
	}
}
