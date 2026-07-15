---
type: wbp-binding-contract
scope: run-map-ui
status: active
updated: 2026-07-15
---

# Run Map WBP 绑定合同

## 资产与父类

| 资产 | 父类 | 用途 |
|---|---|---|
| `/Game/Wacom/UI/Map/WBP_RunMapScreen` | `UWacomRunMapScreen` | 当前 Floor 顶层地图页面 |
| `/Game/Wacom/UI/Map/WBP_RunMapNode` | `UWacomRunMapNodeWidget` | 运行时动态创建的节点按钮 |

顶层注册 tag 是 `UI.Widget.RunMapScreen`。缺失资产或注册加载失败时，`UWacomRunMapScreen` C++ fallback 必须仍可查看、选择、确认和关闭。

## Screen optional bindings

- `MapViewportScaleBox : ScaleBox`：`ScaleToFit` 容纳 1920×1080 设计画布。
- `MapCanvas : CanvasPanel`：只承载运行时动态节点，不在 WBP 静态保存 NodeId。
- `EdgeLayer : UWacomRunMapEdgeLayerWidget`：位于节点下方，一次性批量绘制有向边，不 Tick。
- `FloorTitleText`
- `SelectedNodeTitleText`
- `SelectedNodeDescriptionText`
- `StatusText`
- `TravelButton : UWacomMenuButtonWidget`
- `CloseButton : UWacomMenuButtonWidget`

`WBP_RunMapNode` 使用 `ButtonBackdrop`、`ButtonText`、`NodeSemanticMarker` 和 `NodeTypeText`。Landmark / Revealed / Visited / Resolved / Current 由 C++ ViewData 决定；Blueprint 只能替换颜色、图标和装饰动画，不能决定传送合法性、修改 Handle 或调用 PlayerController。

Focused 与 Hover 必须走同一个琥珀色强调通道，不恢复 CommonUI 默认蓝色焦点框。节点设计坐标合法闭区间为 `[0,1920] × [0,1080]`；坐标只影响地图排版，不参与规则距离。

## 构建与验证

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomBuildRunMapUIAssets -NoSplash -Unattended
```

构建命令可重复运行，会重建两个 WidgetTree、编译并保存资产。手工修改 Designer 前需明确是否放弃可重建合同。自动化负责父类、绑定、fallback 和 tag；720p / 1080p / 1440p 的像素风布局、鼠标双击和手柄焦点留给 PIE。
