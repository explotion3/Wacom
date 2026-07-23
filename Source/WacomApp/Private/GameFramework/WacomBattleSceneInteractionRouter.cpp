// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomBattleSceneInteractionRouter.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/HitResult.h"
#include "GameFramework/WacomPlayerController.h"
#include "Interaction/WacomInteractionTargetHitResolver.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/BattleHUD.h"

namespace
{
	FString GetBattleSceneDebugObjectName(const UObject* Object)
	{
		return IsValid(Object) ? Object->GetName() : TEXT("None");
	}

	bool IsBattleEnemyPartWorldTargetHandle(const FWacomInteractionTargetHandle& Handle)
	{
		return Handle.IsValid()
			&& Handle.TargetKind == EWacomInteractionTargetKind::World
			&& Handle.TargetTag == WacomTags::Interaction_Target_Battle_EnemyPart
			&& Handle.HasBattlePartSlotIdentity();
	}
}

FWacomBattleSceneInteractionRouter::FWacomBattleSceneInteractionRouter(
	AWacomPlayerController& InPlayerController)
	: PlayerController(InPlayerController)
{
}

bool FWacomBattleSceneInteractionRouter::TryRouteTargetClick(bool bRequireTargetSelect)
{
	UBattleHUD* HUD = nullptr;
	if (!PlayerController.CanRouteBattleSceneTargetClick(HUD))
	{
		if (PlayerController.bLogBattleSceneTargetClickRouting)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomBattleSceneClickRouter] NoRoute reason=CannotRouteBattleSceneTargetClick requireTargetSelect=%s"),
				bRequireTargetSelect ? TEXT("true") : TEXT("false"));
		}
		return false;
	}
	if (bRequireTargetSelect && (!HUD || !HUD->IsInTargetSelect()))
	{
		if (PlayerController.bLogBattleSceneTargetClickRouting)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomBattleSceneClickRouter] NoRoute reason=NotInTargetSelect hud=%s requireTargetSelect=%s"),
				*GetBattleSceneDebugObjectName(HUD),
				bRequireTargetSelect ? TEXT("true") : TEXT("false"));
		}
		return false;
	}

	FHitResult HitResult;
	if (!PlayerController.BuildBattleSceneClickHitResult(HitResult))
	{
		if (PlayerController.bLogBattleSceneTargetClickRouting)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomBattleSceneClickRouter] NoRoute reason=NoVisibilityHit hud=%s inTargetSelect=%s"),
				*GetBattleSceneDebugObjectName(HUD),
				HUD && HUD->IsInTargetSelect() ? TEXT("true") : TEXT("false"));
		}
		return false;
	}

	const FWacomInteractionTargetHandle Handle =
		WacomInteractionTargetHitResolver::BuildWorldTargetHandleFromHit(HitResult);
	if (IsBattleEnemyPartWorldTargetHandle(Handle))
	{
		if (!HUD || !HUD->IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(Handle))
		{
			if (PlayerController.bLogBattleSceneTargetClickRouting)
			{
				UE_LOG(LogTemp, Display,
					TEXT("[WacomBattleSceneClickRouter] NoRoute reason=NotInCurrentSceneEnemyHostRegistry handle=%s inTargetSelect=%s"),
					*Handle.ToString(),
					HUD && HUD->IsInTargetSelect() ? TEXT("true") : TEXT("false"));
			}
			return false;
		}
		HUD->OnEnemyPartClickedByUser(Handle);
		if (PlayerController.bLogBattleSceneTargetClickRouting)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomBattleSceneClickRouter] RouteViaProvider handle=%s inTargetSelect=%s"),
				*Handle.ToString(),
				HUD && HUD->IsInTargetSelect() ? TEXT("true") : TEXT("false"));
		}
		return true;
	}

	if (PlayerController.bLogBattleSceneTargetClickRouting)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[WacomBattleSceneClickRouter] NoRoute handleValid=%s hitActor=%s"),
			Handle.IsValid() ? TEXT("true") : TEXT("false"),
			*GetBattleSceneDebugObjectName(HitResult.GetActor()));
	}
	return false;
}

bool FWacomBattleSceneInteractionRouter::TryProbeInteractionTarget(
	FWacomInteractionTargetHandle& OutHandle) const
{
	OutHandle = FWacomInteractionTargetHandle();

	UBattleHUD* HUD = nullptr;
	if (!PlayerController.CanRouteBattleSceneTargetClick(HUD))
	{
		return false;
	}

	FHitResult HitResult;
	if (!PlayerController.BuildBattleSceneClickHitResult(HitResult))
	{
		return false;
	}

	OutHandle = WacomInteractionTargetHitResolver::BuildWorldTargetHandleFromHit(HitResult);
	if (!IsBattleEnemyPartWorldTargetHandle(OutHandle)
		|| !HUD->IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(OutHandle))
	{
		OutHandle = FWacomInteractionTargetHandle();
		return false;
	}
	if (OutHandle.IsValid())
	{
		OutHandle.WorldLocation = HitResult.Location;
		FVector2D ScreenPosition = FVector2D::ZeroVector;
		if (PlayerController.ProjectWorldLocationToScreen(HitResult.Location, ScreenPosition))
		{
			const float ViewportScale =
				FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(&PlayerController));
			OutHandle.ScreenPosition = ScreenPosition / ViewportScale;
		}
	}
	return OutHandle.IsValid();
}

bool FWacomBattleSceneInteractionRouter::TryProbeInteractionTargetAtWidgetPosition(
	const FVector2D& WidgetPosition,
	FWacomInteractionTargetHandle& OutHandle) const
{
	OutHandle = FWacomInteractionTargetHandle();

	UBattleHUD* HUD = nullptr;
	if (!PlayerController.CanRouteBattleSceneTargetClick(HUD))
	{
		return false;
	}

	FHitResult HitResult;
	if (!PlayerController.BuildBattleSceneInteractionTargetHitResultAtWidgetPosition(
		WidgetPosition,
		HitResult))
	{
		return false;
	}

	OutHandle = WacomInteractionTargetHitResolver::BuildWorldTargetHandleFromHit(HitResult);
	if (!IsBattleEnemyPartWorldTargetHandle(OutHandle)
		|| !HUD->IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(OutHandle))
	{
		OutHandle = FWacomInteractionTargetHandle();
		return false;
	}
	if (OutHandle.IsValid())
	{
		OutHandle.WorldLocation = HitResult.Location;
		OutHandle.ScreenPosition = WidgetPosition;
	}
	return OutHandle.IsValid();
}
