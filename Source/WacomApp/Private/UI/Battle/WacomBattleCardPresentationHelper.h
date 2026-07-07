// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Resolution/BattleCardTargetPreview.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

struct FHandCardSnapshot;

struct FWacomBattleCardTargetPreviewPresentationStateKey
{
	int32 SnapshotVersion = INDEX_NONE;
	EWacomBattleCardPreviewTargetKind TargetKind =
		EWacomBattleCardPreviewTargetKind::None;
	FGuid SourceCardInstanceId;
	FGuid TargetEnemyPartInstanceId;
	FBattlePartSlotIdentity TargetEnemyPartIdentity;
	FBattleEnemyPartKey TargetEnemyPartKey;
	FGuid TargetHandCardInstanceId;
	uint32 PreviewFactsHash = 0;
};

struct FWacomBattleCardTargetPreviewPresentation
{
	bool bHasPreview = false;
	FWacomBattleCardTargetPreviewPresentationStateKey StateKey;
	FGuid SourceCardInstanceId;
	FGuid TargetHandCardInstanceId;
	TArray<FWacomFirstPersonCardLayerEntry> CardLayerEntries;
	bool bHasSourceCardDetailViewData = false;
	FWacomCardDetailViewData SourceCardDetailViewData;
	bool bHasTargetHandCardDetailViewData = false;
	FWacomCardDetailViewData TargetHandCardDetailViewData;
};

namespace WacomBattleCardPresentation
{
	FWacomCardPresentationRuntimeContext BuildRuntimeContext(const FHandCardSnapshot& CardSnapshot);
	FWacomCardPresentationRuntimeContext BuildRuntimeContext(
		const FHandCardSnapshot& CardSnapshot,
		const FBattleCardTargetPreview& TargetPreview);
	FWacomCardViewData BuildCardViewData(const FHandCardSnapshot& CardSnapshot);
	FWacomCardViewData BuildCardViewData(
		const FHandCardSnapshot& CardSnapshot,
		const FBattleCardTargetPreview& TargetPreview);
	FWacomCardDetailViewData BuildCardDetailViewData(const FHandCardSnapshot& CardSnapshot);
	FWacomCardDetailViewData BuildCardDetailViewData(
		const FHandCardSnapshot& CardSnapshot,
		const FBattleCardTargetPreview& TargetPreview);
	EWacomFirstPersonCardInteractionIntent ResolveCardLayerInteractionIntent(
		const FHandCardSnapshot& CardSnapshot);
	FWacomFirstPersonCardLayerEntry BuildCardLayerEntry(const FHandCardSnapshot& CardSnapshot);
	FWacomFirstPersonCardLayerEntry BuildCardLayerEntry(
		const FHandCardSnapshot& CardSnapshot,
		const FBattleCardTargetPreview& TargetPreview);
	TArray<FWacomFirstPersonCardLayerEntry> BuildCardLayerEntries(const FBattleSnapshot& Snapshot);
	TArray<FWacomFirstPersonCardLayerEntry> BuildCardLayerEntries(
		const FBattleSnapshot& Snapshot,
		const FBattleCardTargetPreview& TargetPreview);
	uint32 HashTargetPreviewFacts(const FBattleCardTargetPreview& TargetPreview);
	FWacomBattleCardTargetPreviewPresentationStateKey BuildTargetPreviewStateKey(
		int32 SnapshotVersion,
		const FBattleCardTargetPreview& TargetPreview);
	bool AreTargetPreviewStateKeysEquivalent(
		const FWacomBattleCardTargetPreviewPresentationStateKey& Left,
		const FWacomBattleCardTargetPreviewPresentationStateKey& Right);
	FWacomBattleCardTargetPreviewPresentation BuildTargetPreviewPresentation(
		const FBattleSnapshot& Snapshot,
		const FBattleCardTargetPreview& TargetPreview);
}
