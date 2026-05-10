# Phase 2 Plan

本文规划第二阶段的开发方向。第一阶段（S1-S11）已完成战斗内核。第二阶段目标是建立一个**完善的 UI 系统基底**，让后续所有 UI 需求（战斗、背包、商店、菜单、对话）都能在同一套框架上快速搭建。

时间充裕，不急于出最小可玩原型。优先把基础设施做对。

## 1. 第二阶段目标

- 建立基于 CommonUI 的完整 UI 管理框架（不只是战斗 UI）。
- 战斗 UI 作为框架的第一个消费者，验证框架设计。
- UI 框架支持：层级管理、输入路由、动画基类、数据绑定模式、主题/样式系统。
- 保持"规则在内核、UI 只读 Snapshot + 发 Command"的边界。
- 第一阶段的规则补全（保留、中毒、ZoneHook、被动）作为 UI 完成后的快速迭代。

## 2. 切片总览

| 切片 | 名称 | 内容 |
| --- | --- | --- |
| **P1** | **UI 基础设施** | CommonUI 框架搭建、层级管理、输入路由、基类体系 |
| **P2** | **战斗 UI** | 在 P1 框架上实现战斗界面 |
| **P3** | 规则补全 | 保留 / 中毒 / ZoneHook / 伙伴被动 |
| **P4** | Enhanced Input | 键鼠 + 手柄输入统一 |
| **P5** | UI 动画基础 | Widget 动画基类、事件驱动的表现调度 |
| **P6** | 主题与样式 | 颜色/字体/间距的集中管理 |

## 3. P1：UI 基础设施

### 3.1 CommonUI 核心组件

```
UWacomGameUIManagerSubsystem (UGameUIManagerSubsystem)
├── 管理 PrimaryGameLayout
├── 管理 Input Routing
└── 管理 UI Policy

UWacomUIPolicy (UCommonGameUIPolicy)
├── 定义 Layout 类
└── 定义 Input 行为

UWacomPrimaryGameLayout (UPrimaryGameLayout)
├── Layer: Game (HUD、战斗 UI)
├── Layer: GameMenu (暂停、背包)
├── Layer: Modal (确认框、奖励选择)
└── Layer: Overlay (Toast、提示)
```

### 3.2 Layer 设计

| Layer | 用途 | 输入行为 | 示例 |
| --- | --- | --- | --- |
| `Game` | 游戏内常驻 UI | 透传到游戏 | 战斗 HUD、探索 HUD |
| `GameMenu` | 游戏内菜单 | 阻断游戏输入 | 暂停菜单、背包、商店 |
| `Modal` | 模态弹窗 | 阻断下层 UI | 确认框、击倒奖励选择 |
| `Overlay` | 非阻断覆盖 | 不影响输入 | 事件 Toast、伤害数字 |

Layer 用 `FGameplayTag` 标识，在 `UWacomPrimaryGameLayout` 里注册。

### 3.3 Widget 基类体系

```
UWacomActivatableWidget : UCommonActivatableWidget
├── 提供：OnActivated / OnDeactivated 虚函数
├── 提供：GetOwningSession() 便捷访问
├── 提供：RefreshFromSnapshot(FBattleSnapshot) 虚函数
└── 提供：PlayTransitionIn / PlayTransitionOut 动画钩子

UWacomButtonBase : UCommonButtonBase
├── 统一按钮样式
├── 统一音效钩子
└── 统一 Hover/Press 状态

UWacomTabListWidget : UCommonTabListWidgetBase
└── 后续背包/商店的 Tab 切换
```

### 3.4 数据绑定模式

第二阶段不引入完整 MVVM 框架（UE 5.7 的 `UMVVMViewModelBase` 还在实验阶段）。

采用**手动绑定 + Snapshot 驱动**模式：

```
UBattleSession
    → BuildSnapshot()
        → FBattleSnapshot (纯数据)
            → UBattleHUD::RefreshFromSnapshot(Snap)
                → 各子 Widget::Refresh(对应子结构)
```

约定：
- Widget 不持有状态副本。每次 Refresh 都从 Snapshot 完整重建显示。
- Widget 可以持有"上一帧 Snapshot"用于 diff 驱动动画（P5 再做）。
- Widget 不直接调用 `UBattleSession::SubmitCommand`，而是通过委托/事件通知 HUD，由 HUD 统一提交。这样 Widget 可以被复用在不同上下文（例如"出牌预览"和"实际出牌"用同一个 CardWidget）。

### 3.5 输入路由

CommonUI 的输入路由核心：
- `UCommonActivatableWidget` 的 `GetDesiredInputConfig` 控制该 Widget 激活时的输入行为。
- `ECommonInputMode`：`Menu`（鼠标可见、游戏输入阻断）/ `Game`（鼠标隐藏、游戏输入透传）/ `All`。

战斗 UI 的输入模式：
- 战斗 HUD 激活时：`All`（鼠标可见 + 游戏输入透传，允许键盘快捷键）。
- Modal 弹窗激活时：`Menu`（阻断游戏输入，聚焦弹窗）。

### 3.6 代码位置

```
Source/WacomApp/
├── Public/
│   └── UI/
│       ├── Foundation/
│       │   ├── WacomGameUIManagerSubsystem.h
│       │   ├── WacomUIPolicy.h
│       │   ├── WacomPrimaryGameLayout.h
│       │   ├── WacomActivatableWidget.h
│       │   ├── WacomButtonBase.h
│       │   └── WacomUITags.h              // Layer tag 声明
│       ├── Battle/
│       │   ├── BattleHUD.h
│       │   ├── EnemyPanel.h
│       │   ├── EnemyPartWidget.h
│       │   ├── PlayerStatusBar.h
│       │   ├── ActionBar.h
│       │   ├── HandPanel.h
│       │   ├── CardWidget.h
│       │   └── EventToast.h
│       └── Common/
│           ├── WacomProgressBar.h         // HP/Shield 通用进度条
│           ├── WacomTooltip.h             // 通用 Tooltip
│           └── WacomModalDialog.h         // 通用模态确认框
└── Private/
    └── UI/
        ├── Foundation/
        │   ├── WacomGameUIManagerSubsystem.cpp
        │   ├── WacomUIPolicy.cpp
        │   ├── WacomPrimaryGameLayout.cpp
        │   ├── WacomActivatableWidget.cpp
        │   └── WacomButtonBase.cpp
        ├── Battle/
        │   ├── BattleHUD.cpp
        │   ├── EnemyPanel.cpp
        │   ├── EnemyPartWidget.cpp
        │   ├── PlayerStatusBar.cpp
        │   ├── ActionBar.cpp
        │   ├── HandPanel.cpp
        │   ├── CardWidget.cpp
        │   └── EventToast.cpp
        └── Common/
            ├── WacomProgressBar.cpp
            ├── WacomTooltip.cpp
            └── WacomModalDialog.cpp
```

### 3.7 Content 资产

```
Content/Wacom/UI/
├── Foundation/
│   ├── WBP_PrimaryGameLayout.uasset
│   └── WBP_ModalDialog.uasset
├── Battle/
│   ├── WBP_BattleHUD.uasset
│   ├── WBP_EnemyPanel.uasset
│   ├── WBP_EnemyPartWidget.uasset
│   ├── WBP_PlayerStatusBar.uasset
│   ├── WBP_ActionBar.uasset
│   ├── WBP_HandPanel.uasset
│   ├── WBP_CardWidget.uasset
│   └── WBP_EventToast.uasset
└── Common/
    ├── WBP_ProgressBar.uasset
    └── WBP_Tooltip.uasset
```

### 3.8 P1 内部子步骤

| 步骤 | 内容 |
| --- | --- |
| P1.1 | `WacomApp.Build.cs` 加 CommonUI / UMG / Slate 依赖；`uproject` 确认 CommonUI 插件启用 |
| P1.2 | `WacomGameUIManagerSubsystem` + `WacomUIPolicy` + `WacomPrimaryGameLayout`：Layer 注册 |
| P1.3 | `WacomActivatableWidget` 基类：Snapshot 刷新虚函数、动画钩子、Session 访问 |
| P1.4 | `WacomButtonBase`：统一按钮基类 |
| P1.5 | `WacomUITags`：Layer tag 声明（`UI.Layer.Game` / `UI.Layer.GameMenu` / `UI.Layer.Modal` / `UI.Layer.Overlay`） |
| P1.6 | `WacomProgressBar`：通用 HP/Shield 进度条 |
| P1.7 | `WacomModalDialog`：通用模态确认框 |
| P1.8 | 编译验证 + 在编辑器里建 `WBP_PrimaryGameLayout` 测试 Layer 激活 |

### 3.9 扩展性设计

| 未来需求 | 当前设计如何支持 |
| --- | --- |
| 背包 UI | 新建 `UBackpackPanel : UWacomActivatableWidget`，Push 到 `GameMenu` Layer |
| 商店 UI | 同上 |
| 暂停菜单 | Push 到 `GameMenu` Layer，自动阻断游戏输入 |
| 击倒奖励选择 | Push 到 `Modal` Layer，战斗暂停等待玩家选择 |
| 对话系统 | Push 到 `GameMenu` 或 `Modal` Layer |
| 手柄支持 | CommonUI 自动处理输入设备切换，Widget 用 `UCommonButtonBase` 自带 focus 导航 |
| 多分辨率 | UMG 的 Anchor + DPI Scale 处理；Layout 用 `ScaleBox` 包裹 |
| 动画 | `UWacomActivatableWidget` 的 `PlayTransitionIn/Out` 钩子；子类 override 播 UMG Animation |
| 主题换肤 | P6 的 `UWacomStyleSubsystem` 集中管理颜色/字体，Widget 从中读取 |
| MVVM | 后续在 `UWacomActivatableWidget` 和 Snapshot 之间插一层 ViewModel，Widget 绑定属性 |

## 4. P2：战斗 UI

在 P1 框架上实现战斗界面。P2 是 P1 的第一个消费者。

### 4.1 Widget 层级

```
UBattleHUD : UWacomActivatableWidget (Push 到 Game Layer)
├── UEnemyPanel
│   └── [动态] UEnemyPartWidget × N
├── UPlayerStatusBar
├── UActionBar
├── UHandPanel
│   └── [动态] UCardWidget × N
└── UEventToast
```

### 4.2 交互状态机

```
enum class EBattleUIState : uint8
{
    Idle,           // 等待玩家操作
    TargetSelect,   // 选择目标部位
    Resolving,      // 命令已提交，等待结算完成（预留动画播放期）
    BattleEnd,      // 战斗结束，显示结果
};
```

### 4.3 P2 子步骤

| 步骤 | 内容 |
| --- | --- |
| P2.1 | `UBattleHUD`：状态机 + Refresh 入口 + 命令提交委托 |
| P2.2 | `UPlayerStatusBar`：HP/Shield/WaitValue |
| P2.3 | `UEnemyPanel` + `UEnemyPartWidget`：部位信息 + 点击选目标 |
| P2.4 | `UHandPanel` + `UCardWidget`：手牌展示 + 区域标记 + 点击打牌 |
| P2.5 | `UActionBar`：Wait / EndTurn 按钮 |
| P2.6 | `UEventToast`：事件文字队列 |
| P2.7 | `ABattleTestActor` 接入 HUD |
| P2.8 | 在编辑器里建 Widget Blueprint + 端到端测试 |

### 4.4 P2 验收标准

- 能用鼠标点击手牌 → 选目标 → 打出
- 能点 Wait / End Turn 按钮
- 屏幕上实时显示 HP、先机、意图、手牌区域
- 战斗结束时显示胜利/失败
- 现有自动化测试全绿

## 5. P3：规则补全

| 子项 | 内容 |
| --- | --- |
| P3.1 | 保留关键字：Retain 卡回合结束不弃；双手区保留 |
| P3.2 | 中毒结算：部位行动后扣 Stacks HP |
| P3.3 | ZoneHook 消费：朝光暮蝶左手区/右手区效果 |
| P3.4 | 伙伴被动：拂晓飞蛾 OnCompanionCount、暮蛉 OnTwilightTriggered |
| P3.5 | 补充自动化测试 |

## 6. P4：Enhanced Input

- 建 Input Action / Input Mapping Context
- 战斗快捷键：数字键打牌、W 等待、E 结束回合
- Push/Pop IMC 在战斗开始/结束时切换
- 与 CommonUI 输入路由共存

## 7. P5：UI 动画基础

- `UWacomActivatableWidget` 的 `PlayTransitionIn/Out` 实现
- 卡牌飞入/飞出手牌的基础动画
- 伤害数字弹出
- 事件 Toast 淡入淡出
- 部位被破坏时的视觉反馈

## 8. P6：主题与样式

- `UWacomStyleSubsystem`：集中管理颜色、字体、间距
- `FWacomStyleSet`：一组样式常量
- Widget 从 Subsystem 读取样式，不硬编码
- 后续支持暗色/亮色主题切换（如果需要）

## 9. 验收标准（全部完成时）

- 虫妹最小卡组的所有效果都真正生效
- 一场完整战斗从开始到结束的体验流畅
- UI 框架可以快速支撑新界面（背包、商店、菜单）
- 可以开始做第三阶段（Run 外层 / 背包 / 多敌人 / 美术 / 动画）
