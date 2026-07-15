// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WacomPlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UWacomBattleCameraLookComponent;
class UWacomCursorLookDriverComponent;
class UWacomFirstPersonCardAnchorComponent;
class UWacomFirstPersonViewStageBlendComponent;
class UWacomFirstPersonWalkBobComponent;
class UWacomRunPathTraversalComponent;
struct FInputActionValue;

/**
 * Wacom 第一人称玩家 Pawn。
 *
 * 职责：
 *   - 第一人称摄像机（跟随 Controller 旋转）
	 *   - Run Path 探索移动输入（通过 IA_Move / IA_Look）
 *   - 战斗时保留 Possess 状态，只禁用移动输入
 *
 * 输入资产默认通过 ConstructorHelpers 挂载，Blueprint 可重写。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AWacomPlayerCharacter();

	/** 第一人称摄像机。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Camera")
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Components")
	UWacomRunPathTraversalComponent* GetRunPathTraversalComponent() const { return RunPathTraversalComponent; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Components")
	UWacomCursorLookDriverComponent* GetCursorLookDriverComponent() const { return CursorLookDriverComponent; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Components")
	UWacomBattleCameraLookComponent* GetBattleCameraLookComponent() const { return BattleCameraLookComponent; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Components")
	UWacomFirstPersonCardAnchorComponent* GetFirstPersonCardAnchorComponent() const { return FirstPersonCardAnchorComponent; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Components")
	UWacomFirstPersonViewStageBlendComponent* GetFirstPersonViewStageBlendComponent() const { return FirstPersonViewStageBlendComponent; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Components")
	UWacomFirstPersonWalkBobComponent* GetWalkBobComponent() const { return WalkBobComponent; }

	/**
	 * 禁用 / 启用探索期的移动 + 视角输入。
	 * 战斗开始时 SetExplorationInputEnabled(false)，结束时恢复。
	 * 不 UnPossess，也不移除 IMC，保持摄像机静止即可。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Input")
	void SetExplorationInputEnabled(bool bEnabled);

	void SetExplorationInputEnabled(bool bEnabled, bool bPreserveCursorLookOffset);

	// ---- Input Actions（ConstructorHelpers 填默认值，BP 可覆盖）----

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_Look;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** IA_Move 回调：WASD 产生 Vector2D，交给 Run Path traversal。 */
	void HandleMoveInput(const FInputActionValue& Value);

	/** IA_Look 回调：鼠标产生 Vector2D，交给 Run Path traversal。 */
	void HandleLookInput(const FInputActionValue& Value);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Camera",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCamera = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "正式 Run Path 局部样条移动与 View Source。只消费场景 Path，不持有 RunSession 或地图规则。"))
	TObjectPtr<UWacomRunPathTraversalComponent> RunPathTraversalComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "Shared driver that converts the visible mouse cursor position into a smoothed camera look offset."))
	TObjectPtr<UWacomCursorLookDriverComponent> CursorLookDriverComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "Battle camera look component. It applies the shared cursor look offset while battle is active."))
	TObjectPtr<UWacomBattleCameraLookComponent> BattleCameraLookComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "第一人称卡牌锚点。它从 Run Path、战斗镜头或临时 View Stage 的基准视角投影 HUD 手牌。"))
	TObjectPtr<UWacomFirstPersonCardAnchorComponent> FirstPersonCardAnchorComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "第一人称镜头站位过渡组件。它会在镜头模式捕获视角前，把摄像机 View Pose 平滑移动到关卡中摆放的站位。"))
	TObjectPtr<UWacomFirstPersonViewStageBlendComponent> FirstPersonViewStageBlendComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "第一人称走路晃动组件。Run Path 会读取它的轻量位置和旋转偏移，用来模拟沿样条移动时的脚步感。"))
	TObjectPtr<UWacomFirstPersonWalkBobComponent> WalkBobComponent = nullptr;

	/** 当前是否接受探索输入。战斗期间置 false。 */
	bool bExplorationInputEnabled = true;
	bool bLoggedInactiveRunPathForMove = false;
	bool bLoggedInactiveRunPathForLook = false;
};
