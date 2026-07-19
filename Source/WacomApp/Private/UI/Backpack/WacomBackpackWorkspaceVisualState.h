// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomDeckCardWidget;

/** Workspace 卡牌在工作台坐标系中的基础姿态。 */
struct FWacomBackpackWorkspaceCardLayout
{
	FVector2D Center = FVector2D::ZeroVector;
	FVector2D Size = FVector2D::ZeroVector;
	float AngleDegrees = 0.0f;
	int32 ZOrder = 0;
};

/** 与父层无关的已合成视觉姿态，用于 Carry -> Settlement 连续交接。 */
struct FWacomBackpackWorkspaceCardVisualPose
{
	FVector2D Center = FVector2D::ZeroVector;
	float AngleDegrees = 0.0f;
};

struct FWacomBackpackWorkspaceCardLayoutTransition
{
	FWacomBackpackWorkspaceCardLayout Start;
	FWacomBackpackWorkspaceCardLayout Current;
	FWacomBackpackWorkspaceCardLayout Target;
	float ElapsedSeconds = 0.0f;
	float DurationSeconds = 0.0f;
};

/**
 * Workspace 的非反射视觉状态仓库。
 *
 * 状态只属于一个 Workspace Runtime；UWidget 负责把 Slate 输入翻译成意图，
 * 不再直接声明或复制布局、收落与释放交接容器。
 */
class FWacomBackpackWorkspaceVisualState
{
public:
	using FCardKey = TWeakObjectPtr<UWacomDeckCardWidget>;
	using FLayoutMap = TMap<FCardKey, FWacomBackpackWorkspaceCardLayout>;
	using FTransitionMap = TMap<FCardKey, FWacomBackpackWorkspaceCardLayoutTransition>;

	FLayoutMap& BaseLayouts() { return BaseCardLayouts; }
	const FLayoutMap& BaseLayouts() const { return BaseCardLayouts; }
	FTransitionMap& BaseTransitions() { return BaseCardLayoutTransitions; }
	const FTransitionMap& BaseTransitions() const { return BaseCardLayoutTransitions; }
	FLayoutMap& ExpandedFocusLayouts() { return ExpandedPileFocusTargets; }
	const FLayoutMap& ExpandedFocusLayouts() const { return ExpandedPileFocusTargets; }
	FLayoutMap& SelectionFrozenLayouts() { return FrozenSelectionLayouts; }
	const FLayoutMap& SelectionFrozenLayouts() const { return FrozenSelectionLayouts; }

	/** 清理一次 Scene reconcile 后不再可见的卡牌状态。 */
	bool ReconcileVisibleCards(
		const TSet<UWacomDeckCardWidget*>& VisibleWidgets,
		const TSet<FGuid>& VisibleInstanceIds)
	{
		PruneLayoutMap(BaseCardLayouts, VisibleWidgets);
		PruneTransitionMap(BaseCardLayoutTransitions, VisibleWidgets);
		PruneLayoutMap(FrozenSelectionLayouts, VisibleWidgets);
		for (auto It = PendingReleasedVisualHandoffs.CreateIterator(); It; ++It)
		{
			if (!VisibleInstanceIds.Contains(*It))
			{
				PendingReleasedVisualPoses.Remove(*It);
				It.RemoveCurrent();
			}
		}
		return FrozenSelectionLayouts.IsEmpty();
	}

	const FWacomBackpackWorkspaceCardLayout* FindBaseLayout(
		const UWacomDeckCardWidget* Card) const
	{
		return BaseCardLayouts.Find(const_cast<UWacomDeckCardWidget*>(Card));
	}

	const FWacomBackpackWorkspaceCardLayoutTransition* FindBaseTransition(
		const UWacomDeckCardWidget* Card) const
	{
		return BaseCardLayoutTransitions.Find(const_cast<UWacomDeckCardWidget*>(Card));
	}

	bool HasBaseLayout(const UWacomDeckCardWidget* Card) const
	{
		return Card && BaseCardLayouts.Contains(const_cast<UWacomDeckCardWidget*>(Card));
	}

	bool PrimeBaseLayout(
		UWacomDeckCardWidget& Card,
		const FWacomBackpackWorkspaceCardLayout& Layout)
	{
		if (BaseCardLayouts.Contains(&Card))
		{
			return false;
		}
		BaseCardLayouts.Add(&Card, Layout);
		return true;
	}

	/**
	 * 更新权威目标并在需要时从当前过渡姿态连续重定向。
	 * 返回 true 表示本次创建了新的活动过渡，需要唤醒 Workspace Frame Timer。
	 */
	bool RetargetBaseLayout(
		UWacomDeckCardWidget& Card,
		const FWacomBackpackWorkspaceCardLayout& Target,
		bool bAnimate,
		float DurationSeconds)
	{
		const FWacomBackpackWorkspaceCardLayout* Previous = BaseCardLayouts.Find(&Card);
		const bool bChanged = Previous
			&& (!Previous->Center.Equals(Target.Center, 0.5f)
				|| !FMath::IsNearlyEqual(Previous->AngleDegrees, Target.AngleDegrees, 0.1f));
		bool bStartedTransition = false;
		if (bChanged && bAnimate && DurationSeconds > 0.0f)
		{
			const FWacomBackpackWorkspaceCardLayoutTransition* Existing =
				BaseCardLayoutTransitions.Find(&Card);
			const FWacomBackpackWorkspaceCardLayout Start = Existing
				? Existing->Current
				: *Previous;
			FWacomBackpackWorkspaceCardLayoutTransition& Transition =
				BaseCardLayoutTransitions.FindOrAdd(&Card);
			Transition.Start = Start;
			Transition.Current = Start;
			Transition.Target = Target;
			Transition.ElapsedSeconds = 0.0f;
			Transition.DurationSeconds = DurationSeconds;
			bStartedTransition = true;
		}
		else if (bChanged || !Previous || !bAnimate || DurationSeconds <= 0.0f)
		{
			BaseCardLayoutTransitions.Remove(&Card);
		}
		else if (FWacomBackpackWorkspaceCardLayoutTransition* Existing =
			BaseCardLayoutTransitions.Find(&Card))
		{
			// 几何稳定采样可能在过渡完成前再次提交同一目标；保留已走过的路径。
			Existing->Target = Target;
		}
		BaseCardLayouts.Add(&Card, Target);
		return bStartedTransition;
	}

	bool TickBaseTransitions(
		float DeltaSeconds,
		bool bSimplifiedMotion,
		TFunctionRef<void(UWacomDeckCardWidget&)> ApplyCard,
		int32* OutApplyCount = nullptr)
	{
		if (OutApplyCount)
		{
			*OutApplyCount = 0;
		}
		for (auto It = BaseCardLayoutTransitions.CreateIterator(); It; ++It)
		{
			UWacomDeckCardWidget* Card = It.Key().Get();
			if (!Card)
			{
				It.RemoveCurrent();
				continue;
			}

			FWacomBackpackWorkspaceCardLayoutTransition& Transition = It.Value();
			Transition.ElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
			const float Alpha = bSimplifiedMotion
				? 1.0f
				: (Transition.DurationSeconds > 0.0f
					? FMath::Clamp(
						Transition.ElapsedSeconds / Transition.DurationSeconds,
						0.0f,
						1.0f)
					: 1.0f);
			const float Smoothed = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
			Transition.Current.Center = FMath::Lerp(
				Transition.Start.Center, Transition.Target.Center, Smoothed);
			Transition.Current.Size = Transition.Target.Size;
			Transition.Current.AngleDegrees = FMath::Lerp(
				Transition.Start.AngleDegrees, Transition.Target.AngleDegrees, Smoothed);
			Transition.Current.ZOrder = Transition.Target.ZOrder;
			ApplyCard(*Card);
			if (OutApplyCount)
			{
				++*OutApplyCount;
			}
			if (Alpha >= 1.0f)
			{
				It.RemoveCurrent();
			}
		}
		return !BaseCardLayoutTransitions.IsEmpty();
	}

	void RecordReleasedVisualPose(
		FGuid InstanceId,
		const FWacomBackpackWorkspaceCardVisualPose& VisualPose)
	{
		PendingReleasedVisualPoses.Add(InstanceId, VisualPose);
	}

	void MarkReleasedHandoff(FGuid InstanceId)
	{
		PendingReleasedVisualHandoffs.Add(InstanceId);
	}

	void RemoveReleasedHandoff(FGuid InstanceId)
	{
		PendingReleasedVisualHandoffs.Remove(InstanceId);
		PendingReleasedVisualPoses.Remove(InstanceId);
	}

	bool IsReleasedHandoffPending(FGuid InstanceId) const
	{
		return PendingReleasedVisualHandoffs.Contains(InstanceId);
	}

	const FWacomBackpackWorkspaceCardVisualPose* FindReleasedVisualPose(
		FGuid InstanceId) const
	{
		return PendingReleasedVisualPoses.Find(InstanceId);
	}

	bool ConsumeReleasedHandoff(
		FGuid InstanceId,
		FWacomBackpackWorkspaceCardVisualPose& OutVisualPose)
	{
		if (!PendingReleasedVisualHandoffs.Remove(InstanceId))
		{
			return false;
		}
		if (const FWacomBackpackWorkspaceCardVisualPose* Pose =
			PendingReleasedVisualPoses.Find(InstanceId))
		{
			OutVisualPose = *Pose;
		}
		PendingReleasedVisualPoses.Remove(InstanceId);
		return true;
	}

	void ResetReleasedHandoffs()
	{
		PendingReleasedVisualHandoffs.Reset();
		PendingReleasedVisualPoses.Reset();
	}

	void SetSettlementTarget(
		UWacomDeckCardWidget& Card,
		const FWacomBackpackWorkspaceCardLayout& Target)
	{
		SettlementTargets.Add(&Card, Target);
	}

	bool TakeSettlementTarget(
		FCardKey CardKey,
		FWacomBackpackWorkspaceCardLayout& OutTarget)
	{
		UWacomDeckCardWidget* Card = CardKey.Get();
		const FWacomBackpackWorkspaceCardLayout* Target = SettlementTargets.Find(CardKey);
		if (!Card || !Target)
		{
			SettlementTargets.Remove(CardKey);
			return false;
		}
		OutTarget = *Target;
		SettlementTargets.Remove(CardKey);
		return true;
	}

	bool HasActiveSettlements() const { return !SettlementTargets.IsEmpty(); }
	bool HasReleasedHandoffs() const { return !PendingReleasedVisualHandoffs.IsEmpty(); }

	void ResetTransientMotion()
	{
		ResetReleasedHandoffs();
		SettlementTargets.Reset();
		BaseCardLayoutTransitions.Reset();
	}

	void Reset()
	{
		BaseCardLayouts.Reset();
		BaseCardLayoutTransitions.Reset();
		SettlementTargets.Reset();
		ExpandedPileFocusTargets.Reset();
		FrozenSelectionLayouts.Reset();
		PendingReleasedVisualHandoffs.Reset();
		PendingReleasedVisualPoses.Reset();
	}

private:
	static void PruneLayoutMap(
		FLayoutMap& Layouts,
		const TSet<UWacomDeckCardWidget*>& VisibleWidgets)
	{
		for (auto It = Layouts.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid() || !VisibleWidgets.Contains(It.Key().Get()))
			{
				It.RemoveCurrent();
			}
		}
	}

	static void PruneTransitionMap(
		FTransitionMap& Transitions,
		const TSet<UWacomDeckCardWidget*>& VisibleWidgets)
	{
		for (auto It = Transitions.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid() || !VisibleWidgets.Contains(It.Key().Get()))
			{
				It.RemoveCurrent();
			}
		}
	}

	FLayoutMap BaseCardLayouts;
	FTransitionMap BaseCardLayoutTransitions;
	FLayoutMap SettlementTargets;
	FLayoutMap ExpandedPileFocusTargets;
	FLayoutMap FrozenSelectionLayouts;
	TSet<FGuid> PendingReleasedVisualHandoffs;
	TMap<FGuid, FWacomBackpackWorkspaceCardVisualPose> PendingReleasedVisualPoses;
};
