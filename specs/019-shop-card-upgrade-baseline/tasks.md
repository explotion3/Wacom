# Tasks: 商店卡牌强化规则与制作合同基线

**Input**: `specs/019-shop-card-upgrade-baseline/` 下的 spec、plan、research、data-model、contracts 与 quickstart
**Authority**: `AGENTS.md`、长期 Docs 和 live 源码优先
**Delivery gate**: 用户审阅前不 stage、不 commit

## Phase 1：启动审计与 Spec 工件

- [x] T001 确认 main/base、目标复用 worktree、branch、Git/LFS 与 Unreal 进程状态
- [x] T002 从 `main@bc209e7e` 在复用 worktree 创建 `codex/shop-card-upgrade-baseline`
- [x] T003 [US1] 创建 `spec.md`、`plan.md`、`research.md`、`data-model.md`、contracts 与 `quickstart.md`
- [x] T004 [US1] 创建 requirements/content-contract checklists 并完成质量审阅
- [x] T005 [US1] 更新 `.specify/feature.json` 与 `AGENTS.md` 托管指针
- [x] T006 [US1] 运行 Spec Kit prerequisites 与跨工件一致性分析，修正阻断问题

## Phase 2：Card/Shop 静态合同与制作校验

**Checkpoint**: 默认 Unity 编译；Card、Shop、CardUpgrade validation tests。

- [x] T007 [P] [US1] 在 `Source/WacomTests/Private/Data/CardUpgradeValidationSpec.cpp` 先写合法链与非法链测试
- [x] T008 [P] [US1] 扩展 `Source/WacomTests/Private/Run/ShopDefinitionValidationSpec.cpp` 覆盖强化价格表
- [x] T009 [US1] 在 `Source/WacomData/Public/Cards/CardDefinition.h` 与 `Private/Cards/CardDefinition.cpp` 添加强化字段和身份 helper
- [x] T010 [US1] 在 `Source/WacomData/Public/Shops/ShopDefinition.h` 添加可选强化服务价格 schema
- [x] T011 [US1] 扩展 `Source/WacomEditor/Private/Validation/CardDefinitionValidation.cpp` 的直接链/结构校验
- [x] T012 [US1] 新增 `Source/WacomEditor/{Public,Private}/Validation/CardUpgradeCatalogValidation.*` 的 catalog 唯一性/合流/循环校验
- [x] T013 [US1] 扩展 `Source/WacomEditor/Private/Validation/ShopDefinitionValidation.cpp` 的服务价格校验
- [x] T014 编译默认 Unity `WacomEditor Win64 Development`
- [x] T015 运行 `Wacom.Data.Card`、`Wacom.Data.Shop`、`Wacom.Data.CardUpgrade`
- [x] T016 将 Checkpoint 1 编译/测试/风险写入 `quickstart.md`

## Phase 3：Run 原子强化与商店 AP 共用事务

**Checkpoint**: 新 CardUpgrade tests、现有 Shop AP/notification/revision 回归。

- [x] T017 [US2] 在 `Source/WacomTests/Private/Run/ShopCardUpgradeSpec.cpp` 先写精确实例、四区域、连续强化和失败回滚测试
- [x] T018 [US2] 在 `Source/WacomTests/Private/Run/ShopCardUpgradeSpec.cpp` 增加 purchase↔upgrade AP 矩阵与阶段关闭测试
- [x] T019 [US2] 在 `Source/WacomRun/Public/RunState.h` 添加 visit request、service state、Quote/Command/Result
- [x] T020 [US2] 扩展 `Source/WacomRun/Private/Shops/RunShopTransaction.*` 构建 Quote、权威校验和精确实例 Definition 替换
- [x] T021 [US2] 在 `Source/WacomRun/Public/RunSession.h` / `Private/RunSession.cpp` 添加 canonical visit/upgrade API 与共享 commerce settlement
- [x] T022 [US2] 重构现有 `PurchaseShopOffer` 使用共享 settlement 并保持字段/行为兼容
- [x] T023 [US2] 在 `Source/WacomApp/Public/Actors/WacomShopTriggerActor.h` / `.cpp` 构建 canonical visit request
- [x] T024 [US2] 扩展 `WacomPlayerController` 与 `WacomExplorationScreenRouter` overload 传递 visit request，保留旧 Offers wrapper
- [x] T025 编译默认 Unity `WacomEditor Win64 Development`
- [x] T026 运行 `Wacom.Run.Shop.CardUpgrade`、`Wacom.Run.NodeActivity.Shop`、`Wacom.Run.NotificationCoalescing`、`Wacom.Run.SnapshotRevision`
- [x] T027 运行 `Wacom.UI.Shop` 路由/现有 purchase-only screen 回归
- [x] T028 将 Checkpoint 2 编译/测试/风险写入 `quickstart.md`

## Phase 4：身份、Battle/Save 与回收价值兼容

**Checkpoint**: family identity、Save/Battle projection、Run/App recycle value parity。

- [x] T029 [US3] 在 `Source/WacomTests/Private/Run/CardUpgradeCompatibilitySpec.cpp` 先写 family/exact matching 与 Battle/Save tests
- [x] T030 [US3] 将 Run workspace 和 world interaction 的 AllowedCardIds 改用 CardDefinition family helper
- [x] T031 [US3] 将 `RunOwnedCardRequirementEvaluator.cpp` 与 `RunEventExecutor.cpp` 的 AllowedCardIds 改用 family helper
- [x] T032 [US3] 扩展 `FRunDeckRules::GetDeleteGoldRewardForCard` 为 `1/2/3/4/0`
- [x] T033 [US3] 将 `WacomCardFaceViewDataBuilder.cpp` 改为调用 `URunSession::GetDeleteGoldRewardForCard`
- [x] T034 [US3] 补充 Save v5 roundtrip、Battle init 和五档 App presentation parity 测试
- [x] T035 编译默认 Unity `WacomEditor Win64 Development`
- [x] T036 运行 Save、Backpack/workspace、Event、WorldInteraction、Battle init/rule matrix 与 Card presentation 定向测试
- [x] T037 将 Checkpoint 3 编译/测试/风险写入 `quickstart.md`

## Phase 5：长期文档与最终门禁

- [x] T038 [US3] 同步 WacomData、DataAuthoring、Run、Battle、App 与 Architecture
- [x] T039 [US3] 同步 TODO、Questions、Roadmap 与 TechDebt，保留 Spec 020 UI/content follow-up
- [x] T040 关闭 Editor/Live Coding 后执行最终默认 Unity WacomEditor 编译
- [x] T041 运行全部受影响 focused Automation 前缀
- [x] T042 对现有 CardDefinition/ShopDefinition 执行只读 AssetRegistry、failed-load 和前后 SHA-256 审计
- [x] T043 运行 Spec Kit 最终一致性分析、identity/reference `rg`、`git diff --check` 与范围审计
- [x] T044 运行 `git lfs fsck`、Git/LFS 状态检查并确认无二进制变化
- [x] T045 在 `quickstart.md` 汇总 base、编译/测试、资产哈希、跳过项和风险
- [x] T046 向用户提供一次性审阅包，保持 unstaged/uncommitted

## Phase 6：仅在用户确认后提交与交接

- [ ] T047 提交 `feat(data): define card upgrade chains`
- [ ] T048 提交 `feat(run): add atomic shop card upgrades`
- [ ] T049 确认 worktree/LFS 干净并按 AgentIntegrationWorkflow 输出交接；不 merge、不 push

## Dependencies

- Phase 1 → Phase 2 → Phase 3 → Phase 4 → Phase 5 严格串行。
- 每个 C++ phase 先测试合同，再实现，再编译/聚焦测试。
- Phase 3 依赖 Phase 2 的 Data helper 与 Shop schema。
- Phase 4 依赖 Phase 3 的实例 Definition 替换。
- Phase 6 必须等待用户明确确认。

## Parallel execution

- 仅 T007/T008 可在文件不重叠时并行；本任务按用户既定口径由主会话串行实施，不使用 subagent。
