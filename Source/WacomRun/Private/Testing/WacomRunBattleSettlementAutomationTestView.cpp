// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomRunBattleSettlementAutomationTestView.h"

#if WITH_AUTOMATION_TESTS

#include "Battle/RunBattleSettlementResolver.h"
#include "Map/WacomMapTypes.h"

bool FWacomRunBattleSettlementAutomationTestView::Resolve(
	FRunState& State,
	const FBattleResultPacket& Packet)
{
	return FRunBattleSettlementResolver::Resolve(
		State,
		Packet,
		FWacomMapNodeHandle());
}

#endif
