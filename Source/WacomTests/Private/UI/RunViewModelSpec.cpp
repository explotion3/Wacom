// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "FieldNotification/FieldId.h"
#include "FieldNotification/IFieldValueChanged.h"

#include "UI/ViewModels/WacomRunViewModel.h"

/**
 * UI ViewModel 单测（M5 收尾）。
 *
 * 验证 UWacomRunViewModel 的核心契约：
 *   - Setter 改值并触发 FieldNotify 广播
 *   - 派生 FieldNotify 函数（GetExperienceRatio）在依赖字段变化时级联通知
 *   - 重复设同值不重复广播
 *
 * 不测：Provider 的 RunSession 同步逻辑（属于 Run 域，写在 BackpackSpec 间接覆盖）
 *       Widget 端的 SetText（UI 自动化测试不在第一阶段范围）
 */

namespace
{
	/**
	 * FieldNotify 计数器：订阅 ViewModel 上的指定字段，记录广播次数。
	 *
	 * 用法：
	 *   FFieldChangeCounter Counter(VM, "PressureWound");
	 *   VM->SetPressureWound(5);
	 *   Counter.GetCount();  // 1
	 */
	struct FFieldChangeCounter
	{
		FFieldChangeCounter(UWacomRunViewModel* InVM, FName FieldName)
			: VM(InVM)
		{
			if (!VM) { return; }

			const UE::FieldNotification::IClassDescriptor& Descriptor =
				VM->GetFieldNotificationDescriptor();
			Field = Descriptor.GetField(VM->GetClass(), FieldName);
			if (!Field.IsValid()) { return; }

			Handle = VM->AddFieldValueChangedDelegate(
				Field,
				INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda(
					[this](UObject*, UE::FieldNotification::FFieldId)
					{
						++Count;
					}));
		}

		~FFieldChangeCounter()
		{
			if (VM && Handle.IsValid())
			{
				VM->RemoveFieldValueChangedDelegate(Field, Handle);
			}
		}

		int32 GetCount() const { return Count; }

	private:
		TObjectPtr<UWacomRunViewModel> VM;
		UE::FieldNotification::FFieldId Field;
		FDelegateHandle Handle;
		int32 Count = 0;
	};
}

// ================ Setter 触发广播 ================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunViewModelSetterBroadcastsSpec,
	"Wacom.UI.RunViewModel.SetterBroadcasts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunViewModelSetterBroadcastsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunViewModel> VM(NewObject<UWacomRunViewModel>());

	FFieldChangeCounter Wound(VM.Get(), TEXT("PressureWound"));

	TestEqual(TEXT("初始值"),  VM->GetPressureWound(), 0);
	TestEqual(TEXT("初始 broadcast 0 次"), Wound.GetCount(), 0);

	VM->SetPressureWound(5);
	TestEqual(TEXT("Set 后值变 5"), VM->GetPressureWound(), 5);
	TestEqual(TEXT("Set 后广播 1 次"), Wound.GetCount(), 1);

	VM->SetPressureWound(10);
	TestEqual(TEXT("再次 Set 后值变 10"), VM->GetPressureWound(), 10);
	TestEqual(TEXT("再次广播"), Wound.GetCount(), 2);

	return true;
}

// ================ 重复设同值不重复广播（UE_MVVM_SET_PROPERTY_VALUE 内置 dedupe）================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunViewModelSameValueNoBroadcastSpec,
	"Wacom.UI.RunViewModel.SameValueDoesNotRebroadcast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunViewModelSameValueNoBroadcastSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunViewModel> VM(NewObject<UWacomRunViewModel>());

	FFieldChangeCounter Gold(VM.Get(), TEXT("Gold"));

	VM->SetGold(7);
	TestEqual(TEXT("首次 Set 广播"), Gold.GetCount(), 1);

	VM->SetGold(7);
	TestEqual(TEXT("同值 Set 不广播"), Gold.GetCount(), 1);

	VM->SetGold(8);
	TestEqual(TEXT("不同值 Set 广播"), Gold.GetCount(), 2);

	return true;
}

// ================ 派生 FieldNotify 函数级联（GetExperienceRatio）================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunViewModelExperienceRatioCascadeSpec,
	"Wacom.UI.RunViewModel.ExperienceRatioCascadesOnDependencyChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunViewModelExperienceRatioCascadeSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunViewModel> VM(NewObject<UWacomRunViewModel>());

	FFieldChangeCounter RatioCounter(VM.Get(), TEXT("GetExperienceRatio"));

	VM->SetExperienceCapacity(10);
	VM->SetExperienceCurrent(0);
	TestEqual(TEXT("ratio=0"), VM->GetExperienceRatio(), 0.f);

	const int32 BaselineCount = RatioCounter.GetCount();

	VM->SetExperienceCurrent(5);
	TestEqual(TEXT("ratio=0.5"), VM->GetExperienceRatio(), 0.5f);
	TestEqual(TEXT("Current 变 → ratio 级联广播"),
		RatioCounter.GetCount(), BaselineCount + 1);

	VM->SetExperienceCapacity(20);
	TestEqual(TEXT("ratio=0.25"), VM->GetExperienceRatio(), 0.25f);
	TestEqual(TEXT("Capacity 变 → ratio 级联广播"),
		RatioCounter.GetCount(), BaselineCount + 2);

	return true;
}
