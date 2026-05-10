# Dev Log

本文记录第一阶段开发中的切片里程碑、实现中遇到的非平凡约束、以及文档修订点。
不写每日流水。每个新切片完成后在末尾追加。

---

## 项目快照（第一阶段结束）

### 模块

| 模块 | Type | LoadingPhase | 职责 |
| --- | --- | --- | --- |
| `WacomCore` | Runtime | PreDefault | ID / 枚举 / GameplayTags 声明 |
| `WacomData` | Runtime | PreDefault | 卡 / 敌人 / 意图 / 角色 DataAsset |
| `WacomBattle` | Runtime | Default | 战斗内核 |
| `WacomRun` | Runtime | Default | 战斗外状态（第一阶段为空） |
| `WacomApp` | Runtime | Default | 测试 Actor、UI、输入（PRIMARY_GAME_MODULE） |
| `WacomEditor` | Editor | Default | 内容 commandlet、校验 |
| `WacomTests` | DeveloperTool | Default | 自动化测试 |

依赖单向：`Core <- Data <- Battle <- Run <- App`。
`WacomEditor` / `WacomTests` 向运行时模块单向依赖，互不影响。

### 数据内容（Commandlet 生成）

12 个 `.uasset` 全部在 `Content/Wacom/`：

- `Cards/BugGirl/DA_Card_*.uasset` × 7（左手、右手、朝光暮蝶、拂晓飞蛾、赤腹工蚁、烁光蝶、暮蛉）
- `Characters/DA_Character_BugGirl.uasset`
- `Enemies/Snake/DA_Enemy_Snake.uasset` + 3 部位 DataAsset

数据真相见 `Data_Schema_Draft.md §3 / §8`。

### 测试

13 条自动化测试覆盖 `Architecture.md §12`，路径前缀 `Wacom.Battle.*`。

---

## 第一阶段切片里程碑

| 切片 | 内容 | 关键产出 |
| --- | --- | --- |
| S1 | 基础类型 + GameplayTags + BattleState/Command/Snapshot/Event | `WacomCore/Public/Types`、`WacomCore/Public/Tags`、`WacomBattle/Public/{Session,Commands,Snapshots,Events,Runtime}`、`WacomBattle/Private/Core/BattleState.h` |
| S2 | `UBattleSession` + `FBattleResolver` 骨架 | Session 对外 4 个 API：Initialize / SubmitCommand / BuildSnapshot / ConsumeEvents |
| S3 | 起始阶段：抽 5 + 等待值重置 | `DeckService`、`BattleTurnFlow::BeginPlayerTurn` |
| S4 | 手牌区域规则 | `HandZoneService`：生成手牌队列、区域判定、上限处理、三种腾挪 API |
| S5 | 三个命令 Resolver + 战斗结束判定 | `BattleRules`、`PlayCard/Wait/EndTurn` Resolver |
| S6 | 敌方部位行动子流程 + 效果 Executor | `EnemyPartActionResolver`、`EffectContext`、`EffectExecutor` |
| S7 | 卡牌效果接入 + 腾挪 + AfterPlayed 被动 | 打牌流程接通全部 `FCardEffect`；`HandZoneService` 扩展三个腾挪 API |
| S8 | 先机命中 / 抵抗 / 完美释放 | `PlayCardResolver` 按 `Battle_Rules §8` 十步完整重构 |
| S9 | DataAsset 全部通过 Commandlet 生成 | `WacomRegenerateContentCommandlet`、`ContentBuilders/*` |
| S10 | PIE 测试入口 | `ABattleTestActor`：键盘 1-5 / W / E / R / P 交互，屏幕打印 Snapshot |
| S11 | 自动化测试 | 11 个 Spec 覆盖 `Architecture.md §12` 13 条规则 |

---

## 实现中遇到的非平凡约束

### TUniquePtr 在 UObject 头里暴露时的 UHT 限制

`UBattleSession::h` 最初用 `TUniquePtr<FBattleState>`，但 UHT 为 UObject 生成的 `.gen.cpp` 会隐式实例化成员析构函数，要求 `FBattleState` 完整可见。前向声明 + `~UBattleSession()` 非内联仍不够。

**解决**：改成裸 `FBattleState*` + 手动 `new/delete` 的 pImpl。只影响 `BattleSession.cpp` 一处构造/析构写法。

### WacomEditor 的 LoadingPhase

`PostEngineInit` 会导致 `-run=XXX` commandlet 查找时模块未加载，`Error: looked like a commandlet, but we could not find the class`。

**解决**：改为 `Default`。`Wacom.uproject` 和 `project-conventions.md` 同步更新。

### `FBattleState` 非反射时的 GC 引用

`FBattleState` 是非反射 struct，里面的 `TObjectPtr<...Definition>` 不会被 GC 追踪。

**解决**：`UBattleSession` 暴露一个 `UPROPERTY() TArray<TObjectPtr<const UObject>> ReferencedAssets`，`Initialize` 时把 Character / Enemy / 所有 CardDef / 所有 PartDef 镜像进去。Session 活着资产就不会被 GC 回收。

### TestFixture 里的 UObject 生命周期

自动化测试在 transient package 里建 `UCardDefinition` 等对象，GC 会在下次 tick 清掉没引用的。

**解决**：`FWacomBattleFixture` 用 `TStrongObjectPtr<UObject>` 数组锚点，所有 transient 对象都挂进去，fixture 析构时自动释放。

### Automation test 里访问 `TObjectPtr<UCardDefinition>::CardId`

spec cpp 只 include fixture header 时看到的是前向声明，`->CardId` 编译不过。

**解决**：每个 spec 的 include 列表里补 `Cards/CardDefinition.h`。

---

## 文档修订点

### `Battle_Rules.md §10` 晕厥层数

第一阶段实现里把 `Status.Stunned` / `Status.Freeze` 按"每次行动消耗 1 层"处理。原文档没规定，S6 完成时补了一小节：

> 晕厥以层数模型记录。每次该部位行动（无论执行意图还是因晕厥跳过意图）都会消耗 1 层晕厥；层数归零时晕厥状态移除。冻结状态第一阶段共享同一"跳过意图 + 消耗 1 层"分支。

抵抗判定施加 `Status.Stunned` 时也按 1 层施加，与该规则一致。

### `Architecture.md` §1 / §3 / §5 / §14

原文规划"第一阶段不拆 UE 模块，目录模拟边界"。实际实现时改为 **Day 1 就拆 7 模块**（理由：编译器级别反循环依赖在空项目期成本最低）。文档已同步更新，标注依赖由 `Build.cs` 硬约束。

### `project-conventions.md`

Steering 同步更新：`WacomEditor` 的 LoadingPhase 从 `PostEngineInit` 改为 `Default`。

---

## 操作指令

### 编译（Editor 开发配置）

```
"e:\UE_5.7\Engine\Build\BatchFiles\Build.bat" WacomEditor Win64 Development ^
  -Project="d:\UE_Project\5.7\Wacom\Wacom.uproject" -WaitMutex -FromMsBuild
```

成功标记：`Result: Succeeded`。

### 重建 DataAsset

```
"e:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" ^
  "d:\UE_Project\5.7\Wacom\Wacom.uproject" ^
  -run=WacomRegenerateContent -NoSplash -Unattended
```

Commandlet 幂等，每次 Create-or-Replace。

### 跑自动化测试

```
"e:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" ^
  "d:\UE_Project\5.7\Wacom\Wacom.uproject" ^
  -ExecCmds="Automation RunTests Wacom.Battle; Quit" ^
  -Unattended -NoPause -NoSplash -NullRHI
```

结果在 `Saved/Logs/Wacom.log` 里搜 `Test Completed`。

### PIE 测试关卡配置（一次性手工）

1. 编辑器里新建 `Content/Wacom/Maps/TestBattle/L_TestBattle.umap`
2. 场景里放 `ABattleTestActor`
3. Details 面板：
   - `Character` = `DA_Character_BugGirl`
   - `Enemy` = `DA_Enemy_Snake`
   - `RandomSeed` = 0（随机）或任意整数（复现）
   - `bAutoStart` = true
4. 保存关卡
5. `Project Settings → Maps & Modes → Editor Startup Map / Game Default Map` 设为 `L_TestBattle`
6. PIE 里按键：1-5 打手牌 / W 等待 / E 结束回合 / R 重启 / P 打印 Snapshot

---

## 未决项（等后续切片或文档确认）

沿用 `Data_Schema_Draft §13`，不在此重复。除此之外：

- 朝光暮蝶左手区的"完美释放不推进先机"尚未实现（ZoneHook 消费是 S8+ 的工作，当前只建了 hook 结构）。
- 拂晓飞蛾 `OnCompanionCount` 被动未实现。
- 暮蛉 `OnTwilightTriggered` 被动未实现。
- 中毒 / 减速 / 暮气只记录层数不真正生效。
- 保留关键字只对锚点生效，普通卡的保留效果未消费。
- MSVC 工具链还在 14.38.33145，UE 5.7 警告 "not preferred"。建议 VS Installer 升到 14.44.35207+。

---

## 第二阶段方向（超出 Architecture.md §11）

按优先级粗排：

1. **状态完整结算**：中毒每次行动扣血；减速/暮气影响先机或 Cost
2. **ZoneHook 消费**：打通朝光暮蝶左手/右手区特效
3. **完整 AfterPlayed / OnCompanionCount 被动**：拂晓飞蛾回手、暮蛉联动
4. **保留关键字的回合结束过滤**：普通卡持有 `Retain` 时不进弃牌
5. **UMG + CommonUI 真正的战斗 UI**：替换 `AddOnScreenDebugMessage`
6. **Run 外层结构**：背包、关卡路径、掉落
7. **Enhanced Input 迁移**：S10 的按键绑定切到 IA + IMC

---

## 第二阶段里程碑

### P1：UI 基础设施（CommonUI 框架 + 基类体系 + 通用组件）

| 切片 | 内容 | 关键产出 |
| --- | --- | --- |
| P1.1 | Build.cs 依赖 | `WacomApp.Build.cs` 加 `CommonUI` / `CommonInput` |
| P1.2 | PrimaryGameLayout + Layer Tag | `WacomPrimaryGameLayout`（4 层 Layer Stack，Game / GameMenu / Modal / Overlay） + `WacomUITags` |
| P1.3 | 基类体系 | `UWacomActivatableWidget`（领域无关，动画钩子）+ `UWacomBattleWidgetBase`（战斗专用，Snapshot 刷新 + Session 访问，子 Widget 递归下发） |
| P1.4 | 按钮基类 | `UWacomButtonBase` 继承 `UCommonButtonBase` |
| P1.5 | Layer Tag | 并入 P1.2 |
| P1.6 | 通用进度条 | `UWacomProgressBar` |
| P1.7 | 通用模态对话框 | `UWacomModalDialog` + `FWacomDialogButton` + `FWacomModalDialogClosedDynamic` |
| P1.8 | Actor 接入 Layout + 占位 HUD | `ABattleTestActor` BeginPlay 创建 `UWacomPrimaryGameLayout` 并 AddToViewport；`UDebugBattleHUD` 作为 P1 占位 HUD（多行 Snapshot 文本） |

### P2：战斗 UI（在 P1 框架上实现战斗界面）

| 切片 | 内容 | 关键产出 |
| --- | --- | --- |
| P2.1 | `UBattleHUD` 骨架 | 状态机 `EBattleUIState`（Idle / TargetSelect / Resolving / BattleEnd）+ 六个对外交互 API（OnCardClickedByUser / OnEnemyPartClickedByUser / OnWaitRequested / OnEndTurnRequested / CancelTargetSelect / IsInTargetSelect） |
| P2.2 | `UPlayerStatusBar` | HP + Shield + SAN（占位）。第一次打通 HUD → 子 Widget → Snapshot 的完整刷新链 |
| P2.3 | `UCardWidget` + `UHandPanel` | 单卡 Widget + 5 段 Slot 手牌容器（左手区 / 左手锚点 / 双手区 / 右手锚点 / 右手区），Zone Slot 用 ScrollBox 横向滚动 |
| P2.4 | `UEnemyPartWidget` + `UEnemyInfoBar` | 单部位 Widget（HP 条 / Initiative / Intent / Shield / Status）+ 敌人信息条。TargetSelect 状态下部位可点击选目标 |
| P2.5 | `UActionPanel` | Wait / EndTurn 按钮 + 当前等待值显示。UIState != Idle 时自动禁用 |
| P2.6 | `UEquipmentBar` + `UPileCountView` | 装备条占位 + 通用"标签+计数"组件（Draw/Discard/Exhaust 复用） |
| P2.7 | `UEventToast` | 事件淡入淡出 Toast（3 秒生命期，最后 0.8 秒淡出，最多 5 条） |
| P2.8 | Actor 路由 | `ABattleTestActor` 的键盘 1-5/W/E 路由到 HUD 对应 API，事件链路统一 |
| P2.9 | 端到端验证 | 13 条自动化测试全绿，PIE 完整战斗流程可走通 |

## 第二阶段实现中遇到的非平凡约束

### CommonUI 在 UE 5.7 没有 `CommonGame` 模块

Lyra 里的 `UGameUIManagerSubsystem` / `UCommonGameUIPolicy` / `UPrimaryGameLayout` 基类来自 `CommonGame`，但 UE 5.7 原版只有 `CommonUI` / `CommonInput` 两个模块。

**解决**：自己实现等价的 Layer 管理。`UWacomPrimaryGameLayout` 继承 `UCommonUserWidget`，内部持有 4 个 `UCommonActivatableWidgetStack`，用 Layer Tag 查找。由 `ABattleTestActor` 在 BeginPlay 时手动 `CreateWidget + AddToViewport`，不走 Subsystem 注册。

### `TMap<FGameplayTag, ...>` 在 UE 5.7 C++20 模式下冲突

`TMap` key 为 `FGameplayTag` 时，编译器在 `TTuple` 实例化中和 `TWeakFieldPtr::operator==` 冲突。

**解决**：`UWacomPrimaryGameLayout` 改用 `TArray<FLayerEntry>` 线性查找（4 个 Layer 不需要 Map）。同时注意 `FLayerEntry` 持有 `FGameplayTag` 字段需要完整 include `GameplayTagContainer.h`（前向声明不够）。

### `FNativeGameplayTag` 不等于 `FGameplayTag`

`UE_DECLARE_GAMEPLAY_TAG_EXTERN` 声明的是 `FNativeGameplayTag`。赋给 `FGameplayTag` 字段需要调 `.GetTag()`。

### `LoadingPhase = PostEngineInit` 阻止 Commandlet 查找

原 `WacomEditor.uproject` 配置为 `PostEngineInit`，但 `-run=XXX` 的 commandlet 类查找发生在模块加载期间。PostEngineInit 太晚。

**解决**：改为 `Default`。

### 基类字段初始化时序

`UUserWidget` 的生命周期顺序是 `NativeOnInitialized` → `RebuildWidget` → `NativeConstruct`。`BindWidget` 指针**只有在 `RebuildWidget` 之后**才非 null，所以：

- **Button `OnClicked` 绑定**必须在 `NativeConstruct` 做，不能在 `NativeOnInitialized`（第一次被我踩到时按钮全部无反应但 hover 仍正常）
- **`ChildBattleWidgets.Add`** 登记也必须在 `NativeConstruct`
- `NativeConstruct` 里调 `Session->BuildSnapshot()` 兜底一次，防止 Session 在 `RebuildWidget` 之前被 SetSession。

### CommonUI 需要 `GetDesiredInputConfig` 才放行鼠标

`UCommonActivatableWidget::GetDesiredInputConfig` 默认返回空，此时 UI 不接管鼠标，游戏默认 `GameOnly` 模式下鼠标被捕获给 Viewport，**点击事件不到 Widget**（虽然 hover/press 视觉反馈正常，因为那是 Button 的 Style 切换不依赖事件）。

**解决**：HUD 返回 `FUIInputConfig(ECommonInputMode::All, EMouseCaptureMode::NoCapture)` —— UI 和 Game 输入都传递，键盘快捷键仍可用。

### `UButton` / `UCommonButtonBase` 只允许一个直接子 Widget

Designer 里把 TextBlock 直接作为 Button 子时只能放一个。要多控件必须先放 Overlay/VerticalBox 再嵌套。

**解决**：`UCardWidget` / `UEnemyPartWidget` 的 C++ 硬编码布局用 `Overlay` 分层：背景 Border + 内容 VerticalBox + 透明按钮覆盖。

### UMG 的 `Slot` 成员名冲突

`UWidget` 自己有个 `TObjectPtr<UPanelSlot> Slot` 成员。局部变量命名为 `Slot` 会触发 `C4458` 警告。

**解决**：改名 `TargetSlot`。

### `TObjectPtr<T>&` 不等价于 `T*&`

作为函数参数传出引用时，`UButton* MakeBtn(..., UTextBlock*& OutLabel)` 不能接受 `TObjectPtr<UTextBlock>` 实参。

**解决**：签名改为 `TObjectPtr<UTextBlock>& OutLabel` 或在函数内用局部 `UTextBlock*` 再赋值。

### `FUIInputConfig::bHideCursorDuringViewportCapture` 是 protected

不能直接从外部设置。第一阶段忽略这个字段，用默认值。

## 第二阶段文档修订

- `Phase2_Temporary_Decisions.md` 新增 UI 相关临时决定（全量刷新、无 ViewModel、手牌线性布局、目标选择用点击、EventToast 仅文字、WBP 纯色块）
- `Phase2_Temporary_Decisions.md` 标记已正式化：中毒触发时机、中毒穿透护盾
- `Battle_Rules.md §15` 新增状态结算规则章节（中毒/减速/暮气/冻结/连击/晕厥）
- `Data_Schema_Draft.md §10` 中毒行为描述同步更新

## 操作指令（第二阶段新增）

PIE 测试：打开 `L_TestBattle` → 检查 `BattleTestActor` 的字段：
- `Character = DA_Character_BugGirl`
- `Enemy = DA_Enemy_Snake`
- `PrimaryLayoutClass = WBP_PrimaryGameLayout`
- `BattleHUDClass = BattleHUD`（C++ 类，直接用）或 `WBP_DebugBattleHUD`（纯文字调试用）
- `bAutoStart = true`

按 Play，默认 C++ 硬编码布局的战斗 UI 就会出现。

## 当前阶段状态

✅ 第一阶段 S1-S11 战斗内核 + 数据 + 自动化测试 + PIE 测试入口
✅ 第二阶段 P1 UI 基础设施 + P2 战斗 UI
⏳ 第二阶段 P3：规则补全（保留 / 中毒 / ZoneHook / 伙伴被动）
⏳ 第二阶段 P4：Enhanced Input 迁移
⏳ 第二阶段 P5：UI 动画基础
⏳ 第二阶段 P6：主题与样式
