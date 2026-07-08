// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "UI/Run/WacomRunMenuCardDropIntentTypes.h"
#include "UI/Run/WacomRunMenuCardLeaseTypes.h"
#include "WacomRunMenuWidgetBase.generated.h"

/**
 * Run 专用 GameMenu Screen 基类。
 *
 * Backpack / Shop / RunEvent 等 Run 菜单继承该类。Run first-person menu
 * lease / drop 合同只在这条血统上形成正式 ownership。
 */
UCLASS(Abstract, Blueprintable)
class WACOMAPP_API UWacomRunMenuWidgetBase : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	UWacomRunMenuWidgetBase(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	bool SetOwnedRunMenuCardLeaseFromRunCards(
		FWacomRunMenuCardLeaseRequest Request,
		FWacomRunMenuCardLeaseResult& OutResult);

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|First Person Cards")
	FName GetOwnedRunMenuCardLeaseId() const
	{
		return OwnedRunMenuCardLeaseId;
	}

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	void ClearOwnedRunMenuCardLease();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	FWacomRunMenuCardDropResolveResult ResolveRunMenuCardDropIntent(
		const FWacomRunMenuCardDropResolveResult& Candidate) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	bool SubmitRunMenuCardDropIntent(
		const FWacomRunMenuCardDropResolveResult& Resolved,
		FWacomRunMenuCardDropResolveResult& OutSubmitted);

	bool HasOwnedRunMenuCardLease(FName LeaseId) const;

protected:
	virtual void NativeOnDeactivated() override;

private:
	UPROPERTY(Transient)
	FName OwnedRunMenuCardLeaseId = NAME_None;
};
