// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/SubclassOf.h"
#include "UI/Foundation/WacomAppToastTypes.h"
#include "WacomAppToastSubsystem.generated.h"

class UCardDefinition;
class APlayerController;
class UWorld;
class UWacomAppToastWidget;

/**
 * App-level toast outlet for non-battle feedback such as shop purchases,
 * backpack deletion, node rewards, pickups, and future run settlement messages.
 *
 * This widget is added directly to the viewport instead of a CommonUI Stack so
 * it never becomes the leaf-most CommonUI page and never changes input mode.
 */
UCLASS()
class WACOMAPP_API UWacomAppToastSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Toast")
	UWacomAppToastWidget* EnsureAppToastReady();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Toast")
	void ShowToast(const FWacomAppToastView& View);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Toast")
	void ShowTextToast(FText Message, EWacomAppToastTone Tone = EWacomAppToastTone::Neutral);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Toast")
	void ShowCardGained(UCardDefinition* Card);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Toast")
	void ShowGoldChanged(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Toast")
	void ShowWarning(FText Message);

	void SetToastWidgetOverrideForTest(UWacomAppToastWidget* InWidget) { ToastWidget = InWidget; }
	UWacomAppToastWidget* GetToastWidgetForTest() const { return ToastWidget; }

#if WITH_AUTOMATION_TESTS
	bool IsToastOwnerPairUsableForTest(
		const UWorld* WidgetWorld,
		const APlayerController* WidgetOwner,
		const UWorld* CurrentWorld,
		const APlayerController* CurrentPC) const;
#endif

private:
	UPROPERTY(Transient)
	TObjectPtr<UWacomAppToastWidget> ToastWidget = nullptr;

	UPROPERTY(Transient)
	TSubclassOf<UWacomAppToastWidget> ToastWidgetClass;

	UWacomAppToastWidget* EnsureToastWidget();
	APlayerController* FindLocalPlayerController() const;
	bool IsToastWidgetUsableForCurrentOwner(const UWacomAppToastWidget* Widget, const APlayerController* CurrentPC) const;
	bool IsToastOwnerPairUsable(
		const UWorld* WidgetWorld,
		const APlayerController* WidgetOwner,
		const UWorld* CurrentWorld,
		const APlayerController* CurrentPC) const;
	void ClearToastWidget();
};
