// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "WacomInteractionTargetTypes.generated.h"

/**
 * 交互目标类型。
 *
 * 区分拖拽/点击系统实际命中了什么：
 *   - World：场景中的 Actor/Component（敌人部位、Run 物体、NPC 等）
 *   - Card：UMG 卡牌槽位（战斗手牌、背包卡牌视图）
 *   - Zone：UMG 区域槽位（背包备战区、负重区等 DropTarget）
 *
 * 当前仅实现 World 命中来源；Card / Zone 的命中来源后续接入。
 */
UENUM(BlueprintType)
enum class EWacomInteractionTargetKind : uint8
{
	None  UMETA(DisplayName = "None"),
	World UMETA(DisplayName = "World"),
	Card  UMETA(DisplayName = "Card"),
	Zone  UMETA(DisplayName = "Zone"),
};

/**
 * 统一交互目标描述结构。
 *
 * 拖拽/点击系统只需要消费这个 struct，不用知道目标来源是 Actor、Component 还是 Widget。
 *
 * 当前定位是纯数据容器，不带行为。命中层（IWacomInteractionTargetProvider）负责构造，
 * 规则层（Target Resolver）负责判断"当前卡能否作用到这个目标"。
 */
USTRUCT(BlueprintType)
struct WACOMCORE_API FWacomInteractionTargetHandle
{
	GENERATED_BODY()

	/** 当前有效的目标类型。None 表示无效 handle。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Interaction|Target")
	EWacomInteractionTargetKind TargetKind = EWacomInteractionTargetKind::None;

	/** World 目标的有效 ID（如 PartInstanceId）。仅 TargetKind == World 时有效。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Interaction|Target")
	FGuid WorldTargetId;

	/** Card 目标的有效 ID（卡牌实例）。仅 TargetKind == Card 时有效。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Interaction|Target")
	FGuid CardInstanceId;

	/** Zone 目标的有效 ID。仅 TargetKind == Zone 时有效。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Interaction|Target")
	FName ZoneId = NAME_None;

	/** 命中来源对象（Actor 或 Component）。弱引用，不阻止 GC。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Interaction|Target")
	TWeakObjectPtr<UObject> SourceObject;

	/** 目标在世界空间的位置（有效时）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Interaction|Target")
	FVector WorldLocation = FVector::ZeroVector;

	/** 目标在屏幕空间的位置（有效时）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Interaction|Target")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	/** 当 TargetKind != None 时为有效 handle。 */
	bool IsValid() const { return TargetKind != EWacomInteractionTargetKind::None; }

	/** 调试用字符串摘要。 */
	FString ToString() const
	{
		const TCHAR* KindStr = TargetKind == EWacomInteractionTargetKind::None ? TEXT("None")
			: TargetKind == EWacomInteractionTargetKind::World ? TEXT("World")
			: TargetKind == EWacomInteractionTargetKind::Card ? TEXT("Card")
			: TEXT("Zone");

		return FString::Printf(TEXT("FWacomInteractionTargetHandle{Kind=%s WorldTargetId=%s CardInstanceId=%s ZoneId=%s}"),
			KindStr,
			*WorldTargetId.ToString(EGuidFormats::DigitsWithHyphens),
			*CardInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
			*ZoneId.ToString());
	}

	/** 构造一个 World 类型的 handle。 */
	static FWacomInteractionTargetHandle ForWorldTarget(const FGuid& InWorldTargetId, UObject* InSourceObject,
		const FVector& InWorldLocation = FVector::ZeroVector, const FVector2D& InScreenPosition = FVector2D::ZeroVector)
	{
		FWacomInteractionTargetHandle Handle;
		Handle.TargetKind = EWacomInteractionTargetKind::World;
		Handle.WorldTargetId = InWorldTargetId;
		Handle.SourceObject = InSourceObject;
		Handle.WorldLocation = InWorldLocation;
		Handle.ScreenPosition = InScreenPosition;
		return Handle;
	}
};
