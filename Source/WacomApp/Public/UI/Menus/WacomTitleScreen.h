// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Containers/Ticker.h"
#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomTitleScreen.generated.h"

class UTextBlock;
class UWidget;
struct FWacomLocalSettingsSnapshot;
enum class EWacomRuntimeSettingsChangeReason : uint8;
struct FWacomTitleScreenTestAccess;

DECLARE_MULTICAST_DELEGATE(FWacomTitleAdvanceRequested);

/**
 * L_MainMenu 的被动标题页。
 *
 * 标题页常驻 GameMenu layer 栈底，只把键盘、鼠标左键或手柄按键转换为
 * OnAdvanceRequestedNative。页面切换由 AWacomMenuGameMode 负责；ESC / Gamepad B
 * 在这里始终被消费，不能把根页面弹空。
 */
UCLASS(Blueprintable, meta = (ToolTip = "L_MainMenu 的被动 Press Any Key 标题页。只上报继续意图；页面切换由 AWacomMenuGameMode 负责，ESC 与手柄返回键不会关闭根页面。"))
class WACOMAPP_API UWacomTitleScreen : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	UWacomTitleScreen(const FObjectInitializer& ObjectInitializer);

	/** App flow 监听的原生“进入主菜单”意图。 */
	FWacomTitleAdvanceRequested OnAdvanceRequestedNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(
		const FGeometry& InGeometry,
		const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeHandleBackRequested() override;

	/** 正式 WBP 与 fallback 共用的标题内容根。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> TitleContentRoot;

	/** “按任意键继续”提示；Full motion 下执行轻量呼吸表现。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PressAnyKeyText;

private:
	void RequestAdvance();
	void StartPromptPulse();
	void StopPromptPulse();
	bool TickPromptPulse(float DeltaTime);
	void BindRuntimeSettings();
	void UnbindRuntimeSettings();
	void HandleRuntimeSettingsChanged(
		const FWacomLocalSettingsSnapshot& Snapshot,
		EWacomRuntimeSettingsChangeReason Reason);

	FTSTicker::FDelegateHandle PromptPulseTickerHandle;
	float PromptPulseElapsedSeconds = 0.0f;
	bool bAdvanceRequestInFlight = false;
	bool bRuntimeSimplifiedMotion = false;
	FDelegateHandle RuntimeSettingsChangedHandle;

#if WITH_AUTOMATION_TESTS
	friend struct FWacomTitleScreenTestAccess;
#endif
};
