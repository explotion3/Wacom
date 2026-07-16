// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AWacomBattleEnemyActor;

struct WACOMEDITOR_API FWacomBattleSceneEnemyHostSyncResult
{
	TWeakObjectPtr<AWacomBattleEnemyActor> Host;
	FName ResultCode = TEXT("NotRun");
	TArray<FName> AddedPartSlotIds;
	TArray<FName> UpdatedPartSlotIds;
	TArray<FName> InvalidDefinitionPartSlotIds;
	TArray<FName> FailedPartSlotIds;
	bool bChanged = false;
	bool bAddedPart = false;
};

/** Editor-only, transactional mutation service for Scene Enemy Host authoring. */
class WACOMEDITOR_API FWacomBattleSceneEnemyHostAuthoring
{
public:
	static TArray<FWacomBattleSceneEnemyHostSyncResult> SyncPartsFromDefinition(
		TConstArrayView<AWacomBattleEnemyActor*> Hosts);

	/** Legacy debug sample retained behind the Host Details Advanced/Debug section. */
	static FName ConfigureDebugSnakeSample(AWacomBattleEnemyActor& Host);
};
