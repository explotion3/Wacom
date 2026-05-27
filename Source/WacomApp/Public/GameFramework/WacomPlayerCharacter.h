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
class UWacomRunTunnelMovementComponent;
struct FInputActionValue;

/**
 * Wacom 第一人称玩家 Pawn。
 *
 * 职责：
 *   - 第一人称摄像机（跟随 Controller 旋转）
 *   - Run Tunnel 探索移动输入（通过 IA_Move / IA_Look）
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
	UWacomRunTunnelMovementComponent* GetRunTunnelMovementComponent() const { return RunTunnelMovementComponent; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Components")
	UWacomCursorLookDriverComponent* GetCursorLookDriverComponent() const { return CursorLookDriverComponent; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Components")
	UWacomBattleCameraLookComponent* GetBattleCameraLookComponent() const { return BattleCameraLookComponent; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Components")
	UWacomFirstPersonCardAnchorComponent* GetFirstPersonCardAnchorComponent() const { return FirstPersonCardAnchorComponent; }

	/**
	 * 禁用 / 启用探索期的移动 + 视角输入。
	 * 战斗开始时 SetExplorationInputEnabled(false)，结束时恢复。
	 * 不 UnPossess，也不移除 IMC，保持摄像机静止即可。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Input")
	void SetExplorationInputEnabled(bool bEnabled);

	// ---- Input Actions（ConstructorHelpers 填默认值，BP 可覆盖）----

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_Look;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** IA_Move 回调：WASD 产生 Vector2D，交给 Run Tunnel movement。 */
	void HandleMoveInput(const FInputActionValue& Value);

	/** IA_Look 回调：鼠标产生 Vector2D，交给 Run Tunnel movement。 */
	void HandleLookInput(const FInputActionValue& Value);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Camera",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCamera = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "Run Tunnel movement driver component. Tune movement values on the component's Wacom|Run Tunnel|Movement properties."))
	TObjectPtr<UWacomRunTunnelMovementComponent> RunTunnelMovementComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "Shared driver that converts the visible mouse cursor position into a smoothed camera look offset."))
	TObjectPtr<UWacomCursorLookDriverComponent> CursorLookDriverComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "Battle camera look component. It applies the shared cursor look offset while battle is active."))
	TObjectPtr<UWacomBattleCameraLookComponent> BattleCameraLookComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "First-person card anchor component. It projects future HUD-rendered card slots from Run Tunnel or Battle camera base transforms."))
	TObjectPtr<UWacomFirstPersonCardAnchorComponent> FirstPersonCardAnchorComponent = nullptr;

	/** 当前是否接受探索输入。战斗期间置 false。 */
	bool bExplorationInputEnabled = true;
	bool bLoggedMissingRunTunnelMovementForMove = false;
	bool bLoggedMissingRunTunnelMovementForLook = false;
};
