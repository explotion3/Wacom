// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Templates/SubclassOf.h"
#include "WacomMenuGameMode.generated.h"

class UWacomMenuWidgetBase;

/**
 * 菜单关卡的 GameMode。
 *
 * 和 AWacomGameMode 并列：
 *   - AWacomGameMode    ：L_Exploration / 战斗流
 *   - AWacomMenuGameMode：L_MainMenu / 主菜单（未来也可能是结算、角色选择等菜单关）
 *
 * 职责：
 *   - DefaultPawnClass = nullptr（菜单关不要 Pawn）
 *   - 复用 AWacomPlayerController（它在菜单场下不 push IMC，自行处理）
 *   - BeginPlay：EnsurePrimaryLayout + Push MainMenuScreen + UIOnly 输入 + 鼠标可见
 *   - 不管存档 / 战斗 / Run——这些是 AWacomGameMode 的职责
 *
 * 配置：把 L_MainMenu 的 WorldSettings::GameMode 指向本类，或在关卡 BP 里写死。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWacomMenuGameMode();

	/** 主菜单 Widget 类，Push 到 GameMenu 层。默认 `UWacomMainMenuScreen`。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|UI")
	TSubclassOf<UWacomMenuWidgetBase> MainMenuScreenClass;

	/**
	 * 开新游戏：清存档，TearDown UI，OpenLevel。
	 * 由 MainMenuScreen 的按钮回调调用——让 GameMode 控制切关卡更可靠
	 * （Widget 在 Click 处理链里生命周期敏感，容易被中途销毁）。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void RequestStartNewGame();

	/** 继续游戏：只 TearDown + OpenLevel（读档由 AWacomGameMode::BeginPlay 处理）。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void RequestContinueGame();

	/** 关卡名。默认指向 L_Exploration；可在 Blueprint 中覆盖。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|GameFlow")
	FName ExplorationLevelName = FName(TEXT("/Game/Wacom/Maps/L_Exploration.L_Exploration"));

protected:
	virtual void BeginPlay() override;
};
