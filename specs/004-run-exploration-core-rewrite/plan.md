# Implementation Plan: Run 探索规则核心重构

**Branch**: `main` | **Date**: 2026-07-14 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/004-run-exploration-core-rewrite/spec.md`

**Note**: `AGENTS.md`、`Docs/WacomMap.md`、`Docs/WacomRun.md` 和其它领域文档仍是长期规则真相；本目录只用于组织本轮实现。

## Summary

将 Run 探索从“场景 Trigger + Spline + RemainingNodeCount”原型重构为两层结构：`WacomData` 的手工 Logical Map Graph 与 `WacomRun` 的显式探索事务构成规则核心；`WacomApp` 使用新的 Path Traversal 与场景绑定 Registry 呈现规则结果。现有 Cursor Look、CameraShake、ViewStage、first-person card layer、Battle/Shop/RunEvent 页面和纸片场景美术继续复用。

实现按五个可编译切片推进：静态数据与 Validator、Run 核心、节点内容原子结算、新 Traversal/App Seam、调试资产迁移与旧路径删除。现有 `L_Exploration` 迁为单层最小 Debug Journey；正式地图、跨层确认与 Camp UI 延后。

## Wacom Domain Context

**Primary Domain**: Run-exploration / Data-card authoring / UI-App / Architecture-modules

**Required Docs Read**:
- [x] `AGENTS.md`
- [x] `CONTEXT.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/Game_Design.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomWorldInteraction.md`

**Docs To Update**:
- [ ] `CONTEXT.md`
- [ ] `Docs/WacomMap.md`
- [ ] `Docs/WacomRun.md`
- [ ] `Docs/WacomApp.md`
- [ ] `Docs/WacomData.md`
- [ ] `Docs/WacomDataAuthoring.md`
- [ ] `Docs/WacomWorldInteraction.md`
- [ ] `Docs/Architecture.md`
- [ ] `Docs/Game_Design.md`
- [ ] `Docs/TODO.md`
- [ ] `Docs/TechDebt.md`
- [ ] `Docs/Questions.md`

**Owning Module(s)**: WacomData / WacomRun / WacomApp / WacomEditor / WacomTests

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomEditor -> WacomData/WacomApp for authoring and validation only
WacomTests  -> runtime/app/editor validation as test harness
```

不新增 UE Module，不增加反向依赖。`WacomRun` 继续依赖 `WacomBattle` 以消费 Battle Result；`WacomBattle` 不读取 Run 或 Map 类型。

## Technical Context

**Language/Engine**: C++ with Unreal Engine 5.8 toolchain

**Primary UE Systems**: UPrimaryDataAsset、USTRUCT/UENUM reflection、SplineComponent、ActorComponent、Enhanced Input、CameraShake、Automation Tests、Data Validation、AssetRegistry、Editor Commandlet

**Storage/State**: `FRunState` 中的持久候选探索/时间状态；Session-private traversal/activity tokens；Journey/Floor DataAssets；本轮不写 SaveGame

**Runtime Contracts**: Run Initialization Result、Exploration Command/Resolution/Snapshot/Event、Map Node/Edge identity、Node Activity token/result、Scene Binding registry

**Testing**: 新增 `Wacom.Run.Map`、`Wacom.Run.Time`、`Wacom.Run.NodeActivity`、`Wacom.UI.RunPathTraversal`、地图 Data Validation；回归 RunEvent、Shop、Battle Result、WorldInteraction、first-person 与完整 `Wacom`

**Target Platform**: Windows Editor 与最终 Windows packaged build

**Performance Goals**: 地图查询不增加 Tick；小型手工 Floor 的节点/边验证和运行时查找在一次事务内完成；Traversal 维持现有逐帧 Spline 跟随成本；UI/状态刷新只在提交结果时发生

**Constraints**:
- 规则不读取 Actor、Spline、Widget、地图 UI 坐标或世界坐标。
- App 在提交场景相关命令前完成 Scene Binding preflight。
- 失败/重复/过期结果无副作用，不产生累计事件队列。
- 不保留 NodeCount、ConsumeNode、旧 Run Tunnel 类型或 CoreRedirect。
- 不回退当前工作区已有地图设计文档改动。

**Scale/Scope**: 5 个模块、RunSession/FRunState 公共契约、4 类节点内容结算、3 个旧 Run Tunnel 反射类型、Player/GameMode/interaction adapters、一个现有关卡、多个 BP/DataAsset、约 40 个直接或间接受影响测试文件

**Live Impact Audit**: CodeGraph 显示旧 Traversal Component 影响 52 个 symbols；live `rg` 显示旧 NodeCount/ConsumeNode/Run Tunnel token 分布于 44 个源码/配置文件，旧 Run 初始化约有 226 个测试调用点；二进制 token 审计定位 5 个直接迁移资产。静态图未识别的反射/测试引用以 live 文件、AssetRegistry 和最终 Blueprint compile 为准。

**Blueprint Exposure Strategy**:
- Journey/Floor/Node/Edge 与场景绑定字段需要制作和序列化，因此使用 UCLASS/USTRUCT/UENUM，并只提供 designer-facing `EditDefaultsOnly`/`EditInstanceOnly` 字段。
- Traversal Component 与 Path/Anchor Actor 需要放置和 BP 派生，因此反射暴露有限调参和只读诊断。
- Exploration Command、Resolution、opaque token、Coordinator 和规则 Modules 使用普通 C++，不暴露 Blueprint 写入口。
- Map/Camp UI 后续通过 App ViewData/Action 接入，不直接读取或修改 `FRunState`。

**Data/GameplayTag Impact**: 新增 Journey/Floor/Map 数据资产、固定 v1 tagged node payload、Validator 和 Debug 资产；不新增 GameplayTag

**Save/Load Impact**: 不提升 `UWacomSaveGame::CurrentSaveVersion`，不序列化新地图状态，不承诺旧探索存档恢复；SaveGame 正式化继续保持 Deferred

**UI/App Lifecycle Impact**: 不新增 CommonUI Screen；PlayerController-owned private Coordinator 在 BeginPlay 绑定 Session/Scene，在 EndPlay 反订阅；Battle/Shop/Event 打开期间暂停 Traversal，关闭后继续使用现有 ViewStage Return Flow

## Constitution Check

*Pre-design gate: PASS. Post-design re-check: PASS.*

- **Docs and AGENTS Are the Rule Truth**: 已列出规则文档与完成后回写路径；specs 不取代 Docs。
- **Wacom Module Boundaries Are Mandatory**: 静态数据、Run 规则、App 表现、Editor 工具和 Tests 各自归属明确，Build.cs 方向不变。
- **Domain Rules Before Presentation**: 所有地图合法性、Action Point、Camp、Floor Transition 和内容结算位于 WacomRun；App 只发送意图并应用结果。
- **Data, GameplayTags, and Authoring Stay Explicit**: DataAsset、场景绑定、Validator、Debug Builder 与零 GameplayTag 影响均明确。
- **Reusable Systems Over One-Off Work**: 使用 Journey/Floor 定义、私有规则 Modules、Scene Registry 和 Coordinator；不使用 Level Blueprint、临时全局或 Tick 轮询规则。
- **Validation Is Part of the Slice**: 每个切片都有 compile/focused automation，最终包含 AssetRegistry、Blueprint compile 和 PIE。

## Phase 0: Research

研究结论见 [research.md](./research.md)。关键决定：

- Logical Map Graph 与局部 Spline graph 分离，DataAsset 是唯一规则真相。
- 使用终点提交的两阶段 traversal ticket，避免中途返回导致逻辑/画面分叉。
- Begin 与 Complete 前都预检目标 Anchor/Host；成功提交后只允许停留在 target 侧，不得用 source fallback 制造逻辑/画面分叉。
- 使用 working-state + explicit result 取代 bool/void 与外部手工扣点。
- 固定成本节点活动在开始前预留 Action Point；撤离释放预留但保留进度和奖励。
- 旧 Run Tunnel 类整体替换；经过验证的 Cursor Look、CameraShake、ViewStage 和 card layer 作为外围复用。
- Camp 本轮只提供 typed handler seam，不用假恢复数值或临时 UI 填补内容空缺。

## Phase 1: Design Artifacts

- [data-model.md](./data-model.md)：Journey/Floor/Node/Edge、Runtime state、tokens 与状态迁移。
- [contracts/run-exploration-contract.md](./contracts/run-exploration-contract.md)：初始化、命令、结果和幂等合同。
- [contracts/node-activity-contract.md](./contracts/node-activity-contract.md)：Battle/RunEvent/Shop/Treasure/ Camp 原子结算。
- [contracts/scene-binding-contract.md](./contracts/scene-binding-contract.md)：App scene registry、Path Traversal 与失败恢复。
- [contracts/editor-validation-contract.md](./contracts/editor-validation-contract.md)：DataAsset/场景验证和调试资产构建。
- [quickstart.md](./quickstart.md)：分切片编译、自动化、资产和 PIE 验收。

## Project Structure

### Documentation (this feature)

```text
specs/004-run-exploration-core-rewrite/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── checklists/requirements.md
├── contracts/
└── tasks.md
```

### Source Code (expected implementation areas)

```text
Source/WacomData/
├── Public/Map/                 # Journey/Floor/Node/Edge reflected definitions
└── Private/Map/                # data-only helpers, no runtime state

Source/WacomRun/
├── Public/Exploration/         # stable identity/snapshot/command/result contracts
├── Public/RunSession.h         # facade and explicit initialization/result entry
└── Private/Exploration/        # map/time/exposure/camp/activity modules

Source/WacomApp/
├── Public/Components/          # new RunPathTraversal component
├── Public/Actors/              # PathSegment/BranchTarget/NodeAnchor/binding component
└── Private/GameFramework/      # exploration presentation coordinator and registry

Source/WacomEditor/
├── Public/Validation/          # Journey/Floor report contract
└── Private/                    # validators and idempotent debug asset migration

Source/WacomTests/
├── Public/Fixtures/            # initialized Run + transient Journey fixtures
└── Private/{Run,Data,UI}/      # small focused specs
```

**Structure Decision**:
- WacomData 使用 `Map/` 而不是把图字段塞入 Character、RunEvent 或场景 Actor。
- WacomRun 保留 `URunSession` Facade 以复用已验证 UI/provider，但所有新算法进入 `Private/Exploration`。
- WacomApp 新 Traversal 类型整体替换旧类；Coordinator/Registry 保持 Private，场景 Actor 只反射制作映射。
- WacomEditor 使用现有 Data Validation 与 content builder 风格，不新增 runtime dependency 或第二份图数据。
- WacomTests 新建小型 specs，不继续扩大 `BackpackSpec.cpp`、`WorldInteractionAndShopSpec.cpp` 或旧 `RunTunnelMovementSpec.cpp`。

## Migration Strategy

1. **Static contracts**：新增 Map DataAssets、Validator 和 transient test fixture；旧运行路径继续编译。旧 Run 初始化 UFUNCTION 暂时保留原实现且禁止新增调用，不作为新 API wrapper。
2. **Run core**：加入新状态、初始化结果和 explicit exploration resolution；将时间测试迁移为 Action Point 语义。
3. **Node activities**：迁移 Battle/RunEvent/Shop/Treasure 成本与原子结算，移除 GameMode/Screen 外部扣点。
4. **App replacement**：新增 Path Traversal/actors/coordinator，同时暂存旧类以便资产迁移和对照测试。
5. **Asset migration**：构建 Debug Journey，迁移 `GM_Wacom`、`BP_WacomPlayerCharacter`、`L_Exploration` 和 Run Path BP，编译并重存。
6. **One-shot cleanup**：Debug Journey 可加载后迁移约 226 个测试初始化调用、生产调用与 ResetRunState，再删除旧初始化 UFUNCTION、旧类型、字段、DataAsset enum 值、BP 资产和 CoreRedirect；全仓库/AssetRegistry 零引用审计。

每一步必须先完成相应 focused tests 与 `WacomEditor` 编译，禁止在未完成资产重存前删除旧反射类。

## Validation Plan

**Compile**:

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

**Focused Automation**:

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.Run.Map; Automation RunTests Wacom.Run.Time; Automation RunTests Wacom.Run.NodeActivity; Automation RunTests Wacom.UI.RunPathTraversal; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

补充前缀：`Wacom.Run.Event`、`Wacom.UI.Shop`、`Wacom.UI.WorldInteraction`、`Wacom.UI.RunFirstPersonCardLayer`、`Wacom.UI.FirstPersonCardLayer`、`Wacom.UI.Battle`、`Wacom.Settings`。

**Full Automation**:

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

**Manual/Editor Validation**:
- 运行 Debug Journey/Floor 构建命令两次并比较资产数量、身份和 Validator 结果。
- Blueprint 全量编译，确认移除旧反射类后 0 failed load。
- PIE `L_Exploration` 验证 W/S、分支、后退取消、终点提交、Battle/Withdraw、RunEvent、Shop、Treasure、ViewStage return 和 first-person card layer。
- 使用 AssetRegistry 与 `rg` 审计旧类型、NodeCount、ConsumeNode 和 Redirect 为零引用。

## Complexity Tracking

无 Constitution 违规。大型迁移通过五个可编译切片控制风险，不引入临时兼容层或第二份地图真相。
