// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Components/WacomCursorLookDriverComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WacomWorldShopHostActor.generated.h"

class USceneComponent;
class UWacomWorldShopOfferAnchorComponent;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomWorldShopHostValidationResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|World Shop")
	bool bValid = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|World Shop")
	FName FailureReason = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|World Shop")
	int32 EnabledAnchorCount = 0;
};

/**
 * World Shop 的场景表现宿主。默认提供 2×4 商品槽；不拥有库存、购买规则或 Visit。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomWorldShopHostActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomWorldShopHostActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Shop|Widget",
		meta = (ToolTip = "每张世界商品卡的渲染尺寸，单位 Slate 像素。默认 720×976，是 360×488 world-safe 逻辑画布的 2 倍超采样；只影响清晰度和纵横比。"))
	FIntPoint CardDrawSize = FIntPoint(720, 976);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Shop|Widget",
		meta = (ToolTip = "世界商品卡的 Widget Pivot。默认 0.5,0.5 表示 Anchor 对齐卡牌中心。"))
	FVector2D CardPivot = FVector2D(0.5f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Shop|Widget",
		meta = (ToolTip = "超采样 Slate 像素到世界厘米的统一缩放。默认 0.10，使 720×976 RenderTarget 对应约 72×98 厘米；建议 0.06-0.18，只影响实体卡尺寸。"))
	float CardWorldScale = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Shop|Widget",
		meta = (Units = "cm", ToolTip = "鼠标射线与世界卡牌交互的最大距离，单位厘米。推荐 800-3000；必须大于 0。"))
	float InteractionDistance = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Shop|Widget",
		meta = (ToolTip = "世界商品卡是否双面渲染。默认开启，避免 Host 朝向轻微误差导致卡面不可见。"))
	bool bTwoSided = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Shop|Look",
		meta = (ToolTip = "启用后，本 Host 使用下方观察参数；关闭时复制玩家当前 Run Path 的 live 参数。"))
	bool bOverrideCursorLookProfile = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Shop|Look",
		meta = (EditCondition = "bOverrideCursorLookProfile", ToolTip = "本商店的临时 LookOnly 参数。离开后不会写回 Run 或 Battle 组件制作值。"))
	FWacomCursorLookProfile CursorLookProfileOverride;

	UFUNCTION(BlueprintPure, Category = "Wacom|World Shop")
	TArray<UWacomWorldShopOfferAnchorComponent*> GetEnabledOfferAnchorsSorted() const;

	FWacomWorldShopHostValidationResult ValidateForOfferCount(int32 OfferCount) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Wacom|World Shop")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Instanced, Category = "Wacom|World Shop")
	TArray<TObjectPtr<UWacomWorldShopOfferAnchorComponent>> DefaultOfferAnchors;
};
