// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "WacomWorldShopHUDWidget.generated.h"

class UTextBlock;

/** 透明且不拦截指针的 World Shop 说明层。 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomWorldShopHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetGold(int32 Gold);

protected:
	virtual void NativeOnInitialized() override;

private:
#if WITH_AUTOMATION_TESTS
	friend class FWacomWorldShopWidgetTestAccess;
#endif

	void EnsureFallbackWidgetTree();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GoldText = nullptr;
};
