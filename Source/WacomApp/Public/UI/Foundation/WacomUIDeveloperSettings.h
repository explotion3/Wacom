// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
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
	UPROPERTY(EditAnywhere, Category = "Wacom|UI", meta = (Categories = "UI.Widget", ToolTip = "UI Widget 身份 tag；注册表 tag 统一放在 UI.Widget.* 下。"))
	FGameplayTag WidgetTag;

	/** UIManager 在请求该 tag 时同步加载的 Widget 软类。 */
	UPROPERTY(EditAnywhere, Category = "Wacom|UI", meta = (ToolTip = "UIManager 在请求该 tag 时同步加载的 Widget 软类。"))
	TSoftClassPtr<UWacomActivatableWidget> WidgetClass;
};

/** Wacom UI Foundation 的项目级软类注册表。 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Wacom UI Settings"))
class WACOMAPP_API UWacomUIDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** PrimaryLayout WBP 类；必须继承 UWacomPrimaryGameLayout 并提供所需 CommonUI Layer Stack。 */
	UPROPERTY(Config, EditAnywhere, Category = "Wacom|UI", meta = (ToolTip = "PrimaryLayout WBP 类；必须继承 UWacomPrimaryGameLayout 并提供所需 CommonUI Layer Stack。"))
	TSoftClassPtr<UWacomPrimaryGameLayout> PrimaryLayoutClass;

	/** GameplayTag 到 Widget WBP 类的注册表；Widget tag 必须放在 UI.Widget.* 下。 */
	UPROPERTY(Config, EditAnywhere, Category = "Wacom|UI", meta = (ToolTip = "GameplayTag 到 Widget WBP 类的注册表；Widget tag 必须放在 UI.Widget.* 下。"))
	TArray<FWacomUIWidgetClassEntry> WidgetClasses;

	/** AppToast WBP 类；未配置或加载失败时回退到现有路径，再回退到 C++ 类。 */
	UPROPERTY(Config, EditAnywhere, Category = "Wacom|UI", meta = (ToolTip = "AppToast WBP 类；未配置或加载失败时回退到现有路径，再回退到 C++ 类。"))
	TSoftClassPtr<UWacomAppToastWidget> AppToastWidgetClass;
};
