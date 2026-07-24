// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "WacomCardFaceTypes.generated.h"

class UTexture2D;

/** 生成卡面表现时使用的语境；不会复制或改变卡牌实例身份。 */
UENUM(BlueprintType)
enum class EWacomCardFaceContext : uint8
{
	Battle UMETA(DisplayName = "Battle"),
	Run UMETA(DisplayName = "Run")
};

/** Run Face 提交后需要由探索事务解析的目标类型。 */
UENUM(BlueprintType)
enum class EWacomRunCardTargetMode : uint8
{
	None UMETA(DisplayName = "None"),
	Self UMETA(DisplayName = "Self"),
	WorldTarget UMETA(DisplayName = "World Target"),
	Route UMETA(DisplayName = "Route")
};

/** Run Face 成功使用后，未来由 Run/Room 事务执行的卡牌处置语义。 */
UENUM(BlueprintType)
enum class EWacomRunCardUseDisposition : uint8
{
	Retain UMETA(DisplayName = "Retain"),
	ExhaustForCurrentRoom UMETA(DisplayName = "Exhaust For Current Room"),
	ExhaustUntilCamp UMETA(DisplayName = "Exhaust Until Camp"),
	ConsumePermanently UMETA(DisplayName = "Consume Permanently")
};

/** Run Face 的唯一主动作。动作的具体规则由后续 Room/目标事务解释。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomRunCardActionDefinition
{
	GENERATED_BODY()

	/** 探索动作标签。必须属于 Run.Card.Action.*。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Run Face",
		meta = (ToolTip = "探索动作标签。必须属于 Run.Card.Action.*，用于让目标或 Room 事务解释这张卡的探索能力。"))
	FGameplayTag ActionTag;

	/** 动作强度。单位由具体动作合同解释；正式配置必须大于零。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Run Face",
		meta = (ClampMin = "1", ToolTip = "探索动作强度。单位由具体 ActionTag 的目标或 Room 事务解释；正式配置必须为正整数。"))
	int32 Magnitude = 1;
};

/**
 * 同一卡牌定义的 Run 表面静态合同。
 *
 * 本结构不保存 AP、压力或其它 Room 成本，也不执行耗尽；这些职责属于后续 Run 事务。
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomRunCardFaceDefinition
{
	GENERATED_BODY()

	/** 是否正式启用 Run Face。关闭时旧卡牌可以继续只使用 Battle Face。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Run Face",
		meta = (ToolTip = "是否启用这张卡的探索表面。关闭时 Builder 返回安全禁用卡面，Validator 跳过 Run Face 配置检查。"))
	bool bEnabled = false;

	/** 可选探索名称；为空时回退到共享 DisplayName。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Run Face",
		meta = (ToolTip = "可选的探索表面名称。为空时使用 CardDefinition 的共享 DisplayName。"))
	FText DisplayNameOverride;

	/** 探索表面的说明文本。启用 Run Face 后必须填写。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Run Face",
		meta = (MultiLine = true, ToolTip = "探索表面的说明文本。只描述探索用途，不应复制战斗 Effects 或 Passives；启用后必须非空。"))
	FText Description;

	/** 可选探索插画；为空时回退到共享 CardIllustration。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Run Face",
		meta = (ToolTip = "可选的探索表面插画。为空时使用 CardDefinition 的共享 CardIllustration。"))
	TObjectPtr<UTexture2D> IllustrationOverride = nullptr;

	/** 可选探索插画深度图；为空时回退到共享 CardIllustrationDepthMap。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Run Face",
		meta = (ToolTip = "可选的探索插画局部深度图。为空时使用共享 CardIllustrationDepthMap；推荐使用 Masks、sRGB=false、Nearest、NoMipmaps。"))
	TObjectPtr<UTexture2D> IllustrationDepthMapOverride = nullptr;

	/** 探索动作需要的目标类型。启用后不能为 None。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Run Face",
		meta = (ToolTip = "探索动作需要的目标类型。启用 Run Face 后不能为 None；目标合法性由后续 Room/世界交互事务判断。"))
	EWacomRunCardTargetMode TargetMode = EWacomRunCardTargetMode::WorldTarget;

	/** 唯一主动作，避免拖卡到目标后再次要求玩家选择动作。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Run Face",
		meta = (ToolTip = "这张 Run Face 的唯一主动作。动作标签与强度由目标或 Room 事务解释。"))
	FWacomRunCardActionDefinition PrimaryAction;

	/** 成功使用后的处置语义；本轮只保存静态合同，不创建运行态耗尽状态。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Run Face",
		meta = (ToolTip = "成功使用后的卡牌处置语义。默认当前 Room 内耗尽；本轮只保存静态合同，不执行运行态处置。"))
	EWacomRunCardUseDisposition UseDisposition =
		EWacomRunCardUseDisposition::ExhaustForCurrentRoom;
};
