// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Templates/SubclassOf.h"
#include "WacomMenuGameMode.generated.h"

class UWacomMainMenuScreen;
enum class EWacomMainMenuAction : uint8;
struct FWacomMainMenuViewData;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomMenuTravelDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	FName RequestedLevelName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	FName TravelLevelName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	FName Reason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	FString WorldName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	bool bIsPIEWorld = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	bool bRequestedObjectPath = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	bool bTravelTargetUsesPackagePath = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	bool bPrimaryLayoutTeardownRequested = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	bool bPrimaryLayoutTeardownCompleted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	bool bTravelScheduledForNextTick = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	bool bTravelExecuted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	bool bActualTravelSuppressedForAutomation = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	bool bStartNewGameSaveCleanupAttempted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	int32 TeardownOrder = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	int32 ScheduleOrder = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|GameFlow")
	int32 ExecuteOrder = 0;
};

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

	/** 主菜单 Screen 类，Push 到 GameMenu 层。默认 `UWacomMainMenuScreen`。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|UI", meta = (ToolTip = "L_MainMenu Push 到 GameMenu layer 的主菜单 Screen 类。必须继承 UWacomMainMenuScreen；正式 WBP 可以在这里覆盖。"))
	TSubclassOf<UWacomMainMenuScreen> MainMenuScreenClass;

	/**
	 * 开新游戏：存档启用时清存档，然后拆 UI 并在下一帧切到探索关。
	 * 由 MainMenuScreen 的按钮回调调用——让 GameMode 控制切关卡更可靠。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void RequestStartNewGame();

	/** 继续游戏：拆 UI 并在下一帧切到探索关（读档由 AWacomGameMode::BeginPlay 处理）。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void RequestContinueGame();

	/** 关卡 package path。默认指向 L_Exploration；可在 Blueprint 中覆盖，禁止使用 ObjectPath 后缀。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|GameFlow")
	FName ExplorationLevelName = FName(TEXT("/Game/Wacom/Maps/L_Exploration"));

	UFUNCTION(BlueprintPure, Category = "Wacom|GameFlow")
	FWacomMenuTravelDebugView GetLastMenuTravelDebugView() const { return LastMenuTravelDebugView; }

	UFUNCTION(BlueprintPure, Category = "Wacom|GameFlow")
	FString GetMenuTravelDebugSummary() const;

#if WITH_AUTOMATION_TESTS
	void SetSuppressActualTravelForAutomation(bool bSuppress)
	{
		bSuppressActualTravelForAutomation = bSuppress;
	}

	void FlushPendingTravelForAutomation()
	{
		ExecutePendingTravel();
	}
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	static FName NormalizeLevelPackagePath(FName LevelName);
	static bool IsObjectPathLevelName(FName LevelName);
	static bool IsPackagePathLevelName(FName LevelName);

	void RequestTravelToLevel(FName LevelName, FName Reason);
	void ExecutePendingTravel();
	FWacomMainMenuViewData BuildMainMenuViewData() const;
	void BindMainMenuScreen(UWacomMainMenuScreen* Screen);
	void UnbindMainMenuScreen();
	void HandleMainMenuAction(EWacomMainMenuAction Action);
	void RequestQuitGame();

	UPROPERTY(Transient)
	FWacomMenuTravelDebugView LastMenuTravelDebugView;

	UPROPERTY(Transient)
	FName PendingTravelLevelName = NAME_None;

	UPROPERTY(Transient)
	FName PendingTravelReason = NAME_None;

	UPROPERTY(Transient)
	TWeakObjectPtr<UWacomMainMenuScreen> ActiveMainMenuScreen;

	int32 LastMenuTravelOrderCounter = 0;
	bool bSuppressActualTravelForAutomation = false;
};
