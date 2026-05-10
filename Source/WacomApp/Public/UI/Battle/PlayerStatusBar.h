// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "PlayerStatusBar.generated.h"

class UWacomProgressBar;
class UTextBlock;

UCLASS(Blueprintable)
class WACOMAPP_API UPlayerStatusBar : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeRefreshFromSnapshot(const struct FBattleSnapshot& Snap) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWacomProgressBar> HpBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShieldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SanText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|UI")
	bool bHideShieldWhenZero = true;
};
