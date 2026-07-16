# Implementation Plan: Run 正式关卡制作基线收口

**Branch**: `[006-run-level-authoring-baseline]` | **Date**: 2026-07-16 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/006-run-level-authoring-baseline/spec.md`

**Note**: 本计划只定义实施顺序；`AGENTS.md` 与长期 `Docs/` 仍是规则真相。正式实现由主会话逐任务读取 live 文件后执行，不使用一次性 `$speckit-implement`。

## Summary

把 `L_Exploration` 从调试生成器拥有的原型地图迁移为可长期人工制作的正式 Run 场景，同时保留一个完全隔离、可幂等重建的 Debug Run 夹具。每个 Run Floor 关卡通过唯一的 `AWacomRunFloorSceneDescriptorActor` 单向引用 `UWacomFloorMapDefinition`；运行时和编辑器都先解析该声明，再建立或验证 Actor 绑定。新增只读场景验证合同、编辑器菜单与命令行入口，确保制作错误能在 PIE 前被定位。

本切片不设计正式第一层图。当前已验证 8 节点图迁入 `Authoring` 身份的过渡 DataAsset，只作为可替换制作基线；Debug 图继续服务自动化。

## Wacom Domain Context

**Primary Domain**: Run-exploration / Data authoring / Architecture / Tests

**Required Docs Read**:
- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/TODO.md`
- [x] `Docs/Questions.md`

**Docs To Update**:
- [ ] `Docs/WacomMap.md`
- [ ] `Docs/WacomApp.md`
- [ ] `Docs/WacomDataAuthoring.md`
- [ ] `Docs/Architecture.md`
- [ ] `Docs/TODO.md`
- [ ] `Docs/Questions.md`（只保留正式 Floor 1 内容与稳定身份的未决问题）

**Owning Module(s)**: WacomData / WacomApp / WacomEditor / WacomTests

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomEditor -> WacomData/WacomApp for editor-only authoring and validation
WacomTests  -> runtime/app/editor validation as test harness
```

不新增模块，不修改任何 `Build.cs`。

## Technical Context

**Language/Engine**: C++, Unreal Engine 5.8

**Primary UE Systems**: DataAssets / World Actors / AssetRegistry / ToolMenus / Commandlets / Automation Tests

**Storage/State**: `UWacomJourneyDefinition`、`UWacomFloorMapDefinition`、`.umap` 场景 Actor；运行时只读 `FRunExplorationSnapshot`

**Runtime Contracts**: Floor Scene Descriptor、原子 Scene Binding Registry refresh、只读 Scene Binding Diagnostic

**Testing**: `Wacom.UI.RunSceneBinding`、`Wacom.Editor.RunSceneValidation`、`Wacom.Editor.RunExplorationDebugAssets` 及受影响 Run/Battle/Shop/Event 回归；完整 `WacomEditor` 编译

**Target Platform**: Windows editor / packaged Windows build

**Performance Goals**: 运行时仅在 Session/World/Floor 绑定刷新时解析 Descriptor 与枚举 Actor；不新增 Tick、轮询或常驻场景扫描

**Constraints**: UI/规则语义不变；正式地图不可被 Debug builder 保存；Validator 严格只读；Floor DataAsset 不反向引用 World；资产迁移保留 Actor GUID、Transform、Spline 与美术

**Scale/Scope**: 1 个正式地图、1 个 Debug 地图、2 组 Journey/Floor 数据身份、1 个 Debug GameMode、1 个场景声明 Actor、1 套验证合同、2 个命令行入口变更和小型自动化 spec

**Blueprint Exposure Strategy**: `AWacomRunFloorSceneDescriptorActor` 必须是可放置 `UCLASS`；只暴露 `EditAnywhere, BlueprintReadOnly` 的 Floor 引用供关卡制作，不暴露 Blueprint setter、Tick 或规则入口。诊断、resolver、working registry 和工具协调保持普通 C++ / App-private / Editor-private。

**Data/GameplayTag Impact**: 不修改 DataAsset schema，不新增 GameplayTag；新增 `Authoring` Journey/Floor、Debug GameMode 和 Debug map；移除正式地图 Run binding Actor 上的 Debug generated ownership tag

**Save/Load Impact**: 无 schema 变化；`Journey.Authoring` / `Floor.Authoring.01` 在正式存档启用前属于可替换身份，不承诺迁移兼容

**UI/App Lifecycle Impact**: 无新 Screen；PlayerController 的场景刷新先解析 Descriptor 并建立 working registry，全部成功后再替换已绑定 registry/coordinator 状态

## Constitution Check

*GATE: Phase 0 前与 Phase 1 设计后均通过。*

- **Docs and AGENTS Are the Rule Truth**: PASS。已列出读取与更新文档；Spec Kit 不承载最终规则真相。
- **Wacom Module Boundaries Are Mandatory**: PASS。静态数据保持 WacomData，场景 Actor/运行时绑定保持 WacomApp，工具保持 WacomEditor，测试保持 WacomTests；无 Build.cs 变化。
- **Domain Rules Before Presentation**: PASS。`WacomRun` Snapshot/Command/Resolution 不变；场景声明只选择静态 Floor 数据，Actor 不决定合法性。
- **Data, GameplayTags, and Authoring Stay Explicit**: PASS。Authoring/Debug 资产路径、身份、所有权和 validator 均显式记录。
- **Reusable Systems Over One-Off Work**: PASS。Descriptor 和 validator 服务所有 Run Floor；Debug builder 只维护确定性夹具。一次性资产迁移脚本在同切片执行后删除，不成为长期入口。
- **Validation Is Part of the Slice**: PASS。包含编译、定向自动化、builder 幂等、hash guard、Blueprint 编译、命令行验证与 PIE 黄金路径。

## Phase 0: Research

研究结论记录于 [research.md](./research.md)：

1. 正式人工场景与可生成 Debug 场景必须拆为不同 `.umap`，不能继续通过 generated tag 共存。
2. Scene Descriptor 采用 World → Floor DataAsset 单向引用；Floor 不引用 World，也不复制逻辑图到 Actor。
3. 当前图迁入独立 `Authoring` 身份，避免误用 Debug 身份或提前承诺正式 Floor 1 稳定身份。
4. Debug builder 只拥有 Debug map/data/GameMode，现有共享 Blueprint 变为只读依赖。
5. Validator 统一运行时可比对的身份合同和编辑器几何合同，并保持严格只读。
6. 旧 `WacomBuildRunExplorationAssets` 命令被明确改名为 Debug-only 命令，不保留容易误用的兼容 wrapper。

## Phase 1: Design Artifacts

- [data-model.md](./data-model.md)：Scene Descriptor、Authoring baseline、Debug fixture、binding diagnostic 与身份不变量。
- [contracts/runtime-scene-binding.md](./contracts/runtime-scene-binding.md)：运行时 descriptor 解析与原子 binding refresh。
- [contracts/editor-scene-validation.md](./contracts/editor-scene-validation.md)：只读验证、诊断码、阈值、菜单与命令行退出码。
- [contracts/debug-asset-builder.md](./contracts/debug-asset-builder.md)：Debug builder 资产所有权、幂等和禁止写集合。
- [quickstart.md](./quickstart.md)：迁移顺序、编译/测试/命令行/PIE 验证。

### Post-design Constitution Re-check

PASS。设计没有引入规则反向依赖、Level Blueprint 规则、Actor 图副本、字符串 GameplayTag、Tick 扫描或新的 SaveGame 身份承诺。唯一临时元素是可审阅的一次性迁移脚本，任务要求在资产生成与验证后删除。

## Project Structure

### Documentation (this feature)

```text
specs/006-run-level-authoring-baseline/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── checklists/requirements.md
├── contracts/
│   ├── runtime-scene-binding.md
│   ├── editor-scene-validation.md
│   └── debug-asset-builder.md
└── tasks.md
```

### Expected Runtime and Editor Source Changes

```text
Source/WacomApp/
├── Public/Actors/WacomRunFloorSceneDescriptorActor.h                 # 可放置的唯一 Floor 声明
├── Private/Actors/WacomRunFloorSceneDescriptorActor.cpp
├── Private/GameFramework/WacomRunFloorSceneDescriptorResolver.h/.cpp # App-private 唯一解析
├── Private/GameFramework/WacomRunSceneBindingRegistry.h/.cpp         # working registry 原子提交支持
└── Private/GameFramework/WacomPlayerController.cpp                   # descriptor-first refresh

Source/WacomEditor/
├── Public/Validation/WacomRunSceneBindingValidation.h
├── Private/Validation/WacomRunSceneBindingValidation.cpp
├── Private/Commandlets/WacomValidateRunFloorSceneCommandlet.h/.cpp
├── Private/Commandlets/WacomBuildRunExplorationDebugAssetsCommandlet.h/.cpp
├── Private/ContentBuilders/RunExplorationDebugAssetBuilder.h/.cpp
└── Private/WacomEditorModule.cpp                                     # ToolMenus 入口和对称注销

Source/WacomTests/Private/
├── UI/RunFloorSceneDescriptorSpec.cpp
├── UI/RunSceneBindingValidationSpec.cpp
└── Editor/RunExplorationDebugAssetBuilderSpec.cpp
```

旧 `Source/WacomEditor/Private/Commandlets/WacomBuildRunExplorationAssetsCommandlet.{h,cpp}` 删除，不保留 wrapper。

### Expected Content Changes

```text
Content/Wacom/Maps/L_Exploration.umap
Content/Wacom/Maps/Debug/L_RunExploration_Debug.umap
Content/Wacom/Data/Map/Authoring/DA_Journey_LevelAuthoring.uasset
Content/Wacom/Data/Map/Authoring/DA_Floor_LevelAuthoring_01.uasset
Content/Wacom/Debug/GameModes/GM_WacomRunDebug.uasset
Content/Wacom/Core/GameModes/GM_Wacom.uasset
```

`BP_WacomPlayerCharacter` 与 `Content/Wacom/Run/Path/Blueprints/BP_WacomRun*` 不属于 builder 写集合；只允许在验证中读取。

**Structure Decision**: Scene Descriptor 位于 WacomApp，因为它是 World 与领域数据之间的运行时接入层；Floor 图仍由 WacomData 定义。通用验证入口公开在 WacomEditor 的现有 validation namespace，具体菜单/commandlet/builder 保持 Private。测试使用独立小型 spec，不扩张既有巨型测试文件。

## Migration Sequence

1. 先建立 descriptor/runtime resolver 与 descriptor-aware validator，并用瞬态 World 测试其失败矩阵。
2. 创建独立 Authoring Journey/Floor，复制当前已验证逻辑图但赋予 `Journey.Authoring` / `Floor.Authoring.01` 身份。
3. 在任何剥离前把当前 `L_Exploration` 复制为 Debug map；为两张地图放置且只放置一个 Descriptor。
4. Debug map 保留生成 ownership；正式 `L_Exploration` 定向移除 Run Node/Path/Branch 的 Debug generated tag，保留 GUID、Transform、Spline、host 与 art。
5. `GM_Wacom` 指向 Authoring Journey；专用 Debug GameMode 指向 Debug Journey；Debug map 使用 Debug GameMode。
6. 收敛 builder 目标与只读依赖，改名 commandlet；连续运行两次并执行禁止写集合哈希守卫。
7. 对两张地图运行 validator/commandlet，再完成编译、回归、Blueprint 编译和 PIE。

## Validation Plan

**Workspace Guard**:

- 实现前记录 `git status`，保护当前与本功能无关的 DreamMaterials MI 改动。
- 资产迁移或命令行加载前关闭占用相关 Package 的编辑器实例。

**Compile**:

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

**Focused Automation**:

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.UI.RunSceneBinding; Automation RunTests Wacom.Editor.RunSceneValidation; Automation RunTests Wacom.Editor.RunExplorationDebugAssets; Automation RunTests Wacom.UI.RunPathTraversal; Automation RunTests Wacom.Run.Map; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

再运行受影响的 `Wacom.UI.Battle`、`Wacom.UI.Shop`、`Wacom.UI.Event` 返回路径测试；最后与完整 `Wacom` 基线比较。

**Commandlet Validation**:

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomValidateRunFloorScene -Map=/Game/Wacom/Maps/L_Exploration -Unattended -NoPause -NoSplash -NullRHI

& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomValidateRunFloorScene -Map=/Game/Wacom/Maps/Debug/L_RunExploration_Debug -Unattended -NoPause -NoSplash -NullRHI
```

**Asset Integrity**:

- Debug builder 连续运行两次。
- 运行前后计算正式 map、Authoring DataAssets、`GM_Wacom`、玩家 BP 和共享 Run Path BP 的 SHA-256；builder 后必须全部一致。
- 使用 AssetRegistry 检查两张 map 的 descriptor、GameMode、Floor/Journey 引用。
- 确认新 `.umap/.uasset` 通过项目 Git LFS 规则跟踪。

**Manual/Editor Validation**:

- `Tools -> Wacom -> Validate Current Run Floor` 对正式/Debug 地图均通过且不产生 dirty Package。
- Blueprint 全量编译 0 error / 0 failed load。
- PIE 黄金路径：进入 Run 后无需点击可看向/移动；单出口自动前进，多出口选择；地图同层传送；进入并退出 Battle、Shop、RunEvent 后 HUD、镜头、手牌和路径移动恢复。

## Complexity Tracking

无 Constitution 违规，无需例外。
