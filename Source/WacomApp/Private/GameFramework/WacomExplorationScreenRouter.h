// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AWacomPlayerController;
class UWacomRunEventDefinition;
struct FRunShopOfferInput;

/**
 * 探索期 GameMenu 界面的私有打开流程 helper。
 *
 * 只协调 PlayerController / RunSession / UIManager，不暴露 Blueprint。
 * PlayerController 仍保留 public 请求入口和输入绑定。
 */
struct FWacomExplorationScreenRouter
{
	static void OpenBackpack(AWacomPlayerController& PC);
	static bool OpenShop(AWacomPlayerController& PC, FName ShopId, const TArray<FRunShopOfferInput>& Offers);
	static bool OpenRunEvent(AWacomPlayerController& PC, FName PersistentId, UWacomRunEventDefinition* EventDefinition);
};
