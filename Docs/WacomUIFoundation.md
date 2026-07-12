---
type: presentation-contract
scope: wacom-ui-foundation
status: active
updated: 2026-07-08
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
| CardExplanationLexicon | `CardExplanationLexicon` -> C++ generated explanation fallback |
| CardDetailTheme | `CardDetailTheme` -> WBP / RichTextBlock 默认样式 |

Backpack、Pause、Shop、RunEvent 通过 `UWacomGameUIManagerSubsystem::PushRegisteredWidgetToLayerAsync()` 打开。Settings 软类未加载时走异步加载；缺失、加载失败、Cast 失败或 Push 失败时回到对应 fallback 或执行访问 rollback。

`CardExplanationLexicon` 和 `CardDetailTheme` 是卡牌详情制作入口，不保存运行时 UI 状态。Lexicon 用 DataAsset 配置效果 / 被动触发的 typed explanation template；App-private lexicon provider 按 settings 中的软对象路径缓存已加载词典，路径变化或缓存对象失效时才重新同步加载，同一路径加载失败也会缓存为 fallback 结果，未配置时使用 C++ 内置模板。Theme 用 DataAsset 配置详情标题 CommonTextStyle、正文 RichText style set 和 inline 图标 / 状态 brush；App-private theme provider 同样按 settings 软对象路径缓存已加载主题。二者未配置时属于合法 fallback：详情仍会用 C++ 内置模板和 RichTextBlock 默认样式显示。

## §3 Settings 校验

Wacom UI Settings 是顶层 UI WBP 的唯一项目级覆盖入口。未配置顶层 Screen 属于合法 fallback，不是错误。

编辑器 Data Validation 应检查：

- `PrimaryLayoutClass` 非空时必须继承 `UWacomPrimaryGameLayout`。
- `AppToastWidgetClass` 非空时必须继承 `UWacomAppToastWidget`。
- `WidgetClasses` 的 tag 必须属于 `UI.Widget.*` 命名空间。
- `WidgetClasses` 的 class 必须继承 `UWacomActivatableWidget`。
- `WidgetClasses` 中重复 tag 是错误。
- `CardExplanationLexicon` 非空时必须继承 `UWacomCardExplanationLexicon`。
- `CardDetailTheme` 非空时必须继承 `UWacomCardDetailTheme`。
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

`UWacomMainMenuButtonWidget` 是主菜单导航按钮的可实例化 CommonUI 制作入口，继承 `UWacomButtonBase`。C++ fallback 与正式主菜单 WBP 使用同一个按钮类型；正式 `WBP_MainMenuNavButton` 应继承该类，不使用原生 `UButton` 重新建立鼠标专用交互。按钮负责焦点、hover、pressed、disabled、文本和音效表现，`UWacomMainMenuScreen` 仍是 Action 广播所有者。

| Hook | 分类 |
|---|---|
| `SetButtonText / GetButtonText` | `Wacom|UI Foundation|Button Text` |
| `BP_OnButtonClicked / BP_OnHoverChanged / BP_OnInteractabilityChanged` | `Wacom|UI Foundation|Button Events` |
| `BP_PlayClickSound / BP_PlayHoverSound` | `Wacom|UI Foundation|Button Audio` |

通用 CommonUI 样式资产归 UI Foundation 制作目录管理，不放在具体业务 Widget 目录下：

| 目录 | 资产类型 | 语义 |
|---|---|---|
| `/Game/Wacom/UI/Style/Button/` | `CommonButtonStyle` | 跨 Battle / Run / Menu 复用的按钮视觉样式 |
| `/Game/Wacom/UI/Style/Text/` | `CommonTextStyle` | 跨 UI 复用的文字样式 |

业务目录只保存业务 Widget 本体。例如 `WBP_BattleCommandButton` 位于 `/Game/Wacom/UI/Battle/Action/`，但它引用的 `tiny_menu_Button`、`MyCommonTextStyle` 等通用样式应位于 `/Game/Wacom/UI/Style/...`。新增按钮或文字样式时，优先复用 UI/Style 下的资产；只有样式语义确实只属于某个业务界面时，才放入该业务目录并在对应 WBP 绑定文档说明。

## §5 Input 与 Menu Back

CommonUI 的 UIActionRouter 会把输入路由到最前面的可激活 Widget。通用菜单类界面继承 `UWacomMenuWidgetBase`，通过 `GetDesiredInputConfig()` 请求 Menu 输入。Backpack / Shop / RunEvent 这类 Run 领域 GameMenu Screen 继承 `UWacomRunMenuWidgetBase`，Run first-person menu lease / drop 合同由该 Run 专用父类承载。

项目 `GameViewportClientClassName` 使用 `UWacomGameViewportClient : UCommonGameViewportClient`。Battle / Exploration 保持 `All + NoCapture`；该模式下 UE 不保证首次 mouse-down 进入 `ViewportClient::InputKey()`，因此 ViewportClient 在 `Init()` 注册 App-private Slate `IInputProcessor`，在 `DetachViewportClient()` / `BeginDestroy()` 幂等注销。Processor 位于 `Game` priority bucket，在 Widget 路由前只仲裁一条跨 Widget 的 first-person card 输入：右键按下、指针命中路径包含当前 GameViewport 且正式拖拽来源为 `KeyboardShortcut` 时，请求 PlayerController / Anchor 中性取消并消费事件。鼠标来源拖拽、Viewport 外点击和其它右键全部继续交给 Slate、CommonUI 与 gameplay 原路由；`HandleRerouteInput()` / PlayerController `InputKey()` 只保留为其它捕获模式 fallback。该 seam 不提交 Battle / Run 命令，也不依赖鼠标当前命中某个 Slot，因此快捷键瞄准世界目标时仍可在所属 GameViewport 任意位置取消。更换 GameViewportClient 配置或 native 实现后必须完整重启编辑器再做 PIE 验收。

战斗 HUD 和探索 HUD 仍声明自身期望的 UI input config，但底层 gameplay profile 由 `UWacomInputContextCoordinatorSubsystem` 统一应用。探索期固定使用 Run Tunnel 输入模型：Coordinator 切到 `All + NoCapture`、显示鼠标并保持探索 IMC。

`UWacomMenuWidgetBase` 负责 Menu 模式下的返回键口径：ESC 和 Gamepad FaceButton Right 触发 Back 请求，默认广播 `OnBackRequestedNative` 后 `DeactivateWidget()`。子类只在语义不同，例如 ConfirmDialog 把 Back 当 Cancel 时覆盖。

当前兼容例外：`UWacomMenuWidgetBase` 仍保留 deprecated Run first-person menu lease / drop Blueprint 钩子，用于旧资产节点编译和过渡；这些旧钩子只转发到 `UWacomRunMenuWidgetBase`，普通 MainMenu / Pause / Confirm 等 Foundation 菜单不会拥有 Run menu lease。lease / drop 数据 contract 位于 `UI/Run/`，具体规则仍由 `AWacomPlayerController`、Run menu drop coordinator 或 owning menu flow 提交。后续资产清理方向见 `Docs/TechDebt.md`，不要把新的 Run 规则或一次性菜单状态继续加到 Foundation 基类。

## §6 Modal 与 MainMenu

`UWacomModalDialog / FWacomDialogButton` 是通用 Modal layer 对话框合同，只负责标题、正文、按钮和关闭回调，不提交 Battle / Run / travel 命令。

| 类型 | 分类 | 语义 |
|---|---|---|
| `FWacomDialogButton::Label / ButtonClass` | `Wacom|Common UI|Modal Dialog` | 按钮文案和按钮类 |
| `UWacomModalDialog::Show / CloseDialog` | `Wacom|Common UI|Modal Dialog` | Push 到 Modal layer 或关闭当前 dialog |
| `UWacomConfirmDialog` | 无额外分类 | 二按钮确认对话框，Confirm / Cancel 由调用方解释 |

`UWacomMainMenuScreen` 是 `L_MainMenu` 的被动顶层菜单 Screen。它只接收 `FWacomMainMenuViewData`，并通过 `EWacomMainMenuAction + OnActionRequestedNative` 上报 Continue Journey、Start New Journey、Journey History、Settings、Credits 和 Quit 意图；不直接读取 SaveGame、查找 GameMode、切关卡或退出游戏。`AWacomMenuGameMode` 是当前 ViewData producer 和 Action consumer，负责磁盘可用性检查、确认对话框、退出与 travel。

Continue 只有在 `bHasActiveJourney` 时显示，并由 `bCanContinueJourney` 决定是否可交互；History、Settings、Credits 在对应页面未接入时保持 `Collapsed`，不产生死入口。CommonUI 默认焦点优先可用 Continue，否则落到 Start New Journey；Screen 打开 `bAutoRestoreFocus`，Modal 关闭后恢复原菜单焦点。按钮 delegate 在 `NativeConstruct / NativeDestruct` 对称绑定和解绑，重复构建不会重复上报。

`WBP_MainMenuScreen` 的六个可选按钮绑定 `ContinueButton / NewJourneyButton / JourneyHistoryButton / SettingsButton / CreditsButton / QuitButton` 必须是 `UWacomMainMenuButtonWidget` 或其 WBP 子类；交互可用性统一通过 CommonUI `SetIsInteractionEnabled` 应用。摘要绑定仍为 `ActiveJourneyTitleText / ActiveJourneySummaryText`。C++ fallback 提供相同 CommonUI 按钮、左侧导航和右侧旅程摘要，确保没有正式视觉资产时仍可键鼠 / 手柄操作。

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
