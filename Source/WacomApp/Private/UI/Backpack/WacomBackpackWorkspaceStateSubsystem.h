// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WacomBackpackWorkspaceTypes.h"
#include "WacomBackpackWorkspaceStateSubsystem.generated.h"

class URunSession;

/** 可由 GameInstanceSubsystem 和无 GameInstance 的自动化 Screen 共同持有的纯瞬态状态。 */
struct WACOMAPP_API FWacomBackpackWorkspaceStateStore
{
	/** 返回 true 表示绑定身份变化并已清空旧 Run 的全部工作台状态。 */
	bool BindToRun(URunSession* RunSession);
	void Reset();

	URunSession* GetBoundRun() const { return BoundRun.Get(); }
	bool HasActiveZone() const { return ActiveZone.IsSet(); }
	FWacomBackpackZoneKey GetActiveZone() const;
	void SetActiveZone(const FWacomBackpackZoneKey& ZoneKey);

	const FWacomBackpackWorkspaceLayoutEntry* FindLayout(
		const FWacomBackpackZoneKey& ZoneKey,
		FGuid InstanceId) const;
	void SetLayout(
		const FWacomBackpackZoneKey& ZoneKey,
		FGuid InstanceId,
		const FWacomBackpackWorkspaceLayoutEntry& Entry);
	void ClearLayout(const FWacomBackpackZoneKey& ZoneKey, FGuid InstanceId);
	void ClearZoneLayouts(const FWacomBackpackZoneKey& ZoneKey);
	int32 GetManualLayoutCount(const FWacomBackpackZoneKey& ZoneKey) const;

	/** 只保留仍属于该可见区域的 InstanceId；新卡不会自动生成手动布局。 */
	void ReconcileZone(
		const FWacomBackpackZoneKey& ZoneKey,
		TConstArrayView<FGuid> VisibleInstanceIds);

private:
	TWeakObjectPtr<URunSession> BoundRun;
	TOptional<FWacomBackpackZoneKey> ActiveZone;
	TMap<FWacomBackpackZoneKey, TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry>> LayoutsByZone;
};

/** 当前 GameInstance 内唯一的背包工作台瞬态状态 owner；不序列化。 */
UCLASS()
class UWacomBackpackWorkspaceStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FWacomBackpackWorkspaceStateStore& GetStoreForRun(URunSession* RunSession);

private:
	FWacomBackpackWorkspaceStateStore Store;
};
