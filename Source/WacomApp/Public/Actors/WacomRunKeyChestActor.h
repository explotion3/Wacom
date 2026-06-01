// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Interaction/WacomRunWorldClickableInteractable.h"
#include "Interaction/WacomWorldInteractable.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomRunKeyChestActor.generated.h"

class AWacomPlayerController;
class UPrimitiveComponent;
class UStaticMesh;
class URunSession;
class UWacomInteractionTargetComponent;
class UWacomRunKeyChestDefinition;
class UWacomRunWorldCardInteractionDefinition;
class UWacomRunWorldCardDropReceiverComponent;
class UWacomRunWorldInteractionTargetBridgeComponent;

UCLASS(NotBlueprintable, HideDropdown, CollapseCategories,
	HideCategories = (Object, ActorComponent, Physics, Collision, Navigation, Cooking, Events, Tags, AssetUserData,
		ComponentTick, ComponentReplication, Activation, Rendering, HLOD, Mobile, RayTracing, TextureStreaming))
class WACOMAPP_API UWacomRunKeyChestTriggerSphereComponent : public USphereComponent
{
	GENERATED_BODY()
};

UCLASS(NotBlueprintable, HideDropdown, CollapseCategories,
	HideCategories = (Object, ActorComponent, Physics, Collision, Navigation, Cooking, Events, Tags, AssetUserData,
		ComponentTick, ComponentReplication, Activation, Rendering, HLOD, Mobile, RayTracing, TextureStreaming))
class WACOMAPP_API UWacomRunKeyChestClickBoundsComponent : public UBoxComponent
{
	GENERATED_BODY()
};

UCLASS(NotBlueprintable, HideDropdown, CollapseCategories,
	HideCategories = (Object, ActorComponent, Physics, Collision, Navigation, Cooking, Events, Tags, AssetUserData,
		ComponentTick, ComponentReplication, Activation, HLOD, Mobile, RayTracing, TextureStreaming))
class WACOMAPP_API UWacomRunKeyChestVisualComponent : public UStaticMeshComponent
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunKeyChestDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FString ActorName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName PersistentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName DefinitionName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName CardInteractionDefinitionName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName InteractionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName CardInteractionDefinitionConfigWarningReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName DefinitionSource = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName ChestId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName DefinitionConfigWarningReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	bool bHasRunSession = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	bool bCompleted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	bool bCanInteract = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	bool bConfigValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName ConfigWarningReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	bool bDuplicatePersistentIdDetected = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	bool bClickTargetConfigured = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName ClickStableId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	float TriggerRadius = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FVector ClickBoundsExtent = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName VisualName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName VisualMeshName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FVector VisualScale = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName CompletedVisualMeshName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FVector CompletedVisualScale = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FVector CompletedVisualRelativeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName VisualState = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	bool bHasCardDropReceiver = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	bool bReceiverCanSubmit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName ReceiverRejectReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	int32 ReceiverAllowedDefinitionCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	int32 ReceiverAllowedCardIdCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	int32 ReceiverRequiredKeywordCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	int32 ReceiverBlockedKeywordCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	bool bReceiverHasPositiveCardFilter = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	bool bReceiverConsumeCardOnSuccess = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	int32 ReceiverGoldReward = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FString InteractPrompt;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FString HoverPrompt;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FString CompletedPrompt;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FString ReceiverDebugSummary;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Key Chest|Debug")
	FName LastDebugResult = NAME_None;
};

/**
 * Run 世界钥匙宝箱原型 V2。
 *
 * E 键和普通左键只显示提示；真正开箱只通过 first-person card drag release 到
 * CardDropReceiver，再提交 URunSession 的 Run world card interaction 事务。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunKeyChestActor
	: public AActor
	, public IWacomWorldInteractable
	, public IWacomRunWorldClickableInteractable
{
	GENERATED_BODY()

public:
	AWacomRunKeyChestActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Key Chest",
		meta = (ToolTip = "宝箱在当前 Run 内的唯一 ID。拖卡成功后用它写入 CompletedRunWorldInteractionIds；None 会拒绝拖卡提交。"))
	FName PersistentId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Key Chest|Definition",
		meta = (ToolTip = "通用 Run 世界拖卡交互定义。推荐正式 KeyChest 使用它配置卡牌筛选、金币奖励、是否消耗和反馈文案；PersistentId 仍来自场景 Actor。"))
	TObjectPtr<UWacomRunWorldCardInteractionDefinition> CardInteractionDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Key Chest|Definition",
		meta = (ToolTip = "旧 KeyChest 专用静态定义，保留为兼容入口。未填写 CardInteractionDefinition 时才会使用；PersistentId 仍来自场景 Actor。"))
	TObjectPtr<UWacomRunKeyChestDefinition> ChestDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Key Chest",
		meta = (ToolTip = "玩家进入该半径后，探索期按 E 可看到宝箱提示。单位：厘米。",
			ClampMin = "50.0", UIMin = "50.0", UIMax = "1000.0"))
	float TriggerRadius = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Key Chest|Authoring",
		meta = (ToolTip = "鼠标 hover、左键点击和拖卡命中的隐藏盒体半径。单位：厘米。会同步到内部 ClickBounds；不要直接编辑组件 Collision Details。",
			ClampMin = "1.0", UIMin = "1.0"))
	FVector ClickBoundsExtent = FVector(85.f, 65.f, 55.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Key Chest|Authoring",
		meta = (ToolTip = "宝箱原型的安全可见网格。会同步到内部 ChestVisual；留空时只保留碰撞和交互诊断。"))
	TObjectPtr<UStaticMesh> VisualMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Key Chest|Authoring",
		meta = (ToolTip = "宝箱原型可见网格相对缩放。只影响内部 ChestVisual，不影响 ClickBounds 命中范围。"))
	FVector VisualScale = FVector(0.75f, 0.55f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Key Chest|Authoring",
		meta = (ToolTip = "宝箱原型可见网格相对位置。单位：厘米；只影响内部 ChestVisual。"))
	FVector VisualRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Key Chest|Authoring|Completed",
		meta = (ToolTip = "宝箱完成后的安全可见网格。为空时复用关闭态 VisualMesh；只影响内部 ChestVisual，不影响命中范围。"))
	TObjectPtr<UStaticMesh> CompletedVisualMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Key Chest|Authoring|Completed",
		meta = (ToolTip = "宝箱完成后的相对缩放。默认比关闭态更扁；只影响内部 ChestVisual，不影响 ClickBounds 命中范围。"))
	FVector CompletedVisualScale = FVector(0.75f, 0.55f, 0.18f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Key Chest|Authoring|Completed",
		meta = (ToolTip = "宝箱完成后的相对位置。单位：厘米；默认轻微下移配合扁平 scale。"))
	FVector CompletedVisualRelativeLocation = FVector(0.f, 0.f, -18.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Key Chest|Text",
		meta = (ToolTip = "未打开时，玩家在 E 键范围内看到的提示文案。"))
	FText InteractPromptText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Key Chest|Text",
		meta = (ToolTip = "鼠标 hover 到宝箱 ClickBounds 时看到的提示文案。"))
	FText HoverPromptText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Key Chest|Text",
		meta = (ToolTip = "宝箱已经打开后显示的弱提示文案。"))
	FText CompletedPromptText;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Key Chest")
	USphereComponent* GetTriggerSphere() const { return TriggerSphere; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Key Chest")
	UBoxComponent* GetClickBounds() const { return ClickBounds; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Key Chest")
	UStaticMeshComponent* GetChestVisual() const { return ChestVisual; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Key Chest")
	UWacomInteractionTargetComponent* GetClickInteractionTargetComponent() const
	{
		return ClickInteractionTargetComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Key Chest")
	UWacomRunWorldInteractionTargetBridgeComponent* GetClickTargetBridgeComponent() const
	{
		return ClickTargetBridgeComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Key Chest")
	UWacomRunWorldCardDropReceiverComponent* GetCardDropReceiverComponent() const
	{
		return CardDropReceiverComponent;
	}

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Wacom|Run|Key Chest|Authoring",
		meta = (ToolTip = "配置调试钥匙宝箱：按 Actor 名生成 PersistentId，接受 DA_Card_DebugKey，奖励 3 金币，并刷新点击 stable id；不会修改 RunState。"))
	void ConfigureDebugKeyChestSample();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Wacom|Run|Key Chest|Authoring",
		meta = (ToolTip = "配置调试钥匙宝箱并绑定通用 DA_RunWorldCardInteraction_DebugKeyGold3：按 Actor 名生成 PersistentId，清空旧 ChestDefinition，并刷新点击 stable id；不会修改 RunState。"))
	void ConfigureDebugKeyChestInteractionDefinitionSample();

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Key Chest|Debug",
		meta = (ToolTip = "读取钥匙宝箱配置、完成状态、点击目标和拖卡接收器诊断；不会修改 RunState。"))
	FWacomRunKeyChestDebugView GetRunKeyChestDebugView(AWacomPlayerController* PC) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Key Chest|Debug",
		meta = (ToolTip = "返回钥匙宝箱一行诊断摘要。"))
	FString GetRunKeyChestDebugSummary(AWacomPlayerController* PC) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Key Chest|Debug",
		meta = (ToolTip = "将钥匙宝箱诊断摘要写入日志。"))
	void LogRunKeyChestDebugSummary(AWacomPlayerController* PC) const;

	// ---- IWacomWorldInteractable ----
	virtual FText GetInteractPromptText_Implementation(AWacomPlayerController* PC) const override;
	virtual FVector GetInteractLocation_Implementation(AWacomPlayerController* PC) const override;
	virtual bool CanInteract_Implementation(AWacomPlayerController* PC) const override;
	virtual bool TryInteract_Implementation(AWacomPlayerController* PC) override;

	// ---- IWacomRunWorldClickableInteractable ----
	virtual FText GetRunWorldClickHoverPrompt_Implementation(AWacomPlayerController* PC) const override;
	virtual FWacomRunWorldClickableInteractableDebugView GetRunWorldClickableDebugView_Implementation(
		AWacomPlayerController* PC) const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void EnsureRunSessionBinding(AWacomPlayerController* PC);

private:
	void RefreshAuthoringState();
	void RefreshVisualState();
	void RefreshClickTargetBinding();
	void SyncReceiverFromDefinition();
	void TryBindRunSessionFromWorld();
	void BindRunSessionForCompletedVisual(URunSession* Run);
	void UnbindRunSession();
	void HandleRunStateChanged();
	FName BuildConfigWarningReason() const;
	bool HasDuplicatePersistentIdInWorld() const;
	bool IsCompletedForBoundRunSession() const;
	bool IsCompletedFor(AWacomPlayerController* PC) const;
	FText GetHoverPromptText(AWacomPlayerController* PC) const;
	FText ResolveInteractPromptText() const;
	FText ResolveHoverPromptText() const;
	FText ResolveCompletedPromptText() const;
	FText GetDefaultInteractPromptText() const;
	FText GetDefaultHoverPromptText() const;
	FText GetDefaultCompletedPromptText() const;
	static FName BuildDebugKeyChestPersistentIdFromActorName(const FString& ActorName);
	void ShowChestHintToast(AWacomPlayerController* PC) const;

	UPROPERTY(Transient)
	TObjectPtr<UWacomRunKeyChestTriggerSphereComponent> TriggerSphere = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomRunKeyChestClickBoundsComponent> ClickBounds = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomRunKeyChestVisualComponent> ChestVisual = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomInteractionTargetComponent> ClickInteractionTargetComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomRunWorldInteractionTargetBridgeComponent> ClickTargetBridgeComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomRunWorldCardDropReceiverComponent> CardDropReceiverComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<URunSession> BoundRunSession = nullptr;
};
