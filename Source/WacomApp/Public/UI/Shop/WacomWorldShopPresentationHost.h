// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Components/WacomCursorLookDriverComponent.h"
#include "CoreMinimal.h"
#include "UI/Card/WacomWorldCardInteractionTypes.h"
#include "WacomWorldShopPresentationHost.generated.h"

class AActor;
class UWacomWorldShopOfferAnchorComponent;
class UWidgetComponent;

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
 * World Shop 表现宿主的轻量 C++ 描述。
 *
 * 正式组合 Actor 与 transient Host 都可以提供该描述；Coordinator 只依赖
 * Owner、真实 Anchor 和表现参数，不要求额外 ChildActor 或 Blueprint 规则逻辑。
 */
struct WACOMAPP_API FWacomWorldShopPresentationHost
{
	static FWacomWorldShopPresentationHost Make(
		AActor& InOwner,
		const TArray<UWacomWorldShopOfferAnchorComponent*>& InOfferAnchors,
		FIntPoint InCardDrawSize,
		FVector2D InCardPivot,
		float InCardWorldScale,
		float InInteractionDistance,
		bool bInTwoSided,
		bool bInOverrideCursorLookProfile,
		const FWacomCursorLookProfile& InCursorLookProfileOverride,
		const FWacomWorldCardInteractionStyle& InWorldCardInteractionStyle);

	AActor* GetOwner() const { return Owner.Get(); }
	bool IsOwnedBy(const AActor* Candidate) const
	{
		return Owner.Get() == Candidate;
	}
	bool IsSet() const { return Owner.IsValid(); }
	void Reset();

	TArray<UWacomWorldShopOfferAnchorComponent*>
		GetEnabledOfferAnchorsSorted() const;
	FWacomWorldShopHostValidationResult ValidateForOfferCount(
		int32 OfferCount) const;

	/**
	 * 把统一 DrawSize、Pivot、TwoSided 与绝对世界缩放应用到已附着并注册的
	 * 世界商品 WidgetComponent。父 Actor 的非均匀缩放不得改变卡牌尺寸。
	 */
	void ApplyCardWidgetGeometry(UWidgetComponent& Component) const;

	FIntPoint CardDrawSize = FIntPoint(720, 976);
	FVector2D CardPivot = FVector2D(0.5f, 0.5f);
	float CardWorldScale = 0.10f;
	float InteractionDistance = 2000.0f;
	bool bTwoSided = true;
	bool bOverrideCursorLookProfile = false;
	FWacomCursorLookProfile CursorLookProfileOverride;
	FWacomWorldCardInteractionStyle WorldCardInteractionStyle;

private:
	TWeakObjectPtr<AActor> Owner;
	TArray<TWeakObjectPtr<UWacomWorldShopOfferAnchorComponent>>
		OfferAnchors;
};
