// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardEffectBadgeFeedbackPlayback.h"

#include "Sound/SoundBase.h"

namespace
{
	float EaseOutCubicBadge(float Alpha)
	{
		const float Inverse = 1.0f - FMath::Clamp(Alpha, 0.0f, 1.0f);
		return 1.0f - Inverse * Inverse * Inverse;
	}

	float EaseInCubicBadge(float Alpha)
	{
		const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return SafeAlpha * SafeAlpha * SafeAlpha;
	}

	int32 ChangeOrder(EWacomFirstPersonCardEffectBadgeChangeKind Kind)
	{
		switch (Kind)
		{
		case EWacomFirstPersonCardEffectBadgeChangeKind::Removed:
			return 0;
		case EWacomFirstPersonCardEffectBadgeChangeKind::ValueChanged:
			return 1;
		case EWacomFirstPersonCardEffectBadgeChangeKind::Added:
			return 2;
		default:
			return 3;
		}
	}
}

void FWacomFirstPersonCardEffectBadgeFeedbackPlayback::Begin(
	const FWacomFirstPersonCardEffectBadgeFeedbackConfig& InConfig,
	const TArray<FWacomFirstPersonCardEffectBadgeChange>& InChanges)
{
	Reset();
	if (!InConfig.bEnabled || InChanges.IsEmpty())
	{
		return;
	}

	Config = InConfig;
	TArray<FWacomFirstPersonCardEffectBadgeChange> SortedChanges = InChanges;
	SortedChanges.Sort([](
		const FWacomFirstPersonCardEffectBadgeChange& A,
		const FWacomFirstPersonCardEffectBadgeChange& B)
	{
		const int32 AOrder = ChangeOrder(A.ChangeKind);
		const int32 BOrder = ChangeOrder(B.ChangeKind);
		return AOrder == BOrder
			? A.PresentationKey.LexicalLess(B.PresentationKey)
			: AOrder < BOrder;
	});

	const bool bHasRemoved = SortedChanges.ContainsByPredicate([](
		const FWacomFirstPersonCardEffectBadgeChange& Change)
	{
		return Change.ChangeKind == EWacomFirstPersonCardEffectBadgeChangeKind::Removed;
	});
	const float RemovedGroupEnd = bHasRemoved
		? ResolveDuration(EWacomFirstPersonCardEffectBadgeChangeKind::Removed)
		: 0.0f;
	ReflowStartSeconds = RemovedGroupEnd;
	ReflowEndSeconds = bHasRemoved
		? RemovedGroupEnd + FMath::Max(0.0f, Config.Style.ReflowDurationSeconds)
		: 0.0f;

	TMap<EWacomFirstPersonCardEffectBadgeChangeKind, int32> GroupIndices;
	for (const FWacomFirstPersonCardEffectBadgeChange& Change : SortedChanges)
	{
		FItem& Item = Items.AddDefaulted_GetRef();
		Item.Change = Change;
		Item.DurationSeconds = ResolveDuration(Change.ChangeKind);
		const int32 GroupIndex = GroupIndices.FindOrAdd(Change.ChangeKind)++;
		const float Stagger = FMath::Min(
			GroupIndex * FMath::Max(0.0f, Config.Style.SequenceStaggerSeconds),
			FMath::Max(0.0f, Config.Style.MaxSequenceDelaySeconds));
		Item.StartSeconds = Stagger;
		if (bHasRemoved && Change.ChangeKind != EWacomFirstPersonCardEffectBadgeChangeKind::Removed)
		{
			// Added/value feedback begins with survivor reflow. This preserves the
			// semantic order (remove first) without serializing another full 0.14 s.
			Item.StartSeconds += RemovedGroupEnd;
		}
		TotalDurationSeconds = FMath::Max(
			TotalDurationSeconds,
			Item.StartSeconds + Item.DurationSeconds);
	}

	bActive = TotalDurationSeconds > KINDA_SMALL_NUMBER;
}

FWacomFirstPersonCardEffectBadgeFeedbackView
FWacomFirstPersonCardEffectBadgeFeedbackPlayback::Tick(float DeltaTime)
{
	if (!bActive)
	{
		return BuildView();
	}

	ElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	QueueSoundIfNeeded();
	if (ElapsedSeconds >= TotalDurationSeconds)
	{
		FWacomFirstPersonCardEffectBadgeFeedbackView Result = BuildView();
		Result.bCompleted = true;
		Reset();
		return Result;
	}
	return BuildView();
}

FWacomFirstPersonCardEffectBadgeFeedbackView
FWacomFirstPersonCardEffectBadgeFeedbackPlayback::BuildView() const
{
	FWacomFirstPersonCardEffectBadgeFeedbackView View;
	View.bActive = bActive;
	View.bReducedMotion = Config.bReducedMotion;
	View.Style = Config.Style;
	if (bActive && ReflowEndSeconds > ReflowStartSeconds)
	{
		View.ReflowProgress = FMath::Clamp(
			(ElapsedSeconds - ReflowStartSeconds)
				/ FMath::Max(KINDA_SMALL_NUMBER, ReflowEndSeconds - ReflowStartSeconds),
			0.0f,
			1.0f);
	}

	for (const FItem& Item : Items)
	{
		FWacomFirstPersonCardEffectBadgeFeedbackItemView& ItemView = View.Items.AddDefaulted_GetRef();
		ItemView.PresentationKey = Item.Change.PresentationKey;
		ItemView.ChangeKind = Item.Change.ChangeKind;
		ItemView.Direction = Item.Change.Direction;
		ItemView.OldValue = Item.Change.OldValue;
		ItemView.NewValue = Item.Change.NewValue;
		ItemView.Seed = Item.Change.Seed;
		ItemView.bPrepareMaterial = bActive;
		const float LocalTime = ElapsedSeconds - Item.StartSeconds;
		if (!bActive || LocalTime < 0.0f)
		{
			continue;
		}

		ItemView.bActive = LocalTime < Item.DurationSeconds;
		const float Progress = FMath::Clamp(
			LocalTime / FMath::Max(KINDA_SMALL_NUMBER, Item.DurationSeconds),
			0.0f,
			1.0f);
		if (Config.bReducedMotion)
		{
			ItemView.OldDissolveAmount = Progress;
			ItemView.NewRevealAmount = Progress;
			continue;
		}

		switch (Item.Change.ChangeKind)
		{
		case EWacomFirstPersonCardEffectBadgeChangeKind::Added:
			ItemView.OldDissolveAmount = 1.0f;
			ItemView.NewRevealAmount = Progress;
			ItemView.RootScale = Progress < 0.55f
				? FMath::Lerp(0.85f, 1.08f, EaseOutCubicBadge(Progress / 0.55f))
				: FMath::Lerp(1.08f, 1.0f, EaseOutCubicBadge((Progress - 0.55f) / 0.45f));
			break;
		case EWacomFirstPersonCardEffectBadgeChangeKind::Removed:
			ItemView.OldDissolveAmount = FMath::Clamp(Progress / 0.60f, 0.0f, 1.0f);
			ItemView.NewRevealAmount = 0.0f;
			ItemView.RootScale = FMath::Lerp(1.0f, 0.82f, EaseInCubicBadge(Progress));
			ItemView.RootOpacity = 1.0f - EaseInCubicBadge(Progress);
			break;
		case EWacomFirstPersonCardEffectBadgeChangeKind::ValueChanged:
		default:
			ItemView.OldDissolveAmount = FMath::Clamp(Progress / (0.08f / 0.28f), 0.0f, 1.0f);
			ItemView.NewRevealAmount = FMath::Clamp(
				(Progress - 0.08f / 0.28f) / (0.12f / 0.28f), 0.0f, 1.0f);
			if (Progress < 0.285714f)
			{
				ItemView.RootScale = FMath::Lerp(1.0f, 0.94f, EaseInCubicBadge(Progress / 0.285714f));
			}
			else if (Progress < 0.714286f)
			{
				ItemView.RootScale = FMath::Lerp(0.94f, 1.08f, EaseOutCubicBadge((Progress - 0.285714f) / 0.428572f));
			}
			else
			{
				ItemView.RootScale = FMath::Lerp(1.08f, 1.0f, EaseOutCubicBadge((Progress - 0.714286f) / 0.285714f));
			}
			break;
		}
	}
	return View;
}

TOptional<FWacomFirstPersonCardEffectBadgeFeedbackSoundRequest>
FWacomFirstPersonCardEffectBadgeFeedbackPlayback::ConsumePendingSoundRequest()
{
	TOptional<FWacomFirstPersonCardEffectBadgeFeedbackSoundRequest> Result = PendingSoundRequest;
	PendingSoundRequest.Reset();
	return Result;
}

void FWacomFirstPersonCardEffectBadgeFeedbackPlayback::Reset()
{
	Config = FWacomFirstPersonCardEffectBadgeFeedbackConfig();
	Items.Reset();
	ElapsedSeconds = 0.0f;
	TotalDurationSeconds = 0.0f;
	ReflowStartSeconds = 0.0f;
	ReflowEndSeconds = 0.0f;
	bActive = false;
	bSoundRequested = false;
	PendingSoundRequest.Reset();
}

void FWacomFirstPersonCardEffectBadgeFeedbackPlayback::QueueSoundIfNeeded()
{
	if (bSoundRequested || Items.IsEmpty())
	{
		return;
	}

	const FItem& FirstItem = Items[0];
	const float SoundEdge = FirstItem.StartSeconds
		+ (FirstItem.Change.ChangeKind == EWacomFirstPersonCardEffectBadgeChangeKind::Removed
			? 0.0f
			: 0.08f);
	if (ElapsedSeconds < SoundEdge)
	{
		return;
	}
	bSoundRequested = true;
	if (!Config.Style.ChangeSound)
	{
		return;
	}

	FRandomStream RandomStream(FirstItem.Change.Seed);
	const float Variation = FMath::Clamp(Config.Style.ChangeSoundPitchVariation, 0.0f, 0.99f);
	FWacomFirstPersonCardEffectBadgeFeedbackSoundRequest Request;
	Request.Sound = Config.Style.ChangeSound;
	Request.VolumeMultiplier = FMath::Max(0.0f, Config.Style.ChangeSoundVolumeMultiplier);
	Request.PitchMultiplier = FMath::Max(
		0.01f,
		Config.Style.ChangeSoundPitchMultiplier
			* RandomStream.FRandRange(1.0f - Variation, 1.0f + Variation));
	PendingSoundRequest = Request;
}

float FWacomFirstPersonCardEffectBadgeFeedbackPlayback::ResolveDuration(
	EWacomFirstPersonCardEffectBadgeChangeKind ChangeKind) const
{
	if (Config.bReducedMotion)
	{
		return 0.12f;
	}
	switch (ChangeKind)
	{
	case EWacomFirstPersonCardEffectBadgeChangeKind::Added:
		return FMath::Max(KINDA_SMALL_NUMBER, Config.Style.AddedDurationSeconds);
	case EWacomFirstPersonCardEffectBadgeChangeKind::Removed:
		return FMath::Max(KINDA_SMALL_NUMBER, Config.Style.RemovedDurationSeconds);
	case EWacomFirstPersonCardEffectBadgeChangeKind::ValueChanged:
	default:
		return FMath::Max(KINDA_SMALL_NUMBER, Config.Style.ValueChangeDurationSeconds);
	}
}
