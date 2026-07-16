# Research: Run 正式关卡制作基线收口

## Decision 1 — 正式场景与 Debug 生成场景物理分离

**Decision**: `L_Exploration` 只由关卡制作人员维护；调试构建改为只写 `/Game/Wacom/Maps/Debug/L_RunExploration_Debug`。

**Rationale**: 当前 builder 会销毁带 generated tag 的 Actor、重建 Spline 并保存 `L_Exploration`。继续共用同一地图意味着任何人工路径曲线、Actor GUID、宿主布局或美术都可能被覆盖。

**Alternatives rejected**:

- 继续共用地图，只增加确认框：仍然让危险写入口存在，自动化也无法安全运行。
- 只去掉 generated tag：builder 仍保存正式 map、GameMode 和共享 BP，所有权没有真正隔离。
- 把正式关卡也完全数据生成：不适合需要大量手工镜头、路径曲线、HD-2D 构图和交互宿主的项目。

## Decision 2 — World 通过唯一 Scene Descriptor 引用 Floor

**Decision**: 每个独立 Run Floor World 放置且只放置一个 `AWacomRunFloorSceneDescriptorActor`，由它只读引用 `UWacomFloorMapDefinition`。

**Rationale**: World 需要声明自己实现哪一份逻辑图；这比从 Anchor/Path Actor 猜 Floor 更可靠，也避免 DataAsset 反向依赖 World 或出现第二份图真相。

**Alternatives rejected**:

- Floor DataAsset 保存 Soft World 引用：让底层静态规则数据反向知道表现资产，增加加载和循环依赖风险。
- GameMode 按地图名硬编码 Floor：无法支持多个关卡、测试 World 或制作期 validation。
- 每个 Actor 重复 FloorId：信息冗余且不能证明整个 World 只有一个 Floor 所有者。

## Decision 3 — 当前图迁入 Authoring 身份，不宣布为正式 Floor 1

**Decision**: 新建 `Journey.Authoring` / `Floor.Authoring.01`，复制当前已验证图作为过渡制作基线；正式 Floor 1 图、稳定 NodeId 和内容配比后续单独设计。

**Rationale**: 用户已确认本切片只收口制作基线。继续让正式地图引用 `Debug` 身份会污染语义；直接命名 `Floor.01` 又会过早冻结尚未设计的存档身份。

**Alternatives rejected**:

- 继续使用 `DA_Floor_Debug_01`：正式地图与 Debug builder 仍共享数据写集合。
- 立即定义正式 Floor 1：扩大到内容设计、节点密度、门槛和存档身份，超出本切片。
- 让正式地图暂时没有 Floor：运行时无法建立可靠绑定，也无法进行制作验证。

## Decision 4 — Debug builder 只拥有 Debug namespace

**Decision**: builder 可创建/更新 Debug Journey、Floor、GameMode、map 内生成 Actor；共享 Run Actor BP、玩家 BP 和正式 `GM_Wacom` 只能读取并校验。

**Rationale**: 当前 builder 的“顺手修复”会重新编译和保存共享资产，使调试工具成为不可预测的全局资产写入口。

**Alternatives rejected**:

- 保留现有 Ensure Blueprint 行为：无法通过哈希守卫，人工调参仍可能被覆盖。
- 为 Debug 复制全部 Runtime BP：制造两套运行时表现合同，回归价值下降。

## Decision 5 — Runtime binding 使用 working state 原子提交

**Decision**: PlayerController 先解析 descriptor、验证 Floor/Snapshot，再在临时 registry 中枚举和注册 Actor；只有全部成功才替换当前 registry/coordinator 绑定。

**Rationale**: 现有先 Reset 再注册的顺序会在中途失败时留下半绑定状态。规则状态不应因场景制作错误而被部分表现替换。

**Alternatives rejected**:

- 失败后再调用一次全量刷新：错误场景不会因重试变正确，且会产生相机/HUD 抖动。
- 跳过坏 Actor 继续：可能直到 traversal 或 activity 返回时才暴露不可恢复状态。

## Decision 6 — Validator 是唯一只读诊断合同

**Decision**: 编辑器菜单、commandlet、builder post-check 和自动化共用 descriptor-aware `FWacomRunSceneBindingValidation`，诊断使用稳定 Severity/Code/ObjectPath/Message。

**Rationale**: 当前 validator 由调用方额外传 Floor，无法验证 World 自声明；只有 Errors/Warnings 文本也不利于稳定自动化和命令行结果。

**Geometry thresholds**:

- Spline 至少 2 点，长度必须大于 `10 cm`。
- 所有 Spline point transform 必须为有限值。
- 端点至期望 Anchor：`<=100 cm` 通过，`>100 cm && <=300 cm` 警告，`>300 cm` 错误。
- 若反向端点总距离比正向总距离至少小 `1 cm`，报告方向颠倒错误。

**Alternatives rejected**:

- Validator 自动修复：破坏只读保证并可能覆盖关卡制作选择。
- 菜单和 commandlet 分别实现规则：诊断会漂移，测试不能代表编辑器行为。

## Decision 7 — 命令名称必须表达 Debug-only 语义

**Decision**: 删除 `WacomBuildRunExplorationAssets`，新增 `WacomBuildRunExplorationDebugAssets`，不保留 deprecated wrapper。

**Rationale**: 旧名称暗示它可以构建正式 Exploration；在正式关卡人工化后继续保留会提高误操作风险。

## Decision 8 — 不扩展 SaveGame 与模块边界

**Decision**: 本切片不修改 WacomRun 规则类型、SaveGame schema、GameplayTag 或 Build.cs。

**Rationale**: 这是关卡制作和绑定所有权收口，不是旅程内容与持久化切片。Authoring 身份在正式存档启用前可替换。

## Live implementation audit — 2026-07-16

实施前复核确认规划方向正确，但 live 源码有以下必须先收口的危险写入口与测试差异：

- `RunExplorationDebugAssetBuilder.cpp` 当前仍会创建/覆盖 Debug Journey/Floor，同时编译并保存共享 Run Path Blueprint、修改并保存正式 `GM_Wacom` 与 `BP_WacomPlayerCharacter`，销毁 `L_Exploration` 中带 `Wacom.Generated.RunExploration` tag 的 Actor、重建 Anchor/Path/Branch、修改 content host binding 并保存正式地图。
- `AWacomPlayerController::RefreshRunExplorationPresentationBinding()` 当前第一步调用 teardown，随后直接把 Actor 注册到已安装 registry；重复/无效注册只记 warning 并继续，初始化失败还会停用当前 Traversal。因此它不满足 descriptor-first 或失败保留旧绑定的原子合同。
- `FWacomRunSceneBindingValidation::ValidateLoadedWorld()` 当前由调用方传入任意 Floor，只有 `Errors/Warnings` 文本数组，不校验 World 自声明，也不覆盖 Spline 几何。
- live builder 自动化名称是 `Wacom.Editor.RunExploration.DebugAssetBuilder.IdempotentAndStable`，而不是规划中的 `Wacom.Editor.RunExplorationDebugAssets`；更重要的是该测试会实际调用上述危险 builder，不能在 Phase 1 作为只读基线执行。Phase 1 只记录跳过原因；待 Phase 5 写集合收敛和禁止写守卫落地后再运行重命名后的测试。
- Spec Kit prerequisite script 要求数字前缀 branch，无法接受用户指定的 `codex/run-level-authoring-baseline`。本功能保留指定 branch，并以 `specs/006-run-level-authoring-baseline`、完整 checklist 和手动 prerequisites 审计替代，不创建或切换额外 branch。
