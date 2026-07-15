// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Map/WacomRunMapScreenTypes.h"
#include "WacomRunMapEdgeLayerWidget.generated.h"

/** 无 Tick 的地图有向边批量绘制层。 */
UCLASS(Blueprintable, meta = (ToolTip = "Run 地图有向边绘制层。一次接收完整边数组并在 UMG Paint 中批量绘制，不参与选择或规则判断。"))
class WACOMAPP_API UWacomRunMapEdgeLayerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ApplyEdges(const TArray<FWacomRunMapEdgeViewData>& InEdges);
	const TArray<FWacomRunMapEdgeViewData>& GetEdges() const { return Edges; }

protected:
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	UPROPERTY(Transient)
	TArray<FWacomRunMapEdgeViewData> Edges;
};
