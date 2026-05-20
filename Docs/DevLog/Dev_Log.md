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

### P3：规则补全

| 切片 | 内容 | 关键产出 |
| --- | --- | --- |
| P3.1 | 中毒结算（Battle_Rules §15） | `WacomBattle/Private/Status/PoisonResolver.h/.cpp`；`FBattleState` 加 `PlayerStatuses` + `PlayerStatusStacks`；`FPlayerSnapshot` 镜像；`EffectExecutor` 玩家中毒分支写入 State；`PlayCardResolver` 在 AfterPlayed 之后调用 + `EnemyPartActionResolver::ActOnce` 末尾调用；`PoisonSpec.cpp` 4 条测试（TickOnCardPlay / TickOnEnemyAct / PenetratesShield / StacksUnchanged）|
| P3.2 | 保留关键字 + 双手区保留（Hand_Zone_Rules §7 + Battle_Rules §12） | `HandZoneService` 加 `ShouldRetainCardAtTurnEnd` / `DiscardNonRetainedNormalCardsAtTurnEnd`；`EndTurnResolver` 在敌方行动前调用弃牌；保留判定覆盖"锚点 / Retain 关键字（Def + Temp）/ 左右手都在时的双手区普通卡"；`RetainSpec.cpp` 4 条测试（NormalCardRetainKeeps / NormalCardNoRetainDiscards / BothZoneKeepsWhenAnchorsPresent / BothZoneDiscardsWhenAnchorMissing） |
| P3.3 | ZoneHook 消费（朝光暮蝶左/右手区） | 新增 3 个 tag：`Effect.Card.AddCost` / `Effect.Card.ReduceCost` / `Target.LastShuffledCard`；`FEffectContext::LastShuffledCardId`；`FEffectExecutor::Execute` 签名改为 `FEffectContext&`（Shuffle 写回被移动卡 ID）；`EffectExecutor` 新增 AddCost / ReduceCost 分支；`PlayCardResolver` 加 `RunOnPlayZoneHooks` + `ShouldSkipInitiativePushByZoneHook` 两个 helper，`ExecuteCardEffectOnce` 额外透传 `InOutLastShuffledCardId`；`FillTargetFromCardEffect` 映射 `Target.Self` → 本卡（Card.AddCost/ReduceCost 语义），映射 `Target.LastShuffledCard`；`BugGirlBuilder` 朝光暮蝶右手区 OnPlay ExtraEffects 配 `Shuffle.Random + ReduceCost(LastShuffled) + AddCost(Self)`；`ZoneHookSpec.cpp` 5 条测试（LeftHitSkipsInitiativePush / RightPlayTransfersCost / RightPlayCostAccumulates / Effect.AddCostWorksOnSelf / Effect.ReduceCostClampsAtZero） |
| P3.4 | OnCompanionCount 被动（拂晓飞蛾） | `FBattleState::CompanionPlayedCount`；`FBattleSnapshot::CompanionPlayedCount` + `BattleSnapshotBuilder` 镜像；`PlayCardResolver` 在卡牌去向之后累加 Companion 计数 + AfterPlayed 之后调 `RunOnCompanionCountPassives`；`MoveCardToHandFromAnywhere` helper（从 Draw/Discard/Exhaust/Limbo 随机插入 Hand，并立即执行普通卡上限）；触发后计数清零；`CompanionCountSpec.cpp` 2 条测试（CompanionCountTriggersReturn / CompanionCountResetsAfterTrigger） |
| P3.5 | OnTwilightTriggered 被动占位（暮蛉） | 新增 `EBattleEventType::PassiveTriggered`；`EffectExecutor` 的 Twilight 分支施加成功后遍历 AllCards 对拥有 `OnTwilightTriggered` 被动的卡发 `PassiveTriggered` 事件（`Tag = Passive.Trigger.OnTwilightTriggered`）；不真正改中毒层数（`EffectMagnitudeModifiers` 未引入）；`BattleHUD` / `BattleTestActor` 的 EventTypeToString 补 `PassiveTriggered` 分支；无新增测试（占位，规则未决） |

#### P3.1 实现约束

- **Unity Build 命名空间冲突**：`BattleHUD.cpp` 和 `BattleTestActor.cpp` 都在匿名 namespace 定义 `EventTypeToString`，在 Unity Build 合并 TU 时冲突。把 HUD 那份改名 `HUDEventTypeToString`。
- **中毒穿透护盾**不走 `EffectExecutor::ApplyDamage`（那份会先扣 Shield），`PoisonResolver` 直接扣 `CurrentHp`。发 `DamageDealt` 事件时 `Tag = Status.Poison` 标明来源。
- **调用点选位**：
  - 玩家打牌 → 放在 AfterPlayed 被动之后、`ResolveInitiativeZeroActions` 之前。这样中毒破坏部位能触发 `EnemyPartHpEmptied`，随后行动子流程只考虑存活部位。
  - 敌方部位行动 → 放在 `AdvanceToNextIntent` 之后。此时该部位可能本身就是中毒载体被结算致死；`bDestroyed` 在中毒后置位，`AdvanceToNextIntent` 内部对 bDestroyed 已有 no-op，顺序上没问题。
- **`PoisonSpec` 辅助卡工厂**：用 `MakeShieldThenPoisonPlayerCard` 组合两个效果，验证护盾穿透时不能先加毒再加盾（否则毒触发时盾还没建）。

#### P3.2 实现约束

- **保留判定顺序**：先判断锚点（`IsHandAnchor`），再查 `Keywords` 和 `TemporaryKeywords`，最后判断双手区兜底。保证锚点永不被误弃。
- **弃牌扫描方向**：`DiscardNonRetainedNormalCardsAtTurnEnd` 从数组末尾向前扫，保证索引稳定；扫描过程中不移除锚点，所以`GetZoneOf` 在整个过程中的"双手区"判断一致。
- **双手区判定时机**：P3.2 按 Hand_Zone_Rules §7 的字面口径实现——"回合结束时左右手锚点都还在手牌"才启用双手区保留。若玩家本回合打出了一张锚点，`bLeftHandPresent` / `bRightHandPresent` 其中一个为 false，保留降级为只看 `Retain` 关键字。
- **测试的 Reshuffle 坑**：`NormalCardNoRetainDiscards` 第一版用 `Snap.PileCounts.DiscardCount >= N` 验证，但新回合 BeginPlayerTurn 会在 DrawPile 空时触发 ReshuffleDiscardIntoDraw 把 Discard 清空。第二版改用 InstanceId 精确追踪 + 把 Deck 做大到 15 张避免 Reshuffle。
- **历史记录**：当时 `Hand_Zone_Rules §3` 采用过"双手区被保留的普通卡保持原有相对位置"口径，并依赖 `GenerateHandQueueOnTurnStart` 的 "两锚点都在" 分支。该口径已被后续规则替换：保留只保留卡牌仍在手牌池中，不保留 index、相对顺序或区域；每回合开始统一重建手牌队列。

#### P3.3 实现约束

- **`FEffectExecutor::Execute` 签名改成 `FEffectContext&`**：Shuffle 分支把被移动卡 ID 写回 `Ctx.LastShuffledCardId`，后续 `AddCost/ReduceCost + Target.LastShuffledCard` 读取。同一批效果链内 `PlayCardResolver::ExecuteCardEffectOnce` 额外接一个 `FGuid& InOutLastShuffledCardId` 引用，跨 `Execute` 调用共享。
- **独立效果链**：主效果 / 完美释放 / AfterPlayed 被动 / ZoneHook ExtraEffects 各自维护自己的 `LastShuffledCardId` 局部变量，语义上互不串。朝光暮蝶主效果里也有 `Shuffle.Random`，但它不会覆盖 ZoneHook 的 `LastShuffledCardId`（因为后者已经在 ZoneHook 阶段消费完）。
- **`FillTargetFromCardEffect` 的 `Target.Self` 消歧**：原本只有 `Shuffle.ToRandomZone` 时才指向本卡。P3.3 把 `Effect.Card.AddCost` / `Effect.Card.ReduceCost` 也归入"指向本卡"分支。其它 `Target.Self` 仍指玩家（治疗、加盾、给玩家施毒）。
- **ZoneHook 触发点**：
  - `OnPlay`：放在 `CardPlayed` 事件之后、"记录出牌前先机"之前。此时 `RuntimeCost` 已定档（上面 `ComputeRuntimeCost` 一次性算）。AddCost 影响的是本卡"下次进手牌后"的 RuntimeCost。
  - `OnPerfectReleaseHit`：判断条件为 `!HitPartIds.IsEmpty()`。命中即跳过先机推进——一次判断一次生效，不执行 ExtraEffects（P3.3 朝光暮蝶左手区的规则仅限"不推先机"）。
- **ZoneHook 顺序要求**：ZoneHook(OnPlay) 在 `CardPlayed` 之后运行，所以若 Hook 里的 `Shuffle.Random` 改变了本卡所在区域，`GetZoneOfCard` 仍应用"进入 Resolve 时的区域"去判断哪些 Hook 触发。当前实现每个 Hook 迭代时都重查 `GetZoneOfCard`，为避免 Hook 内 Shuffle 自动扰动本卡区域，Hook 的效果链只 Shuffle 其他手牌，不 Shuffle 本卡。朝光暮蝶配法符合此约束。
- **DataAsset 重建**：修改 `BugGirlBuilder` 后必须跑 `WacomRegenerateContent` commandlet 重新生成 `DA_Card_ZhaoguangMudie.uasset`，否则 PIE / PIE 下的战斗仍是旧数据。自动化测试用的是 fixture 构造的 transient 卡，不受影响。

## 当前阶段状态

✅ 第一阶段 S1-S11 战斗内核 + 数据 + 自动化测试 + PIE 测试入口
✅ 第二阶段 P1 UI 基础设施 + P2 战斗 UI
⏳ 第二阶段 P3：规则补全（P3.1-P3.5 全部完成）
⏳ 第二阶段 P4：Enhanced Input 迁移
⏳ 第二阶段 P5：UI 动画基础
⏳ 第二阶段 P6：主题与样式

---

## Phase 3 R1–R5：战斗外 Run 骨架

Run 骨架五个切片一次性落地。战斗外探索 + 战斗自动切换 + 退出回归探索的闭环跑通。

### R1：GameMode / PlayerController / EGameFlowState 骨架
- `AWacomGameMode` 管理 `EGameFlowState { Exploration, Battle }`，提供 `EnterBattle` / `ExitBattle` 入口（R1 阶段只做状态切换日志）
- `AWacomPlayerController`：`RequestEnterBattle` / `RequestExitBattle` 转发，统一 `Push/PopMappingContext` 入口

### R2：第一人称 Pawn + IMC_Exploration
- `AWacomPlayerCharacter`：`UCameraComponent` + 跟随 Controller 旋转，WASD 通过 `IA_Move`（Axis2D + Negate/Swizzle 修饰符），鼠标通过 `IA_Look`（Axis2D + Y Negate 使"上移 = 抬头"）
- 扩展 `WacomCreateInputAssetsCommandlet`：新增 `IA_Move` / `IA_Look` / `IMC_Exploration`
- 默认资产引用用 `LoadObject` 在 `SetupPlayerInputComponent` / `BeginPlay` 解析，取代 `ConstructorHelpers::FObjectFinder`（CDO 阶段首次运行资产不存在会崩溃；命令行重新生成资产时也更友好）
- `ContentBuilderHelpers::CreateOrReplaceAsset` 对 rooted 占位对象跳过 `MarkAsGarbage`，修复首次 bootstrap assertion

### R3：BattleTriggerActor
- `ABattleTriggerActor`：`USphereComponent` + `UEnemyDefinition` 配置，Overlap 只接受被 Player 控制的 Pawn，转发到 `AWacomPlayerController::RequestEnterBattle`
- `bConsumeOnTrigger` + `bTriggered` + `SetGenerateOverlapEvents(false)` 三重防重入

### R4：EnterBattle / ExitBattle 完整切换
- GameMode 内创建 `UBattleSession` / `UWacomPrimaryGameLayout` / `UBattleHUD`，Push 到 Game Layer
- IMC 切换：`Pop Exploration` → `Push Battle(prio 1)`；退出时反向
- `UBattleHUD::OnBattleEndedNative`（新增 FMulticastDelegate）在 `Snapshot.Phase == BattleEnd` 时广播，GameMode 订阅后自动触发 `ExitBattle`
- 战斗结束：移除 HUD / Session，恢复 `GameOnly` 输入模式，`SetExplorationInputEnabled(true)`，`Trigger->Destroy()`
- `BattleHUDClass` 默认指向 C++ 类 `UBattleHUD`，避免 WBP 布局缺组件时 HUD 残缺（WBP 有全部 BindWidget 时再在 Details 覆盖）

### R5：URunSession + FRunState 骨架
- `URunSession`（`UObject`）+ `FRunState`（`USTRUCT`）拆分：Session 是行为入口，State 是数据
- `FRunState`：`Character` / `BattleSeed` / `DefeatedEnemies` / `bRunActive`
- PlayerController BeginPlay 创建 RunSession，`Initialize(GameMode->DefaultCharacter)`
- GameMode::EnterBattle 优先走 `Run->BuildInitParamsForBattle(EnemyDef)`，未就绪时回退到 GameMode 字段
- GameMode::ExitBattle 调 `Run->OnBattleFinished(Outcome, PendingEnemyDef)`：胜利添加 DefeatedEnemies，失败置 `bRunActive = false`
- `URunSession` 放在 `WacomRun` 模块；`WacomApp` 已有 `WacomRun` Public 依赖，无需改 Build.cs

### 关键约束 / 踩坑

- `AController::Character` 是 UE 内置字段，局部变量同名会触发 `C4458` 警告转错误——用 `CharDef` 之类的短名
- CommonUI 需要 `GameViewportClientClassName=/Script/CommonUI.CommonGameViewportClient`（`DefaultEngine.ini [/Script/Engine.Engine]`），否则 `LogUIActionRouter` 报错导致输入路由异常
- `APawn::SetupPlayerInputComponent` 可能早于 `BeginPlay` 被调用（possession 时），资产 `LoadObject` 要挪到绑定之前同一函数里做
- Battle HUD 用 C++ 默认布局时可直接铺全部组件；改用 WBP 时必须把全部 `BindWidget` 都放进去，否则 `RebuildWidget` 的默认布局分支被跳过

### 文档同步

- `Phase3_Run_Skeleton_Plan.md` 切片表 R1–R5 标记完成

---

## Phase 3.5 存档骨架 S1–S4

### S1：SaveGame 基础设施
- `UWacomSaveGame`（USaveGame）磁盘层；`FRunState` 扩展 `DestroyedTriggerIds`（TSet<FName>）/ `PlayerTransform` / `bHasPlayerTransform`
- `URunSession` 补 `SaveToSlot` / `LoadFromSlot` / `HasSaveInSlot` / `ResetRunState` / `BuildSaveGameFromRunState` / `ApplySaveGameToRunState` / `MarkTriggerDestroyed` / `IsTriggerDestroyed` / `SetPlayerTransform`
- 资产引用用 `FSoftObjectPath` 存盘，内存层保持 `TObjectPtr`
- `TSet<FName>` 落盘用 `TArray<FName>` 规避 UPROPERTY(SaveGame) 对容器的历史兼容问题
- 新测试 `Wacom.Run.Save.Roundtrip` 覆盖字段往返 + 空指针拒绝 + 未来版本拒绝 + 时序

### S2：GameMode 接入
- `AWacomGameMode::SlotName_Main` / `SlotName_Auto`
- `BeginPlay` 延后一帧 `BootstrapRunFromSave`（等 PlayerController 创建完 RunSession）
- Bootstrap 顺序：`Main` → `Auto` → 新开
- `ExitBattle` 末尾先写 `Auto` 再写 `Main`，防止单次崩溃丢两份
- `EndPlay` 只在 Exploration 状态写 `Main`；战斗中退出按规则丢弃进度

### S3：场景 Actor + 玩家位置
- `ABattleTriggerActor::PersistentId`（EditAnywhere FName）；BeginPlay 做 NAME_None warning + 同 id 唯一性检查 + RunSession 查询自销毁
- `ExitBattle` 胜利时 `Run->MarkTriggerDestroyed(id)` 再 Destroy Actor（非胜利不标记，下次重建触发器可再战）
- `BootstrapRunFromSave` 读档成功后先**清理已销毁 Trigger**再 teleport 玩家；顺序颠倒会让 `TeleportPhysics` 触发同一场战斗
- `SaveRunToSlot` 在 Exploration 状态存档前把当前 Pawn Transform 写入 `FRunState`

### S4：版本迁移骨架
- `UWacomSaveGame::MigrateIfNeeded(SaveGame)` 静态方法，switch + fallthrough 的链式迁移模板
- `ApplySaveGameToRunState` 把旧版本从"拒绝"改为"走迁移"，新版本仍拒绝
- Roundtrip 测试补 v0→Current 迁移用例 + 迁移后字段完整性断言
- 约束：已发布 case 不改只加

### 关键踩坑
- UE 5.7 的 UBT `GitSourceFileWorkingSet` 对 git 非 ASCII 路径敏感；`git config core.quotepath false` 修好
- `FAutomationTestBase::TestEqual` 对 `TObjectPtr<T>` 模板推导不明确，`.Get()` 拉出原始指针
- `AController::Character` UE 字段与局部变量同名会触发 `C4458` 警告转错误
- `SetupPlayerInputComponent` 可能早于 `BeginPlay` 触发，资产加载要挪到绑定前同函数
- `TeleportPhysics` 在瞬移路径上会生成 Overlap——读档顺序必须先清 Trigger 再 teleport
- GameMode + PlayerController 的 BeginPlay 顺序不保证；Bootstrap 用 `SetTimerForNextTick` 延一帧
- WBP_BattleHUD 有 BindWidget 但设计器里不全时，`RebuildWidget` 默认布局分支被跳过——第一版用 C++ 类 UBattleHUD 避开
- CommonUI 需要 `GameViewportClientClassName=/Script/CommonUI.CommonGameViewportClient`，否则 `LogUIActionRouter` 路由异常

### 文档同步
- `Save_System_Plan.md` 切片表与实现对齐
- `Phase3_Run_Skeleton_Plan.md` 标记 R1–R5 完成（历史切片）

---

## M1–M2：UI 基底 + 主菜单

### M1：Subsystem + PrimaryLayout 搬家
- `UWacomGameInstance`（空壳）+ `UWacomGameUIManagerSubsystem`（GameInstance Subsystem）
- PrimaryLayout 生命周期从 GameMode 迁到 Subsystem：EnsurePrimaryLayout / TearDown / Push / Pop / ClearLayer / ClearAllLayers
- GameMode 不再 CreateWidget / AddToViewport，改走 Subsystem 接口
- `DefaultEngine.ini` 加 `GameInstanceClass=/Script/WacomApp.WacomGameInstance`
- ClearLayer 实现：快照 WidgetList → 逐个 DeactivateWidget + RemoveWidget → ClearWidgets 兜底
- TearDownPrimaryLayout：先 ClearAllLayers（让 CommonUI Router 释放 UIInputConfig），再 RemoveFromParent
- EnsurePrimaryLayout 加 stale 检测：若 PrimaryLayout 的 OwningPlayer 不是当前 PC，TearDown 重建

### M2：菜单基类 + 主菜单 + L_MainMenu
- `UWacomMenuWidgetBase`：菜单血统基类，`SetIsFocusable(true)`，`GetDesiredInputConfig` 返回 `Menu + NoCapture`
- `UWacomMainMenuScreen`：C++ 默认布局（标题 + 三按钮），按钮回调委托给 MenuGameMode
- `AWacomMenuGameMode`：菜单关 GameMode，DefaultPawn=nullptr，BeginPlay Push MainMenu 到 GameMenu 层
  - `RequestStartNewGame`：清存档 → TearDown UI → OpenLevel(L_Exploration)
  - `RequestContinueGame`：TearDown UI → OpenLevel(L_Exploration)
- `UWacomExplorationHUD`：探索关 Game 层常驻占位 Widget，声明 `FUIInputConfig(Game, CapturePermanently)`
  - 让 CommonUI Router 在进入探索关卡时切回 Game 输入模式
  - 未来放小地图 / 任务提示 / 探索 HUD 元素
- `AWacomGameMode::BeginPlay`：Push ExplorationHUD + 主动 Push IMC_Exploration（幂等，防 PIE 复用 PC 时 IMC 丢失）
- `L_MainMenu` 关卡手动创建，WorldSettings 配 `AWacomMenuGameMode`
- `DefaultEngine.ini`：`GameDefaultMap` / `EditorStartupMap` 指向 `L_MainMenu`

### 关键踩坑

**PIE 里 OpenLevel 后输入失效（Standalone / 打包正常）：**
- 根因：PIE 模式下 `OpenLevel` 不销毁 `PlayerController`（跟着 LocalPlayer 走），PC 的 `BeginPlay` 不再调用，IMC_Exploration 从未被 Push
- 同时 PIE 的 Slate 焦点在 OpenLevel 后被编辑器主窗口抢走，PIE 视口收不到键盘事件
- 这是 UE5 PIE + CommonUI + OpenLevel 的已知交互问题，Standalone Game / 打包后完全正常
- 解决方案：`AWacomGameMode::BeginPlay` 主动 Push IMC_Exploration（幂等无害）；日常测试主菜单流程用 Standalone Game

**CommonUI UIActionRouter 跨关卡残留 UIInputConfig：**
- Router 是 LocalPlayer Subsystem，跨关卡存活
- 旧 Widget 如果没正常 Deactivate，Router 的 leaf-most config 会卡在 Menu 模式
- 解决方案：TearDown 时先 DeactivateWidget + RemoveWidget 每个 Widget；新关卡 Push ExplorationHUD 强制覆盖 Router config

**Widget 按钮 Click 回调里不应直接 OpenLevel：**
- Click 在 Slate 派发栈里执行，同帧 OpenLevel 会销毁 World，Widget 生命周期不稳定
- 解决方案：Widget 只发请求给 GameMode，由 GameMode 控制切关卡

**UWidget::IsFocusable() 在 UE5.7 不存在：**
- 改用 `SetIsFocusable(true)` 在构造函数里设置

**UWidget::Slot 成员名冲突：**
- Widget 派生类的函数里局部变量不能叫 `Slot`，会触发 C4458

**NativeOnInitialized 早于 RebuildWidget：**
- 按钮绑定（OnClicked.AddDynamic）必须放在 NativeConstruct，不能放 NativeOnInitialized

### 测试约定

- 主菜单 → 探索完整流程：用 **Standalone Game** 测试
- 探索 / 战斗 / 存档：直接 PIE Play L_Exploration（跳过主菜单）
- 自动化测试：30 个全绿（不涉及 UI / 关卡切换）

---

## M3–M4：暂停菜单 + 确认对话框 + 焦点收尾

### M3：暂停菜单 + ConfirmDialog + ESC 路由
- `UWacomPauseMenuScreen`：Resume / Save / Quit to Menu 三按钮
- `UWacomConfirmDialog`：通用确认框，静态工厂 `Show(WorldContext, Title, Message, OnConfirm, OnCancel)`，Push 到 Modal 层
- `IA_OpenMenu`（ESC 键）加到 `IMC_Exploration` 和 `IMC_Battle` 两个 IMC
- `AWacomPlayerController::OnOpenMenuPressed`：GameMenu 层有 Widget → ESC = Pop（Resume）；无 Widget → Push PauseMenu
- MainMenu 的 New Game（有存档时）/ Quit Game 改走 ConfirmDialog
- PauseMenu 的 Quit to Menu 改走 ConfirmDialog
- PIE 设置：`StopPIEOnESC=False`（编辑器偏好设置取消"按 ESC 停止 PIE"）

### M4：焦点自动聚焦
- `UWacomMenuWidgetBase::NativeOnActivated`：延迟一帧后遍历 WidgetTree 找第一个 enabled 的 UButton，调 `SetKeyboardFocus`
- 延迟一帧的原因：CommonUI Router 在 Push 后需要一帧完成 leaf-most 切换；同帧 SetFocus 会被下层 Widget 抢回
- 键盘 Enter/Space 可直接确认聚焦按钮，方向键/Tab 可在按钮间切换
- Modal（ConfirmDialog）打开时焦点正确落在 Confirm 按钮，不会跑到下层 PauseMenu

### 踩坑
- CommonUI 的 leaf-most 切换是异步的（下一帧才生效），同帧 SetKeyboardFocus 会被旧 leaf-most 抢回
- PIE 默认 ESC 退出 PIE，需要在编辑器偏好设置里关闭
- Standalone Game 看日志：启动参数加 `-log` 或看 `Saved/Logs/Wacom.log`

### 文件清单

新增：
- `WacomApp/Public/UI/Menus/WacomPauseMenuScreen.h` + `.cpp`
- `WacomApp/Public/UI/Menus/WacomConfirmDialog.h` + `.cpp`
- `Content/Wacom/Input/IA_OpenMenu.uasset`

修改：
- `WacomCreateInputAssetsCommandlet.cpp`：加 IA_OpenMenu + ESC 映射到两个 IMC
- `WacomPlayerController.h/.cpp`：加 IA_OpenMenu 字段 + OnOpenMenuPressed
- `WacomMainMenuScreen.cpp`：New Game / Quit 改走 ConfirmDialog
- `WacomPauseMenuScreen.cpp`：Quit to Menu 改走 ConfirmDialog
- `WacomMenuWidgetBase.h/.cpp`：焦点延迟一帧 + FocusFirstButton 提取为独立方法
