// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "BattleCombatLogTurnDividerWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;

UENUM(BlueprintType)
enum class EWacomBattleCombatLogTurnBoundaryKind : uint8
{
	Start UMETA(DisplayName = "Turn Start"),
	End UMETA(DisplayName = "Turn End"),
};

/** 战斗日志详情页的回合开始/结束分割线。 */
UCLASS(Blueprintable)
class WACOMAPP_API UBattleCombatLogTurnDividerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Combat Log Details",
		meta = (ToolTip = "设置回合分割线的回合数、开始/结束语义和沙漏图标。只刷新 UI。"))
	void SetTurnDividerData(
		int32 InTurnNumber,
		EWacomBattleCombatLogTurnBoundaryKind InBoundaryKind,
		const FSlateBrush& InTurnIconBrush);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> DividerRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TurnIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TurnText;

private:
	void ApplyCurrentData();

	int32 TurnNumber = 1;
	EWacomBattleCombatLogTurnBoundaryKind BoundaryKind = EWacomBattleCombatLogTurnBoundaryKind::Start;
	FSlateBrush TurnIconBrush;
};
