---
type: presentation-contract
scope: wacom-ui-foundation
status: active
updated: 2026-07-14
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
| SettingsScreen | `UI.Widget.SettingsScreen` -> `UWacomSettingsScreen` C++ fallback |
| ShopScreen | `UI.Widget.ShopScreen` -> `UWacomShopScreen` C++ fallback |
| RunEventScreen | `UI.Widget.RunEventScreen` -> `UWacomRunEventScreen` C++ fallback |
| AppToast | `AppToastWidgetClass` -> `UWacomAppToastWidget` C++ fallback |
| BattleEnemyPanel | Host `EnemyPanelWidgetClass` -> `DefaultBattleEnemyPanelWidgetClass` -> hidden + error |
| CardExplanationLexicon | `CardExplanationLexicon` -> C++ generated explanation fallback |
| CardDetailTheme | `CardDetailTheme` -> WBP / RichTextBlock 默认样式 |

Backpack、Pause、Settings、Shop、RunEvent 通过 `UWacomGameUIManagerSubsystem::PushRegisteredWidgetToLayerAsync()` 打开。软类未加载时走异步加载；缺失、加载失败、Cast 失败或 Push 失败时回到对应 fallback 或执行访问 rollback。

`CardExplanationLexicon` 和 `CardDetailTheme` 是卡牌详情制作入口，不保存运行时 UI 状态。Lexicon 用 DataAsset 配置效果 / 被动触发的 typed explanation template；App-private lexicon provider 按 settings 中的软对象路径缓存已加载词典，路径变化或缓存对象失效时才重新同步加载，同一路径加载失败也会缓存为 fallback 结果，未配置时使用 C++ 内置模板。Theme 用 DataAsset 配置详情标题 CommonTextStyle、正文 RichText style set 和 inline 图标 / 状态 brush；App-private theme provider 同样按 settings 软对象路径缓存已加载主题。二者未配置时属于合法 fallback：详情仍会用 C++ 内置模板和 RichTextBlock 默认样式显示。

## §3 Settings 校验

Wacom UI Settings 是顶层 UI WBP 的唯一项目级覆盖入口。未配置顶层 Screen 属于合法 fallback，不是错误。

编辑器 Data Validation 应检查：

- `PrimaryLayoutClass` 非空时必须继承 `UWacomPrimaryGameLayout`。
- `AppToastWidgetClass` 非空时必须继承 `UWacomAppToastWidget`。
- `DefaultBattleEnemyPanelWidgetClass` 必须配置且继承 abstract `UWacomBattleEnemyPanelWidget`；Scene Enemy Host 可显式覆盖，二者都无有效类时面板隐藏而不是创建 C++ 布局 fallback。
- `WidgetClasses` 的 tag 必须属于 `UI.Widget.*` 命名空间。
- `WidgetClasses` 的 class 必须继承 `UWacomActivatableWidget`。
- `UI.Widget.SettingsScreen` 必须继承 `UWacomSettingsScreen`；其它已知顶层 Screen tag 同样校验自己的具体 C++ 父类。
- `UI.Widget.BattleKnockdownChoiceDialog` 必须继承 `UWacomKnockdownChoiceDialog`；正式类为 `/Game/Wacom/UI/Battle/Knockdown/WBP_BattleKnockdownChoiceDialog`。BattleHUD 激活时同步解析并缓存该类；条目缺失、加载失败或父类错误时使用功能性 C++ fallback，不允许让阻塞式击倒选择失去提交入口。
- 击倒选择 Dialog / Option 的初始正式 WidgetTree 由 Editor-only `WacomBuildKnockdownChoiceUI` Builder 确定性生成；它只管理两个固定 Package，`-InspectOnly` 只读校验 Registry 目标、父类、BindWidget 与 `WBP_CardView` 引用。运行时不得调用 Builder，也不得把 C++ fallback 扩成第二套正式视觉。
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

`UWacomPrimaryGameLayout` 缓存每个 Layer Stack 的 `OnTransitioningChanged` 状态，并通过 native delegate 广播 Layer Tag 与开始/结束状态。该合同只暴露 CommonUI Shell 的表现生命周期，不承载业务状态；需要避免在层 Alpha 过渡中烘入离屏缓存的 Screen（当前包括承载 `WBP_FPCardView` 的背包卡面）应在 Construct/Activate 时查询当前状态、订阅变化，并在 Destruct 时解除订阅。背包在过渡中暂停 retained caching，结束后恢复静态按需补绘与“最多一张实时卡”的表现预算；不要用固定帧数或持续 Tick 猜测过渡结束时间。

## §4 Activatable 与 Button 基类

`UWacomActivatableWidget` 是项目通用 CommonUI activatable 基类，只提供生命周期和转场 WBP 钩子，不承载战斗、Run 或背包规则。

| Hook | 分类 | 语义 |
|---|---|---|
| `BP_OnPrepareActivation` | `Wacom|UI Foundation|Lifecycle` | 激活前准备 UI 本地状态或表现订阅 |
| `BP_PlayTransitionIn` | `Wacom|UI Foundation|Transitions` | 激活进入动画 |
| `BP_PlayTransitionOut` | `Wacom|UI Foundation|Transitions` | 失活退出动画 |

`UWacomButtonBase` 是 CommonUI 按钮基础合同，统一文本、点击、hover、interactable 和音效表现钩子。`ButtonText_Cached` 是可序列化的制作属性，`SynchronizeProperties()` 会把它应用到绑定的 `ButtonText`，因此嵌套在其它 WBP 内的按钮实例可以各自保存文案。具体业务命令仍由 Screen、HUD 或调用方监听并提交。

`UWacomMenuButtonWidget` 同时服务 WBP 与 Pause / Run Map 等 C++ fallback。原生 fallback 必须在 `InitializeNativeClassData()` 建立非空 `WidgetTree.RootWidget`，让后续 `UCommonButtonBase::Initialize()` 把内容包入并绑定真正的内部 `SCommonButton`；不能把首次建树延后到 `RebuildWidget()`，否则控件虽然可见且可聚焦，却不会进入 Slate 鼠标点击链。该建树逻辑不能放在 `Initialize()` 的 `Super` 调用前，否则会阻止 Blueprint Generated Class 复制 authored widget tree。`Wacom.UI.GameMenu.PointerRouting.NativeButtonsReceiveSlateMouseClicks` 使用实际 Slate hit-test、hover、mouse-down / mouse-up 序列锁定 Pause Resume 与正式 Run Map Travel 两条原生按钮路径。

`UWacomMainMenuButtonWidget` 是主菜单导航按钮的可实例化 CommonUI 制作入口，继承 `UWacomButtonBase`。C++ fallback 与正式主菜单 WBP 使用同一个按钮类型；`/Game/Wacom/UI/Menus/WBP_MainMenuNavButton` 已继承该类，不使用原生 `UButton` 重新建立鼠标专用交互。按钮负责焦点、hover、pressed、disabled、文本和音效表现，当前原生表现驱动只在状态过渡期间短暂 Tick，统一控制背景、左侧强调条、焦点符号、文字色、位移与缩放；`UWacomMainMenuScreen` 仍是 Action 广播所有者。

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

Enemy HUD 的紧凑数字使用 `/Game/Wacom/UI/Foundation/Fonts/Silkscreen/F_Silkscreen` Runtime Composite Font，来源 FontFace 为 Regular / Bold。它只服务拉丁数字、基本符号与短英文战术标识；中文敌人名、部位名和详情文案继续使用项目现有中文 UI 字体，不允许依赖 Silkscreen 的缺字 fallback 猜测排版。字体采用 SIL Open Font License 1.1，来源与随附许可记录见 [ThirdPartyLicenses.md](./ThirdPartyLicenses.md)。

Enemy Panel 是每个 Host 的唯一 Local Settings 订阅者。它把 Simplified Motion 与 Flash policy 推送给稳定复用的 Part Entry；Entry 不直接持有设置 Subsystem，也不建立逐部位订阅。Panel `NativeDestruct()` 集中解绑，Entry teardown 只停止自身动画、弱 Intro timer、MID 和瞬时 Intent 状态。

## §5 Input 与 Menu Back

CommonUI 的 UIActionRouter 会把输入路由到最前面的可激活 Widget。通用菜单类界面继承 `UWacomMenuWidgetBase`，通过 `GetDesiredInputConfig()` 请求 Menu 输入。Backpack / Shop / RunEvent 这类 Run 领域 GameMenu Screen 继承 `UWacomRunMenuWidgetBase`，Run first-person menu lease / drop 合同由该 Run 专用父类承载。

Battle 二级信息面板是明确例外：`UWacomBattleSecondaryPanelScreenBase` 仍 Push 到 `UI.Layer.GameMenu`，但返回 `All + NoCapture`，不取得 Menu/UIOnly 的镜头锁。它只负责 Backdrop、关闭按钮、Esc、右键、Gamepad B 与 Viewport focus 恢复；战斗命令抑制由 BattleHUD 的 secondary-panel coordinator 独立持有。这样战斗表现与镜头继续运行，同时卡牌、Wait、EndTurn、目标提交和世界点击不会穿透。新的 Battle 敌人/人物/牌堆详情页应复用此基类和 coordinator，而不是复制一套输入 gate。

项目 `GameViewportClientClassName` 使用 `UWacomGameViewportClient : UCommonGameViewportClient`。Battle / Exploration 保持 `All + NoCapture`；该模式下 UE 不保证首次 mouse-down 进入 `ViewportClient::InputKey()`，因此 ViewportClient 在 `Init()` 注册 App-private Slate `IInputProcessor`，在 `DetachViewportClient()` / `BeginDestroy()` 幂等注销。Processor 位于 `Game` priority bucket，在 Widget 路由前只仲裁一条跨 Widget 的 first-person card 输入：右键按下、指针命中路径包含当前 GameViewport 且正式拖拽来源为 `KeyboardShortcut` 时，请求 PlayerController / Anchor 中性取消并消费事件。鼠标来源拖拽、Viewport 外点击和其它右键全部继续交给 Slate、CommonUI 与 gameplay 原路由；`HandleRerouteInput()` / PlayerController `InputKey()` 只保留为其它捕获模式 fallback。该 seam 不提交 Battle / Run 命令，也不依赖鼠标当前命中某个 Slot，因此快捷键瞄准世界目标时仍可在所属 GameViewport 任意位置取消。更换 GameViewportClient 配置或 native 实现后必须完整重启编辑器再做 PIE 验收。

战斗 HUD 和探索 HUD 仍声明自身期望的 UI input config，但底层 gameplay profile 由 `UWacomInputContextCoordinatorSubsystem` 统一应用。探索期固定使用 Run Tunnel 输入模型：Coordinator 切到 `All + NoCapture`、显示鼠标并保持探索 IMC。

`UWacomMenuWidgetBase` 负责 Menu 模式下的返回键口径：ESC 和 Gamepad FaceButton Right 触发 Back 请求，默认广播 `OnBackRequestedNative` 后 `DeactivateWidget()`。子类只在语义不同，例如 ConfirmDialog 把 Back 当 Cancel、MainMenu 把 Back 交给 App flow、TitleScreen 作为稳定根消费 Back 时覆盖。激活后的延迟焦点会优先使用 `NativeGetDesiredFocusTarget()`，再寻找第一个可交互 `UCommonButtonBase`，最后才兼容旧 `UButton`，因此标题页、主菜单、暂停菜单和 Settings fallback 都保持键盘 / 手柄可用。

`UWacomKnockdownChoiceDialog` 是 Battle 领域的强制选择 Modal 例外：仍继承 Menu 基类以取得 UIOnly 输入和 CommonUI 焦点管理，但覆盖 `NativeHandleBackRequested()`，统一消费 ESC / Gamepad B 而不 Pop。其默认焦点顺序由 C++ 固定为 Aid → Destroy → Withdraw；WBP 不通过 Designer 顺序或事件图改写该规则。提交成功后 CommonUI Pop 并自动归还 Battle 输入，提交失败则保持 Modal active、恢复合法选项交互并只调用 WBP 反馈 hook。

当前兼容例外：`UWacomMenuWidgetBase` 仍保留 deprecated Run first-person menu lease / drop Blueprint 钩子，用于旧资产节点编译和过渡；这些旧钩子只转发到 `UWacomRunMenuWidgetBase`，普通 MainMenu / Pause / Confirm 等 Foundation 菜单不会拥有 Run menu lease。lease / drop 数据 contract 位于 `UI/Run/`，具体规则仍由 `AWacomPlayerController`、Run menu drop coordinator 或 owning menu flow 提交。后续资产清理方向见 `Docs/TechDebt.md`，不要把新的 Run 规则或一次性菜单状态继续加到 Foundation 基类。

## §6 Modal 与 MainMenu

`UWacomModalDialog / FWacomDialogButton` 是通用 Modal layer 对话框合同，只负责标题、正文、按钮和关闭回调，不提交 Battle / Run / travel 命令。

| 类型 | 分类 | 语义 |
|---|---|---|
| `FWacomDialogButton::Label / ButtonClass` | `Wacom|Common UI|Modal Dialog` | 按钮文案和按钮类 |
| `UWacomModalDialog::Show / CloseDialog` | `Wacom|Common UI|Modal Dialog` | Push 到 Modal layer 或关闭当前 dialog |
| `UWacomConfirmDialog` | 无额外分类 | 二按钮确认对话框，Confirm / Cancel 由调用方解释 |

`UWacomTitleScreen` 是 `L_MainMenu` 的稳定栈底页面。它没有存档或旅程数据，只通过 `OnAdvanceRequestedNative` 上报进入主菜单意图；键盘按键、鼠标左键和手柄按键可以继续，ESC / Gamepad B 始终被消费且不会关闭页面。`AWacomMenuGameMode` 在每次进入主菜单关时先 Push TitleScreen，继续后才在同一 `UI.Layer.GameMenu` Stack 上 Push MainMenu；MainMenu 的 `ReturnToTitle` 由 GameMode 校验栈底仍存在后再 Pop。标题 WBP 加载失败时回退原生 TitleScreen，主菜单 flow 失败时标题页保持 active 并允许玩家再次输入重试。

`UWacomMainMenuScreen` 是 `L_MainMenu` 的被动导航 Screen。它只接收 `FWacomMainMenuViewData`，并通过 `EWacomMainMenuAction + OnActionRequestedNative` 上报 Continue Journey、Start New Journey、Journey History、Settings、Credits、Quit 和 ReturnToTitle 意图；不直接读取 SaveGame、查找 GameMode、切关卡、Pop 页面或退出游戏。`AWacomMenuGameMode` 是当前 ViewData producer 和 Action consumer，负责页面栈、磁盘可用性检查、确认对话框、退出与 travel。没有有效 GameMode consumer 时 MainMenu Back 只会上报意图并保持 active，不能自行弹出空栈。

Continue 只有在 `bHasActiveJourney` 时显示，并由 `bCanContinueJourney` 决定是否可交互；Settings 已开放，History、Credits 在对应页面未接入时保持 `Collapsed`，不产生死入口。CommonUI 默认焦点优先可用 Continue，否则落到 Start New Journey；Screen 打开 `bAutoRestoreFocus`，Modal 或 Settings Screen 关闭后恢复原菜单焦点。按钮 delegate 在 `NativeConstruct / NativeDestruct` 对称绑定和解绑，重复构建不会重复上报。

`/Game/Wacom/UI/Menus/WBP_TitleScreen` 与 `/Game/Wacom/UI/Menus/WBP_MainMenuScreen` 共用 `L_MainMenu` 实时场景。TitleScreen 提供 `TitleContentRoot / PressAnyKeyText` 固定绑定、中央品牌和输入提示；Full UI Motion 下提示执行轻量呼吸透明度，Simplified 下固定显示。MainMenu 使用透明深色像素遮罩，左侧为品牌与六个导航入口，右侧为活动旅程摘要。六个可选按钮绑定 `ContinueButton / NewJourneyButton / JourneyHistoryButton / SettingsButton / CreditsButton / QuitButton` 均使用 `WBP_MainMenuNavButton`；摘要绑定为 `ActiveJourneyTitleText / ActiveJourneySummaryText`，表现根绑定为 `MenuContentRoot / JourneySummaryPanel`。Screen 激活时由原生表现驱动执行一次左右错峰淡入位移，不改变 ViewData 或 Action 合同。交互可用性统一通过 CommonUI `SetIsInteractionEnabled` 应用。C++ fallback 提供相同核心绑定与行为，确保资产缺失或损坏时仍可键鼠 / 手柄操作。

`UWacomSettingsScreen` 的数据源固定为 `UWacomSettingsSubsystem`，而不是直接编辑 `UWacomGameUserSettings`：Screen 激活时持有一次 `FWacomSettingsEditSession`，选项行只上报步进 / Slider 意图，Screen 修改本地 draft 并提交 `Preview / Apply / Cancel`。显示、图形、音频、视角和辅助五类由一个 App-private 字段描述表统一；分辨率行消费 Subsystem 返回的 `FWacomScreenResolutionOptions`，不会自行查询平台或合成任意候选项。无边框窗口仍显示当前桌面分辨率但禁用，独占全屏和窗口模式只展示符合项目白名单及平台限制的档位；合法的桌面原生 / 当前自定义分辨率可以作为例外保留。`GetDefaultSnapshot()` 返回与首次启动相同的项目平衡档；Screen 底部“恢复默认”一次恢复全部分类，只装入并预览当前 Draft，仍需 Apply 才保存，Cancel 可完整回到 Baseline。

全局 UMG DPI 使用 `UIScaleRule=Custom` 和 `UWacomCappedDesignDPIScalingRule`，设计基准固定为 `1920 × 1080`，`ApplicationScale=1.0`。规则按 `min(1, ViewportWidth / 1920, ViewportHeight / 1080)` 计算缩放：`1280 × 720` 约为 `0.667`、`1920 × 1080` 为 `1.0`，`2560 × 1440` 与 `3840 × 2160` 仍为 `1.0`。因此较小视口会完整容纳基准画布，而较高分辨率不会再次放大主菜单、Settings、暂停菜单、Run / Battle HUD 框架等按设计单位制作的固定尺寸元素；16:10 和超宽屏同样按较短比例适配并封顶。`UIScaleCurve` 与引擎 `ScaleToFit` 不再作为其它缩放来源；WBP 与 C++ fallback 都继承这一全局结果。本阶段最低视口为 `1280 × 720`，不增加 Settings 行或 Footer 的响应式重排，也不嵌套额外 `ScaleBox`。

Backpack 正式 Screen 直接 Fill `UI.Layer.GameMenu`，不再保留固定 `1600×900` 子画布，也不叠加第二个全屏 `ScaleBox`。完整卡牌的 `CardFaceScaleBox` 资产制作值保持 `1.0`，保存原生 `296×420` authored 卡面坐标；运行时由 `DA_BackpackWorkspaceStyle.CardDisplayScale=0.78` 统一得到约 `231×328` 的布局、渲染和命中尺寸。折叠牌堆同样显示全部真实 `WBP_FPCardView`，以固定显示尺寸和水平露出适配，不再使用简化预览。这个局部缩放不是 viewport profile，不消费 first-person `PresentationScale`，分辨率变化统一由上述全局 DPI 和统一 Workspace 内的折叠/手风琴布局吸收。

CommonUI 继续拥有键盘和手柄的焦点导航。主菜单导航按钮与通用 `UWacomMenuButtonWidget` 都在 Construct / Destruct 中对称订阅 `UCommonButtonBase::OnFocusReceived / OnFocusLost`，将 CommonButton 内部 Slate 焦点与鼠标 Hover 合并为同一个强调状态；主菜单继续使用自己的底板、强调条、箭头和插值动画，Settings / Pause / Modal 共用按钮则使用深色底板、琥珀强调条和文字色反馈。`UWacomSettingsOptionRow` 额外通过 focus-path 事件统一处理“焦点在行本身、左右步进按钮或 Slider 内”的整行高亮，因此切换分类后落到选项行也有明确反馈。

Backpack Workspace 是复合 Canvas，因此真实 Slate 焦点保持在 Workspace 根，由 App-private Navigation Controller 按 `InstanceId / Zone` 绘制虚拟焦点并执行空间导航；这不绕过 CommonUI 的 Screen 激活、返回和 Modal 焦点所有权。Backpack Screen 在 Activate/Construct 有效期订阅 `UCommonInputSubsystem` 输入类型变化，Deactivate/Destruct 对称退订；情境提示随鼠标键盘/手柄切换。操作说明层打开前保存当前 Slate 焦点，关闭后恢复，且 Focus 与 Selected/Valid/Rejected 使用独立图标，不依赖颜色。

`UWacomGameViewportClient` 只在当前焦点属于已经声明项目焦点皮肤的 `UWacomMainMenuButtonWidget`、`UWacomMenuButtonWidget` 或 `UWacomSettingsOptionRow` 时抑制 UE 通用蓝色 `FocusRectangle`，避免同一控件同时绘制两套选中反馈。该策略不关闭全局 `RenderFocusRule`；其它尚未拥有自绘焦点反馈的控件仍保留引擎默认焦点框，同时不改变控件 focusability、CommonUI 导航、确认或焦点恢复所有权。

First-person 卡牌是明确的局部表现例外，不是第二套全局 DPI。美术真源 `148 × 210`、WBP 制作画布 `296 × 420` 均保持不变；Anchor 的 App-private 策略在全局 DPI 之后追加以 `2560 × 1440` 为参照、`0.5–1.0` 物理封顶的 `PresentationScale`。720p / 1080p / 1440p 的最终物理倍率分别为 `0.5 / 0.75 / 1.0`，4K 不继续放大。该倍率只服务 Battle / Run first-person 手牌、first-person 详情和相关空间特效；背包详情及其它 HUD 不消费它。

视频变化使用同一 token Push `UWacomSettingsConfirmationDialog` 到 Modal layer；Modal 只上报“保留 / 恢复 / 超时”，由 Screen 调用 `ConfirmVideoMode / RevertVideoMode`。Apply 完成后页面保持打开并开启新 token；未保存返回先确认放弃；Deactivate / Destruct 对 edit、视频确认、ticker 和 delegate 做对称清理。`FWacomSettingsScreenFlow` 是主菜单与暂停菜单的共享打开 seam，负责 `UI.Widget.SettingsScreen` 软类解析、异步 Push 和重复打开保护。Screen、行和 Modal 不接触玩家档案或旅程 SaveGame，也不自行保存 ini。

正式可重建资产位于 `/Game/Wacom/UI/Settings/`：`WBP_SettingsScreen`、`WBP_SettingsOptionRow`、`WBP_SettingsButton`、`WBP_SettingsConfirmationDialog`。它们沿用主菜单的深色像素面板、青色数值和琥珀色焦点语义；C++ fallback 提供相同绑定与行为合同。Settings footer 固定为状态文本、`RestoreDefaultsButton`、`ApplyButton`、`BackButton`；恢复成功后焦点转到 Apply，若无需应用则转到 Back，避免手柄焦点滞留在已禁用的恢复按钮。暂停菜单按钮已迁到 `UWacomMenuButtonWidget`，Construct / Destruct 对称绑定，并通过同一 flow 打开 Settings。

`EWacomUIMotionMode::Simplified` 是全局运行时表现策略，不修改 WBP 或 Anchor 制作参数。当前它让 TitleScreen 输入提示停止呼吸、主菜单 Screen 入场和导航按钮状态插值立即完成，并强制 first-person 卡牌使用已有 reduced-motion 路径；默认 `Full` 下现有标题、主菜单和卡牌节奏不变。

主菜单三份 WBP（`WBP_TitleScreen / WBP_MainMenuScreen / WBP_MainMenuNavButton`）可以在编辑器关闭时通过 `UnrealEditor-Cmd.exe Wacom.uproject -run=WacomBuildMainMenuAssets` 重建；Settings 四份 WBP、音频总线资产和玩家 CameraShake / WalkBob 制作开关通过 `-run=WacomBuildSettingsAssets` 重建与校验；Run Map 的 `WBP_RunMapScreen / WBP_RunMapNode` 通过 `-run=WacomBuildRunMapUIAssets` 重建。Run Map 使用 `UI.Widget.RunMapScreen` 软类注册，具体父类必须是 `UWacomRunMapScreen`，缺失或加载失败时保留完整 C++ fallback。构建器位于 `WacomEditor` Private，只负责项目内固定资产的制作树、编译和保存，不进入运行时模块；手工在 Designer 调整资产前应先确认是否还需要保留“可重建”合同，避免下一次运行构建器覆盖视觉改动。

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
