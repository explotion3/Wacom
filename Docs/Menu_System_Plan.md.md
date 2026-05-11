# 菜单与 UI 基底规划

  

本文定义 Wacom 项目的 UI 长期可扩展基底。当前阶段只落骨架和关键 invariant，

为主菜单、暂停菜单、通用对话框、将来的设置 / 结算 / 背包等界面预留接入点。

  

---

  

## 1. 目标与非目标

  

### 目标

  

- 长存的 PrimaryGameLayout：从游戏启动到退出一直在 Viewport，跨关卡不销毁

- 跨关卡的 UI 服务：由 `UGameInstanceSubsystem` 持有，谁都能拿到

- 分层清晰：Game / GameMenu / Modal / Overlay 四层各司其职

- 菜单血统独立：`UWacomMenuWidgetBase` 与战斗血统 `UWacomBattleWidgetBase` 并列

- 启动流：L_MainMenu → 主菜单 → 新游戏 / 继续 / 退出

- 运行时：探索 / 战斗中 ESC 唤出暂停菜单，支持存档 / 回主菜单

- 通用确认对话框：两按钮 + 两委托，覆盖所有"覆盖存档 / 退出游戏"这类分支

  

### 非目标（第一版不做）

  

- 设置面板的实际选项（音量、图形、绑定）只留接入位，不实现

- 键盘可导航焦点的完整方案（先做首项自动聚焦，够用即可）

- UI 动画 / 过场 / 音效

- 本地化 / 多语言

- 云存档、账号系统

  

---

  

## 2. 决策摘要

  

| 项 | 决定 |

|---|---|

| GameInstance 类 | 立 `UWacomGameInstance`，即使现在只是空壳 |

| UI 管理层 | `UWacomGameUIManagerSubsystem`（GameInstance Subsystem）|

| PrimaryLayout 生命周期 | 跨关卡长存，Subsystem 持有 |

| 主菜单场景 | 极简纯色背景（L_MainMenu 新建），没有 3D 装饰 |

| ESC 语义 | 暂停菜单（探索禁用移动 + 显示光标；战斗 HUD 进 Disabled） |

| 新游戏存档策略 | 有档时弹 Modal 确认覆盖，无档直接开新 |

| L_TestBattle 兼容 | 保留 `ABattleTestActor` 独立流程，存档 / 主菜单不接 |

| 异步加载 | 接口用 `TSubclassOf` 先同步，未来替换为 `TSoftClassPtr` 不改调用方 |

  

---

  

## 3. 架构全景

  

```

UWacomGameInstance

    └── UWacomGameUIManagerSubsystem (GameInstance Subsystem)

            └── UWacomPrimaryGameLayout (AddToViewport 一次，跨关卡保留)

                    ├── Game Layer       ← 战斗 HUD / 探索 HUD

                    ├── GameMenu Layer   ← 主菜单 / 暂停菜单 / 设置

                    ├── Modal Layer      ← 确认框

                    └── Overlay Layer    ← Toast / 提示

```

  

关键约束：

  

- **GameMode 不直接 CreateWidget / AddToViewport**。所有 Push 都走 Subsystem。

- **Subsystem 不写业务逻辑**。它只是门面（facade）：Push / Pop / Clear by Layer Tag。

- **Widget 不反向调 Subsystem**。Widget 的 Click 委托冒泡到持有它的 C++（GameMode / Controller / Widget 父）再路由。

  

---

  

## 4. 关卡 / GameMode 对应

  

| 关卡 | 默认 GameMode | 默认 Pawn | 责任 |

|---|---|---|---|

| `L_MainMenu` 新建 | `AWacomMenuGameMode` 新增 | `None` | 主菜单 |

| `L_Exploration` 已有 | `AWacomGameMode` 已有 | `AWacomPlayerCharacter` | 探索 + 战斗 |

| `L_TestBattle` 已有 | 继承父默认 | `DefaultPawn_Simple` | 纯战斗测试（不走存档 / 菜单） |

  

`Config/DefaultEngine.ini`：

- `[/Script/EngineSettings.GameMapsSettings]` `GameDefaultMap=/Game/Wacom/Maps/L_MainMenu.L_MainMenu`

- `EditorStartupMap=/Game/Wacom/Maps/L_MainMenu.L_MainMenu`

  

L_TestBattle 保留为可直接打开的战斗测试场，不走菜单流程。

  

---

  

## 5. Widget 基类层次

  

```

UCommonActivatableWidget (CommonUI 提供)

    └── UWacomActivatableWidget (已有，保持为"Wacom UI 根基类")

            ├── UWacomMenuWidgetBase (新增)

            │       ├── UWacomMainMenuScreen

            │       ├── UWacomPauseMenuScreen

            │       ├── UWacomConfirmDialog

            │       └── UWacomSettingsScreen (占位)

            └── UWacomBattleWidgetBase (已有)

                    ├── UBattleHUD

                    ├── UCardWidget

                    ├── UHandPanel

                    └── ...

```

  

**`UWacomActivatableWidget` 的定位重新说明**：

它是"Wacom UI 根"，只提供跨全项目的通用能力（主题钩子、生命周期日志等），

不混入任何 Session / Snapshot 概念。当前的 Session 相关能力已在 `UWacomBattleWidgetBase`，

不需要额外迁移。

  

**`UWacomMenuWidgetBase` 职责**：

- 菜单基类：`NativeOnActivated` 尝试聚焦第一个可聚焦子控件（默认从 WidgetTree 头顺序查）

- 暴露 `FOnBackRequested` 委托——ESC / Gamepad B 默认路由到这个委托

- 不做 Snapshot 机制

- 默认 `GetDesiredInputConfig` 返回 `{MenuCategoryMode, DoNotLock}`，鼠标可见

  

---

  

## 6. 核心组件

  

### 6.1 `UWacomGameInstance`

  

位置：`WacomApp/Public/Core/WacomGameInstance.h`

  

```

UCLASS()

class WACOMAPP_API UWacomGameInstance : public UGameInstance

{

    GENERATED_BODY()

    // 第一版骨架，无成员

    // 未来放"跨关卡全局状态"的窝：音量、账号、云存档句柄

};

```

  

配置：`Config/DefaultEngine.ini` `[/Script/EngineSettings.GameMapsSettings]`

`GameInstanceClass=/Script/WacomApp.WacomGameInstance`

  

### 6.2 `UWacomGameUIManagerSubsystem`

  

位置：`WacomApp/Public/UI/Foundation/WacomGameUIManagerSubsystem.h`

  

职责：

- 创建 / 持有 `UWacomPrimaryGameLayout`

- 首次进入有 PlayerController 的关卡时 `AddToViewport`

- 对外暴露分层 Push / Pop / Clear 接口

  

接口草案（第一版同步加载）：

  

```

class UWacomGameUIManagerSubsystem : public UGameInstanceSubsystem

{

    UFUNCTION(BlueprintCallable)

    void EnsurePrimaryLayout(APlayerController* PC);

  

    UFUNCTION(BlueprintCallable)

    UCommonActivatableWidget* PushContentToLayer(

        FGameplayTag LayerTag,

        TSubclassOf<UCommonActivatableWidget> WidgetClass);

  

    UFUNCTION(BlueprintCallable)

    void PopContentFromLayer(UCommonActivatableWidget* Widget);

  

    UFUNCTION(BlueprintCallable)

    void ClearLayer(FGameplayTag LayerTag);

  

    UFUNCTION(BlueprintPure)

    UWacomPrimaryGameLayout* GetPrimaryLayout() const;

};

```

  

**生命周期钩子**：

- `Initialize`：什么都不做；PrimaryLayout 需要 PlayerController 才能 CreateWidget

- `Deinitialize`：RemoveFromParent、清空引用

- PlayerController 在关卡里出现后，GameMode / PlayerController 显式调 `EnsurePrimaryLayout`

  

### 6.3 `AWacomMenuGameMode`

  

位置：`WacomApp/Public/GameFramework/WacomMenuGameMode.h`

  

```

AWacomMenuGameMode

    DefaultPawnClass = nullptr（不生成 Pawn）

    PlayerControllerClass = AWacomMenuPlayerController (或复用 AWacomPlayerController)

    BeginPlay:

        EnsurePrimaryLayout(PC)

        Push MainMenuScreen 到 GameMenu 层

        SetInputMode(UIOnly)

        bShowMouseCursor = true

```

  

MenuPlayerController 是否复用：第一版直接用 `AWacomPlayerController`——它已经有 IMC 切换逻辑，

只是在菜单场不 Push 任何 IMC 而已。避免做到一半又要重构。

  

### 6.4 `UWacomMainMenuScreen`

  

- C++ 基类：定义三个按钮的委托接口，委托接到 Widget 的 bp event

- WBP 子类：放三个 UCommonButton，绑 OnClicked 调 C++ 里定义的 Handler

- 按钮：

  - `New Game`：若有存档弹 ConfirmDialog；否则直接 `OpenLevel(L_Exploration)`

  - `Continue`：`UWacomGameUIManagerSubsystem` + `URunSession::HasSaveInSlot(Main)` 都就位时才启用

    - 需要一个特殊入口：因为此时还没进 L_Exploration，`URunSession` 甚至可能还不存在

    - 方案：把"能否继续"的判断下放到 `UWacomGameUIManagerSubsystem::CanContinueRun()`

      或独立 UI 帮助函数，内部直接查 `UGameplayStatics::DoesSaveGameExist(Main)`

  - `Quit Game`：弹 ConfirmDialog（"确定退出？"）→ 确认后 `ConsoleCommand("quit")`

  

### 6.5 `UWacomPauseMenuScreen`

  

触发：`AWacomPlayerController` 的 ESC 键

  

按钮：

- `Resume`：Pop 自身

- `Save`：`RunSession->SaveToSlot(Main)` + Toast / Overlay 简单提示（第一版不做提示，只日志）

- `Settings`：Push SettingsScreen（占位页面）

- `Quit to Main Menu`：弹 ConfirmDialog → 确认后 `OpenLevel(L_MainMenu)`

  

暂停语义：

- 探索：`Pawn->SetExplorationInputEnabled(false)` + 鼠标可见

- 战斗：战斗 UI 进入 Disabled（不消费输入，但继续显示）——第一版简单做：HUD 背后叠一层菜单即可

  

### 6.6 `UWacomConfirmDialog`（Modal 层）

  

一个通用确认框：

  

```

class UWacomConfirmDialog : public UWacomMenuWidgetBase

{

    FText TitleText;

    FText MessageText;

    FText ConfirmButtonText;   // 默认 "Confirm"

    FText CancelButtonText;    // 默认 "Cancel"

  

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConfirmed);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCancelled);

    FOnConfirmed OnConfirmed;

    FOnCancelled OnCancelled;

  

    static UWacomConfirmDialog* Show(

        UObject* WorldContext,

        const FText& Title,

        const FText& Message,

        TFunction<void()> OnConfirm,

        TFunction<void()> OnCancel = nullptr);

};

```

  

`Show` 是静态工厂：

- 从 Subsystem 拿 PrimaryLayout

- Push 到 Modal 层

- 绑委托

- 返回实例给调用方（一般不用）

  

### 6.7 `UWacomSettingsScreen`（占位）

  

第一版只是一个空的 Activatable Widget，有个 Back 按钮。

等策划真给设置清单再加具体项。

  

---

  

## 7. 输入协调

  

### 7.1 IMC 层次

  

```

IMC_Exploration        ← Pawn possession 时 Push（已有）

IMC_Battle             ← EnterBattle 时 Push（已有）

IMC_Menu（新增？）     ← 暂不加，见下方

```

  

第一版不加 `IMC_Menu`：CommonUI 的 `FUIInputConfig` + `GetDesiredInputConfig` 已经足够。

菜单 Widget 在 `Activated` 状态下的 UIInputConfig 设为 `UIOnly`，Push 时自动抑制 Game 输入。

  

### 7.2 ESC 键的路由

  

多个按键源头的 ESC 都应指向"返回 / 暂停"：

- Menu Widget 激活时：ESC → Pop 当前 Widget（`OnBackRequested`）

- Exploration / Battle 中：ESC → Push PauseMenu

  

技术上：

- Menu Widget 里：在 `NativeOnActivated` 里 `RegisterUIActionBinding(Back)`，handler 调 `DeactivateWidget`

- 游戏内 ESC：在 `AWacomPlayerController::SetupInputComponent` 里绑 `IA_OpenMenu`（新增），handler 请求 Subsystem Push PauseMenu

  - 若当前 GameMenu 层已有内容（比如已打开暂停菜单），忽略或交给 Widget 自己处理

  

### 7.3 焦点

  

第一版实现：`UWacomMenuWidgetBase::NativeOnActivated` 遍历 WidgetTree 找第一个 Focusable 子控件 `SetFocus()`。

够键盘玩家用到下一次迭代。完整方案等加了手柄再说。

  

### 7.4 鼠标光标

  

- `L_MainMenu`：`bShowMouseCursor = true`，InputMode = UIOnly

- `L_Exploration` 探索中：`bShowMouseCursor = false`，InputMode = GameOnly

- `L_Exploration` 暂停中：`bShowMouseCursor = true`，InputMode = GameAndUI

  

由 GameMode / PauseMenu 的 Activated / Deactivated 钩子来切。

  

---

  

## 8. 存档 / 读档交互

  

新游戏流程：

```

Click "New Game"

    ├── 无存档：OpenLevel(L_Exploration)

    └── 有存档：ConfirmDialog::Show(

                "开始新游戏将覆盖现有存档，确定？",

                OnConfirm = { 删 Main + Auto; OpenLevel(L_Exploration) },

                OnCancel  = {}

            )

```

  

继续游戏流程：

```

Click "Continue"（仅当 HasSaveInSlot(Main) 启用）

    OpenLevel(L_Exploration)

    → AWacomGameMode::BeginPlay → 一帧后 BootstrapRunFromSave → LoadFromSlot(Main)

```

  

暂停时存档：

```

PauseMenu Click "Save"

    RunSession->SaveToSlot(Main)

    （第一版不做成功提示，只靠日志；未来可以 Push 一个 Overlay Toast）

```

  

退出主菜单：

```

PauseMenu Click "Quit to Main Menu"

    ConfirmDialog::Show(

        "未保存的进度将丢失。返回主菜单？",

        OnConfirm = { OpenLevel(L_MainMenu) },

        OnCancel  = {}

    )

```

  

OpenLevel 切到 L_MainMenu 时：

- World 会销毁，`L_Exploration` 的 GameMode / RunSession 都销毁

- `UWacomGameUIManagerSubsystem`（GameInstance 级）保持

- PrimaryLayout 保持（但所有 Activatable Widget 会被清）——`ClearLayer(All)` 由 MenuGameMode::BeginPlay 负责

  

---

  

## 9. 切片计划

  

### M1：Subsystem + PrimaryLayout 搬家（约 1 天）

  

**目标**：PrimaryLayout 生命周期脱离 GameMode，挪到 GameInstance Subsystem。

  

- 新增 `UWacomGameInstance`（空壳）

- 新增 `UWacomGameUIManagerSubsystem`

- `AWacomGameMode::EnterBattle` / `ExitBattle` 改为走 Subsystem 的 Push / Pop

- `Config/DefaultEngine.ini` 加 `GameInstanceClass`

- BattleTestActor 不改（独立流程）

  

**验收**：

- L_Exploration 的战斗流完全不变

- 自动化测试全绿

- 日志能看出 PrimaryLayout 是 Subsystem 创建 / 持有

  

### M2：MenuWidgetBase + MainMenu + L_MainMenu（约 0.5–1 天）

  

**目标**：启动游戏先进主菜单。

  

- 新增 `UWacomMenuWidgetBase`

- 新增 `UWacomMainMenuScreen`（C++ 基类 + 默认布局；未来蓝图覆盖样式）

- 新增 `AWacomMenuGameMode`

- 手动在编辑器里新建 `L_MainMenu` 关卡，WorldSettings 配 `AWacomMenuGameMode`

- 修改 `Config/DefaultEngine.ini`：`GameDefaultMap` 指向 `L_MainMenu`

- MainMenu 的"Continue"按钮根据 `HasSaveInSlot(Main)` 灰显

- "Quit Game" 直接 `ConsoleCommand("quit")`（确认框 M3 再接）

  

**验收**：

- 启动 PIE 进入 L_MainMenu 黑底 + 三按钮

- 点 New Game → OpenLevel(L_Exploration)，原探索 / 战斗流程依然跑通

- 点 Continue（需先有存档）→ OpenLevel(L_Exploration)，存档正常加载

  

### M3：暂停菜单 + 通用确认对话框（约 0.5–1 天）

  

**目标**：L_Exploration 里 ESC 可暂停，确认对话框覆盖覆盖存档 / 退出场景。

  

- 新增 `UWacomConfirmDialog`

- 新增 `UWacomPauseMenuScreen`

- `AWacomPlayerController` 加 `IA_OpenMenu`（ESC）+ 回调 Push PauseMenu

- PauseMenu 四按钮接好：Resume / Save / Settings / Quit to Main Menu

- MainMenu 的 Quit Game 改用 ConfirmDialog

- MainMenu 的 New Game 若有存档，先弹 ConfirmDialog

- PauseMenu Quit to Main Menu 先弹 ConfirmDialog

- 新增 `IA_OpenMenu` 到 commandlet，生成 IMC_Battle / IMC_Exploration 各加一条

  

**验收**：

- 在 L_Exploration 按 ESC 出暂停菜单

- 点 Save → 存档日志，返回游戏继续

- Quit to Main Menu 先弹确认，确认后切到 L_MainMenu

- 再次 New Game 弹"将覆盖现有存档"确认

- Quit Game 弹"确定退出"确认

  

### M4：焦点 / 输入抑制收尾（约 0.5 天）

  

**目标**：键盘可导航的最小可用版本。

  

- `UWacomMenuWidgetBase::NativeOnActivated` 聚焦第一个 Focusable 子控件

- 确保 Menu 激活时 Game 输入（1-7、WASD 等）被抑制

- Modal 激活时下层菜单输入被抑制

- 跑一次手动验证清单（见 §10）

  

**验收**：打开任何菜单都有明显的焦点指示、可用键盘确认按钮、Modal 在菜单之上。

  

---

  

## 10. 手动测试清单（四切片完成后走一遍）

  

- [ ] 首次启动 PIE：进入 L_MainMenu，光标可见，第一个按钮有焦点

- [ ] Continue 按钮：首次启动无存档时灰显；打过一仗后重启，Continue 变高亮

- [ ] New Game（无档）：直接进 L_Exploration，PlayerStart 位置出现

- [ ] New Game（有档）：弹确认，取消=留在菜单；确认=清档进 L_Exploration，起点在 PlayerStart

- [ ] Quit Game：弹确认，取消=留在菜单；确认=关闭 PIE

- [ ] L_Exploration 按 ESC：弹暂停菜单，Pawn 停住，鼠标可见

- [ ] Pause 点 Resume：菜单关闭，输入恢复

- [ ] Pause 点 Save：日志显示存档成功，继续游戏

- [ ] Pause 点 Quit to Main Menu：弹确认，确认=切到 L_MainMenu，Continue 可用（刚才 Save 过）

- [ ] 战斗中按 ESC：战斗 HUD 变灰，暂停菜单出现（回合制不会推进）

- [ ] 战斗中 Quit to Main Menu：按存档规则丢弃战斗进度，回主菜单 Continue 恢复到战斗前状态

- [ ] 切回 L_MainMenu：PrimaryLayout 还在（通过日志验证），GameMenu 层只剩 MainMenuScreen

  

---

  

## 11. 对现有代码的影响

  

| 位置 | 改动 | 切片 |

|---|---|---|

| `WacomApp/Public/Core/WacomGameInstance.h` + `.cpp` | 新增空壳 | M1 |

| `WacomApp/Public/UI/Foundation/WacomGameUIManagerSubsystem.h` + `.cpp` | 新增 | M1 |

| `WacomApp/Private/GameFramework/WacomGameMode.cpp` | EnterBattle/ExitBattle 改走 Subsystem，不再自己 Create PrimaryLayout | M1 |

| `Config/DefaultEngine.ini` | GameInstanceClass + GameDefaultMap | M1 → M2 |

| `WacomApp/Public/UI/Foundation/WacomMenuWidgetBase.h` + `.cpp` | 新增菜单基类 | M2 |

| `WacomApp/Public/UI/Menus/WacomMainMenuScreen.h` + `.cpp` | 新增主菜单屏幕 | M2 |

| `WacomApp/Public/GameFramework/WacomMenuGameMode.h` + `.cpp` | 新增菜单 GameMode | M2 |

| `Content/Wacom/Maps/L_MainMenu.umap` | 新建（手动在编辑器做） | M2 |

| `WacomApp/Public/UI/Menus/WacomPauseMenuScreen.h` + `.cpp` | 新增暂停屏 | M3 |

| `WacomApp/Public/UI/Menus/WacomConfirmDialog.h` + `.cpp` | 新增确认框 | M3 |

| `WacomApp/Public/UI/Menus/WacomSettingsScreen.h` + `.cpp` | 新增（占位） | M3 |

| `WacomApp/Public/GameFramework/WacomPlayerController.*` | 加 IA_OpenMenu + Menu 打开 | M3 |

| `WacomEditor/Private/Commandlets/WacomCreateInputAssetsCommandlet.cpp` | 加 IA_OpenMenu + IMC 映射 ESC | M3 |

  

---

  

## 12. 可能踩的坑

  

- **Subsystem 在 PIE 结束时 Deinitialize 的顺序**：PrimaryLayout 可能在 World 已销毁后才被 RemoveFromParent，

  要做空指针保护；用弱引用持有 PC 也稳一些

- **OpenLevel 后 PrimaryLayout 的 Widget Tree 状态**：Activatable Widget 可能因为 World 切换而 OnDeactivated。

  切 Level 前先 `ClearLayer(All)`，切完后由 GameMode::BeginPlay 重新 Push 新 Level 的 UI

- **`OpenLevel` 是异步**：Click 回调触发 OpenLevel 之后不能立刻销毁 Widget；让 Widget 上的按钮只调 `OpenLevel`，

  清理自然跟关卡切换一起完成

- **MainMenu 的 "Continue" 判断时机**：按钮激活的 tick 判断 HasSaveInSlot(Main) 就好，不需要缓存

- **光标和锁窗**：进主菜单时如果上一场是探索锁窗状态，要显式切 UIOnly + ShowCursor；MenuGameMode BeginPlay 统一处理

- **CommonUI 的 UIInputConfig 冲突**：多个 Activatable 叠着时，只有最顶层的 UIInputConfig 生效。战斗 HUD + 暂停菜单

  叠加时，暂停菜单的 UIInputConfig 会覆盖战斗 HUD 的——天然满足"暂停抑制战斗输入"

- **ConfirmDialog 连续弹出**：新游戏覆盖 + 主菜单退出两条链如果逻辑错了会出现两层 Modal；保证每个入口 Click 前

  检查 Modal 层是否已有内容，有就忽略本次 Click

  

---

  

## 13. 和项目约定的关系

  

- UI 基底必须走 `CommonUI`（项目约定）：本方案完全在 CommonUI 框架内

- Widget 类必须是 UCLASS（项目约定）：本方案所有 Widget 都是

- 避免业务代码拼 Tag（项目约定）：已有的 `WacomUITags` 下加新 Tag 用 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`

- 随机性走 `State.Rng`（项目约定）：UI 不涉及

- 重要里程碑写 `Dev_Log.md`（项目约定）：M1–M4 全部完成后一次追加

  

---

  

## 14. 后续扩展位

  

几个第一版留的接入点：

  

- **设置面板**：`UWacomSettingsScreen` 占位已有，策划来了把具体选项加进去即可

- **Overlay Toast**：`UWacomGameUIManagerSubsystem` 可加 `ShowToast(FText)` 方法，内部 Push 到 Overlay 层

- **结算屏**：战斗后可以 Push 到 GameMenu 层，用同一套 `UWacomMenuWidgetBase` 血统

- **UI 动画**：现在菜单基类只有 Activated / Deactivated 钩子；未来加 `PlayIntroAnimation` / `PlayOutroAnimation`

  抽象方法，子 WBP override 即可

- **主题换皮**：所有颜色 / 字体 / 按钮样式用 Common UI Style Asset 指定，C++ 不直接写颜色值