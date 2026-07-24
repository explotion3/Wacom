// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "WacomBattleFloatingCombatTextEntryWidget.generated.h"

class UImage;
class UTextBlock;
class UWacomBattleFloatingCombatTextStyle;
struct FWacomBattleFloatingCombatTextRow;

/** 单条池化战斗飘字。无 Tick，只应用 Layer 计算后的视觉帧。 */
UCLASS(Blueprintable, meta = (ToolTip = "单条 HUD 战斗飘字。由 BattleHUD 池化播放，不直接读取规则或自行 Tick。"))
class WACOMAPP_API UWacomBattleFloatingCombatTextEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void ApplyRow(
		const FWacomBattleFloatingCombatTextRow& Row,
		const UWacomBattleFloatingCombatTextStyle& Style);
	void ApplyPlaybackFrame(float Opacity, const FVector2D& Translation, float Scale);
	void ResetForPool();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SemanticIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CriticalText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ValueText = nullptr;
};
