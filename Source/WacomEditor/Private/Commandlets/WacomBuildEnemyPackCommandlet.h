// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "WacomBuildEnemyPackCommandlet.generated.h"

/**
 * 构建单个正式敌人内容包。
 *
 * 用法：
 *   -run=WacomBuildEnemyPack -Pack=TrainingWarrior [-PromoteArt] [-ForceArtRefresh]
 */
UCLASS()
class UWacomBuildEnemyPackCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildEnemyPackCommandlet();

	virtual int32 Main(const FString& Params) override;
};
