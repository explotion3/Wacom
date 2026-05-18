// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomBackpackScreen.generated.h"

class UButton;
class UTextBlock;
class UWrapBox;
class UVerticalBox;
class UCardDefinition;
class URunSession;
class UWacomDeleteZoneDropTarget;
class UWacomDeckCardWidget;
class UWacomZoneDropTarget;
class UWacomRunViewModel;
class UWacomRunViewModelProvider;
struct FCardInstance;

/**
 * 背包界面（GDD §11）。Push 到 GameMenu 层。
 *
 * 由 PlayerController 在按 B 时 Push（或 Console Command）。
 *
 * 三大区域（垂直堆叠）：
 *   - 删牌区（DeleteZone）：玩家拖卡过来 → 置换金币
 *   - 备战区（BattleDeckZone）：BattleDeck 内容
 *   - 背包区（BackpackZone）：Backpack 内容（通量 + 负重区，第一阶段同一 WrapBox 区分前后）
 *
 * Stage 4.2 用点击切换语义：
 *   - Backpack 卡点击主体 → AddCardToBattleDeck
 *   - BattleDeck 卡点击主体 → RemoveCardFromBattleDeck
 *   - 任何卡的删除按钮 → DeleteCardForGold（弹 Confirm）
 *
 * MVVM 数据流（M2）：
 *   - 顶部标量（金币 / 备战区 N/M / 背包区 N/M）：读 RunViewModel 字段
 *   - WrapBox 列表内容：直接读 RunSession.GetBackpack/GetBattleDeck
 *     （UE MVVM 不擅长数组绑定，列表数据保留命令式读取）
 *   - 刷新触发：订阅 Provider.OnRunViewModelRefreshedNative，事件驱动
 *   - 操作命令：Move/Delete 仍直接写 RunSession，写完事件自动回流刷新
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomBackpackScreen : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	UWacomBackpackScreen(const FObjectInitializer& ObjectInitializer);

	/** 从 PC 拿当前 RunSession（每帧都拿，不持引用）。DropTarget 通过这里提交 MoveInstance。 */
	URunSession* GetRunSession() const;

	static FText BuildSpecialZoneTitleText(const FText& OwnerName, int32 CardCount, int32 Capacity);
	static ESlateVisibility GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind OwnerZone);
	static FText BuildBurdenZoneTitleText(int32 CardCount);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** 全量重建：顶部读 ViewModel，WrapBox 子项读 RunSession 列表。 */
	void RebuildAll();

	/** Provider 广播回调。 */
	void HandleViewModelRefreshed();

	/** 订阅 Provider（如果还没订阅）+ 刷新一次。 */
	void TrySubscribeAndRefresh();

	/** 卡按钮回调：删牌（弹 Confirm 后调 DeleteCardForGold）。 */
	void HandleDeleteCard(UCardDefinition* Card);

	/** SpecialZone 卡右键切换入战标记。 */
	void HandleBattleEnabledToggle(FGuid InstanceId);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeleteZoneTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BattleDeckTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BackpackTitleText;

	/** 备战区卡牌容器（WrapBox 自动横向流式 + 换行）。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> BattleDeckCardsBox;

	/** 背包通量+负重统一容器。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> BackpackCardsBox;

	UPROPERTY(Transient)
	TObjectPtr<UWacomZoneDropTarget> BattleDeckDropTarget;

	UPROPERTY(Transient)
	TObjectPtr<UWacomZoneDropTarget> BackpackDropTarget;

	UPROPERTY(Transient)
	TObjectPtr<UWacomDeleteZoneDropTarget> DeleteDropTarget;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> SpecialZonesPanel;

	UPROPERTY(Transient)
	TObjectPtr<UWrapBox> BurdenCardsBox;

	UPROPERTY(Transient)
	TObjectPtr<UWacomZoneDropTarget> BurdenDropTarget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BurdenZoneTitleText;

	/** 关闭按钮（点击 = DeactivateWidget）。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UFUNCTION()
	void HandleCloseClicked();

private:
	UWacomRunViewModelProvider* GetProvider() const;
	UWacomRunViewModel* GetViewModel() const;

	UPROPERTY(Transient)
	TObjectPtr<UWacomRunViewModelProvider> SubscribedProvider = nullptr;

	/** 销毁现有 WrapBox 子项。 */
	void ClearCardBoxes();

	/** 子控件类（默认 UWacomDeckCardWidget，蓝图可覆盖）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack")
	TSubclassOf<UWacomDeckCardWidget> CardWidgetClass;

	/** 创建一张卡的 widget 并接好回调。 */
	UWacomDeckCardWidget* CreateCardWidget(const FCardInstance& Inst, EZoneKind FromZone, FGuid FromZoneOwnerInstanceId);

	void RebuildTopStats(UWacomRunViewModel* VM, URunSession* Run);
	void RebuildBattleDeckZone(URunSession* Run);
	void AddBattleEnabledSpecialZoneCardsToBattleDeckView(URunSession* Run);
	void RebuildBackpackZone(URunSession* Run);
	void RebuildSpecialZones(URunSession* Run);
	void RebuildBurdenZone(URunSession* Run);
};
