// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Map/WacomMapTypes.h"

class AWacomPlayerController;
class URunSession;
class UWacomRunMapScreen;
class FWacomRunExplorationPresentationCoordinator;
struct FWacomRunMapScreenActionRequest;
struct FRunFloorMapSnapshot;

/** PlayerController-owned Run Map Screen 生命周期、刷新和 MapTravel 提交 owner。 */
class FWacomRunMapScreenFlow
{
public:
	~FWacomRunMapScreenFlow();

	void Initialize(
		AWacomPlayerController& InOwner,
		FWacomRunExplorationPresentationCoordinator& InCoordinator);
	int32 BeginOpenRequest();
	bool IsOpenRequestCurrent(int32 RequestGeneration) const;
	void CancelOpenRequest(int32 RequestGeneration);
	bool AttachScreen(
		URunSession& InSession,
		UWacomRunMapScreen& InScreen,
		bool bPreferRecommendedTarget,
		int32 RequestGeneration = 0);
	void HandleSessionChanged(URunSession* NewSession);
	void Shutdown();

	bool IsActive() const { return Screen.IsValid(); }
	bool IsOpening() const { return bOpening; }
	bool IsTravelSubmissionPending() const { return bTravelSubmissionPending; }
	int32 GetGeneration() const { return Generation; }
	int32 GetLastPresentedVersion() const { return LastPresentedVersion; }
	FName GetLastOutcomeDetail() const { return LastOutcomeDetail; }

private:
	void HandleScreenAction(const FWacomRunMapScreenActionRequest& Request);
	void HandleScreenDeactivated();
	void HandleRunStateChanged();
	void RefreshFromSnapshot(
		const FRunFloorMapSnapshot& Snapshot,
		const FText& StatusOverride = FText::GetEmpty());
	void ConfirmTravel(const FWacomRunMapScreenActionRequest& Request);
	void CloseAndCleanup();
	void Cleanup(bool bFocusGameViewport);
	FText BuildFailureText(FName Detail) const;

	TWeakObjectPtr<AWacomPlayerController> Owner;
	TWeakObjectPtr<URunSession> Session;
	TWeakObjectPtr<UWacomRunMapScreen> Screen;
	FWacomRunExplorationPresentationCoordinator* Coordinator = nullptr;
	TOptional<FWacomMapNodeHandle> SelectedNode;
	FName OpenedFloorId = NAME_None;
	FName LastOutcomeDetail = NAME_None;
	FDelegateHandle ScreenActionHandle;
	FDelegateHandle ScreenDeactivatedHandle;
	FDelegateHandle SessionChangedHandle;
	int32 Generation = 0;
	int32 LastPresentedVersion = 0;
	bool bPreferRecommended = false;
	bool bOpening = false;
	bool bTravelSubmissionPending = false;
	bool bCleaningUp = false;
};
