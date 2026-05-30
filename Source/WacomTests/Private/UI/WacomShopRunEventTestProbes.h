// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WacomPlayerController.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Events/WacomRunEventScreen.h"
#include "UI/Shop/WacomShopScreen.h"
#include "Engine/HitResult.h"
#include "WacomShopRunEventTestProbes.generated.h"

class URunSession;
class UWacomAppToastSubsystem;
class UPrimitiveComponent;

UCLASS()
class AWacomPlayerControllerProbe : public AWacomPlayerController
{
	GENERATED_BODY()

public:
	AActor* ReadClosestInteractable() const
	{
		return PickClosestInteractable();
	}

	FText ReadCurrentInteractPrompt() const
	{
		return BuildCurrentInteractPrompt();
	}

	void SetRunSceneHitForTest(AActor* InActor, UPrimitiveComponent* InComponent = nullptr)
	{
		bHasRunSceneHitOverride = true;
		RunSceneHitOverride = FHitResult();
		RunSceneHitOverride.HitObjectHandle = FActorInstanceHandle(InActor);
		RunSceneHitOverride.Component = InComponent;
	}

	void ClearRunSceneHitForTest()
	{
		bHasRunSceneHitOverride = false;
		RunSceneHitOverride = FHitResult();
	}

	bool ProbeRunSceneTargetForTest(FWacomInteractionTargetHandle& OutHandle) const
	{
		return TryProbeRunSceneInteractionTarget(OutHandle);
	}

	bool ProbeRunSceneTargetAtWidgetPositionForTest(
		const FVector2D& WidgetPosition,
		FWacomInteractionTargetHandle& OutHandle) const
	{
		return TryProbeRunSceneInteractionTargetAtWidgetPosition(WidgetPosition, OutHandle);
	}

	void UpdateRunWorldTargetProbePreviewForTest()
	{
		UpdateRunWorldTargetProbePreview();
	}

	void ClearRunWorldTargetProbePreviewForTest()
	{
		ClearRunWorldTargetProbePreview();
	}

protected:
	virtual bool IsInExplorationFlow() const override
	{
		return bRunProbeExplorationFlowForTest;
	}

	virtual bool BuildRunSceneClickHitResult(FHitResult& OutHitResult) const override
	{
		if (!bHasRunSceneHitOverride)
		{
			return false;
		}

		OutHitResult = RunSceneHitOverride;
		return OutHitResult.GetActor() || OutHitResult.GetComponent();
	}

	virtual bool BuildRunSceneInteractionTargetHitResultAtWidgetPosition(
		const FVector2D& WidgetPosition,
		FHitResult& OutHitResult) const override
	{
		if (!bHasRunSceneHitOverride)
		{
			return false;
		}

		OutHitResult = RunSceneHitOverride;
		OutHitResult.Location = FVector(WidgetPosition.X, WidgetPosition.Y, 0.0f);
		return OutHitResult.GetActor() || OutHitResult.GetComponent();
	}

private:
	bool bRunProbeExplorationFlowForTest = true;
	FHitResult RunSceneHitOverride;
	bool bHasRunSceneHitOverride = false;
};

UCLASS()
class UWacomShopScreenProbe : public UWacomShopScreen
{
	GENERATED_BODY()

public:
	void SetRunSession(URunSession* InRunSession)
	{
		RunSession = InRunSession;
	}

	void SetToastSubsystem(UWacomAppToastSubsystem* InToastSubsystem)
	{
		ToastSubsystem = InToastSubsystem;
	}

	FText ReadGoldText() const
	{
		return GetDisplayedGoldText();
	}

	int32 ReadOfferCount() const
	{
		return GetCachedOfferCount();
	}

	FWacomShopOfferPresentationView ReadOfferPresentationView(int32 Index) const
	{
		return GetCachedOfferView(Index);
	}

	bool PurchaseOfferAt(int32 Index)
	{
		return PurchaseOfferByIndex(Index);
	}

	void SuppressEndOnNextDeactivateForTest()
	{
		SuppressEndShopVisitOnNextDeactivate();
	}

	static FText FormatPurchaseFailureToast(FName DisabledReason)
	{
		return BuildPurchaseFailureToastText(DisabledReason);
	}

protected:
	virtual URunSession* ResolveRunSession() const override
	{
		return RunSession ? RunSession.Get() : UWacomShopScreen::ResolveRunSession();
	}

	virtual UWacomAppToastSubsystem* ResolveToastSubsystem() const override
	{
		return ToastSubsystem ? ToastSubsystem.Get() : UWacomShopScreen::ResolveToastSubsystem();
	}

private:
	UPROPERTY(Transient)
	TObjectPtr<URunSession> RunSession = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomAppToastSubsystem> ToastSubsystem = nullptr;
};

UCLASS()
class UWacomRunEventScreenProbe : public UWacomRunEventScreen
{
	GENERATED_BODY()

public:
	void SetRunSession(URunSession* InRunSession)
	{
		RunSession = InRunSession;
	}

	void SetToastSubsystem(UWacomAppToastSubsystem* InToastSubsystem)
	{
		ToastSubsystem = InToastSubsystem;
	}

	int32 ReadChoiceCount() const
	{
		return GetChoiceCount();
	}

	FRunEventChoiceSnapshot ReadChoiceSnapshot(int32 Index) const
	{
		return GetCachedChoiceSnapshot(Index);
	}

	bool ChooseChoiceAt(int32 Index)
	{
		return ChooseChoiceByIndex(Index);
	}

	void SuppressEndOnNextDeactivateForTest()
	{
		SuppressEndRunEventOnNextDeactivate();
	}

	FText ReadTitleText() const
	{
		return GetDisplayedTitleText();
	}

	FText ReadBodyText() const
	{
		return GetDisplayedBodyText();
	}

protected:
	virtual URunSession* ResolveRunSession() const override
	{
		return RunSession ? RunSession.Get() : UWacomRunEventScreen::ResolveRunSession();
	}

	virtual UWacomAppToastSubsystem* ResolveToastSubsystem() const override
	{
		return ToastSubsystem ? ToastSubsystem.Get() : UWacomRunEventScreen::ResolveToastSubsystem();
	}

private:
	UPROPERTY(Transient)
	TObjectPtr<URunSession> RunSession = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomAppToastSubsystem> ToastSubsystem = nullptr;
};
