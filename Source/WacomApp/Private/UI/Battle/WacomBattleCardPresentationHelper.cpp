// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleCardPresentationHelper.h"

#include "Cards/CardDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Snapshots/HandSnapshot.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

namespace
{
	bool IsSourcePreviewForCard(
		const FHandCardSnapshot& CardSnapshot,
		const FBattleCardTargetPreview& TargetPreview)
	{
		return TargetPreview.bHasPreview
			&& TargetPreview.SourceCardInstanceId.IsValid()
			&& TargetPreview.SourceCardInstanceId == CardSnapshot.InstanceId;
	}

	bool IsTargetHandCardPreviewForCard(
		const FHandCardSnapshot& CardSnapshot,
		const FBattleCardTargetPreview& TargetPreview)
	{
		return TargetPreview.bHasPreview
			&& TargetPreview.TargetKind == EWacomBattleCardPreviewTargetKind::HandCard
			&& TargetPreview.TargetHandCardInstanceId.IsValid()
			&& TargetPreview.TargetHandCardInstanceId == CardSnapshot.InstanceId;
	}

	uint32 HashBoolForTargetPreviewState(bool bValue)
	{
		return bValue ? 0x9E3779B9u : 0x85EBCA6Bu;
	}

	uint32 HashNameForTargetPreviewState(FName Name)
	{
		return GetTypeHash(Name);
	}

	uint32 HashTagForTargetPreviewState(const FGameplayTag& Tag)
	{
		return HashNameForTargetPreviewState(Tag.GetTagName());
	}

	uint32 HashIntForTargetPreviewState(int32 Value)
	{
		return static_cast<uint32>(Value);
	}

	uint32 HashTargetPreviewEffectState(const FBattleCardTargetPreviewEffect& EffectPreview)
	{
		uint32 Hash = 0xC2B2AE35u;
		Hash = HashCombineFast(Hash, HashIntForTargetPreviewState(EffectPreview.EffectIndex));
		Hash = HashCombineFast(Hash, HashTagForTargetPreviewState(EffectPreview.EffectType));
		Hash = HashCombineFast(Hash, HashTagForTargetPreviewState(EffectPreview.Target));
		Hash = HashCombineFast(Hash, HashBoolForTargetPreviewState(EffectPreview.bSkipped));
		Hash = HashCombineFast(Hash, static_cast<uint32>(EffectPreview.SkipReason));
		Hash = HashCombineFast(Hash, HashBoolForTargetPreviewState(EffectPreview.bHasMagnitude));
		Hash = HashCombineFast(Hash, HashIntForTargetPreviewState(EffectPreview.Magnitude));
		Hash = HashCombineFast(Hash, HashBoolForTargetPreviewState(EffectPreview.bTargetsSelectedHandCard));
		Hash = HashCombineFast(Hash, HashBoolForTargetPreviewState(EffectPreview.bHasTargetHandCardCostPreview));
		Hash = HashCombineFast(Hash, HashIntForTargetPreviewState(EffectPreview.TargetHandCardRuntimeCostBefore));
		Hash = HashCombineFast(Hash, HashIntForTargetPreviewState(EffectPreview.TargetHandCardRuntimeCostAfter));
		Hash = HashCombineFast(Hash, HashBoolForTargetPreviewState(EffectPreview.bWouldDiscardTargetHandCard));
		Hash = HashCombineFast(Hash, HashBoolForTargetPreviewState(EffectPreview.bWouldExhaustTargetHandCard));
		Hash = HashCombineFast(Hash, HashBoolForTargetPreviewState(EffectPreview.bWouldGainTargetHandCardKeyword));
		Hash = HashCombineFast(Hash, HashTagForTargetPreviewState(EffectPreview.TargetHandCardKeyword));
		return Hash;
	}

	EWacomFirstPersonCardInteractionIntent ResolveBattleFirstPersonCardInteractionIntent(
		ECardTargetMode TargetMode)
	{
		switch (TargetMode)
		{
		case ECardTargetMode::SingleEnemyPart:
			return EWacomFirstPersonCardInteractionIntent::AimWorldTarget;
		case ECardTargetMode::HandCard:
			return EWacomFirstPersonCardInteractionIntent::AimCardTarget;
		case ECardTargetMode::None:
		case ECardTargetMode::Self:
		case ECardTargetMode::AllEnemyParts:
		default:
			return EWacomFirstPersonCardInteractionIntent::CommitNoTarget;
		}
	}

	void ApplySourcePreviewToContext(
		const FBattleCardTargetPreview& TargetPreview,
		FWacomCardPresentationRuntimeContext& Context)
	{
		Context.EffectPreviews.Reserve(TargetPreview.Effects.Num());
		for (const FBattleCardTargetPreviewEffect& EffectPreview : TargetPreview.Effects)
		{
			FWacomCardPresentationRuntimeContext::FEffectPreview PresentationEffectPreview;
			PresentationEffectPreview.EffectIndex = EffectPreview.EffectIndex;
			PresentationEffectPreview.bSkip = EffectPreview.bSkipped;
			PresentationEffectPreview.bHasMagnitude = EffectPreview.bHasMagnitude;
			PresentationEffectPreview.Magnitude = EffectPreview.Magnitude;
			Context.EffectPreviews.Add(PresentationEffectPreview);
		}
	}

	void ApplyTargetHandCardPreviewToContext(
		const FBattleCardTargetPreview& TargetPreview,
		FWacomCardPresentationRuntimeContext& Context)
	{
		if (TargetPreview.bHasTargetHandCardCostPreview)
		{
			Context.bHasRuntimeCostPreview = true;
			Context.RuntimeCostPreview = TargetPreview.TargetHandCardRuntimeCostAfter;
		}
	}
}

namespace WacomBattleCardPresentation
{
	FWacomCardPresentationRuntimeContext BuildRuntimeContext(const FHandCardSnapshot& CardSnapshot)
	{
		FWacomCardPresentationRuntimeContext Context;
		Context.bHasRuntimeCost = true;
		Context.RuntimeCost = CardSnapshot.RuntimeCost;
		Context.bHasPlayableState = true;
		Context.bIsPlayable = CardSnapshot.bIsPlayable;
		return Context;
	}

	FWacomCardPresentationRuntimeContext BuildRuntimeContext(
		const FHandCardSnapshot& CardSnapshot,
		const FBattleCardTargetPreview& TargetPreview)
	{
		FWacomCardPresentationRuntimeContext Context = BuildRuntimeContext(CardSnapshot);
		if (IsSourcePreviewForCard(CardSnapshot, TargetPreview))
		{
			ApplySourcePreviewToContext(TargetPreview, Context);
		}
		else if (IsTargetHandCardPreviewForCard(CardSnapshot, TargetPreview))
		{
			ApplyTargetHandCardPreviewToContext(TargetPreview, Context);
		}
		return Context;
	}

	FWacomCardViewData BuildCardViewData(const FHandCardSnapshot& CardSnapshot)
	{
		return UWacomCardPresentationBuilder::BuildCardViewData(
			CardSnapshot.Definition,
			BuildRuntimeContext(CardSnapshot));
	}

	FWacomCardViewData BuildCardViewData(
		const FHandCardSnapshot& CardSnapshot,
		const FBattleCardTargetPreview& TargetPreview)
	{
		return UWacomCardPresentationBuilder::BuildCardViewData(
			CardSnapshot.Definition,
			BuildRuntimeContext(CardSnapshot, TargetPreview));
	}

	FWacomCardDetailViewData BuildCardDetailViewData(const FHandCardSnapshot& CardSnapshot)
	{
		return BuildCardDetailViewData(CardSnapshot, EWacomCardFaceContext::Battle);
	}

	FWacomCardDetailViewData BuildCardDetailViewData(
		const FHandCardSnapshot& CardSnapshot,
		const EWacomCardFaceContext FaceContext)
	{
		return FaceContext == EWacomCardFaceContext::Battle
			? UWacomCardPresentationBuilder::BuildCardDetailViewData(
				CardSnapshot.Definition,
				FaceContext,
				BuildRuntimeContext(CardSnapshot))
			: UWacomCardPresentationBuilder::BuildCardDetailViewDataForFace(
				CardSnapshot.Definition,
				FaceContext);
	}

	FWacomCardDetailViewData BuildCardDetailViewData(
		const FHandCardSnapshot& CardSnapshot,
		const FBattleCardTargetPreview& TargetPreview)
	{
		return BuildCardDetailViewData(
			CardSnapshot,
			TargetPreview,
			EWacomCardFaceContext::Battle);
	}

	FWacomCardDetailViewData BuildCardDetailViewData(
		const FHandCardSnapshot& CardSnapshot,
		const FBattleCardTargetPreview& TargetPreview,
		const EWacomCardFaceContext FaceContext)
	{
		return FaceContext == EWacomCardFaceContext::Battle
			? UWacomCardPresentationBuilder::BuildCardDetailViewData(
				CardSnapshot.Definition,
				FaceContext,
				BuildRuntimeContext(CardSnapshot, TargetPreview))
			: UWacomCardPresentationBuilder::BuildCardDetailViewDataForFace(
				CardSnapshot.Definition,
				FaceContext);
	}

	EWacomFirstPersonCardInteractionIntent ResolveCardLayerInteractionIntent(
		const FHandCardSnapshot& CardSnapshot)
	{
		return ResolveBattleFirstPersonCardInteractionIntent(
			CardSnapshot.Definition
				? CardSnapshot.Definition->TargetMode
				: ECardTargetMode::None);
	}

	FWacomFirstPersonCardLayerEntry BuildCardLayerEntry(const FHandCardSnapshot& CardSnapshot)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = CardSnapshot.InstanceId;
		Entry.CardViewData = BuildCardViewData(CardSnapshot);
		Entry.DefaultFaceContext = EWacomCardFaceContext::Battle;
		if (CardSnapshot.Definition && CardSnapshot.Definition->HasEnabledRunFace())
		{
			Entry.AlternateFaceCardViewData =
				UWacomCardPresentationBuilder::BuildCardViewDataForFace(
					CardSnapshot.Definition,
					EWacomCardFaceContext::Run);
			Entry.bHasAlternateFace = true;
			Entry.bAllowLockedFaceInspection = true;
		}
		Entry.Zone = CardSnapshot.Zone;
		Entry.bIsHandAnchor = CardSnapshot.bIsHandAnchor;
		Entry.bIsPlayable = CardSnapshot.bIsPlayable;
		Entry.InteractionIntent = ResolveCardLayerInteractionIntent(CardSnapshot);
		return Entry;
	}

	FWacomFirstPersonCardLayerEntry BuildCardLayerEntry(
		const FHandCardSnapshot& CardSnapshot,
		const FBattleCardTargetPreview& TargetPreview)
	{
		FWacomFirstPersonCardLayerEntry Entry = BuildCardLayerEntry(CardSnapshot);
		Entry.CardViewData = BuildCardViewData(CardSnapshot, TargetPreview);
		Entry.bIsPlayable = Entry.bIsPlayable && !Entry.CardViewData.bDisabled;
		return Entry;
	}

	TArray<FWacomFirstPersonCardLayerEntry> BuildCardLayerEntries(const FBattleSnapshot& Snapshot)
	{
		TArray<FWacomFirstPersonCardLayerEntry> Entries;
		Entries.Reserve(Snapshot.Hand.Cards.Num());
		for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
		{
			Entries.Add(BuildCardLayerEntry(CardSnapshot));
		}
		return Entries;
	}

	TArray<FWacomFirstPersonCardLayerEntry> BuildCardLayerEntries(
		const FBattleSnapshot& Snapshot,
		const FBattleCardTargetPreview& TargetPreview)
	{
		TArray<FWacomFirstPersonCardLayerEntry> Entries;
		Entries.Reserve(Snapshot.Hand.Cards.Num());
		for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
		{
			Entries.Add(BuildCardLayerEntry(CardSnapshot, TargetPreview));
		}
		return Entries;
	}

	uint32 HashTargetPreviewFacts(const FBattleCardTargetPreview& TargetPreview)
	{
		uint32 Hash = 0x165667B1u;
		Hash = HashCombineFast(Hash, HashBoolForTargetPreviewState(TargetPreview.bHasPreview));
		Hash = HashCombineFast(Hash, HashBoolForTargetPreviewState(TargetPreview.Validation.bCanTarget));
		Hash = HashCombineFast(Hash, static_cast<uint32>(TargetPreview.Validation.RejectReason));
		Hash = HashCombineFast(Hash, GetTypeHash(TargetPreview.Validation.ResolvedPartInstanceId));
		Hash = HashCombineFast(Hash, GetTypeHash(TargetPreview.Validation.ResolvedPartIdentity));
		Hash = HashCombineFast(Hash, GetTypeHash(TargetPreview.Validation.ResolvedPartKey));
		Hash = HashCombineFast(Hash, static_cast<uint32>(TargetPreview.TargetKind));
		Hash = HashCombineFast(Hash, GetTypeHash(TargetPreview.SourceCardInstanceId));
		Hash = HashCombineFast(Hash, HashIntForTargetPreviewState(TargetPreview.SourceCardRuntimeCost));
		Hash = HashCombineFast(Hash, HashBoolForTargetPreviewState(TargetPreview.bSourceCardSwift));
		Hash = HashCombineFast(Hash, GetTypeHash(TargetPreview.TargetEnemyPartInstanceId));
		Hash = HashCombineFast(Hash, GetTypeHash(TargetPreview.TargetEnemyPartIdentity));
		Hash = HashCombineFast(Hash, GetTypeHash(TargetPreview.TargetEnemyPartKey));
		Hash = HashCombineFast(Hash, GetTypeHash(TargetPreview.TargetHandCardInstanceId));
		Hash = HashCombineFast(Hash, HashBoolForTargetPreviewState(TargetPreview.bHasTargetHandCardCostPreview));
		Hash = HashCombineFast(Hash, HashIntForTargetPreviewState(TargetPreview.TargetHandCardRuntimeCostBefore));
		Hash = HashCombineFast(Hash, HashIntForTargetPreviewState(TargetPreview.TargetHandCardRuntimeCostAfter));
		Hash = HashCombineFast(Hash, HashBoolForTargetPreviewState(TargetPreview.bWouldDiscardTargetHandCard));
		Hash = HashCombineFast(Hash, HashBoolForTargetPreviewState(TargetPreview.bWouldExhaustTargetHandCard));
		Hash = HashCombineFast(Hash, HashBoolForTargetPreviewState(TargetPreview.bWouldGainTargetHandCardKeyword));
		Hash = HashCombineFast(Hash, HashTagForTargetPreviewState(TargetPreview.TargetHandCardKeyword));
		Hash = HashCombineFast(Hash, static_cast<uint32>(TargetPreview.Effects.Num()));
		for (const FBattleCardTargetPreviewEffect& EffectPreview : TargetPreview.Effects)
		{
			Hash = HashCombineFast(Hash, HashTargetPreviewEffectState(EffectPreview));
		}
		return Hash;
	}

	FWacomBattleCardTargetPreviewPresentationStateKey BuildTargetPreviewStateKey(
		int32 SnapshotVersion,
		const FBattleCardTargetPreview& TargetPreview)
	{
		FWacomBattleCardTargetPreviewPresentationStateKey Key;
		if (!TargetPreview.bHasPreview)
		{
			return Key;
		}

		Key.SnapshotVersion = SnapshotVersion;
		Key.TargetKind = TargetPreview.TargetKind;
		Key.SourceCardInstanceId = TargetPreview.SourceCardInstanceId;
		Key.TargetEnemyPartInstanceId = TargetPreview.TargetEnemyPartInstanceId;
		Key.TargetEnemyPartIdentity = TargetPreview.TargetEnemyPartIdentity;
		Key.TargetEnemyPartKey = TargetPreview.TargetEnemyPartKey;
		Key.TargetHandCardInstanceId = TargetPreview.TargetHandCardInstanceId;
		Key.PreviewFactsHash = HashTargetPreviewFacts(TargetPreview);
		return Key;
	}

	bool AreTargetPreviewStateKeysEquivalent(
		const FWacomBattleCardTargetPreviewPresentationStateKey& Left,
		const FWacomBattleCardTargetPreviewPresentationStateKey& Right)
	{
		return Left.SnapshotVersion == Right.SnapshotVersion
			&& Left.TargetKind == Right.TargetKind
			&& Left.SourceCardInstanceId == Right.SourceCardInstanceId
			&& Left.TargetEnemyPartInstanceId == Right.TargetEnemyPartInstanceId
			&& Left.TargetEnemyPartIdentity == Right.TargetEnemyPartIdentity
			&& Left.TargetEnemyPartKey == Right.TargetEnemyPartKey
			&& Left.TargetHandCardInstanceId == Right.TargetHandCardInstanceId
			&& Left.PreviewFactsHash == Right.PreviewFactsHash;
	}

	FWacomBattleCardTargetPreviewPresentation BuildTargetPreviewPresentation(
		const FBattleSnapshot& Snapshot,
		const FBattleCardTargetPreview& TargetPreview)
	{
		FWacomBattleCardTargetPreviewPresentation Presentation;
		if (!TargetPreview.bHasPreview)
		{
			return Presentation;
		}

		Presentation.bHasPreview = true;
		Presentation.StateKey = BuildTargetPreviewStateKey(Snapshot.Version, TargetPreview);
		Presentation.SourceCardInstanceId = TargetPreview.SourceCardInstanceId;
		Presentation.TargetHandCardInstanceId = TargetPreview.TargetHandCardInstanceId;
		Presentation.CardLayerEntries = BuildCardLayerEntries(Snapshot, TargetPreview);

		for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
		{
			if (CardSnapshot.InstanceId == TargetPreview.SourceCardInstanceId)
			{
				Presentation.SourceCardDetailViewData =
					BuildCardDetailViewData(CardSnapshot, TargetPreview);
				Presentation.bHasSourceCardDetailViewData = true;
			}
			else if (CardSnapshot.InstanceId == TargetPreview.TargetHandCardInstanceId)
			{
				Presentation.TargetHandCardDetailViewData =
					BuildCardDetailViewData(CardSnapshot, TargetPreview);
				Presentation.bHasTargetHandCardDetailViewData = true;
			}
		}
		return Presentation;
	}

	const FEnemyPartSnapshot* FindEnemyPartSnapshot(
		const FBattleSnapshot& Snapshot,
		const FEnemyPartSnapshot& Candidate)
	{
		for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
		{
			for (const FEnemyPartSnapshot& Part : Enemy.Parts)
			{
				if ((Candidate.InstanceId.IsValid() && Part.InstanceId == Candidate.InstanceId)
					|| (Candidate.Identity.IsValidSlot() && Part.Identity == Candidate.Identity))
				{
					return &Part;
				}
			}
		}
		return nullptr;
	}

	FWacomBattleEnemyPartEntryViewData BuildEnemyPartEntryViewDataFromPreviewSnapshot(
		const FEnemyPartSnapshot& PartSnapshot,
		const bool bWillAct,
		const bool bWillSkipActionDueToStun,
		const bool bPerfectReleaseCandidate,
		const FEnemyPartSnapshot* BaselinePart)
	{
		const FEnemyPartSnapshot& IntentFacts =
			(bWillAct || bWillSkipActionDueToStun) && BaselinePart
			? *BaselinePart
			: PartSnapshot;
		FWacomBattleEnemyPartEntryViewData View;
		View.PartInstanceId = PartSnapshot.InstanceId;
		View.Identity = PartSnapshot.Identity;
		View.EnemySlotId = PartSnapshot.EnemySlotId;
		View.PartSlotId = PartSnapshot.PartSlotId;
		View.PartDisplayName = PartSnapshot.Definition
			? PartSnapshot.Definition->DisplayName
			: FText::FromName(PartSnapshot.PartSlotId);
		View.CurrentHp = PartSnapshot.CurrentHp;
		View.MaxHp = PartSnapshot.MaxHp;
		View.Shield = PartSnapshot.Shield;
		View.CurrentInitiative = bWillAct ? 0 : PartSnapshot.CurrentInitiative;
		View.CurrentIntentId = IntentFacts.CurrentIntentId;
		View.CurrentIntentDisplayName = IntentFacts.CurrentIntent.DisplayName;
		View.CurrentIntentInitiative = IntentFacts.CurrentIntent.Initiative;
		View.bCurrentIntentIsAttack = IntentFacts.CurrentIntent.bIsAttackIntent;
		View.CurrentIntentPeakAttackDamage = IntentFacts.CurrentIntent.PeakAttackDamage;
		View.RuntimeStatuses = PartSnapshot.Statuses;
		View.RuntimeStatusStacks = PartSnapshot.StatusStacks;
		View.bDestroyed = PartSnapshot.bDestroyed;
		View.bActionPreviewWillAct = bWillAct;
		View.bActionPreviewWillSkipActionDueToStun = bWillSkipActionDueToStun;
		View.bActionPreviewPerfectReleaseCandidate = bPerfectReleaseCandidate;
		return View;
	}

	FWacomBattleActionPreviewPresentation BuildActionPreviewPresentation(
		const FBattleSnapshot& Snapshot,
		const FBattleCardActionPreview& ActionPreview,
		const bool bBuildTargetPreviewPresentation)
	{
		FWacomBattleActionPreviewPresentation Presentation;
		if (!ActionPreview.bHasPreview)
		{
			return Presentation;
		}

		Presentation.bHasPreview = true;
		if (bBuildTargetPreviewPresentation)
		{
			Presentation.TargetPreviewPresentation =
				BuildTargetPreviewPresentation(Snapshot, ActionPreview.TargetPreview);
		}
		Presentation.bHasProjectedPlayer = ActionPreview.bHasProjectedPlayer;
		Presentation.ProjectedPlayer = ActionPreview.ProjectedPlayer;
		Presentation.ProjectedEnemyParts.Reserve(ActionPreview.ProjectedEnemyParts.Num());

		for (const FBattleCardActionPreviewEnemyPartState& PartState : ActionPreview.ProjectedEnemyParts)
		{
			const FEnemyPartSnapshot* BaselinePart =
				FindEnemyPartSnapshot(Snapshot, PartState.Snapshot);
			Presentation.ProjectedEnemyParts.Add(
				BuildEnemyPartEntryViewDataFromPreviewSnapshot(
					PartState.Snapshot,
					PartState.bWillAct,
					PartState.bWillSkipActionDueToStun,
					PartState.bPerfectReleaseCandidate,
					BaselinePart));
		}

		for (const FBattleCardResistancePreview& Resistance : ActionPreview.ResistancePreviews)
		{
			FWacomBattleEnemyPartEntryViewData* View =
				Presentation.ProjectedEnemyParts.FindByPredicate(
					[&Resistance](const FWacomBattleEnemyPartEntryViewData& Candidate)
					{
						return Candidate.PartInstanceId == Resistance.TargetEnemyPartInstanceId
							|| (Resistance.TargetEnemyPartIdentity.IsValidSlot()
								&& Candidate.Identity == Resistance.TargetEnemyPartIdentity);
					});
			if (!View)
			{
				const FEnemyPartSnapshot* BaselinePart = nullptr;
				for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
				{
					BaselinePart = Enemy.Parts.FindByPredicate(
						[&Resistance](const FEnemyPartSnapshot& Candidate)
						{
							return Candidate.InstanceId == Resistance.TargetEnemyPartInstanceId;
						});
					if (BaselinePart) break;
				}
				if (!BaselinePart)
				{
					continue;
				}
				View = &Presentation.ProjectedEnemyParts.Add_GetRef(
					BuildEnemyPartEntryViewDataFromPreviewSnapshot(
						*BaselinePart,
						false,
						false,
						true,
						BaselinePart));
			}

			View->bHasResistancePreview = true;
			View->ResistancePreviewPlayerPeakDamage =
				Resistance.PlayerPeakSingleHitDamage;
			View->ResistancePreviewEnemyPeakDamage =
				Resistance.EnemyPeakSingleHitDamage;
			View->bResistancePreviewWillStun = Resistance.bWillStun;
			View->bActionPreviewPerfectReleaseCandidate = true;
		}
		return Presentation;
	}
}
