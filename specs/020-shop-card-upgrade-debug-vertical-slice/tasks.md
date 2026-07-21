# Tasks: Debug Shop 卡牌强化可玩竖切

**Authority**: `AGENTS.md`、长期 Docs、Spec 019 runtime contract 与 live source 优先
**Delivery gate**: 用户完整 PIE 验收前不 stage、不 commit

## Phase 1：基线与 Spec

- [x] T001 确认 Spec 019 已进入 `main@a8d0c356`，编译/89 tests/LFS 门禁通过
- [x] T002 复用 D 盘 worktree 创建 `codex/shop-card-upgrade-debug-vertical-slice`
- [x] T003 [US1] 审计 Shop Screen、Run Quote、UI registry、Debug ShopBuilder 与 MCP workflow
- [x] T004 [US1] 创建 Spec 020 全套工件、contracts 与 checklists
- [x] T005 [US1] 更新 `.specify/feature.json` 与 `AGENTS.md` 托管指针
- [x] T006 [US1] 运行 Spec Kit 跨工件一致性分析并修正阻断问题

## Phase 2：Shop 强化 UI 代码

- [x] T007 [US1] 新增 upgrade presentation ViewData/builder 与差异摘要测试
- [x] T008 [US1] 新增 InstanceId row/reconciler 与金币不足/重复实例测试
- [x] T009 [US2] 扩展 Screen flow，提交 stale guard、映射 Toast、处理 Resolution/visit close
- [x] T010 [US2] 扩展 refresh signature 覆盖 service、quote、gold 和 definitions
- [x] T011 [US1] 将 C++ fallback 改为购买/强化双页签和当前/下一卡面摘要
- [x] T012 [US2] 实现页签/选择保留、服务关闭回退、Activate/Deactivate/focus 对称
- [x] T013 编译默认 Unity WacomEditor
- [x] T014 运行 UpgradePresentation/UpgradeScreen 与现有 Shop/Run 回归
- [x] T015 写入 quickstart checkpoint

## Phase 3：PIE 命令与定向资产工具

- [x] T016 [US3] 新增 `Wacom.Shop.SeedUpgradePIEValidation` 资格策略和命令测试
- [x] T017 [US3] 新增四 Package manifest/inspect/seed 服务与 collision policy 测试
- [x] T018 [US3] 新增 `WacomSeedDebugShopUpgradeVerticalSlice` Editor command
- [x] T019 [US3] 同步 `ShopBuilder` 的第 25 Offer 与 2/3/4 服务默认值，不执行 builder
- [x] T020 编译并运行 UI/Editor/Data 定向测试
- [x] T021 写入 quickstart checkpoint

## Phase 4：MCP 资产播种

- [x] T022 AssertReady 校验 run endpoint、ProjectRoot、branch、HEAD、PID/Session
- [x] T023 对四个完整 Package 取得 writer lease
- [x] T024 用户在已验证 Editor 执行一次定向 seed 命令
- [x] T025 审计 first pass `3/1/4`、实际 dirty paths、SHA-256 与 LFS
- [x] T026 执行第二次 seed 并确认 `0/0/0`
- [x] T027 保存 audit JSON、释放 writer、正常关闭 Editor

## Phase 5：最终自动化与文档

- [x] T028 AssertClosedForBuild 后默认 Unity 编译
- [x] T029 运行全部受影响 focused tests
- [x] T030 运行 Blueprint compile、AssetRegistry/failed-load、config/parent/binding/closure 审计
- [x] T031 核对地图、角色、CardView 与 Floor 1/2 Production 资产保护哈希
- [x] T032 同步长期 Docs、TODO、Questions、Roadmap 与必要 TechDebt
- [x] T033 运行 Spec Kit analyze、`git diff --check`、范围审计和 `git lfs fsck`
- [x] T034 将完整证据写入 quickstart

## Phase 6：用户 PIE 与提交门禁

- [x] T035 向用户交付一次性 PIE 路线并保持全部修改未提交
- [x] T036 用户确认完整路线无问题；若有修复则重跑相关自动化和完整 PIE
- [ ] T037 仅在用户确认后提交 `feat(ui): add shop card upgrade flow`
- [ ] T038 仅在用户确认后提交 `feat(content): add debug shop upgrade vertical slice`
- [ ] T039 按 AgentIntegrationWorkflow 输出交接；不 merge、不 push

## Dependencies

Phase 1 → 2 → 3 → 4 → 5 → 6 严格串行。资产 mutation 只能在源码编译通过后进行；任何编译前必须释放 writer、关闭 Editor并通过 AssertClosedForBuild。
