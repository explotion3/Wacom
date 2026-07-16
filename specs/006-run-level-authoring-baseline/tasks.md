---
description: "Run 正式关卡制作基线收口的依赖有序实施任务"
---

# Tasks: Run 正式关卡制作基线收口

**Input**: Design documents from `/specs/006-run-level-authoring-baseline/`

**Prerequisites**: `plan.md`、`spec.md`、`research.md`、`data-model.md`、`contracts/`、`quickstart.md`

**Tests**: Descriptor、原子绑定、只读 validator、commandlet 退出码、Debug builder 幂等和禁止写集合均要求自动化；地图美术与完整玩家路径由 PIE 验收。

**Organization**: 先完成所有故事共用的 Descriptor 与原子绑定基础，再依次交付正式场景、制作验证和 Debug 夹具。每个故事结束均有独立 checkpoint。

## Format: `[ID] [P?] [Story] Description`

- **[P]**: 文件范围独立且不依赖同阶段未完成任务，可并行执行
- **[Story]**: `US1` 正式场景、`US2` 制作验证、`US3` Debug 夹具
- 每项均包含明确文件或资产路径

## Phase 1: Setup、工作区保护与基线取证

**Purpose**: 在任何源码或资产迁移前冻结 live workspace、危险写入口和回归基线。

- [ ] T001 在 `specs/006-run-level-authoring-baseline/quickstart.md` 记录 `git status --short --branch`，明确保护当前无关的 `Content/DreamMaterials/World/MI_WacomBattleEnemyPartImpactPixel_Default.uasset` 改动且不纳入本切片
- [ ] T002 在 `specs/006-run-level-authoring-baseline/quickstart.md` 记录相关 Unreal Editor 实例已关闭，并列出 `L_Exploration`、Authoring/Debug DataAsset 与 GameMode 的资产锁风险
- [ ] T003 在 `specs/006-run-level-authoring-baseline/research.md` 复核 `Source/WacomEditor/Private/ContentBuilders/RunExplorationDebugAssetBuilder.cpp`、`Source/WacomApp/Private/GameFramework/WacomPlayerController.cpp` 和 `Source/WacomEditor/Private/Validation/WacomRunSceneBindingValidation.cpp` 的 live 调用/写集合，若与计划不同先修订工件
- [ ] T004 [P] 在 `specs/006-run-level-authoring-baseline/quickstart.md` 记录 `Content/Wacom/Maps/L_Exploration.umap`、`Content/Wacom/Core/GameModes/GM_Wacom.uasset`、`Content/Wacom/Core/Player/BP_WacomPlayerCharacter.uasset` 和 `Content/Wacom/Run/Path/Blueprints/BP_WacomRun*.uasset` 的初始 SHA-256
- [ ] T005 [P] 编译 `D:/UE_Project/5.7/Wacom/Wacom.uproject` 的 `WacomEditor` 并运行当前 `Wacom.UI.RunSceneBinding`、`Wacom.Editor.RunExplorationDebugAssets` 基线，将结果写入 `specs/006-run-level-authoring-baseline/quickstart.md`

---

## Phase 2: Foundational — Floor 声明与原子运行时绑定

**Purpose**: 建立所有正式/Debug Run Floor 共用的唯一场景声明和 descriptor-first working registry；在此完成前不得迁移地图资产。

**CRITICAL**: Descriptor 失败矩阵和 working registry 原子性必须先有自动化，再触碰 `L_Exploration`。

- [ ] T006 [P] 在 `Source/WacomTests/Private/UI/RunFloorSceneDescriptorSpec.cpp` 先覆盖无 Descriptor、重复 Descriptor、空 Floor、空 FloorId、预期 FloorId 不匹配和唯一合法 Descriptor
- [ ] T007 在 `Source/WacomApp/Public/Actors/WacomRunFloorSceneDescriptorActor.h` 与 `Source/WacomApp/Private/Actors/WacomRunFloorSceneDescriptorActor.cpp` 实现无 Tick、无碰撞、HiddenInGame、只读 FloorDefinition 的场景声明 Actor
- [ ] T008 在 `Source/WacomApp/Private/GameFramework/WacomRunFloorSceneDescriptorResolver.h` 与 `Source/WacomApp/Private/GameFramework/WacomRunFloorSceneDescriptorResolver.cpp` 实现 World 枚举、唯一性、空引用和 expected FloorId 的稳定只读解析
- [ ] T009 [P] 在 `Source/WacomApp/Public/Testing/WacomRunFloorSceneBindingAutomationTestView.h`、`Source/WacomApp/Private/Testing/WacomRunFloorSceneBindingAutomationTestView.cpp` 与 `Source/WacomTests/Private/UI/WacomRunFloorSceneBindingTestAccess.h/.cpp` 建立非反射测试观察 seam，禁止新增 Blueprint 测试 API 或散落 ForTest getter
- [ ] T010 在 `Source/WacomTests/Private/UI/RunSceneBindingValidationSpec.cpp` 先覆盖 working registry 注册中途失败、Snapshot 版本漂移和 Floor 漂移时不替换已安装绑定
- [ ] T011 在 `Source/WacomApp/Private/GameFramework/WacomRunSceneBindingRegistry.h` 与 `Source/WacomApp/Private/GameFramework/WacomRunSceneBindingRegistry.cpp` 增加完整性检查和可一次性安装的 working-state 支持，不在失败路径 Reset 当前 registry
- [ ] T012 在 `Source/WacomApp/Private/GameFramework/WacomPlayerController.cpp` 将 `RefreshRunExplorationPresentationBinding` 改为 Snapshot→Descriptor→working registry→版本复验→一次提交的固定顺序
- [ ] T013 在 `Source/WacomApp/Public/GameFramework/WacomPlayerController.h` 仅补充必要前向声明/私有 helper，确认不公开 Descriptor setter、规则写 API 或额外 Blueprint 反射
- [ ] T014 编译 `D:/UE_Project/5.7/Wacom/Wacom.uproject` 的 `WacomEditor` 并运行 `Wacom.UI.RunSceneBinding`，把 Descriptor 失败矩阵和原子绑定结果写入 `specs/006-run-level-authoring-baseline/quickstart.md`

**Checkpoint**: 瞬态/测试 World 已能通过唯一 Descriptor 建立完整 binding；所有失败均无部分 registry、镜头、HUD 或规则副作用。

---

## Phase 3: User Story 1 — 安全制作正式探索关卡 (Priority: P1) MVP

**Goal**: `L_Exploration` 脱离 Debug builder 所有权，关联独立 Authoring Floor，且当前可玩布局和 Actor 身份完整保留。

**Independent Test**: `Wacom.UI.RunSceneBinding.AuthoringBaseline` + AssetRegistry 合同 + 迁移前后 GUID/Transform/Spline 审计。

### Tests and Validation for User Story 1

- [ ] T015 [P] [US1] 在 `Source/WacomTests/Private/Editor/RunLevelAuthoringBaselineAssetContractSpec.cpp` 先覆盖 `L_Exploration` 唯一 Descriptor、Authoring Floor 引用、`GM_Wacom` Journey 引用、无 Debug generated ownership 和必要 Actor 数量/身份
- [ ] T016 [P] [US1] 在 `Source/WacomTests/Private/UI/RunSceneBindingValidationSpec.cpp` 增加 `Floor.Authoring.01` 与 runtime Snapshot 匹配时成功、错误 Floor 时原子拒绝的回归

### Implementation for User Story 1

- [ ] T017 [US1] 使用 `Scripts/MigrateRunLevelAuthoringBaseline.py` 编写一次性、显式资产迁移：先复制 `Content/Wacom/Maps/L_Exploration.umap` 到 `Content/Wacom/Maps/Debug/L_RunExploration_Debug.umap`，再创建 Authoring Journey/Floor 并定向更新两张地图 Descriptor/GameMode
- [ ] T018 [US1] 运行 `Scripts/MigrateRunLevelAuthoringBaseline.py` 生成 `Content/Wacom/Data/Map/Authoring/DA_Journey_LevelAuthoring.uasset`、`DA_Floor_LevelAuthoring_01.uasset`、`Content/Wacom/Maps/Debug/L_RunExploration_Debug.umap` 和 `Content/Wacom/Debug/GameModes/GM_WacomRunDebug.uasset`
- [ ] T019 [US1] 在 `Content/Wacom/Maps/L_Exploration.umap` 定向移除 Run Node/Path/Branch 的 Debug generated ownership，保留 Actor GUID、Transform、Spline points、activity hosts 和所有场景美术
- [ ] T020 [US1] 在 `Content/Wacom/Core/GameModes/GM_Wacom.uasset` 将默认 Journey 指向 `DA_Journey_LevelAuthoring`，并确认 `L_Exploration` Descriptor 指向 `DA_Floor_LevelAuthoring_01`
- [ ] T021 [US1] 删除已执行的 `Scripts/MigrateRunLevelAuthoringBaseline.py`，确保仓库不保留可再次覆盖正式关卡的一次性迁移入口
- [ ] T022 [US1] 使用 AssetRegistry 与 `Source/WacomTests/Private/Editor/RunLevelAuthoringBaselineAssetContractSpec.cpp` 验证正式地图、Authoring DataAssets、GM 引用、Actor GUID/Transform/Spline 保留和新资产 Git LFS 跟踪
- [ ] T023 [US1] 编译 `D:/UE_Project/5.7/Wacom/Wacom.uproject` 的 `WacomEditor` 并运行 `Wacom.UI.RunSceneBinding.AuthoringBaseline`、`Wacom.UI.RunPathTraversal`、`Wacom.Run.Map`，将结果写入 `specs/006-run-level-authoring-baseline/quickstart.md`

**Checkpoint**: 正式关卡可以独立加载和绑定 Authoring Floor，当前路径仍可玩，且不存在 Debug builder 所有权标记。

---

## Phase 4: User Story 2 — 在进入 PIE 前验证当前 Run Floor (Priority: P2)

**Goal**: 制作人员、自动化和命令行共用一套严格只读、可定位、descriptor-aware 的 Run Floor 场景验证。

**Independent Test**: `Wacom.Editor.RunSceneValidation` 覆盖诊断码矩阵、几何阈值、稳定排序和 dirty-package 不变量；commandlet 覆盖 0/1/2 退出结果。

### Tests and Validation for User Story 2

- [ ] T024 [P] [US2] 在 `Source/WacomTests/Private/Editor/RunFloorSceneValidationSpec.cpp` 先覆盖 Descriptor 缺失/重复/空引用、Anchor/Path/Branch/host 缺失/重复/意外身份和稳定诊断排序
- [ ] T025 [P] [US2] 在 `Source/WacomTests/Private/Editor/RunFloorSceneValidationGeometrySpec.cpp` 先覆盖 Spline 少于 2 点、长度不超过 10cm、非有限 Transform、方向颠倒以及 100cm/300cm 端点阈值边界
- [ ] T026 [US2] 在 `Source/WacomTests/Private/Editor/RunFloorSceneValidationReadOnlySpec.cpp` 先覆盖有效与无效 World 验证前后 Actor 状态、DataAsset 和 Package dirty flags 均不改变
- [ ] T027 [P] [US2] 在 `Source/WacomTests/Private/Editor/RunFloorSceneValidationCommandletSpec.cpp` 先覆盖有效地图返回 0、合同错误返回 1、缺参数/加载或 Descriptor 解析失败返回 2

### Implementation for User Story 2

- [ ] T028 [US2] 在 `Source/WacomEditor/Public/Validation/WacomRunSceneBindingValidation.h` 将文本数组收敛为 Severity/Code/ObjectPath/Message 的稳定诊断结构，并保留 `IsValid/HasErrors` 只读 helper
- [ ] T029 [US2] 在 `Source/WacomEditor/Private/Validation/WacomRunSceneBindingValidation.cpp` 改为只接收 World 并内部解析唯一 Descriptor，统一实现节点、道路、分支、host 与 Spline 几何验证且禁止任何修复/保存
- [ ] T030 [US2] 在 `Source/WacomEditor/Private/Commandlets/WacomValidateRunFloorSceneCommandlet.h` 与 `Source/WacomEditor/Private/Commandlets/WacomValidateRunFloorSceneCommandlet.cpp` 实现 `-Map` 加载、排序输出和稳定 0/1/2 退出结果
- [ ] T031 [US2] 在 `Source/WacomEditor/Public/WacomEditorModule.h` 与 `Source/WacomEditor/Private/WacomEditorModule.cpp` 对称注册/注销 `Tools -> Wacom -> Validate Current Run Floor` 并复用同一 validator
- [ ] T032 [US2] 迁移 `Source/WacomEditor/Private/ContentBuilders/RunExplorationDebugAssetBuilder.cpp` 和 `Source/WacomTests/Private/UI/RunSceneBindingValidationSpec.cpp` 的旧 validator 调用，删除由调用方传入任意 Floor 的旁路
- [ ] T033 [US2] 对 `Content/Wacom/Maps/L_Exploration.umap` 与 `Content/Wacom/Maps/Debug/L_RunExploration_Debug.umap` 运行 editor menu 和 `WacomValidateRunFloorScene`，确认退出 0、诊断可定位且所有 Package 保持 clean
- [ ] T034 [US2] 编译 `D:/UE_Project/5.7/Wacom/Wacom.uproject` 的 `WacomEditor` 并运行 `Wacom.Editor.RunSceneValidation`、`Wacom.UI.RunSceneBinding`，将结果写入 `specs/006-run-level-authoring-baseline/quickstart.md`

**Checkpoint**: 两张正式/Debug Run map 均可通过相同只读入口验证；所有故障样例有稳定诊断和退出结果。

---

## Phase 5: User Story 3 — 独立重建 Run 调试夹具 (Priority: P3)

**Goal**: Debug builder 只写 Debug map/data/GameMode，可幂等运行，并以 hash guard 证明不触碰正式或共享资产。

**Independent Test**: 连续运行 `WacomBuildRunExplorationDebugAssets` 两次；`Wacom.Editor.RunExplorationDebugAssets` 验证确定性和禁止写集合 SHA-256 全部不变。

### Tests and Validation for User Story 3

- [ ] T035 [P] [US3] 在 `Source/WacomTests/Private/Editor/RunExplorationDebugAssetBuilderSpec.cpp` 先把期望目标改为 Debug map/Debug GameMode，并覆盖唯一 Descriptor、graph/actor counts 和连续两次构建幂等
- [ ] T036 [US3] 在 `Source/WacomTests/Private/Editor/RunExplorationDebugAssetBuilderSpec.cpp` 增加正式 map、Authoring DataAssets、`GM_Wacom`、玩家 BP 和共享 Run Path BP 的禁止写集合 SHA-256/dirty-state 守卫
- [ ] T037 [P] [US3] 在 `Source/WacomTests/Private/Editor/RunExplorationDebugAssetBuilderDependencySpec.cpp` 先覆盖共享 BP 缺失或父类错误时构建失败且不创建替代资产

### Implementation for User Story 3

- [ ] T038 [US3] 将 `Source/WacomEditor/Private/Commandlets/WacomBuildRunExplorationAssetsCommandlet.h/.cpp` 删除并新增 `Source/WacomEditor/Private/Commandlets/WacomBuildRunExplorationDebugAssetsCommandlet.h/.cpp`，不保留旧命令 wrapper
- [ ] T039 [US3] 在 `Source/WacomEditor/Private/ContentBuilders/RunExplorationDebugAssetBuilder.h` 收敛 result 字段为 Debug data/GameMode/map/validation 状态，移除正式 runtime 配置与 `L_Exploration` migration 语义
- [ ] T040 [US3] 在 `Source/WacomEditor/Private/ContentBuilders/RunExplorationDebugAssetBuilder.cpp` 将写集合限制为 Debug Journey/Floor/GameMode/map，删除对 `GM_Wacom`、玩家 BP 和共享 Run Path BP 的编译、修改与保存
- [ ] T041 [US3] 在 `Source/WacomEditor/Private/ContentBuilders/RunExplorationDebugAssetBuilder.cpp` 将共享 BP 收敛为父类校验后的只读依赖，并在 Debug map 中幂等生成唯一 Descriptor、Anchor、Path、必要 BranchTarget 与 activity host
- [ ] T042 [US3] 连续两次运行 `WacomBuildRunExplorationDebugAssets` 更新 `Content/Wacom/Data/Map/DA_Journey_Debug.uasset`、`DA_Floor_Debug_01.uasset`、`Content/Wacom/Debug/GameModes/GM_WacomRunDebug.uasset` 与 `Content/Wacom/Maps/Debug/L_RunExploration_Debug.umap`
- [ ] T043 [US3] 在 `Source/WacomEditor/Private/ContentBuilders/RunExplorationDebugAssetBuilder.cpp` 构建保存前运行 descriptor-aware validator，任何 Error 均拒绝保存并输出稳定诊断
- [ ] T044 [US3] 复算 `specs/006-run-level-authoring-baseline/quickstart.md` 中禁止写集合 SHA-256，确认两次 builder 后全部与 Phase 3 稳定基线一致并记录 LFS 状态
- [ ] T045 [US3] 编译 `D:/UE_Project/5.7/Wacom/Wacom.uproject` 的 `WacomEditor` 并运行 `Wacom.Editor.RunExplorationDebugAssets`、`Wacom.Editor.RunSceneValidation`、`Wacom.UI.RunSceneBinding`，将结果写入 `specs/006-run-level-authoring-baseline/quickstart.md`

**Checkpoint**: Debug 夹具可被工具完全重建，正式关卡和共享资产在 builder 前后逐文件不变。

---

## Phase 6: Polish、长期文档与完整验证

**Purpose**: 清理旧入口、回写长期事实并确认正式 Run 黄金路径无回归。

- [ ] T046 [P] 更新 `Docs/WacomMap.md`：正式/Authoring/Debug 资产边界、唯一 Scene Descriptor、场景验证合同，并修正“Map Screen 尚未交付”的过期表述
- [ ] T047 [P] 更新 `Docs/WacomApp.md` 与 `Docs/Architecture.md`：descriptor-first working registry、原子提交、WacomData/App/Editor 所有权和无 Build.cs 变化
- [ ] T048 [P] 更新 `Docs/WacomDataAuthoring.md`：Authoring baseline 非正式 Floor 1、Debug builder 写集合、validator menu/commandlet、Spline 阈值和共享 Blueprint 只读合同
- [ ] T049 [P] 更新 `Docs/TODO.md` 与 `Docs/Questions.md`：保留正式 Floor 1 节点图、稳定 NodeId、跨层入口、Camp 和 SaveGame 身份为后续，不重复登记已完成基线
- [ ] T050 使用 `rg` 审计 `WacomBuildRunExplorationAssets` 在 `Source/`、`Config/`、`Scripts/`、`Docs/` 和 `specs/006-run-level-authoring-baseline/` 中除迁移说明外为零运行入口，并把结果写入 `specs/006-run-level-authoring-baseline/quickstart.md`
- [ ] T051 使用 AssetRegistry/引用审计确认 `Content/Wacom/Maps/L_Exploration.umap` 不依赖 Debug Journey/Floor/GameMode，`Content/Wacom/Maps/Debug/L_RunExploration_Debug.umap` 不被正式 `GM_Wacom` 引用，并记录到 `specs/006-run-level-authoring-baseline/quickstart.md`
- [ ] T052 编译 `D:/UE_Project/5.7/Wacom/Wacom.uproject` 的完整 `WacomEditor` Win64 Development 目标并记录结果到 `specs/006-run-level-authoring-baseline/quickstart.md`
- [ ] T053 运行 `Wacom.UI.RunSceneBinding`、`Wacom.Editor.RunSceneValidation`、`Wacom.Editor.RunExplorationDebugAssets`、`Wacom.UI.RunPathTraversal`、`Wacom.Run.Map`、`Wacom.UI.Battle`、`Wacom.UI.Shop` 与 `Wacom.UI.Event`，记录结果到 `specs/006-run-level-authoring-baseline/quickstart.md`
- [ ] T054 运行完整 `Automation RunTests Wacom` 并与实施前已知失败基线比较，任何新增失败均在 `specs/006-run-level-authoring-baseline/quickstart.md` 定位或修复
- [ ] T055 执行 Blueprint 全量编译、两张地图 commandlet validation、Debug builder 二次幂等、Package dirty 审计、Git LFS 检查和 `git diff --check`，记录结果到 `specs/006-run-level-authoring-baseline/quickstart.md`
- [ ] T056 按 `specs/006-run-level-authoring-baseline/quickstart.md` 在 `Content/Wacom/Maps/L_Exploration.umap` 完成首次进入、单/多出口、地图传送、Battle/Shop/RunEvent 往返的 PIE 黄金路径；无法执行时明确记录人工验收风险
- [ ] T057 审核 `specs/006-run-level-authoring-baseline/spec.md`、`plan.md`、`data-model.md`、`contracts/` 与最终 `Docs/` 一致，确认长期事实不只停留在 Spec Kit 工件且当前 8 节点图未被承诺为正式 Floor 1

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 Setup**: 无依赖；先保护 dirty workspace 与资产哈希。
- **Phase 2 Foundational**: 依赖 Phase 1；阻塞任何 `.umap/.uasset` 迁移。
- **US1 正式场景**: 依赖 descriptor/runtime 原子绑定；产出 Authoring baseline 和 main/debug 初始拆分。
- **US2 制作验证**: 依赖两张 map 均有 Descriptor；完成后为 builder 提供唯一 post-check。
- **US3 Debug 夹具**: 依赖 validator 与初始 Debug map；收敛旧危险 builder。
- **Final Validation**: 依赖所有故事完成。

### Critical Order

```text
Descriptor tests
  -> Descriptor/resolver
  -> working registry atomic refresh
  -> Authoring data + main/debug map migration
  -> structured read-only validator
  -> Debug-only builder
  -> docs + regression + PIE
```

### Parallel Opportunities

- T004/T005 可并行记录 hash 与编译基线。
- T006/T009 可并行准备 descriptor spec 与 test seam，但 T009 命名需遵循 T006 断言。
- T015/T016 可并行准备资产合同与 runtime mismatch 测试。
- T024/T025/T027 分属不同小型 spec，可在诊断 code 命名冻结后并行。
- T035/T037 分属 builder 行为与依赖错误 spec，可并行准备。
- T046/T047/T048/T049 修改不同长期文档，可在实现事实稳定后并行。

## Implementation Strategy

### MVP First

1. Phase 1–2 完成并编译，证明坏 Descriptor 不会污染运行时。
2. 完成 US1，使 `L_Exploration` 成为安全人工关卡；此时即可暂停做一次 PIE。
3. 完成 US2，让制作人员在 PIE 前发现绑定/几何问题。
4. 最后完成 US3，恢复独立自动化夹具和可重复 Debug 生成。
5. 完成长期文档与全量回归。

### Wacom Review Checklist

- 正式 map、Authoring 数据、Debug map/data/GameMode 三类所有权明确。
- World → Floor 单向引用；Floor 不引用 World，Actor 不复制图。
- descriptor-first，working registry 一次提交，失败无 UI/规则副作用。
- validator 只读且菜单/commandlet/builder/tests 共用。
- builder 禁止写集合有 hash 与 dirty-state 双重守卫。
- 不修改 Run 规则、SaveGame、GameplayTags、Build.cs 或 first-person 表现语义。
- 正式 Floor 1 内容问题仍在 TODO/Questions，未被过渡资产名称偷偷冻结。

## Notes

- 本任务清单是实施顺序，不表示本轮 Spec 文档阶段已经修改 C++ 或资产。
- `specs/` 若被 `.gitignore` 忽略，后续提交规划工件时需要显式纳入；不能只提交指向缺失 plan 的 `AGENTS.md`。
- 不使用 `$speckit-implement` 一次性执行；主会话逐任务读取 live 文件、编译、验证和收口。
