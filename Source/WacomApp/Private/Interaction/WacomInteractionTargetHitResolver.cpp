// Copyright Wacom. All Rights Reserved.

#include "Interaction/WacomInteractionTargetHitResolver.h"

#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "GameFramework/Actor.h"
#include "Interaction/WacomInteractionTargetProvider.h"

namespace WacomInteractionTargetHitResolver
{
	FWacomInteractionTargetHandle BuildWorldTargetHandleFromHit(const FHitResult& HitResult)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!HitActor)
		{
			return FWacomInteractionTargetHandle();
		}

		TArray<UActorComponent*> Components;
		HitActor->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (IWacomInteractionTargetProvider* Provider = Cast<IWacomInteractionTargetProvider>(Component))
			{
				FWacomInteractionTargetHandle Handle = Provider->BuildWorldTargetHandle();
				if (Handle.IsValid())
				{
					if (HitResult.HasValidHitObjectHandle() || HitResult.Location != FVector::ZeroVector)
					{
						Handle.WorldLocation = HitResult.Location;
					}
					return Handle;
				}
			}
		}

		return FWacomInteractionTargetHandle();
	}
}
