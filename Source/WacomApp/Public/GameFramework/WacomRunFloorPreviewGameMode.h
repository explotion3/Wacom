// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WacomGameMode.h"
#include "WacomRunFloorPreviewGameMode.generated.h"

class AWacomRunFloorSceneDescriptorActor;
class UWacomFloorMapDefinition;

/**
 * 直接打开单个 Run Floor 做 Editor PIE 时使用的临时启动层。
 *
 * 它只从当前 World 的唯一 Floor Descriptor 构造 transient Journey，不能用于
 * Standalone、Game 或发行启动。正式 Journey 落地后，Production map 必须移除
 * 对该 GameMode 的依赖。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunFloorPreviewGameMode : public AWacomGameMode
{
	GENERATED_BODY()

public:
	virtual UWacomJourneyDefinition* ResolveJourneyDefinitionForNewRun() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UWacomJourneyDefinition> PreviewJourney = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AWacomRunFloorSceneDescriptorActor> ResolvedPreviewDescriptor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomFloorMapDefinition> ResolvedPreviewFloor = nullptr;

	FName ResolvedPreviewFloorId = NAME_None;
};
