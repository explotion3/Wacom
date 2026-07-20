// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AWacomPlayerController;
class UWacomRunEventDefinition;
struct FWacomFirstPersonViewStageRequest;
struct FRunShopOfferInput;
struct FRunShopVisitRequest;

/**
 * 探索期 GameMenu 界面的私有打开流程 helper。
 *
 * 只协调 PlayerController / RunSession / UIManager，不暴露 Blueprint。
 * PlayerController 仍保留 public 请求入口和输入绑定。
 */
struct FWacomExplorationScreenRouter
{
	static void OpenBackpack(AWacomPlayerController& PC);
	static bool ToggleMap(AWacomPlayerController& PC);
	static void TogglePauseMenu(AWacomPlayerController& PC);
	static void CloseTopGameMenu(AWacomPlayerController& PC);
	static bool OpenShop(AWacomPlayerController& PC, FName ShopId, const TArray<FRunShopOfferInput>& Offers);
	static bool OpenShop(
		AWacomPlayerController& PC,
		FName ShopId,
		const TArray<FRunShopOfferInput>& Offers,
		const FWacomFirstPersonViewStageRequest& StageRequest);
	static bool OpenShop(AWacomPlayerController& PC, const FRunShopVisitRequest& Request);
	static bool OpenShop(
		AWacomPlayerController& PC,
		const FRunShopVisitRequest& Request,
		const FWacomFirstPersonViewStageRequest& StageRequest);
	static bool OpenRunEvent(AWacomPlayerController& PC, FName PersistentId, UWacomRunEventDefinition* EventDefinition);
	static bool OpenRunEvent(
		AWacomPlayerController& PC,
		FName PersistentId,
		UWacomRunEventDefinition* EventDefinition,
		const FWacomFirstPersonViewStageRequest& StageRequest);
};
