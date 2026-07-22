// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleEnemyPanelViewData.h"
#include "UObject/GCObject.h"

class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
enum class EWacomBattleEnemySegmentRole : uint8;

/** Enemy Vitals 可触发的表现语义；WBP 仍独占具体曲线。 */
enum class EWacomBattleEnemyMotionCue : uint8
{
	Intro,
	Damage,
	ShieldImpact,
	ShieldBreak,
	InitiativeStep,
	IntentChange,
	ContextEnter,
	ContextExit,
	Destroyed,
};

struct FWacomBattleEnemyPartPresentationUpdate
{
	TArray<EWacomBattleEnemyMotionCue, TInlineAllocator<4>> Cues;
	FWacomBattleEnemyPartEntryViewData PreviousView;
	bool bHadPreviousView = false;
	bool bIntentChanged = false;
	bool bEnteredReducedMotion = false;
};

/** 一次性 Material 输入帧；不持有 Widget 或战斗状态。 */
struct FWacomBattleEnemyVitalsFrame
{
	float HpCurrentPercent = 0.0f;
	float HpTrailStartPercent = 0.0f;
	float HpPreviewPercent = 0.0f;
	float HpPreviewMode = 0.0f;
	float DamageStartTimeSeconds = -1000.0f;
	float DamageTrailHoldSeconds = 0.0f;
	float DamageTrailRecoverySeconds = 0.001f;
	float ShieldVisible = 0.0f;
	float ShieldPreviewMode = 0.0f;
	float ShieldImpactStartTimeSeconds = -1000.0f;
	float SegmentRole = 0.0f;
	float DestroyedAmount = 0.0f;
	float LowHealthAmount = 0.0f;
	float FlashIntensity = 1.0f;
	float ReducedMotion = 0.0f;
};

enum class EWacomBattleEnemyResistancePreviewOutcome : uint8
{
	None,
	Success,
	Failure,
};

/** Action Preview 的一次性紧凑语义帧；不持有 Widget，也不重算战斗规则。 */
struct FWacomBattleEnemyActionPreviewFrame
{
	bool bActive = false;
	bool bPerfectRelease = false;
	bool bShowResistanceComparison = false;
	bool bWillAct = false;
	bool bWillSkipActionDueToStun = false;
	int32 PlayerPeakDamage = 0;
	int32 EnemyPeakDamage = 0;
	EWacomBattleEnemyResistancePreviewOutcome ResistanceOutcome =
		EWacomBattleEnemyResistancePreviewOutcome::None;
	FText ComparatorText;
};

/**
 * 单个部位条目的纯表现状态。
 * Snapshot 更新产生 typed cue；Preview 只改变显示帧，不生成战斗事实 cue。
 */
class FWacomBattleEnemyPartPresentationState final
{
public:
	FWacomBattleEnemyPartPresentationState();

	FWacomBattleEnemyPartPresentationUpdate SetRealView(
		const FWacomBattleEnemyPartEntryViewData& InView,
		float WorldTimeSeconds);
	FWacomBattleEnemyPartPresentationUpdate SetActionPreview(
		const FWacomBattleEnemyPartEntryViewData& InPreviewView);
	FWacomBattleEnemyPartPresentationUpdate ClearActionPreview();
	FWacomBattleEnemyPartPresentationUpdate SetContextHighlighted(bool bHighlighted);
	void SetSegmentLayout(int32 PartIndex, int32 PartCount);
	FWacomBattleEnemyPartPresentationUpdate SetMotionPolicy(
		bool bSimplifiedMotion,
		float FlashIntensity);

	FWacomBattleEnemyVitalsFrame BuildVitalsFrame(
		float DamageTrailHoldSeconds,
		float DamageTrailRecoverySeconds) const;
	WACOMAPP_API FWacomBattleEnemyActionPreviewFrame BuildActionPreviewFrame() const;
	void ResetTransientPresentation();

	const FWacomBattleEnemyPartEntryViewData& GetRealView() const { return CurrentView; }
	const FWacomBattleEnemyPartEntryViewData& GetDisplayView() const;
	bool HasRealView() const { return bHasRealView; }
	bool HasActionPreview() const { return bHasActionPreview; }
	bool IsContextActive() const { return bContextHighlighted || bHasActionPreview; }
	bool IsUsingSimplifiedMotion() const { return bSimplifiedMotion; }
	float GetFlashIntensity() const { return RuntimeFlashIntensity; }
	float GetDamageTrailStartPercent() const { return DamageTrailStartPercent; }
	EWacomBattleEnemySegmentRole GetSegmentRole() const { return SegmentRole; }
	int32 GetSegmentCount() const { return SegmentCount; }

private:
	static bool AreViewsEquivalent(
		const FWacomBattleEnemyPartEntryViewData& Left,
		const FWacomBattleEnemyPartEntryViewData& Right);
	static float ResolveHpPercent(const FWacomBattleEnemyPartEntryViewData& View);
	static EWacomBattleEnemySegmentRole ResolveSegmentRole(int32 PartIndex, int32 PartCount);

	FWacomBattleEnemyPartEntryViewData CurrentView;
	FWacomBattleEnemyPartEntryViewData ActionPreviewView;
	float DamageTrailStartPercent = 0.0f;
	float DamageStartTimeSeconds = -1000.0f;
	float ShieldImpactStartTimeSeconds = -1000.0f;
	float RuntimeFlashIntensity = 1.0f;
	int32 SegmentIndex = 0;
	int32 SegmentCount = 1;
	EWacomBattleEnemySegmentRole SegmentRole;
	bool bHasRealView = false;
	bool bHasActionPreview = false;
	bool bContextHighlighted = false;
	bool bSimplifiedMotion = false;
};

/** 独占 Enemy Vitals MID 生命周期与参数名，Widget 不直接操作材质参数。 */
class FWacomBattleEnemyVitalsMaterialAdapter final : public FGCObject
{
public:
	bool Initialize(UImage* InVitalsImage);
	void ApplyFrame(const FWacomBattleEnemyVitalsFrame& Frame);
	void RestoreAuthoredBrush();

	UMaterialInstanceDynamic* GetMaterialInstance() const { return MaterialInstance; }

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

private:
	TWeakObjectPtr<UImage> VitalsImage;
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance = nullptr;
	TObjectPtr<UMaterialInterface> AuthoredMaterial = nullptr;
};
