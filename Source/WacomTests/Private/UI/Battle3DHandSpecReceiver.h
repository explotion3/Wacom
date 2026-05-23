// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Battle3DHandSpecReceiver.generated.h"

class AWacomBattleCardVisualActor;

UCLASS()
class UWacomBattle3DHandSpecReceiver : public UObject
{
	GENERATED_BODY()

public:
	int32 ClickCount = 0;
	FGuid LastClickedId;

	UFUNCTION()
	void HandleClicked(FGuid InstanceId)
	{
		++ClickCount;
		LastClickedId = InstanceId;
	}

	int32 HoverCount = 0;
	int32 UnhoverCount = 0;
	int32 HoverChangedCount = 0;
	FGuid LastHoveredId;
	FGuid LastUnhoveredId;
	FGuid LastHoverChangedId;
	bool bLastHoverChangedState = false;

	UFUNCTION()
	void HandleHovered(FGuid InstanceId)
	{
		++HoverCount;
		LastHoveredId = InstanceId;
	}

	UFUNCTION()
	void HandleUnhovered(FGuid InstanceId)
	{
		++UnhoverCount;
		LastUnhoveredId = InstanceId;
	}

	UFUNCTION()
	void HandleHoverChanged(FGuid InstanceId, bool bHovered)
	{
		++HoverChangedCount;
		LastHoverChangedId = InstanceId;
		bLastHoverChangedState = bHovered;
	}

	int32 ActorClickCount = 0;
	int32 ActorHoverCount = 0;
	int32 ActorUnhoverCount = 0;
	FGuid LastActorClickedId;
	FGuid LastActorHoveredId;
	FGuid LastActorUnhoveredId;
	AWacomBattleCardVisualActor* LastActorSource = nullptr;

	void HandleActorClicked(AWacomBattleCardVisualActor* SourceActor, FGuid InstanceId)
	{
		++ActorClickCount;
		LastActorSource = SourceActor;
		LastActorClickedId = InstanceId;
	}

	void HandleActorHovered(AWacomBattleCardVisualActor* SourceActor, FGuid InstanceId)
	{
		++ActorHoverCount;
		LastActorSource = SourceActor;
		LastActorHoveredId = InstanceId;
	}

	void HandleActorUnhovered(AWacomBattleCardVisualActor* SourceActor, FGuid InstanceId)
	{
		++ActorUnhoverCount;
		LastActorSource = SourceActor;
		LastActorUnhoveredId = InstanceId;
	}
};
