// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunStateTypes.h"
#include "WacomZoneDropTarget.generated.h"

class UWacomBackpackScreen;
class UWacomCardDragOperation;
class UBorder;
class UDragDropOperation;
class UWidget;
struct FRunDeckOperationValidation;

UENUM(BlueprintType)
enum class EWacomDropTargetState : uint8
{
	Normal,
	HoverValid,
	HoverInvalid,
	DropAccepted,
	DropRejected,
	ConfirmPending
};

/**
 * 背包 zone 拖拽接收器。
 *
 * Widget 生命周期声明：
 *   数据源：父 UWacomBackpackScreen 注入的 URunSession，仅用于 drop 预览校验。
 *   更新触发：自身不刷新列表；MoveInstance 成功后走 RunSession -> ViewModelProvider -> BackpackScreen RebuildAll。
 *   订阅时机：不订阅任何事件。
 *   反订阅时机：不持有委托，NativeDestruct 无需反订阅。
 *   焦点/输入：只处理 UMG DragOver / Drop，不主动获取键盘焦点。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomZoneDropTarget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(EZoneKind InZoneKind, FGuid InOwnerInstanceId);
	void SetOwnerScreen(UWacomBackpackScreen* InScreen);
	void SetDropContent(UWidget* InContent);
	bool TryHandleDropOperation(UDragDropOperation* InOperation);
	void SetDropTargetState(EWacomDropTargetState InState);
	static bool ShouldPreviewDrop(EZoneKind TargetZone, EZoneKind SourceZone, int32 BattleDeckCount, int32 BattleDeckCapacity);
	static FText FormatZoneNameForToast(EZoneKind Zone);
	static FText FormatMoveFailureReasonForToast(FName DisabledReason);

	UFUNCTION(BlueprintPure, Category = "Wacom|Backpack|Drop")
	EWacomDropTargetState GetDropTargetState() const { return DropTargetState; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Backpack|Drop")
	void BP_OnDropTargetStateChanged(EWacomDropTargetState NewState);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack")
	EZoneKind ZoneKind = EZoneKind::Backpack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack")
	FGuid OwnerInstanceId;

	UPROPERTY(Transient)
	TWeakObjectPtr<UWacomBackpackScreen> OwnerScreen;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> DropContent;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Wacom|Backpack|Drop", meta = (AllowPrivateAccess = "true"))
	EWacomDropTargetState DropTargetState = EWacomDropTargetState::Normal;

	bool CanPreviewDrop(const UWacomCardDragOperation& CardOp) const;
};
