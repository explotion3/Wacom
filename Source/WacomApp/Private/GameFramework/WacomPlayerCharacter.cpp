// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"

AWacomPlayerCharacter::AWacomPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// 第一人称：摄像机跟随 Controller 的 Yaw + Pitch，但 Capsule 只跟 Yaw。
	bUseControllerRotationYaw   = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll  = false;

	// 第一人称摄像机：挂在 Capsule 顶部，跟随 Pawn 旋转。
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 60.f)); // 约眼睛高度
	FirstPersonCamera->bUsePawnControlRotation = true;

	CursorLookDriverComponent = CreateDefaultSubobject<UWacomCursorLookDriverComponent>(TEXT("CursorLookDriverComponent"));
	RunTunnelMovementComponent = CreateDefaultSubobject<UWacomRunTunnelMovementComponent>(TEXT("RunTunnelMovementComponent"));
	BattleCameraLookComponent = CreateDefaultSubobject<UWacomBattleCameraLookComponent>(TEXT("BattleCameraLookComponent"));
	FirstPersonCardAnchorComponent = CreateDefaultSubobject<UWacomFirstPersonCardAnchorComponent>(TEXT("FirstPersonCardAnchorComponent"));

	// IA 资产延迟到 BeginPlay 里 LoadObject 解析，避免 CDO 阶段 FObjectFinder
	// 在 commandlet 首次运行前 assets 不存在而崩溃。
}

void AWacomPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 资产加载挪到 SetupPlayerInputComponent 里做，
	// 因为 SetupPlayerInputComponent 在 possession 时被调用，可能早于 BeginPlay。
}

void AWacomPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 此时可能早于 BeginPlay，IA 属性还是 nullptr——本地懒加载。
	if (!IA_Move)
	{
		IA_Move = LoadObject<UInputAction>(nullptr,
			TEXT("/Game/Wacom/Input/IA_Move.IA_Move"));
	}
	if (!IA_Look)
	{
		IA_Look = LoadObject<UInputAction>(nullptr,
			TEXT("/Game/Wacom/Input/IA_Look.IA_Look"));
	}

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomPlayerCharacter] InputComponent is not UEnhancedInputComponent"));
		return;
	}

	if (IA_Move)
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this,
			&AWacomPlayerCharacter::HandleMoveInput);
		EIC->BindAction(IA_Move, ETriggerEvent::Completed, this,
			&AWacomPlayerCharacter::HandleMoveInput);
		EIC->BindAction(IA_Move, ETriggerEvent::Canceled, this,
			&AWacomPlayerCharacter::HandleMoveInput);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomPlayerCharacter] IA_Move 未找到，WASD 移动不可用"));
	}

	if (IA_Look)
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this,
			&AWacomPlayerCharacter::HandleLookInput);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomPlayerCharacter] IA_Look 未找到，鼠标视角不可用"));
	}

	UE_LOG(LogTemp, Display,
		TEXT("[WacomPlayerCharacter] SetupPlayerInputComponent: IA_Move=%s IA_Look=%s"),
		IA_Move ? TEXT("OK") : TEXT("null"),
		IA_Look ? TEXT("OK") : TEXT("null"));
}

void AWacomPlayerCharacter::SetExplorationInputEnabled(bool bEnabled)
{
	if (RunTunnelMovementComponent)
	{
		if (bEnabled)
		{
			RunTunnelMovementComponent->ResumeRunTunnel();
		}
		else
		{
			RunTunnelMovementComponent->SuspendRunTunnel();
		}
	}
	bExplorationInputEnabled = bEnabled;
}

void AWacomPlayerCharacter::HandleMoveInput(const FInputActionValue& Value)
{
	if (!bExplorationInputEnabled || !Controller) { return; }

	const FVector2D Input = Value.Get<FVector2D>();
	if (RunTunnelMovementComponent && RunTunnelMovementComponent->HandleMoveInput(Input))
	{
		return;
	}
	if (!bLoggedMissingRunTunnelMovementForMove)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomPlayerCharacter] Ignored IA_Move because RunTunnelMovement is inactive or missing; normal FPS exploration fallback is disabled."));
		bLoggedMissingRunTunnelMovementForMove = true;
	}
}

void AWacomPlayerCharacter::HandleLookInput(const FInputActionValue& Value)
{
	if (!bExplorationInputEnabled) { return; }

	const FVector2D Input = Value.Get<FVector2D>();
	if (RunTunnelMovementComponent && RunTunnelMovementComponent->HandleLookInput(Input))
	{
		return;
	}
	if (!bLoggedMissingRunTunnelMovementForLook)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomPlayerCharacter] Ignored IA_Look because RunTunnelMovement is inactive or missing; normal FPS exploration fallback is disabled."));
		bLoggedMissingRunTunnelMovementForLook = true;
	}
}
