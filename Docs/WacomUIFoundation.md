---
type: presentation-contract
scope: wacom-ui-foundation
status: active
updated: 2026-06-05
tags:
  - wacom/ui
  - wacom/commonui
  - wacom/mvvm
---

# Wacom UI Foundation 文档

> [!info] 本文职责
> 本文记录 Wacom UI Foundation 的当前事实：CommonUI shell、PrimaryLayout layer stack、项目级 UI Settings、Widget registry、通用 Widget 基类、Modal / MainMenu、Run MVVM provider 和 AppToast 例外路径。

> [!warning] 边界
> Foundation 只管理 UI shell、生命周期、软类解析和通用显示合同，不承载 Battle / Run 业务规则。具体 Screen 数据流见 [WacomUI.md](./WacomUI.md)，App 编排和输入流程见 [WacomApp.md](./WacomApp.md)。

## §1 Shell 与 Layer Stack

`UWacomGameUIManagerSubsystem` 持有当前本地 PlayerController 的 `UWacomPrimaryGameLayout`。PrimaryLayout 跟随当前 PlayerController 创建，切关卡或主菜单 travel 前通过 `TearDownPrimaryLayout()` 拆除。

默认 PrimaryLayout WBP：

- 路径：`/Game/Wacom/UI/Foundation/WBP_PrimaryGameLayout.WBP_PrimaryGameLayout_C`
- 父类：`UWacomPrimaryGameLayout`

`WBP_PrimaryGameLayout` 必须绑定 4 个 `UCommonActivatableWidgetStack`：

| 控件名 | Layer tag | 用途 |
|---|---|---|
| `GameLayerStack` | `UI.Layer.Game` | 探索 HUD、BattleHUD |
| `GameMenuLayerStack` | `UI.Layer.GameMenu` | 主菜单、暂停菜单、背包、商店、探索事件 |
| `ModalLayerStack` | `UI.Layer.Modal` | 确认弹窗、通用 modal |
| `OverlayLayerStack` | `UI.Layer.Overlay` | CommonUI 内 overlay 入口；当前 AppToast 不走该 stack |

Game 层同一时间只应有一个主要 HUD 处于 active 状态：

```text
探索 BeginPlay -> Push ExplorationHUD 到 Game 层
EnterBattle -> Push BattleHUD 到 Game 层，ExplorationHUD 非 active
ExitBattle -> Pop BattleHUD，ExplorationHUD 重新 active 并补刷新
```

## §2 顶层 UI 类解析

Wacom UI Settings 使用 `UWacomUIDeveloperSettings` 作为项目级软类注册表。顶层 UI 类解析优先级为：

```text
Wacom UI Settings
-> C++ fallback 或固定 PrimaryLayout fallback
```

`AWacomPlayerController` 不再提供顶层 ScreenClass 覆盖入口。需要替换顶层界面时，在 `Edit > Project Settings > Wacom UI Settings` 注册对应类。

| 入口 | 优先级 |
|---|---|
| PrimaryLayout | `PrimaryLayoutClass` -> 固定 `WBP_PrimaryGameLayout` 路径 -> null |
| BackpackScreen | `UI.Widget.BackpackScreen` -> `UWacomBackpackScreen` C++ fallback |
| PauseMenuScreen | `UI.Widget.PauseMenuScreen` -> `UWacomPauseMenuScreen` C++ fallback |
| ShopScreen | `UI.Widget.ShopScreen` -> `UWacomShopScreen` C++ fallback |
| RunEventScreen | `UI.Widget.RunEventScreen` -> `UWacomRunEventScreen` C++ fallback |
| AppToast | `AppToastWidgetClass` -> `UWacomAppToastWidget` C++ fallback |

Backpack、Pause、Shop、RunEvent 通过 `UWacomGameUIManagerSubsystem::PushRegisteredWidgetToLayerAsync()` 打开。Settings 软类未加载时走异步加载；缺失、加载失败、Cast 失败或 Push 失败时回到对应 fallback 或执行访问 rollback。

## §3 Settings 校验

Wacom UI Settings 是顶层 UI WBP 的唯一项目级覆盖入口。未配置顶层 Screen 属于合法 fallback，不是错误。

编辑器 Data Validation 应检查：

- `PrimaryLayoutClass` 非空时必须继承 `UWacomPrimaryGameLayout`。
- `AppToastWidgetClass` 非空时必须继承 `UWacomAppToastWidget`。
- `WidgetClasses` 的 tag 必须属于 `UI.Widget.*` 命名空间。
- `WidgetClasses` 的 class 必须继承 `UWacomActivatableWidget`。
- `WidgetClasses` 中重复 tag 是错误。
- `WidgetClasses` 中空 class 是错误；需要 fallback 时删除该条目。

Details / Blueprint 分类口径：

| 类型 | 分类 |
|---|---|
| UI manager shell lifecycle | `Wacom|UI Foundation|Shell Lifecycle` |
| UI manager layer stack | `Wacom|UI Foundation|Layer Stack` |
| PrimaryLayout push | `Wacom|UI Foundation|Primary Layout` |
| UI settings | `Wacom|UI Foundation|Settings` |
| Widget registry entry | `Wacom|UI Foundation|Widget Registry` |

## §4 Activatable 与 Button 基类

`UWacomActivatableWidget` 是项目通用 CommonUI activatable 基类，只提供生命周期和转场 WBP 钩子，不承载战斗、Run 或背包规则。

| Hook | 分类 | 语义 |
|---|---|---|
| `BP_OnPrepareActivation` | `Wacom|UI Foundation|Lifecycle` | 激活前准备 UI 本地状态或表现订阅 |
| `BP_PlayTransitionIn` | `Wacom|UI Foundation|Transitions` | 激活进入动画 |
| `BP_PlayTransitionOut` | `Wacom|UI Foundation|Transitions` | 失活退出动画 |

`UWacomButtonBase` 是 CommonUI 按钮基础合同，统一文本、点击、hover、interactable 和音效表现钩子。具体业务命令仍由 Screen、HUD 或调用方监听并提交。

| Hook | 分类 |
|---|---|
| `SetButtonText / GetButtonText` | `Wacom|UI Foundation|Button Text` |
| `BP_OnButtonClicked / BP_OnHoverChanged / BP_OnInteractabilityChanged` | `Wacom|UI Foundation|Button Events` |
| `BP_PlayClickSound / BP_PlayHoverSound` | `Wacom|UI Foundation|Button Audio` |

## §5 Input 与 Menu Back

CommonUI 的 UIActionRouter 会把输入路由到最前面的可激活 Widget。菜单类界面继承 `UWacomMenuWidgetBase`，通过 `GetDesiredInputConfig()` 请求 Menu 输入。

战斗 HUD 和探索 HUD 仍声明自身期望的 UI input config，但底层 gameplay profile 由 `UWacomInputContextCoordinatorSubsystem` 统一应用。探索期固定使用 Run Tunnel 输入模型：Coordinator 切到 `All + NoCapture`、显示鼠标并保持探索 IMC。

`UWacomMenuWidgetBase` 负责 Menu 模式下的返回键口径：ESC 和 Gamepad FaceButton Right 触发 Back 请求，默认广播 `OnBackRequestedNative` 后 `DeactivateWidget()`。子类只在语义不同，例如 ConfirmDialog 把 Back 当 Cancel 时覆盖。

## §6 Modal 与 MainMenu

`UWacomModalDialog / FWacomDialogButton` 是通用 Modal layer 对话框合同，只负责标题、正文、按钮和关闭回调，不提交 Battle / Run / travel 命令。

| 类型 | 分类 | 语义 |
|---|---|---|
| `FWacomDialogButton::Label / ButtonClass` | `Wacom|Common UI|Modal Dialog` | 按钮文案和按钮类 |
| `UWacomModalDialog::Show / CloseDialog` | `Wacom|Common UI|Modal Dialog` | Push 到 Modal layer 或关闭当前 dialog |
| `UWacomConfirmDialog` | 无额外分类 | 二按钮确认对话框，Confirm / Cancel 由调用方解释 |

`UWacomMainMenuScreen` 是 `L_MainMenu` 的顶层菜单 Screen，玩家意图委托给 `AWacomMenuGameMode` / controller 流程。`ExplorationLevelName` 位于 `Wacom|Main Menu|Authoring`，必须是 UE package path，不使用 `.AssetName` object path。

菜单按钮不直接 `OpenLevel`；切关卡由 GameMode 或 PlayerController 执行。主菜单和暂停菜单切关前先 `TearDownPrimaryLayout()`，再在下一帧 `OpenLevel()`，避免在 CommonUI deactivate 链中立即切关。

## §7 Run MVVM Provider

`UWacomRunViewModelProvider` 是 Run MVVM Global ViewModel provider，只提供只读 ViewModel 访问和 RunSession 同步。`GetRunViewModel()` 分类为 `Wacom|UI Foundation|MVVM`。

Run 域 HUD 使用 `UWacomRunViewModelProvider + UWacomRunViewModel` 显示探索状态。Shop / RunEvent / Backpack Screen 仍读取 `URunSession` Snapshot 或 ViewData；Battle UI 不走 Run MVVM。

## §8 AppToast 例外路径

`UWacomAppToastSubsystem` 是战斗外通用反馈出口。它持有唯一 `UWacomAppToastWidget`，在真实本地玩家和 `LocalPlayer` 就绪时直接 `AddToViewport(ZOrder=10000)`，不进入 CommonUI Stack，不改变探索或菜单输入。自动化测试或预热路径可以注入离屏 Widget；这类 Widget 只接收数据和队列刷新，不会被强制加入 viewport。

Toast WBP 注册：

- 父类：`UWacomAppToastWidget`
- 设置入口：`Wacom UI Settings.AppToastWidgetClass`
- 可选绑定：`Container : VerticalBox`
- 未注册、软类加载失败或类型不匹配时，回退 `UWacomAppToastWidget` C++ fallback。

当前生命周期：

- 探索局开始时 PlayerController 会预热；首次 Toast 也会懒加载兜底；两者都需要真实本地玩家和 `LocalPlayer` 才创建 viewport Widget。
- 无消息时 Widget `Collapsed`；入队后 `HitTestInvisible`；消息播完后只隐藏，不销毁。
- Widget `SetIsFocusable(false)`，`GetDesiredInputConfig()` 返回空。
- Subsystem 跨关卡跟随 GameInstance，但只复用属于当前 World 和当前本地 PlayerController 的缓存 Widget；没有运行时 World / PlayerController 的离屏自动化注入 Widget 可以作为测试替身复用。
- 缓存归属检查只从 owning player / outer 推导 World 和 PlayerController，不直接询问 Widget `GetWorld()`，避免离屏 Widget 在自动化中污染日志。
- `Deinitialize()` 会移除并清空缓存 Widget。

当前接入包括商店购买、背包移动 / 删除、RunEvent 结果、Run world card drop release 失败和 pickup reward。战斗内玩家日志不走 AppToast，见 [WacomBattleUI.md](./WacomBattleUI.md)。
