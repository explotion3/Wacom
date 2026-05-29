// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "CameraCursorLookSpecReceiver.generated.h"

UCLASS()
class UWacomBattleCameraLookSpecProbeComponent : public UWacomBattleCameraLookComponent
{
	GENERATED_BODY()

public:
	FVector2D ForcedNormalizedCursor = FVector2D::ZeroVector;

	void TickForTest(float DeltaTime)
	{
		TickComponent(DeltaTime, LEVELTICK_All, nullptr);
	}

protected:
	virtual void UpdateCursorLookOffset(float DeltaTime) override
	{
		if (HasCursorLookOverrideForTest())
		{
			Super::UpdateCursorLookOffset(DeltaTime);
			return;
		}

		const AWacomPlayerCharacter* Character = Cast<AWacomPlayerCharacter>(GetOwner());
		if (UWacomCursorLookDriverComponent* Driver = Character ? Character->GetCursorLookDriverComponent() : nullptr)
		{
			Driver->UpdateFromNormalizedCursor(
				ForcedNormalizedCursor,
				DeltaTime,
				YawClampDegrees,
				PitchClampDegrees,
				LookYawScale,
				LookPitchScale,
				LookInterpSpeed);
		}
	}
};
