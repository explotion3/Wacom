// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "UObject/SoftObjectPtr.h"
#include "WacomUIDeveloperSettings.generated.h"

class UWacomActivatableWidget;
class UWacomAppToastWidget;
class UWacomPrimaryGameLayout;

/** 按 GameplayTag 注册的 UI Widget 软类。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomUIWidgetClassEntry
{
	GENERATED_BODY()

	/** UI Widget 身份 tag；注册表 tag 统一放在 UI.Widget.* 下。 */
	UPROPERTY(EditAnywhere, Category = "Wacom|UI Foundation|Widget Registry", meta = (Categories = "UI.Widget", ToolTip = "项目级 UI Widget 身份 tag；注册表 tag 统一放在 UI.Widget.* 下。它只用于软类解析，不代表运行时状态。"))
	FGameplayTag WidgetTag;

	/** UIManager 在请求该 tag 时同步加载的 Widget 软类。 */
	UPROPERTY(EditAnywhere, Category = "Wacom|UI Foundation|Widget Registry", meta = (ToolTip = "UI manager 在请求该 tag 时加载的 Widget 软类。它是项目级注册表配置，不是运行时创建规则入口。"))
	TSoftClassPtr<UWacomActivatableWidget> WidgetClass;
};

/** Wacom UI Foundation 的项目级软类注册表。 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Wacom UI Settings", ToolTip = "Wacom UI Foundation 的项目级软类注册表。用于配置 PrimaryLayout、顶层 Widget 和 AppToast 的 WBP 覆盖，不保存运行时 UI 状态。"))
class WACOMAPP_API UWacomUIDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	bool ValidateSettings(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/** PrimaryLayout WBP 类；必须继承 UWacomPrimaryGameLayout 并提供所需 CommonUI Layer Stack。 */
	UPROPERTY(Config, EditAnywhere, Category = "Wacom|UI Foundation|Settings", meta = (ToolTip = "PrimaryLayout WBP 类；必须继承 UWacomPrimaryGameLayout 并提供所需 CommonUI Layer Stack。它是 UI shell 软类配置，不是运行时状态来源。"))
	TSoftClassPtr<UWacomPrimaryGameLayout> PrimaryLayoutClass;

	/** GameplayTag 到 Widget WBP 类的注册表；Widget tag 必须放在 UI.Widget.* 下，已知顶层 screen tag 会校验具体 screen 父类。 */
	UPROPERTY(Config, EditAnywhere, Category = "Wacom|UI Foundation|Settings", meta = (ToolTip = "GameplayTag 到 Widget WBP 类的项目级注册表；Widget tag 必须放在 UI.Widget.* 下，已知顶层 screen tag 会校验具体 screen 父类。它只影响类解析。"))
	TArray<FWacomUIWidgetClassEntry> WidgetClasses;

	/** AppToast WBP 类；未配置或加载失败时回退到 C++ 类。 */
	UPROPERTY(Config, EditAnywhere, Category = "Wacom|UI Foundation|Settings", meta = (ToolTip = "AppToast WBP 类；未配置或加载失败时回退到 C++ 类。它只配置 Toast 表现类，不改变 Toast 触发规则。"))
	TSoftClassPtr<UWacomAppToastWidget> AppToastWidgetClass;
};
