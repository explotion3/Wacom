# Quickstart Validation: Run 探索规则核心重构

## Prerequisites

- Unreal Editor 已关闭后执行 C++/反射类迁移构建。
- 工作目录：`D:\UE_Project\5.7\Wacom`
- 当前工作区已有地图设计文档改动必须保留。
- 资产迁移前保留旧反射类，迁移/重存完成后才能执行旧类删除切片。

## Execution baseline (T001-T004)

### Workspace boundary

- 实现开始时当前分支为 `main`。
- 必须保留既有设计文档改动：`CONTEXT.md`、`Docs/Architecture.md`、`Docs/Game_Design.md`、`Docs/Questions.md`、`Docs/Roadmap.md`、`Docs/TODO.md`、`Docs/WacomRun.md` 与新增 `Docs/WacomMap.md`。
- 本 feature 允许定向修改 `.specify/`、上述领域文档、`Source/WacomData`、`Source/WacomRun`、`Source/WacomApp`、`Source/WacomEditor`、`Source/WacomTests`、相关 `Content/Wacom` 调试资产与 `Config/DefaultEngine.ini`；不得回退无关工作区改动。
- `specs/` 当前被 `.gitignore` 忽略，规划工件只作为本地执行清单，不会自动进入普通 Git 提交。

### Live impact audit

- CodeGraph 识别 `UWacomRunTunnelMovementComponent` 影响 52 个 symbols，覆盖 Player/Controller、first-person card、ViewStage、Settings builder/spec、Battle/Run UI tests。
- live `rg` 在源码/配置中发现旧 NodeCount、ConsumeNode 与 Run Tunnel 相关 token 共 344 行、44 个文件；CodeGraph 对 `ConsumeNode` 的静态 caller 数量偏低，迁移以 live 源码和最终零引用审计为准。
- `URunSession::Initialize(UCharacterDefinition*)` 在自动化测试中约有 226 个调用点，必须在 Debug Journey 可用后一次性迁移。
- 二进制 token 审计发现 5 个直接受影响资产：`BP_WacomPlayerCharacter`、`DA_Event_DebugSnakeGift`、`L_Exploration`、`BP_RunTunnelSegment`、`BP_WacomRunTunnelBranchTargetActor`。

### Pre-migration validation

- `WacomEditor` baseline compile：通过（2026-07-14，UE 5.8，Win64 Development，target up-to-date）。
- `Wacom.Run`：176/176 Success。
- `Wacom.UI.RunTunnel`：17/17 Success。
- `Wacom.UI.Event`：12/12 Success。
- `Wacom.UI.Shop`：14/14 Success。
- `Wacom.UI.RunFirstPersonCardLayer`：51/51 Success。
- `Wacom.UI.FirstPersonCardLayer.Anchor`：13/13 Success。
- 所有进程均记录 `TEST COMPLETE. EXIT CODE: 0`；日志位于 `Saved/Logs/RunExplorationBaseline_*.log`，不作为提交内容。

## Slice 1: Data contracts and validation

1. 编译 `WacomEditor`。
2. 运行 Journey/Floor Data Validation focused tests。
3. 验证 transient valid/invalid graph fixtures。

预期：合法图通过；重复身份、无效边、不可达节点和错误 payload 被明确阻断。

Phase 2 foundational validation（2026-07-14）：

- `WacomEditor Win64 Development`：通过；UHT 已验证 Journey/Floor、组合状态和 floor-qualified handle 反射合同。
- `Wacom.Run.Map.ContractsSmoke`：1/1 Success，`TEST COMPLETE. EXIT CODE: 0`。
- transient fixture 已验证正式初始化返回版本 1、显式事件、有效当前 NodeHandle，以及 Morning Planning 后剩余 1 AP。
- 旧 `Initialize(UCharacterDefinition*)` 和旧 Run 运行路径仍保留原实现；未在本阶段切换正式资产。

US1 transient traversal validation（2026-07-14）：

- `WacomEditor Win64 Development`：通过；新 Path Actor、Traversal Component、Scene Registry、Presentation Coordinator 与 automation test view 均已链接。
- `Wacom.Run.Map`：5/5 Success。
- `Wacom.UI.RunPathTraversal`：9/9 Success，`TEST COMPLETE. EXIT CODE: 0`。
- 已覆盖 W/S 局部前后移动、start/end one-shot、Suspend/Resume、source/target/content-host 双阶段预检、版本漂移、路径启动失败补偿 Cancel、commit 失败补偿 Cancel、目标绑定失效回源，以及成功提交后只使用目标缓存且禁止回源。
- PlayerCharacter / PlayerController、first-person Anchor、card pointer look 与 ViewStage return 已采用新 Path 优先；`Wacom.UI.Battle.FirstPersonViewStageReturnFlow` 4/4、`Wacom.UI.RunFirstPersonCardLayer` 51/51、`Wacom.UI.FirstPersonCardLayer.Anchor` 13/13 Success。
- 已补齐 scene-binding contract 中 tasks 漏列的 `UWacomRunMapNodeBindingComponent`，内容 Host 可声明 NodeId + NodeType，不需要 Controller 硬编码 Actor 类型。
- 正式 `L_Exploration` 尚未迁移；旧 Run Tunnel 只在新地图状态 / 场景绑定未就绪时作为后续资产迁移 fallback。

## Slice 2: Run core

运行：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.Run.Map; Automation RunTests Wacom.Run.Time; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

预期：初始化原子性、lifecycle、traversal ticket、MapTravel、FloorExposure、Camp/transition token 全部通过。

## Slice 3: Node activities

运行 `Wacom.Run.NodeActivity`、RunEvent、Shop、Battle Result 与 WorldInteraction focused tests。

重点确认：

- Victory 1 AP；Withdraw 0 AP 且保留进度/奖励。
- Withdraw 仍 Fatigue +1，并按现有 packet facts 结算 Wound。
- Shop 首次成功 purchase 1 AP，同 visit 后续 0。
- RunEvent 无 ConsumeNode effect，选项成本原子回滚。
- Treasure 失败 0、首次成功 1。

## Slice 4: Path traversal and peripheral regression

运行：

- `Wacom.UI.RunPathTraversal`
- `Wacom.UI.RunFirstPersonCardLayer`
- `Wacom.UI.FirstPersonCardLayer`
- ViewStage return tests
- Settings camera motion tests

预期：W/S、边界单次事件、Suspend/Resume、CameraShake、CursorLook、ViewStage 和 card anchor 行为与迁移前一致。

## Slice 5: Asset migration

1. 运行 Run Exploration Debug asset builder 两次。
2. 运行 Journey/Floor/Scene Binding validation。
3. 执行 Blueprint 全量编译。
4. 使用 AssetRegistry/`rg -a` 确认旧类型和字段零引用。
5. 删除旧反射源码/Redirect 后重新编译和 Blueprint compile。

## Compile

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

## Full automation

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

已知无关失败必须与执行前基线对比；不得把新失败归为既有问题而无证据。

## PIE acceptance (`L_Exploration`)

- 进入调试 Journey 后位置与 current node 一致。
- W/S 沿 Path 移动；中途退回起点取消 traversal。
- 到达终点才进入目标 node，后续分支/内容只在提交后开放。
- Battle Victory 正常完成节点；Withdraw 回到探索、保留破坏进度且不消耗 AP。
- RunEvent、Shop、Treasure 仍使用现有页面/交互，成本符合矩阵。
- 战斗/页面关闭后 ViewStage、CursorLook、CameraShake 和 first-person cards 正常恢复。
- 死胡同显示未来 MapTravel 所需规则 facts；本轮不要求正式 Map Screen。

## Docs completion check

功能完成后将最终规则、制作合同、接入方式、验证结果和剩余内容工作回写 `Docs/`；Spec Kit 文件不能成为唯一事实来源。

## Final validation (T091-T095, 2026-07-14)

### Compile and authoring

- `WacomEditor Win64 Development`：通过；旧反射类和 Blueprint 删除后的完整 UHT / link 通过。
- `Wacom.Editor.RunExploration.DebugAssetBuilder`：1/1 Success；单次测试内部连续构建两次，JourneyId、FloorId、NodeId、EdgeId、内容引用和 Path Blueprint 身份稳定。
- `Wacom.Data.Map.Validation`：3/3 Success。
- `Wacom.UI.RunSceneBinding.Validation`：1/1 Success。
- Blueprint 全量编译：0 error、7 warning。warning 均为既有 PaperZD 配置、DreamShader include cache、MCP EULA、Python 同名枚举和 `WBP_MenuTest` deprecated Run menu lease；没有旧 Run 探索类型或 missing parent。
- `git diff --check`：通过，仅输出工作区既有 LF -> CRLF 提示。

### Focused automation

- `Wacom.Run`：202/202 Success，覆盖 Map、Time、NodeActivity、Camp、FloorTransition、Save schema 3、RunEvent、Shop、Treasure、Battle settlement 和初始化迁移。
- `Wacom.UI.RunPathTraversal`：16/16 Success。
- `Wacom.UI.GameMenu`：7/7 Success。
- `Wacom.UI.WorldInteraction`：198/198 Success。
- `Wacom.UI.Event`：12/12 Success。
- `Wacom.Settings`：6/6 Success。
- 额外定向：`Wacom.Run.NotificationCoalescing` 5/5、`Wacom.Run.Event` 24/24、`Wacom.Run.WorldCardInteraction` 9/9、`Wacom.Run.SnapshotRevisions` 5/5 Success。
- 本轮早期已通过：`Wacom.UI.Battle.FirstPersonViewStageReturnFlow` 4/4、`Wacom.UI.Battle.EntryFirstPersonViewpoint` 12/12、`Wacom.UI.FirstPersonCardLayer.Anchor` 13/13。

### Legacy / AssetRegistry audit

- 旧 Movement / Segment / Branch 六个原生文件和两个 Blueprint 资产路径均不存在。
- Blueprint 全量加载编译使 AssetRegistry 扫描项目后，`rg -a` 对 `Content/Wacom` 未发现旧类型或旧 Blueprint package token。
- `Source / Config / Content / Docs / CONTEXT.md`（排除阶段性 `specs/` 迁移记录）对旧初始化签名、旧预算字段 / effect、旧 Movement / Segment / Branch 类型、旧 Blueprint 名和 redirect token 均为 0 引用。
- 仅保留独立美术 `AWacomRunTunnelPaperLayerActor` 及既有纸片材质 / 资产命名；它不参与 Run Path 或规则。

### Full Wacom regression

- 完整发现并执行 1368 项：1365 Success、3 Fail，测试进程完整结束，无 assertion / crash。
- 失败均位于未被本 feature 修改的 Battle first-person 无目标出牌链：
  - `Wacom.UI.Battle.BattleHUD.HandPresentation.ShortcutStartsDragByHandIndex`
  - `Wacom.UI.Battle.FirstPersonTargetPreview.NoTargetCommitShowsPlayerActionPreview`
  - `Wacom.UI.Battle.PresentationQueue.NonblockingInput`
- 前两项是本 feature 开始前已知的独立 Battle first-person 失败；第三项复用同一 no-target submit 路径，原测试在 stack 只有一项时直接访问索引 1 并导致全量中止。本轮只增加测试边界检查，使它稳定报告同根失败，没有修改 Battle 生产行为。Run、Run Path、Settings、WorldInteraction 和 GameMenu 未新增失败。

### Manual PIE

- 自动化和资产迁移已完成；`L_Exploration` 的手感 / 场景验收仍需用户打开编辑器按上方 PIE acceptance 清单执行。

### PIE finding: Anchored character gravity (2026-07-15)

- 首轮 `L_Exploration` PIE 发现角色在当前节点成功锚定后仍会立即向下掉落。
- 根因是 `UWacomRunPathTraversalComponent::AnchorAtTransform()` 只应用了一次 Pawn / View Transform，没有像 Traversing 状态一样停止并禁用 `CharacterMovement`；下一帧保留的 Falling / Velocity 继续受重力驱动。
- 修复后 Anchored 与 Traversing 统一取得 movement ownership：清除速度并进入 `MOVE_None`。回归断言覆盖从 Falling + 向下速度进入 Anchored。
- `Wacom.UI.RunPathTraversal.StateAndBoundaryOneShot` 修复前按预期失败、修复后通过；完整 `Wacom.UI.RunPathTraversal` 16/16、`Wacom.UI.Battle.FirstPersonViewStageReturnFlow` 4/4、`Wacom.UI.GameMenu` 7/7 通过。仍需重新执行本页人工 PIE 检查后完成 T095。

### PIE finding: initial viewport focus and premature BattleTrigger interaction (2026-07-15)

- 首次进入 `L_Exploration` 时 CommonUI 已应用 `GameAndUI + NoCapture`，但初始 `UWacomExplorationHUD` 没有把 Slate focus 交给嵌入式 PIE 游戏视口，导致必须先点击画面一次，W/S 和 cursor-look 才生效。HUD 现于激活后下一帧请求游戏视口焦点，并在失活 / 销毁时取消请求。
- 移动途中点击目标 Encounter 的 BattleTrigger 时，Run 规则以 `ExplorationActivityAlreadyActive` 正确拒绝了 Encounter；但 PlayerController 在 GameMode 校验之前提前清空 Run hand，形成“手牌消失、HUD 和镜头仍在 Run”的半切换。现在 `RequestEnterBattle()` 只上报意图，Run hand 仅在 Encounter ticket、BattleSession 和 BattleHUD 全部成功后清理。
- 带地图绑定的 BattleTrigger 现要求绑定节点已正式提交为 current `Visited Encounter` 且不存在活动探索事务；未抵达、Traversal 中、已解决或错绑定均不可交互，并返回明确 hover 提示。
- 新回归 `Wacom.UI.RunBattleEntry` 2/2 与 `Wacom.UI.ExplorationHUD` 1/1 通过；既有 `Wacom.UI.WorldInteraction.BattleTrigger` 15/15、`Wacom.UI.RunPathTraversal` 16/16、`Wacom.UI.Battle.EntryFirstPersonViewpoint` 12/12、`Wacom.UI.GameMenu` 7/7 通过。仍需人工 PIE 确认初始输入无需点击、到达 Encounter 前不可进入、到达后 HUD / 手牌 / 镜头原子切换。
