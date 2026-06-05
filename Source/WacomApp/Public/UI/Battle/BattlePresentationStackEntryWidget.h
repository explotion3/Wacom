// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "BattlePresentationStackEntryWidget.generated.h"

class UPanelWidget;
class USizeBox;
class UWacomCardView;

USTRUCT(BlueprintType, meta = (ToolTip = "Battle 表现栈单张只读小卡的 UI ViewData。仅用于显示已提交卡牌的表现 backlog，不代表规则栈或可交互卡牌。"))
struct WACOMAPP_API FWacomBattlePresentationStackEntryView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation Stack", meta = (ToolTip = "表现栈 entry 的运行时 UI id，用于区分同一张卡的不同表现条目。"))
	int32 EntryId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation Stack", meta = (ToolTip = "该表现条目对应的卡牌实例 id。只用于 UI 识别和调试读取。"))
	FGuid CardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation Stack", meta = (ToolTip = "该条目是否正在播放退出表现。为 true 时 Widget 只做视觉退出，不提交规则命令。"))
	bool bIsExiting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation Stack", meta = (ToolTip = "表现栈小卡使用的只读卡面 ViewData。"))
	FWacomCardViewData CardViewData;
};

/**
 * Read-only mini-card entry for the Battle presentation stack.
 *
 * It renders card view data only; it does not handle click, drag, or command submission.
 */
UCLASS(Blueprintable, meta = (ToolTip = "Battle 表现栈中的单张只读小卡 Widget。只渲染卡面 ViewData，不处理点击、拖拽或命令提交。"))
class WACOMAPP_API UBattlePresentationStackEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Presentation Stack", meta = (ToolTip = "设置表现栈小卡的只读 entry ViewData，并触发 WBP 更新事件。"))
	void SetPresentationStackEntryData(const FWacomBattlePresentationStackEntryView& InEntry);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation Stack", meta = (ToolTip = "当前表现栈小卡显示的 entry ViewData。只用于展示或调试读取。"))
	const FWacomBattlePresentationStackEntryView& GetCurrentEntry() const { return CurrentEntry; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Presentation Stack", meta = (ToolTip = "设置表现栈小卡使用的 CardView 类。为空时由上层表现栈使用默认解析。"))
	void SetMiniCardViewClass(TSubclassOf<UWacomCardView> InClass);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation Stack", meta = (ToolTip = "当前用于显示完整卡面的只读 Mini CardView。"))
	UWacomCardView* GetMiniCardView() const { return MiniCardView; }

#if WITH_AUTOMATION_TESTS
	bool HasHeaderOrTargetTextWidgetsForTest() const { return false; }
	bool HasMiniCardScaleHostForTest() const { return MiniCardScaleHost != nullptr; }
	void TickExitForTest(float DeltaSeconds);
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Presentation Stack", DisplayName = "On Entry Updated", meta = (ToolTip = "表现栈 entry ViewData 已刷新时的 WBP 表现事件。只能用于更新样式或动画，不应提交 BattleSession 命令。"))
	void BP_OnEntryUpdated(const FWacomBattlePresentationStackEntryView& Entry);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> CardHost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> MiniCardScaleHost;

private:
	UPROPERTY(Transient)
	FWacomBattlePresentationStackEntryView CurrentEntry;

	UPROPERTY(Transient)
	TObjectPtr<UWacomCardView> MiniCardView;

	UPROPERTY(Transient)
	TSubclassOf<UWacomCardView> RuntimeMiniCardViewClass;

	float ExitElapsedSeconds = 0.0f;
	bool bPreviousExitingState = false;

	void EnsureMiniCardView();
	void ApplyCurrentEntry();
	void ApplyExitVisual(float NormalizedAlpha);
};
