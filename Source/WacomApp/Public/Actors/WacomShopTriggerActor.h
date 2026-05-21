// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WacomWorldInteractable.h"
#include "RunState.h"
#include "WacomShopTriggerActor.generated.h"

class USphereComponent;

/**
 * 场景中的商店交互触发器。
 *
 * 玩家进入范围后，探索期按 E 打开商店 UI。库存、已购买状态和关闭时节点消耗由 URunSession 管理；
 * 本 Actor 只负责把关卡配置的 ShopId 和 Offers 传入 Run 层。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomShopTriggerActor : public AActor, public IWacomWorldInteractable
{
	GENERATED_BODY()

public:
	AWacomShopTriggerActor();

	/**
	 * 商店节点持久化 ID。当前 Run 内必须唯一。
	 *
	 * RunSession 使用该 ID 保存库存和已购买状态；再次打开同一 ID 会复用旧库存。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop",
		meta = (ToolTip = "商店节点在当前 Run 内的唯一 ID。RunSession 使用它保存库存和已购买状态；None 会拒绝打开商店。"))
	FName PersistentId;

	/**
	 * 关卡中手动配置的商店商品列表。
	 *
	 * 同一个 PersistentId 第一次打开时用本列表初始化库存；之后再次打开会保留已有库存并忽略新的列表。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop",
		meta = (ToolTip = "该商店第一次打开时用于初始化库存的商品列表。每个商品包含卡牌定义和金币价格。"))
	TArray<FRunShopOfferInput> Offers;

	/** 触发半径（cm）。玩家进入该范围后，探索期按 E 可以打开商店。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop",
		meta = (ToolTip = "玩家进入该半径后，探索期按 E 可以打开商店。单位：厘米。建议范围 50-1000。",
			ClampMin = "50.0", UIMin = "50.0", UIMax = "1000.0"))
	float TriggerRadius = 200.f;

	/** 探索 HUD 上显示的交互提示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop",
		meta = (ToolTip = "玩家处于商店交互范围内时显示在探索 HUD 上的提示文本。"))
	FText InteractPromptText;

	UFUNCTION(BlueprintPure, Category = "Wacom|Shop")
	USphereComponent* GetTriggerSphere() const { return TriggerSphere; }

	// ---- IWacomWorldInteractable ----
	virtual FText GetInteractPromptText_Implementation(AWacomPlayerController* PC) const override;
	virtual FVector GetInteractLocation_Implementation(AWacomPlayerController* PC) const override;
	virtual bool CanInteract_Implementation(AWacomPlayerController* PC) const override;
	virtual bool TryInteract_Implementation(AWacomPlayerController* PC) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> TriggerSphere = nullptr;
};
