// Copyright Wacom. All Rights Reserved.

#include "Interaction/WacomInteractionTargetHitResolver.h"

#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "GameFramework/Actor.h"
#include "Interaction/WacomInteractionTargetProvider.h"

namespace WacomInteractionTargetHitResolver
{
	FWacomInteractionTargetHandle BuildWorldTargetHandleFromHit(const FHitResult& HitResult)
	{
		if (UPrimitiveComponent* HitComponent = HitResult.GetComponent())
		{
			if (IWacomInteractionTargetProvider* Provider =
				Cast<IWacomInteractionTargetProvider>(HitComponent))
			{
				FWacomInteractionTargetHandle Handle = Provider->BuildWorldTargetHandle();
				if (Handle.IsValid())
				{
					Handle.WorldLocation = HitResult.Location;
					return Handle;
				}
			}
		}

		AActor* HitActor = HitResult.GetActor();
		if (!HitActor)
		{
			return FWacomInteractionTargetHandle();
		}

		// Actor 级 fallback 只服务没有在实际命中组件上实现 provider 的普通世界目标。
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
