// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomResult.generated.h"

/**
 * 通用错误码。命令结算、资产查询、手牌腾挪等返回路径共享。
 *
 * 原则：错误码仅用于路径分支（"这步能不能继续"），不承载用户可读文本。
 * 文本消息由上层结合 Context 生成。
 */
UENUM(BlueprintType)
enum class EWacomError : uint8
{
	None                     UMETA(DisplayName = "None"),
	NotFound                 UMETA(DisplayName = "NotFound"),
	InvalidArgument          UMETA(DisplayName = "InvalidArgument"),
	InvalidState             UMETA(DisplayName = "InvalidState"),
	NotEnoughInitiative      UMETA(DisplayName = "NotEnoughInitiative"),      // 卡费用 > 敌方先机总和
	IllegalTarget            UMETA(DisplayName = "IllegalTarget"),
	IllegalCardZone          UMETA(DisplayName = "IllegalCardZone"),
	IllegalHandAnchor        UMETA(DisplayName = "IllegalHandAnchor"),
	SpecialConditionFailed   UMETA(DisplayName = "SpecialConditionFailed"),
	CardForbidden            UMETA(DisplayName = "CardForbidden"),            // 冻结或类似状态
};

/**
 * 通用轻量结果。
 *
 * 用法：
 *   FWacomStatus Status = TryDoThing();
 *   if (!Status.IsOk()) { ... }
 *
 * 与 TOptional / TResult 的取舍：
 * - 反射层需要一个 USTRUCT 返回，自研模板不被反射。
 * - 真需要带 payload 的路径由具体业务结构承载，不走这个通用 Result。
 */
USTRUCT(BlueprintType)
struct WACOMCORE_API FWacomStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Result")
	EWacomError Code = EWacomError::None;

	/** 可选附加标识，用于区分相同 Code 下的子分支。为空表示无附加信息。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Result")
	FName Detail = NAME_None;

	FWacomStatus() = default;
	explicit FWacomStatus(EWacomError InCode, FName InDetail = NAME_None)
		: Code(InCode), Detail(InDetail) {}

	bool IsOk() const { return Code == EWacomError::None; }

	static FWacomStatus Ok() { return FWacomStatus(EWacomError::None); }
	static FWacomStatus Fail(EWacomError InCode, FName InDetail = NAME_None)
	{
		return FWacomStatus(InCode, InDetail);
	}
};
