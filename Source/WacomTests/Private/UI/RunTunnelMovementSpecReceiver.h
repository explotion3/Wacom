// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "InputActionValue.h"
#include "RunTunnelMovementSpecReceiver.generated.h"

UCLASS()
class AWacomRunTunnelMovementCharacterProbe : public AWacomPlayerCharacter
{
	GENERATED_BODY()

public:
	void HandleMoveInputForTest(const FVector2D& Input)
	{
		HandleMoveInput(FInputActionValue(Input));
	}

	void HandleLookInputForTest(const FVector2D& Input)
	{
		HandleLookInput(FInputActionValue(Input));
	}
};
