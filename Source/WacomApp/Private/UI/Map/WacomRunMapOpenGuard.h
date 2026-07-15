// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** PlayerController 在读取当前运行时对象后交给纯策略判断的地图开启事实。 */
struct FWacomRunMapOpenGuardFacts
{
	bool bExplorationFlow = false;
	bool bHasSession = false;
	bool bHasCoordinator = false;
	bool bHasFlow = false;
	bool bHasTraversal = false;
	bool bTraversalAnchored = false;
	bool bCoordinatorTraversalActive = false;
	bool bSnapshotValid = false;
	bool bActiveActivity = false;
	bool bVersionsMatch = false;
	bool bDeadEnd = false;
};

struct FWacomRunMapOpenGuardDecision
{
	bool bCanOpen = false;
	bool bPreferRecommendedTarget = false;
	FName RejectDetail = NAME_None;
};

/** Run Map 打开条件的 App-private 单一纯策略。 */
class FWacomRunMapOpenGuard
{
public:
	static FWacomRunMapOpenGuardDecision Evaluate(
		const FWacomRunMapOpenGuardFacts& Facts)
	{
		FWacomRunMapOpenGuardDecision Decision;
		if (!Facts.bExplorationFlow)
		{
			Decision.RejectDetail = TEXT("NotInExplorationFlow");
		}
		else if (!Facts.bHasSession)
		{
			Decision.RejectDetail = TEXT("RunSessionUnavailable");
		}
		else if (!Facts.bHasCoordinator)
		{
			Decision.RejectDetail = TEXT("RunCoordinatorUnavailable");
		}
		else if (!Facts.bHasFlow)
		{
			Decision.RejectDetail = TEXT("RunMapFlowUnavailable");
		}
		else if (!Facts.bHasTraversal)
		{
			Decision.RejectDetail = TEXT("RunTraversalUnavailable");
		}
		else if (Facts.bCoordinatorTraversalActive)
		{
			Decision.RejectDetail = TEXT("TraversalTransactionActive");
		}
		else if (!Facts.bTraversalAnchored)
		{
			Decision.RejectDetail = TEXT("TraversalNotAnchored");
		}
		else if (!Facts.bSnapshotValid)
		{
			Decision.RejectDetail = TEXT("RunMapSnapshotInvalid");
		}
		else if (Facts.bActiveActivity)
		{
			Decision.RejectDetail = TEXT("RunActivityActive");
		}
		else if (!Facts.bVersionsMatch)
		{
			Decision.RejectDetail = TEXT("RunMapVersionDrift");
		}
		else
		{
			Decision.bCanOpen = true;
		}
		Decision.bPreferRecommendedTarget = Decision.bCanOpen && Facts.bDeadEnd;
		return Decision;
	}

	static bool IsGameMenuSlotAvailable(
		const bool bHasOtherActiveGameMenu,
		const bool bHasPendingGameMenu)
	{
		return !bHasOtherActiveGameMenu && !bHasPendingGameMenu;
	}
};
