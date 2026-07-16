// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleSceneEnemyAuthoringHelpers.h"

#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"

#define LOCTEXT_NAMESPACE "WacomBattleSceneEnemyAuthoringHelpers"

namespace WacomBattleSceneEnemyAuthoring
{
	namespace
	{
		FString JoinNames(const TArray<FName>& Names, const TCHAR* Separator)
		{
			TArray<FString> Strings;
			Strings.Reserve(Names.Num());
			for (const FName& Name : Names)
			{
				Strings.Add(Name.ToString());
			}
			return FString::Join(Strings, Separator);
		}

		FName BuildDefinitionPartSlotId(const FEnemyPartSlot& PartSlot)
		{
			return PartSlot.PartSlotId;
		}

		TSet<FName> BuildDefinitionPartIdSet(const UEnemyDefinition* EnemyDefinition)
		{
			TSet<FName> PartIds;
			if (!EnemyDefinition)
			{
				return PartIds;
			}

			for (const FEnemyPartSlot& PartSlot : EnemyDefinition->Parts)
			{
				if (PartSlot.PartDef && !PartSlot.PartDef->PartId.IsNone())
				{
					PartIds.Add(PartSlot.PartDef->PartId);
				}
			}
			return PartIds;
		}

		TSet<FName> BuildDefinitionPartSlotIdSet(const UEnemyDefinition* EnemyDefinition)
		{
			TSet<FName> PartSlotIds;
			if (!EnemyDefinition)
			{
				return PartSlotIds;
			}

			for (const FEnemyPartSlot& PartSlot : EnemyDefinition->Parts)
			{
				const FName PartSlotId = BuildDefinitionPartSlotId(PartSlot);
				if (!PartSlotId.IsNone())
				{
					PartSlotIds.Add(PartSlotId);
				}
			}
			return PartSlotIds;
		}

		TMap<FName, FName> BuildDefinitionPartIdsBySlotId(
			const UEnemyDefinition* EnemyDefinition)
		{
			TMap<FName, FName> PartIdsBySlotId;
			if (!EnemyDefinition)
			{
				return PartIdsBySlotId;
			}

			for (const FEnemyPartSlot& PartSlot : EnemyDefinition->Parts)
			{
				if (!PartSlot.PartSlotId.IsNone()
					&& PartSlot.PartDef
					&& !PartSlot.PartDef->PartId.IsNone())
				{
					PartIdsBySlotId.FindOrAdd(
						PartSlot.PartSlotId,
						PartSlot.PartDef->PartId);
				}
			}
			return PartIdsBySlotId;
		}

		TArray<FName> BuildConfiguredPartIds(const TArray<AWacomBattleEnemyPartActor*>& PartActors)
		{
			TArray<FName> PartIds;
			for (const AWacomBattleEnemyPartActor* PartActor : PartActors)
			{
				if (PartActor && !PartActor->GetEffectivePartDefinitionId().IsNone())
				{
					PartIds.AddUnique(PartActor->GetEffectivePartDefinitionId());
				}
			}
			return PartIds;
		}

		TArray<FName> BuildConfiguredPartSlotIds(const TArray<AWacomBattleEnemyPartActor*>& PartActors)
		{
			TArray<FName> PartSlotIds;
			for (const AWacomBattleEnemyPartActor* PartActor : PartActors)
			{
				if (PartActor && !PartActor->GetEffectivePartSlotId().IsNone())
				{
					PartSlotIds.AddUnique(PartActor->GetEffectivePartSlotId());
				}
			}
			return PartSlotIds;
		}

		TArray<FName> BuildDuplicateConfiguredPartSlotIds(const TArray<AWacomBattleEnemyPartActor*>& PartActors)
		{
			TArray<FName> DuplicatePartSlotIds;
			TSet<FName> SeenPartSlotIds;

			for (const AWacomBattleEnemyPartActor* PartActor : PartActors)
			{
				if (!PartActor)
				{
					continue;
				}

				const FName PartSlotId = PartActor->GetEffectivePartSlotId();
				if (PartSlotId.IsNone())
				{
					continue;
				}

				if (SeenPartSlotIds.Contains(PartSlotId))
				{
					DuplicatePartSlotIds.AddUnique(PartSlotId);
				}
				else
				{
					SeenPartSlotIds.Add(PartSlotId);
				}
			}
			return DuplicatePartSlotIds;
		}

		bool HasAnyNonPositiveExtent(const FVector& Extent)
		{
			return Extent.X <= 0.0f || Extent.Y <= 0.0f || Extent.Z <= 0.0f;
		}

		bool HasAnyZeroScaleAxis(const FVector& Scale)
		{
			return FMath::IsNearlyZero(Scale.X)
				|| FMath::IsNearlyZero(Scale.Y)
				|| FMath::IsNearlyZero(Scale.Z);
		}

		bool VisualLayerHasAsset(const FWacomBattleEnemyPartVisualLayer& Layer)
		{
			switch (Layer.LayerMode)
			{
			case EWacomBattleEnemyPartVisualLayerMode::Flipbook:
				return Layer.Flipbook != nullptr;
			case EWacomBattleEnemyPartVisualLayerMode::StaticSprite:
			default:
				return Layer.Sprite != nullptr;
			}
		}

		const TCHAR* GetVisualLayerModeDebugName(EWacomBattleEnemyPartVisualLayerMode LayerMode)
		{
			switch (LayerMode)
			{
			case EWacomBattleEnemyPartVisualLayerMode::Flipbook:
				return TEXT("Flipbook");
			case EWacomBattleEnemyPartVisualLayerMode::StaticSprite:
			default:
				return TEXT("StaticSprite");
			}
		}

		bool HasHostVisualContext(const AWacomBattleEnemyPartActor& PartActor)
		{
			if (PartActor.IsHostVisualContextActive())
			{
				return true;
			}

			const AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(PartActor.GetAttachParentActor());
			return Host && Host->IsHostVisualActive();
		}

#if WITH_EDITOR
		EDataValidationResult KeepInvalidResult(EDataValidationResult Result)
		{
			return Result == EDataValidationResult::Invalid
				? EDataValidationResult::Invalid
				: EDataValidationResult::Valid;
		}
#endif
	}

	bool ShouldValidateHostPlacementActor(const AWacomBattleEnemyActor& EnemyActor)
	{
		return !EnemyActor.HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
			&& !EnemyActor.IsTemplate();
	}

	bool ShouldValidatePartPlacementActor(const AWacomBattleEnemyPartActor& PartActor)
	{
		return !PartActor.HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
			&& !PartActor.IsTemplate();
	}

	const TCHAR* GetHostVisualModeDebugString(EWacomBattleEnemyHostVisualMode VisualMode)
	{
		switch (VisualMode)
		{
		case EWacomBattleEnemyHostVisualMode::Flipbook:
			return TEXT("Flipbook");
		case EWacomBattleEnemyHostVisualMode::StaticSprite:
		default:
			return TEXT("StaticSprite");
		}
	}

	const TCHAR* GetHostAuthoringModeDebugString(
		EWacomBattleEnemyHostAuthoringMode AuthoringMode)
	{
		switch (AuthoringMode)
		{
		case EWacomBattleEnemyHostAuthoringMode::MultiPartVisualLayers:
			return TEXT("MultiPartVisualLayers");
		case EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual:
		default:
			return TEXT("SimpleHostVisual");
		}
	}

	TMap<FName, int32> BuildDefinitionPartOrder(const UEnemyDefinition* EnemyDefinition)
	{
		TMap<FName, int32> PartOrder;
		if (!EnemyDefinition)
		{
			return PartOrder;
		}

		for (int32 Index = 0; Index < EnemyDefinition->Parts.Num(); ++Index)
		{
			const FEnemyPartSlot& PartSlot = EnemyDefinition->Parts[Index];
			if (PartSlot.PartDef && !PartSlot.PartDef->PartId.IsNone())
			{
				PartOrder.FindOrAdd(PartSlot.PartDef->PartId, Index);
			}
		}
		return PartOrder;
	}

	FWacomBattleSceneEnemyHostIdentityAudit BuildHostPartIdentityAudit(
		const UEnemyDefinition* EnemyDefinition,
		const TArray<AWacomBattleEnemyPartActor*>& PartActors)
	{
		FWacomBattleSceneEnemyHostIdentityAudit Audit;
		if (!EnemyDefinition)
		{
			Audit.DuplicatePartSlotIds = BuildDuplicateConfiguredPartSlotIds(PartActors);
			for (const AWacomBattleEnemyPartActor* PartActor : PartActors)
			{
				if (PartActor)
				{
					Audit.SurplusPartActorNames.Add(PartActor->GetName());
				}
			}
			return Audit;
		}

		const TArray<FName> ConfiguredPartIds = BuildConfiguredPartIds(PartActors);
		const TArray<FName> ConfiguredPartSlotIds = BuildConfiguredPartSlotIds(PartActors);
		const TSet<FName> DefinitionPartIds = BuildDefinitionPartIdSet(EnemyDefinition);
		const TSet<FName> DefinitionPartSlotIds = BuildDefinitionPartSlotIdSet(EnemyDefinition);
		const TMap<FName, FName> DefinitionPartIdsBySlotId =
			BuildDefinitionPartIdsBySlotId(EnemyDefinition);

		for (const FName& PartId : ConfiguredPartIds)
		{
			if (!DefinitionPartIds.Contains(PartId))
			{
				Audit.UnknownPartIds.AddUnique(PartId);
			}
		}

		for (const FName& PartSlotId : ConfiguredPartSlotIds)
		{
			if (!DefinitionPartSlotIds.Contains(PartSlotId))
			{
				Audit.UnknownPartSlotIds.AddUnique(PartSlotId);
			}
		}

		for (const FEnemyPartSlot& PartSlot : EnemyDefinition->Parts)
		{
			if (PartSlot.PartDef && !PartSlot.PartDef->PartId.IsNone()
				&& !ConfiguredPartIds.Contains(PartSlot.PartDef->PartId))
			{
				Audit.MissingDefinitionPartIds.AddUnique(PartSlot.PartDef->PartId);
			}

			const FName PartSlotId = BuildDefinitionPartSlotId(PartSlot);
			if (!PartSlotId.IsNone() && !ConfiguredPartSlotIds.Contains(PartSlotId))
			{
				Audit.MissingDefinitionPartSlotIds.AddUnique(PartSlotId);
			}
		}

		Audit.DuplicatePartSlotIds = BuildDuplicateConfiguredPartSlotIds(PartActors);
		TSet<FName> ClaimedPartSlotIds;
		for (const AWacomBattleEnemyPartActor* PartActor : PartActors)
		{
			if (!PartActor)
			{
				continue;
			}

			const FName PartSlotId = PartActor->GetEffectivePartSlotId();
			const FName* ExpectedPartId = DefinitionPartIdsBySlotId.Find(PartSlotId);
			if (PartSlotId.IsNone()
				|| !ExpectedPartId
				|| ClaimedPartSlotIds.Contains(PartSlotId))
			{
				Audit.SurplusPartActorNames.Add(PartActor->GetName());
				continue;
			}

			ClaimedPartSlotIds.Add(PartSlotId);
			if (PartActor->GetEffectivePartDefinitionId() != *ExpectedPartId)
			{
				Audit.PartDefinitionMismatchSlotIds.Add(PartSlotId);
			}
		}
		return Audit;
	}

	FName BuildHostAuthoringStateName(
		const UEnemyDefinition* EnemyDefinition,
		int32 PartActorCount,
		const FWacomBattleSceneEnemyHostIdentityAudit& Audit)
	{
		if (!EnemyDefinition)
		{
			return TEXT("MissingEnemyDefinition");
		}
		if (PartActorCount <= 0)
		{
			return TEXT("NoPartActors");
		}
		if (Audit.DuplicatePartSlotIds.Num() > 0)
		{
			return TEXT("DuplicatePartSlotIds");
		}
		if (Audit.UnknownPartSlotIds.Num() > 0 || Audit.MissingDefinitionPartSlotIds.Num() > 0)
		{
			return TEXT("PartSlotMismatch");
		}
		if (Audit.UnknownPartIds.Num() > 0
			|| Audit.MissingDefinitionPartIds.Num() > 0
			|| Audit.PartDefinitionMismatchSlotIds.Num() > 0)
		{
			return TEXT("PartDefinitionMismatch");
		}
		return TEXT("Ready");
	}

	FString FormatHostDebugSummary(const FWacomBattleSceneEnemyDebugView& View)
	{
		return FString::Printf(
			TEXT("BattleSceneEnemy{Actor=%s Definition=%s EnemyId=%s EnemySlotId=%s AuthoringMode=%s HostVisualMode=%s UsingHostVisual=%s HostVisualAsset=%s HostAnimationStyle=%s CurrentHostAnimationClip=%s CurrentHostAnimationIntent=%s HostAnimationActive=%s HostAnimationTerminal=%s HostAnimationPlayCount=%d HostAnimationWatchdogCompletions=%d GeneratedHostVisualComponents=%d RegisteredHostVisualComponents=%d VisibleHostVisualComponents=%d AuthoringState=%s AuthoringReady=%s PartCount=%d BoundParts=%d UnboundParts=%d RuntimeFacts=%d RuntimeInitiativeTotal=%d HoveredParts=%d PredictionVisibleParts=%d BadgeLayoutAppliedParts=%d UsedByBattleHUD=%s ActiveBattleHUD=%s PartIds=[%s] PartSlotIds=[%s] StableSceneTargets=[%s] UnknownPartIds=[%s] UnknownPartSlotIds=[%s] MissingDefinitionPartIds=[%s] MissingDefinitionPartSlotIds=[%s] DuplicatePartSlotIds=[%s] PartDefinitionMismatchSlotIds=[%s] SurplusPartActors=[%s]}"),
			*View.ActorName,
			*View.EnemyDefinitionName.ToString(),
			*View.EnemyId.ToString(),
			*View.EnemySlotId.ToString(),
			*View.AuthoringMode.ToString(),
			*View.HostVisualMode.ToString(),
			View.bUsingHostVisual ? TEXT("true") : TEXT("false"),
			*View.HostVisualAssetName.ToString(),
			*View.HostAnimationStyleAssetName.ToString(),
			*View.CurrentHostAnimationClipName.ToString(),
			*View.CurrentHostAnimationIntentId.ToString(),
			View.bHostAnimationPlaybackActive ? TEXT("true") : TEXT("false"),
			View.bHostAnimationTerminalState ? TEXT("true") : TEXT("false"),
			View.HostAnimationPlayCount,
			View.HostAnimationWatchdogCompletionCount,
			View.GeneratedHostVisualComponentCount,
			View.RegisteredHostVisualComponentCount,
			View.VisibleHostVisualComponentCount,
			*View.AuthoringState.ToString(),
			View.bAuthoringReady ? TEXT("true") : TEXT("false"),
			View.AttachedPartActorCount,
			View.BoundPartActorCount,
			View.UnboundPartActorCount,
			View.RuntimeFactsPartActorCount,
			View.RuntimeInitiativeTotal,
			View.HoveredPartActorCount,
			View.PredictionVisiblePartActorCount,
			View.BadgeLayoutAppliedPartActorCount,
			View.bUsedByBattleHUD ? TEXT("true") : TEXT("false"),
			*View.ActiveBattleHUDName,
			*JoinNames(View.AttachedPartIds, TEXT(",")),
			*JoinNames(View.AttachedPartSlotIds, TEXT(",")),
			*JoinNames(View.StableSceneTargetIds, TEXT(",")),
			*JoinNames(View.UnknownPartIds, TEXT(",")),
			*JoinNames(View.UnknownPartSlotIds, TEXT(",")),
			*JoinNames(View.MissingDefinitionPartIds, TEXT(",")),
			*JoinNames(View.MissingDefinitionPartSlotIds, TEXT(",")),
			*JoinNames(View.DuplicatePartSlotIds, TEXT(",")),
			*JoinNames(View.PartDefinitionMismatchSlotIds, TEXT(",")),
			*FString::Join(View.SurplusPartActorNames, TEXT(",")));
	}

	FName BuildVisualAuthoringModeName(
		const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers,
		bool bHostVisualContextActive)
	{
		if (VisualLayers.Num() > 0)
		{
			return TEXT("VisualLayers");
		}
		if (bHostVisualContextActive)
		{
			return TEXT("HitOnly");
		}
		return TEXT("None");
	}

	FName BuildPartAuthoringStateName(
		FName PartId,
		FName PartSlotId,
		const FVector& HitBoundsExtent,
		FName VisualAuthoringMode)
	{
		if (PartId.IsNone() || PartSlotId.IsNone())
		{
			return TEXT("MissingIdentity");
		}
		if (HasAnyNonPositiveExtent(HitBoundsExtent))
		{
			return TEXT("InvalidHitBounds");
		}
		if (VisualAuthoringMode == FName(TEXT("VisualLayers")))
		{
			return TEXT("UsingVisualLayers");
		}
		if (VisualAuthoringMode == FName(TEXT("HitOnly")))
		{
			return TEXT("HitOnly");
		}
		return TEXT("MissingVisualResource");
	}

	FString FormatPartDebugSummary(const FWacomBattleSceneEnemyPartDebugView& View)
	{
		const FWacomBattleEnemyPartWorldTargetDebugView& BridgeView = View.BridgeDebugView;
		const FWacomBattleEnemyPartPresentationDebugView& PresentationView = View.PresentationDebugView;
		return FString::Printf(
			TEXT("BattleSceneEnemyPart{Actor=%s EnemySlotId=%s PartSlotId=%s StableSceneTargetId=%s PartId=%s AuthoringState=%s AuthoringReady=%s VisualAuthoringMode=%s UsingHostVisual=%s HitOnlyVisual=%s HitBounds=%s UsingVisualLayers=%s VisualLayerCount=%d GeneratedVisualLayerComponents=%d GeneratedStaticVisualLayerComponents=%d GeneratedFlipbookVisualLayerComponents=%d RegisteredVisualLayerComponents=%d RegisteredStaticVisualLayerComponents=%d RegisteredFlipbookVisualLayerComponents=%d VisibleVisualLayerComponents=%d VisibleStaticVisualLayerComponents=%d VisibleFlipbookVisualLayerComponents=%d MissingVisualLayerAssets=%d MissingVisualLayerSprites=%d MissingVisualLayerFlipbooks=%d VisualLayerIds=%s VisualLayerAssets=%s FeedbackTarget=%s ImpactAnchorReady=%s ImpactAnchor=%s ImpactAnchorWorld=%s PredictionWidget=%s PredictionBadgeLocation=%s PredictionBadgeDrawSize=%s PredictionBadgeScale=%.2f PredictionBadgeZOffset=%.1f BadgeStaggerIndex=%d BadgeStaggerOffset=%s InteractionConfigured=%s InteractionTargetId=%s InteractionStableId=%s BridgePartId=%s Bound=%s Registered=%s RuntimeFacts=%s Initiative=%d Destroyed=%s Intent=%s Targetable=%s LastBind=%s LastCue=%s CueType=%d CueAmount=%d CueCount=%d ActiveCue=%s CueActive=%s CueProgress=%.3f CueDuration=%.3f DragPreview=%d DragPreviewActive=%s DragSource=%s DragCost=%d DragSwift=%s DragCanSubmit=%s DragReject=%s HoverActive=%s HoverReason=%s HoverStableId=%s HoverWorldTargetId=%s HoverScreen=%s PredictionVisible=%s PredictionMode=%d PredictedInitiative=%d PerfectCandidate=%s ActionRisk=%s PredictionReject=%s PredictionBadgeOffsetActive=%s}"),
			*View.ActorName,
			*View.EnemySlotId.ToString(),
			*View.PartSlotId.ToString(),
			*View.StableSceneTargetId.ToString(),
			*View.PartId.ToString(),
			*View.AuthoringState.ToString(),
			View.bAuthoringReady ? TEXT("true") : TEXT("false"),
			*View.VisualAuthoringMode.ToString(),
			View.bUsingHostVisual ? TEXT("true") : TEXT("false"),
			View.bHitOnlyVisual ? TEXT("true") : TEXT("false"),
			*View.HitBoundsExtent.ToCompactString(),
			View.bUsingVisualLayers ? TEXT("true") : TEXT("false"),
			View.VisualLayerCount,
			View.GeneratedVisualLayerComponentCount,
			View.GeneratedStaticVisualLayerComponentCount,
			View.GeneratedFlipbookVisualLayerComponentCount,
			View.RegisteredVisualLayerComponentCount,
			View.RegisteredStaticVisualLayerComponentCount,
			View.RegisteredFlipbookVisualLayerComponentCount,
			View.VisibleVisualLayerComponentCount,
			View.VisibleStaticVisualLayerComponentCount,
			View.VisibleFlipbookVisualLayerComponentCount,
			View.MissingVisualLayerAssetCount,
			View.MissingVisualLayerSpriteCount,
			View.MissingVisualLayerFlipbookCount,
			*JoinNames(View.VisualLayerIds, TEXT("|")),
			*JoinNames(View.VisualLayerAssetNames, TEXT("|")),
			*View.FeedbackTargetName.ToString(),
			View.bImpactAnchorReady ? TEXT("true") : TEXT("false"),
			*View.ImpactAnchorName.ToString(),
			*View.ImpactAnchorWorldLocation.ToCompactString(),
			*View.PredictionWidgetName.ToString(),
			*View.PredictionBadgeRelativeLocation.ToCompactString(),
			*View.PredictionBadgeDrawSize.ToString(),
			View.PredictionBadgeScale,
			View.PredictionBadgeZOffsetWhenVisible,
			View.BadgeLayoutStaggerIndex,
			*View.BadgeLayoutStaggerOffset.ToCompactString(),
			View.bInteractionTargetConfigured ? TEXT("true") : TEXT("false"),
			*View.InteractionTargetId.ToString(),
			*View.InteractionTargetStableId.ToString(),
			*BridgeView.PartId.ToString(),
			BridgeView.bBoundToSnapshot ? TEXT("true") : TEXT("false"),
			BridgeView.bRegisteredWithBattleHUD ? TEXT("true") : TEXT("false"),
			PresentationView.bHasRuntimePartFacts ? TEXT("true") : TEXT("false"),
			PresentationView.CurrentInitiative,
			PresentationView.bRuntimePartDestroyed ? TEXT("true") : TEXT("false"),
			*PresentationView.CurrentIntentId.ToString(),
			BridgeView.bTargetable ? TEXT("true") : TEXT("false"),
			*BridgeView.LastBindResult.ToString(),
			*PresentationView.LastCueKind.ToString(),
			static_cast<int32>(PresentationView.LastCueType),
			PresentationView.LastCueAmount,
			PresentationView.CuePlayCount,
			*PresentationView.ActiveCueKind.ToString(),
			PresentationView.bCuePlaybackActive ? TEXT("true") : TEXT("false"),
			PresentationView.CuePlaybackProgress,
			PresentationView.CuePlaybackDurationSeconds,
			static_cast<int32>(PresentationView.DragPreviewState),
			PresentationView.bDragPreviewActive ? TEXT("true") : TEXT("false"),
			*PresentationView.LastDragPredictionDebugInput.SourceCardInstanceId.ToString(
				EGuidFormats::DigitsWithHyphens),
			PresentationView.LastDragPredictionDebugInput.SourceCardRuntimeCost,
			PresentationView.LastDragPredictionDebugInput.bSourceCardSwift ? TEXT("true") : TEXT("false"),
			PresentationView.LastDragPredictionDebugInput.bPreviewCanSubmit ? TEXT("true") : TEXT("false"),
			*PresentationView.LastDragPredictionDebugInput.PreviewRejectReason.ToString(),
			PresentationView.bHoverActive ? TEXT("true") : TEXT("false"),
			*PresentationView.HoverReason.ToString(),
			*PresentationView.HoverStableId.ToString(),
			*PresentationView.HoverWorldTargetId.ToString(EGuidFormats::DigitsWithHyphens),
			*PresentationView.HoverScreenPosition.ToString(),
			PresentationView.PredictionView.bVisible ? TEXT("true") : TEXT("false"),
			static_cast<int32>(PresentationView.PredictionView.Mode),
			PresentationView.PredictionView.PredictedInitiative,
			PresentationView.PredictionView.bPerfectReleaseCandidate ? TEXT("true") : TEXT("false"),
			PresentationView.PredictionView.bActionRisk ? TEXT("true") : TEXT("false"),
			*PresentationView.PredictionView.RejectReason.ToString(),
			PresentationView.bPredictionBadgeOffsetActive ? TEXT("true") : TEXT("false"));
	}
#if WITH_EDITOR
	EDataValidationResult ValidateHostPlacement(
		const AWacomBattleEnemyActor& EnemyActor,
		FDataValidationContext& Context,
		EDataValidationResult BaseResult)
	{
		EDataValidationResult Result = BaseResult;
		if (!ShouldValidateHostPlacementActor(EnemyActor))
		{
			return KeepInvalidResult(Result);
		}

		const FWacomBattleSceneEnemyHostAuthoringReport Report =
			FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(EnemyActor);
		const FWacomBattleSceneEnemyHostIdentityAudit& Audit = Report.IdentityAudit;
		if (!EnemyActor.EnemyDefinition)
		{
			Context.AddError(FText::Format(
				LOCTEXT("PlacementMissingEnemyDefinition",
					"BattleEnemy Host 摆放配置错误：Actor={0} 缺少 EnemyDefinition；无法校验子 PartActor 的 PartId / PartSlotId。"),
				FText::FromString(EnemyActor.GetName())));
			Result = EDataValidationResult::Invalid;
		}

		if (Report.PartActorCount == 0)
		{
			Context.AddError(FText::Format(
				LOCTEXT("PlacementNoAttachedParts",
					"BattleEnemy Host 摆放配置错误：Actor={0} 没有配置任何 BattleEnemyPartActor；无法注册场景敌人部位目标。"),
				FText::FromString(EnemyActor.GetName())));
			Result = EDataValidationResult::Invalid;
		}

		if (Audit.DuplicatePartSlotIds.Num() > 0)
		{
			Context.AddError(FText::Format(
				LOCTEXT("PlacementDuplicatePartSlotIds",
					"BattleEnemy Host 摆放配置错误：Actor={0} 下有重复 PartSlotId：{1}。显式同步会保留重复 Actor 并标记为 surplus，不会静默删除；请人工确认保留项。"),
				FText::FromString(EnemyActor.GetName()),
				FText::FromString(JoinNames(Audit.DuplicatePartSlotIds, TEXT(",")))));
			Result = EDataValidationResult::Invalid;
		}

		if (Audit.PartDefinitionMismatchSlotIds.Num() > 0)
		{
			Context.AddWarning(FText::Format(
				LOCTEXT("PlacementPartDefinitionMismatchSlotIds",
					"BattleEnemy Host 摆放警告：Actor={0} 的这些 PartSlotId 已匹配定义，但 PartId 与对应 PartDefinition.PartId 不一致：{1}。在 Host Details 执行“从 EnemyDefinition 同步部位”可安全派生并修正 PartId。"),
				FText::FromString(EnemyActor.GetName()),
				FText::FromString(JoinNames(Audit.PartDefinitionMismatchSlotIds, TEXT(",")))));
			Result = KeepInvalidResult(Result);
		}

		if (Audit.SurplusPartActorNames.Num() > 0)
		{
			Context.AddWarning(FText::Format(
				LOCTEXT("PlacementSurplusPartActors",
					"BattleEnemy Host 摆放警告：Actor={0} 有未唯一匹配 EnemyDefinition.PartSlotId 的 surplus PartActor：{1}。显式同步会保留它们，需人工确认后再删除。"),
				FText::FromString(EnemyActor.GetName()),
				FText::FromString(FString::Join(Audit.SurplusPartActorNames, TEXT(",")))));
			Result = KeepInvalidResult(Result);
		}

		if (EnemyActor.EnemyDefinition && Audit.UnknownPartIds.Num() > 0)
		{
			Context.AddWarning(FText::Format(
				LOCTEXT("PlacementUnknownPartIds",
					"BattleEnemy Host 摆放警告：Actor={0} EnemyDefinition={1} 下有未在定义中声明的 PartId：{2}。已知 PartSlotId 可通过 Host Details 同步操作从 PartDefinition 自动派生。"),
				FText::FromString(EnemyActor.GetName()),
				FText::FromString(EnemyActor.EnemyDefinition->GetName()),
				FText::FromString(JoinNames(Audit.UnknownPartIds, TEXT(",")))));
			Result = KeepInvalidResult(Result);
		}

		if (EnemyActor.EnemyDefinition && Audit.UnknownPartSlotIds.Num() > 0)
		{
			Context.AddWarning(FText::Format(
				LOCTEXT("PlacementUnknownPartSlotIds",
					"BattleEnemy Host 摆放警告：Actor={0} EnemyDefinition={1} 下有未在定义中声明的 PartSlotId：{2}。请确认 Host 子 PartActor 的 PartSlotId 是否对应 EnemyDefinition.Parts[].PartSlotId。"),
				FText::FromString(EnemyActor.GetName()),
				FText::FromString(EnemyActor.EnemyDefinition->GetName()),
				FText::FromString(JoinNames(Audit.UnknownPartSlotIds, TEXT(",")))));
			Result = KeepInvalidResult(Result);
		}

		if (EnemyActor.EnemyDefinition && Audit.MissingDefinitionPartIds.Num() > 0)
		{
			Context.AddWarning(FText::Format(
				LOCTEXT("PlacementMissingDefinitionPartIds",
					"BattleEnemy Host 摆放警告：Actor={0} EnemyDefinition={1} 中有未映射到 Host 的 PartId：{2}。"),
				FText::FromString(EnemyActor.GetName()),
				FText::FromString(EnemyActor.EnemyDefinition->GetName()),
				FText::FromString(JoinNames(Audit.MissingDefinitionPartIds, TEXT(",")))));
			Result = KeepInvalidResult(Result);
		}

		if (EnemyActor.EnemyDefinition && Audit.MissingDefinitionPartSlotIds.Num() > 0)
		{
			Context.AddWarning(FText::Format(
				LOCTEXT("PlacementMissingDefinitionPartSlotIds",
					"BattleEnemy Host 摆放警告：Actor={0} EnemyDefinition={1} 中有未映射到 Host 的 PartSlotId：{2}。在 Host Details 执行“从 EnemyDefinition 同步部位”可自动创建缺失部位；创建前对应槽位无法绑定场景目标。"),
				FText::FromString(EnemyActor.GetName()),
				FText::FromString(EnemyActor.EnemyDefinition->GetName()),
				FText::FromString(JoinNames(Audit.MissingDefinitionPartSlotIds, TEXT(",")))));
			Result = KeepInvalidResult(Result);
		}

		if (EnemyActor.HostAuthoringMode ==
			EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual)
		{
			if (!Report.bUsingHostVisual)
			{
				Context.AddWarning(FText::Format(
					LOCTEXT("PlacementSimpleModeMissingHostVisual",
						"BattleEnemy Host 摆放警告：Actor={0} 使用 SimpleHostVisual 制作模式，但没有可见 Host 整体视觉；敌人只有命中体和调试信息可见。"),
					FText::FromString(EnemyActor.GetName())));
				Result = KeepInvalidResult(Result);
			}
		}
		else
		{
			if (!Report.MissingVisualLayerPartSlotIds.IsEmpty())
			{
				Context.AddWarning(FText::Format(
					LOCTEXT("PlacementMultiPartModeMissingVisualLayers",
						"BattleEnemy Host 摆放警告：Actor={0} 使用 MultiPartVisualLayers 制作模式，但这些部位尚未配置 VisualLayers：{1}。同步不会覆盖已有视觉层。"),
					FText::FromString(EnemyActor.GetName()),
					FText::FromString(JoinNames(
						Report.MissingVisualLayerPartSlotIds,
						TEXT(",")))));
				Result = KeepInvalidResult(Result);
			}
		}

		if (!Report.bHostAnimationStyleApplicable)
		{
			Context.AddWarning(FText::Format(
				LOCTEXT("PlacementHostAnimationStyleWithoutFlipbookHost",
					"BattleEnemy Host 摆放警告：Actor={0} 配置了 HostAnimationStyle，但不是可见的 SimpleHostVisual Flipbook Host。语义动画请求会立即跳过；请改为 SimpleHostVisual + Flipbook 并配置 HostFlipbook，或移除 Style。"),
				FText::FromString(EnemyActor.GetName())));
			Result = KeepInvalidResult(Result);
		}

		return KeepInvalidResult(Result);
	}

	EDataValidationResult ValidatePartPlacement(
		const AWacomBattleEnemyPartActor& PartActor,
		FDataValidationContext& Context,
		EDataValidationResult BaseResult)
	{
		EDataValidationResult Result = BaseResult;
		if (!ShouldValidatePartPlacementActor(PartActor))
		{
			return KeepInvalidResult(Result);
		}

		if (PartActor.PartId.IsNone() || PartActor.GetEffectivePartSlotId().IsNone())
		{
			Context.AddError(FText::Format(
				LOCTEXT("PlacementMissingPartId",
					"BattleEnemyPart 摆放配置错误：Actor={0} 缺少 PartId 或 PartSlotId。"),
				FText::FromString(PartActor.GetName())));
			Result = EDataValidationResult::Invalid;
		}

		if (HasAnyNonPositiveExtent(PartActor.HitBoundsExtent))
		{
			Context.AddError(FText::Format(
				LOCTEXT("PlacementInvalidHitBounds",
					"BattleEnemyPart 摆放配置错误：Actor={0} PartId={1} HitBoundsExtent 必须全部大于 0，当前为 {2}。"),
				FText::FromString(PartActor.GetName()),
				FText::FromName(PartActor.GetEffectivePartDefinitionId()),
				FText::FromString(PartActor.HitBoundsExtent.ToCompactString())));
			Result = EDataValidationResult::Invalid;
		}

		TSet<FName> UsedLayerIds;
		for (int32 LayerIndex = 0; LayerIndex < PartActor.VisualLayers.Num(); ++LayerIndex)
		{
			const FWacomBattleEnemyPartVisualLayer& Layer = PartActor.VisualLayers[LayerIndex];
			if (Layer.LayerId.IsNone())
			{
				Context.AddError(FText::Format(
					LOCTEXT("PlacementVisualLayerMissingId",
						"BattleEnemyPart 摆放配置错误：Actor={0} VisualLayers[{1}] 缺少 LayerId。"),
					FText::FromString(PartActor.GetName()),
					FText::AsNumber(LayerIndex)));
				Result = EDataValidationResult::Invalid;
			}
			else if (UsedLayerIds.Contains(Layer.LayerId))
			{
				Context.AddError(FText::Format(
					LOCTEXT("PlacementVisualLayerDuplicateId",
						"BattleEnemyPart 摆放配置错误：Actor={0} VisualLayers 中 LayerId={1} 重复。"),
					FText::FromString(PartActor.GetName()),
					FText::FromName(Layer.LayerId)));
				Result = EDataValidationResult::Invalid;
			}
			else
			{
				UsedLayerIds.Add(Layer.LayerId);
			}

			if (HasAnyZeroScaleAxis(Layer.RelativeScale3D))
			{
				Context.AddError(FText::Format(
					LOCTEXT("PlacementVisualLayerInvalidScale",
						"BattleEnemyPart 摆放配置错误：Actor={0} VisualLayers[{1}] RelativeScale3D 任一轴不能为 0，当前为 {2}。"),
					FText::FromString(PartActor.GetName()),
					FText::AsNumber(LayerIndex),
					FText::FromString(Layer.RelativeScale3D.ToCompactString())));
				Result = EDataValidationResult::Invalid;
			}

			if (!VisualLayerHasAsset(Layer))
			{
				Context.AddWarning(FText::Format(
					LOCTEXT("PlacementVisualLayerMissingAsset",
						"BattleEnemyPart 摆放警告：Actor={0} VisualLayers[{1}] Mode={2} 缺少对应视觉资源；该层不会生成可见组件。"),
					FText::FromString(PartActor.GetName()),
					FText::AsNumber(LayerIndex),
					FText::FromString(GetVisualLayerModeDebugName(Layer.LayerMode))));
			}
		}

		if (PartActor.VisualLayers.Num() == 0 && !HasHostVisualContext(PartActor))
		{
			Context.AddWarning(FText::Format(
				LOCTEXT("PlacementMissingVisualResource",
					"BattleEnemyPart 摆放警告：Actor={0} 没有 VisualLayers，也没有 Host 整体视觉；该部位只有命中体和调试信息可见。"),
				FText::FromString(PartActor.GetName())));
		}

		return KeepInvalidResult(Result);
	}
#endif
}

#undef LOCTEXT_NAMESPACE
