// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "WacomRunViewModel.generated.h"

/**
 * 探索 / 背包 UI 的 ViewModel（GDD §3 / §8 / §11）。
 *
 * 设计：
 *   - 纯数据 + FieldNotify。**不持有 Session 指针，不订阅事件**。
 *   - 由 UWacomRunViewModelProvider Subsystem 在 RunSession 字段变化时调 Setter。
 *   - 字段粒度：每个独立显示出来的数值一个 FieldNotify 字段，让 WBP ViewBinding 细粒度更新。
 *   - 注册到 Global Viewmodel Collection 让多个 widget 共享同一个实例。
 *
 * 数据流：
 *   RunSession 写 API
 *     → OnRunStateChangedNative.Broadcast()
 *     → UWacomRunViewModelProvider 监听
 *     → Provider 读 RunState 字段
 *     → 调本类 Setter（UE_MVVM_SET_PROPERTY_VALUE 自动比较 + 广播）
 *     → WBP ViewBinding 收到 FieldNotify
 *     → Widget 自动更新显示
 *
 * 单测友好：可以独立 NewObject<UWacomRunViewModel>() 调 Setter 验证 FieldNotify 触发。
 */
UCLASS(BlueprintType)
class WACOMAPP_API UWacomRunViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	// ---- 时段 / 节点 / 天数（GDD §8）----

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	FText PhaseDisplay;  // "清晨" / "日间" / ...

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 RemainingNodeCount = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 CurrentDayNumber = 1;

	// ---- 手指 / 经验 / 技能（GDD §3.1 / §3.3）----

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 FingerCount = 10;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 ExperienceCurrent = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 ExperienceCapacity = 10;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 AcquiredSkillCount = 0;

	// ---- 经济 ----

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 Gold = 0;

	// ---- 容量（GDD §11.4）----

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 FluxCapacity = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 BattleDeckCapacity = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 BackpackCount = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 BattleDeckCount = 0;

	// ---- 8 条压力（GDD §3.2）----

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 PressureHunger = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 PressureWound = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 PressureFatigue = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 PressureBurden = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 PressureDecay = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 PressureMisdeed = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 PressureBloodlust = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 PressureDisability = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
	int32 PressureTotal = 0;

	// ---- 派生 FieldNotify 函数（用于 WBP 直接绑文本/进度条）----

	/** 经验进度比例 [0, 1]。 */
	UFUNCTION(BlueprintPure, FieldNotify)
	float GetExperienceRatio() const
	{
		const int32 Cap = FMath::Max(1, ExperienceCapacity);
		return static_cast<float>(ExperienceCurrent) / static_cast<float>(Cap);
	}

	// ---- Setter 集中区（C++ 不自动调，由 ViewModelProvider 调）----
	// UPROPERTY 上的 Setter= 让蓝图侧的 SetX 节点自动走我们的实现，
	// 但 C++ 必须显式调这些函数。

	void SetPhaseDisplay(FText InValue)
	{
		UE_MVVM_SET_PROPERTY_VALUE(PhaseDisplay, InValue);
	}
	FText GetPhaseDisplay() const { return PhaseDisplay; }

	void SetRemainingNodeCount(int32 InValue)
	{
		if (UE_MVVM_SET_PROPERTY_VALUE(RemainingNodeCount, InValue))
		{
			// RemainingNodeCount 没有派生函数，无需级联通知。
		}
	}
	int32 GetRemainingNodeCount() const { return RemainingNodeCount; }

	void SetCurrentDayNumber(int32 InValue)     { UE_MVVM_SET_PROPERTY_VALUE(CurrentDayNumber, InValue); }
	int32 GetCurrentDayNumber() const           { return CurrentDayNumber; }

	void SetFingerCount(int32 InValue)          { UE_MVVM_SET_PROPERTY_VALUE(FingerCount, InValue); }
	int32 GetFingerCount() const                { return FingerCount; }

	void SetExperienceCurrent(int32 InValue)
	{
		if (UE_MVVM_SET_PROPERTY_VALUE(ExperienceCurrent, InValue))
		{
			// 派生函数 GetExperienceRatio 依赖此值，手动级联通知。
			UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetExperienceRatio);
		}
	}
	int32 GetExperienceCurrent() const          { return ExperienceCurrent; }

	void SetExperienceCapacity(int32 InValue)
	{
		if (UE_MVVM_SET_PROPERTY_VALUE(ExperienceCapacity, InValue))
		{
			UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetExperienceRatio);
		}
	}
	int32 GetExperienceCapacity() const         { return ExperienceCapacity; }

	void SetAcquiredSkillCount(int32 InValue)   { UE_MVVM_SET_PROPERTY_VALUE(AcquiredSkillCount, InValue); }
	int32 GetAcquiredSkillCount() const         { return AcquiredSkillCount; }

	void SetGold(int32 InValue)                 { UE_MVVM_SET_PROPERTY_VALUE(Gold, InValue); }
	int32 GetGold() const                       { return Gold; }

	void SetFluxCapacity(int32 InValue)         { UE_MVVM_SET_PROPERTY_VALUE(FluxCapacity, InValue); }
	int32 GetFluxCapacity() const               { return FluxCapacity; }

	void SetBattleDeckCapacity(int32 InValue)   { UE_MVVM_SET_PROPERTY_VALUE(BattleDeckCapacity, InValue); }
	int32 GetBattleDeckCapacity() const         { return BattleDeckCapacity; }

	void SetBackpackCount(int32 InValue)        { UE_MVVM_SET_PROPERTY_VALUE(BackpackCount, InValue); }
	int32 GetBackpackCount() const              { return BackpackCount; }

	void SetBattleDeckCount(int32 InValue)      { UE_MVVM_SET_PROPERTY_VALUE(BattleDeckCount, InValue); }
	int32 GetBattleDeckCount() const            { return BattleDeckCount; }

	void SetPressureHunger(int32 InValue)       { UE_MVVM_SET_PROPERTY_VALUE(PressureHunger, InValue); }
	int32 GetPressureHunger() const             { return PressureHunger; }

	void SetPressureWound(int32 InValue)        { UE_MVVM_SET_PROPERTY_VALUE(PressureWound, InValue); }
	int32 GetPressureWound() const              { return PressureWound; }

	void SetPressureFatigue(int32 InValue)      { UE_MVVM_SET_PROPERTY_VALUE(PressureFatigue, InValue); }
	int32 GetPressureFatigue() const            { return PressureFatigue; }

	void SetPressureBurden(int32 InValue)       { UE_MVVM_SET_PROPERTY_VALUE(PressureBurden, InValue); }
	int32 GetPressureBurden() const             { return PressureBurden; }

	void SetPressureDecay(int32 InValue)        { UE_MVVM_SET_PROPERTY_VALUE(PressureDecay, InValue); }
	int32 GetPressureDecay() const              { return PressureDecay; }

	void SetPressureMisdeed(int32 InValue)      { UE_MVVM_SET_PROPERTY_VALUE(PressureMisdeed, InValue); }
	int32 GetPressureMisdeed() const            { return PressureMisdeed; }

	void SetPressureBloodlust(int32 InValue)    { UE_MVVM_SET_PROPERTY_VALUE(PressureBloodlust, InValue); }
	int32 GetPressureBloodlust() const          { return PressureBloodlust; }

	void SetPressureDisability(int32 InValue)   { UE_MVVM_SET_PROPERTY_VALUE(PressureDisability, InValue); }
	int32 GetPressureDisability() const         { return PressureDisability; }

	void SetPressureTotal(int32 InValue)        { UE_MVVM_SET_PROPERTY_VALUE(PressureTotal, InValue); }
	int32 GetPressureTotal() const              { return PressureTotal; }
};
