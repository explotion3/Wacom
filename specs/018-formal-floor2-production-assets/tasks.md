# Tasks: Floor 2 Production 47 DataAsset 播种与校验

**Input**: `specs/018-formal-floor2-production-assets/` 下的规格、计划、研究、数据模型与合同
**Authority**: `AGENTS.md`、长期 `Docs/`、live 源码与 Spec 017 的已冻结内容合同优先
**Delivery gate**: 用户审阅完整资产与验证报告前，不 stage、不 commit

## Phase 1：启动审计与 Spec 018 工件

- [x] T001 确认 `main@8e54505e7c28b3721b5155d642de9933f380f170`、目标 worktree、目标 branch、Git/LFS 状态与 Unreal Editor/Live Coding 进程
- [x] T002 从最新 main 在复用 worktree 创建 `codex/formal-floor2-production-assets`，不创建第二份工作区
- [x] T003 [US1] 创建 `spec.md`、`plan.md`、`research.md`、`data-model.md` 和 `quickstart.md`
- [x] T004 [US1] 创建 exact 47-package manifest、播种命令、MCP 写入与验证合同
- [x] T005 [US1] 创建 requirements 与 production-assets 质量检查清单
- [x] T006 [US1] 更新 `.specify/feature.json` 与 `AGENTS.md` 托管指针到 Spec 018
- [x] T007 [US1] 运行 Spec Kit prerequisite 与跨工件一致性分析并修正阻断问题

## Phase 2：共享播种内核

**Checkpoint**: 默认 Unity 编译；Floor 1 manifest/data 回归；Floor 1 的 46 个资产哈希与 Git 状态不变。

- [x] T008 记录 Floor 1 46 个资产、三个 Floor 2 只读依赖和当前 dirty 文本文件的基线 SHA-256
- [x] T009 [US2] 在 `WacomEditor/Private` 新增共享 FormalProductionContentSeedService 私有类型与接口
- [x] T010 [US2] 抽取通用参数解析、未知/危险参数拒绝和组选择逻辑
- [x] T011 [US2] 抽取 manifest/class/stable-ID 预检与依赖解析逻辑
- [x] T012 [US2] 抽取 expected transient object 构建、strict/structural comparator 与人工可调字段归一化逻辑
- [x] T013 [US2] 抽取 inspect-only、seed-missing-only、no-overwrite、顺序保存、重载与失败收敛逻辑
- [x] T014 [US2] 抽取 JSON 报告 schema、退出码与失败分类，保持 Floor 1 报告兼容
- [x] T015 [US2] 将 Floor 1 builder 改为 SerpentWood profile + 共享内核 wrapper，保留原命令和测试接口
- [x] T016 [US2] 增加共享服务小型测试，覆盖参数、inspect-only、no-overwrite、comparator 与 partial-save 报告合同
- [x] T017 编译 `WacomEditor Win64 Development`（默认 Unity，`-NoDreamShaderEditorBridge`）
- [x] T018 运行 `Wacom.Editor.FormalProductionContentSeedService`、`Wacom.Editor.FormalFloor1Content.Manifest` 与 `Wacom.Data.FormalFloor1Content`
- [x] T019 对比 Floor 1 46 资产基线哈希、Git/LFS 状态，确认重构未保存 Floor 1 内容
- [x] T020 将 Phase 2 编译、测试、哈希与风险写入 `quickstart.md`

## Phase 3：Floor 2 profile、入口与 transient 校验

**Checkpoint**: 第二次默认 Unity 编译；空资产目录 inspect-only 精确报告 `47 missing / 0 created / 0 saved`。

- [x] T021 [US3] 实现 MoltCavern Cards profile：4 Pickup/Run cards + 8 Aid/Destroy cards
- [x] T022 [US3] 实现 MoltCavern EnemyGraph profile：4 Enemy、4 Behavior、12 Part 与 26 Intent
- [x] T023 [US3] 实现 MoltCavern NodeDefinitions profile：7 Encounter、3 Event、4 Pickup、1 Shop 与只读依赖
- [x] T024 [US3] 实现 Floor 2 manifest/count/class/stable-ID/content-specific invariants
- [x] T025 [US3] 新增 `WacomBuildFormalFloor2Content` commandlet
- [x] T026 [US3] 新增 `Wacom.BuildFormalFloor2Content` Editor command
- [x] T027 [US3] 新增 Floor 2 automation test view，不向运行时或 Blueprint 暴露测试 API
- [x] T028 [US3] 新增 `Wacom.Editor.FormalFloor2Content.Manifest` transient/manifest/comparator tests
- [x] T029 [US3] 新增 `Wacom.Data.FormalFloor2Content` persisted asset validation tests
- [x] T030 编译 `WacomEditor Win64 Development`（默认 Unity，`-NoDreamShaderEditorBridge`）
- [x] T031 运行共享服务、Floor 1 回归和 Floor 2 manifest/transient tests
- [x] T032 以 inspect-only 运行 Floor 2 All 并确认 `47 missing / 0 created / 0 saved`、统一退出码和 JSON 报告
- [x] T033 审计七个 MoltCavern 根目录没有因 inspect-only 产生 package 或目录内容
- [x] T034 再次核对三个只读依赖 SHA-256 与 Floor 1 46 资产哈希不变
- [x] T035 将 Phase 3 编译、测试、空目录 inspect 报告与风险写入 `quickstart.md`

## Phase 4：Cards 12 串行播种

- [x] T036 关闭编译/命令进程，启动前记录所有本轮 owned dirty 文本路径与 SHA-256
- [x] T037 [US4] `AssertReady` 校验 run endpoint、ProjectRoot、branch、HEAD、SessionId、PID 与 port owner
- [x] T038 [US4] 以 exact 12 Cards package allowlist 取得 writer lease
- [x] T039 [US4] 执行 `Group=Cards SeedMissing CompareSeedDefaults` 并保存 audit/report JSON
- [x] T040 [US4] 执行 Cards strict inspect，确认 `12 existing / 0 created / 0 saved / 0 failed`
- [x] T041 [US4] 记录 12 个 `.uasset`、SHA-256、Git/LFS 状态并释放 writer

## Phase 5：EnemyGraph 20 串行播种

- [x] T042 [US5] 再次 `AssertReady` 并确认 Cards assets/源码 dirty 状态与 session 身份一致
- [x] T043 [US5] 以 exact 20 EnemyGraph package allowlist 取得 writer lease
- [x] T044 [US5] 执行 `Group=EnemyGraph SeedMissing CompareSeedDefaults` 并保存 audit/report JSON
- [x] T045 [US5] 执行 EnemyGraph strict inspect，确认 `20 existing / 0 created / 0 saved / 0 failed`
- [x] T046 [US5] 记录 20 个 `.uasset`、SHA-256、Git/LFS 状态并释放 writer

## Phase 6：NodeDefinitions 15 串行播种

- [x] T047 [US6] 再次 `AssertReady`，复核三个只读依赖 SHA-256 与 Cards/EnemyGraph 状态
- [x] T048 [US6] 以 exact 15 NodeDefinitions package allowlist 取得 writer lease
- [x] T049 [US6] 执行 `Group=NodeDefinitions SeedMissing CompareSeedDefaults` 并保存 audit/report JSON
- [x] T050 [US6] 执行 NodeDefinitions strict inspect，确认 `15 existing / 0 created / 0 saved / 0 failed`
- [x] T051 [US6] 记录 15 个 `.uasset`、SHA-256、Git/LFS 状态并释放 writer
- [x] T052 正常关闭 Editor，执行 `AssertClosedForBuild`

## Phase 7：最终编译、自动化与真实资产门禁

- [x] T053 编译 `WacomEditor Win64 Development`（默认 Unity，`-NoDreamShaderEditorBridge`）
- [x] T054 运行 Floor 1/2 formal content、Card、EnemyPart FormalProduction、Enemy、Behavior、Encounter、RunEvent、RunPickup 和 Shop validation
- [x] T055 运行 `Wacom.Battle.RuleContentMatrix`、`Wacom.Battle.KnockdownReward` 及 Run BattleReward/Shop/Event/Pickup/Notification smoke
- [x] T056 [US7] 验证 47 资产 class/count、26 Intent、10 Choice、12 explicit Aid/Destroy 与七场 Encounter HP
- [x] T057 [US7] 验证 Guardian Destroy、MoltSeal card+credential 与全部稳定身份/引用
- [x] T058 [US7] 运行 AssetRegistry、failed-load、forbidden dependency closure 与 legacy 字段审计
- [x] T059 [US7] 对 Cards/EnemyGraph/NodeDefinitions 各执行第二次 strict inspect，确认聚合 `0 created / 0 saved / 0 failed`
- [x] T060 [US7] 对比 47 文件聚合 SHA-256、三个只读依赖及 Floor 1 46 资产基线哈希
- [x] T061 运行 `git lfs fsck`、`git diff --check`、范围审计与 Git/LFS 状态检查
- [x] T062 明确记录未运行 Builder、Blueprint compile 与 PIE 的原因和 Floor 2 Golden Path 后续门禁

## Phase 8：长期文档与用户审阅门禁

- [x] T063 [US8] 同步 `Docs/WacomData.md`、`WacomDataAuthoring.md`、`WacomBattle.md`、`WacomRun.md` 与 `WacomMap.md`
- [x] T064 [US8] 同步 `Docs/Architecture.md`、`TODO.md`、`Questions.md` 与 `Roadmap.md`
- [x] T065 [US8] 在 `quickstart.md` 汇总 base、编译/测试、MCP provenance、writer audit、47 资产哈希、幂等结果、跳过项和已知外部污染
- [x] T066 运行最终 Spec Kit 跨工件一致性分析、身份/forbidden-reference `rg` 审计与 `git diff --check`
- [x] T067 向用户提供一次性审阅包；保持所有内容 unstaged/uncommitted，等待明确确认

## Phase 9：仅在用户确认后提交与交接

- [x] T068 提交 `feat(editor): generalize formal production content seeding`
- [x] T069 提交 `feat(content): seed formal floor2 production assets`
- [x] T070 确认 worktree 与 Git LFS 干净，按 `Docs/AgentIntegrationWorkflow.md` 输出标准交接；不 merge、不 push

## Dependencies

- Phase 1 → Phase 2 → Phase 3 严格串行。
- Phase 4/5/6 的资产依赖固定为 `Cards → EnemyGraph → NodeDefinitions`，不得并行。
- 每组必须在释放 writer 并重新 `AssertReady` 后才能进入下一组。
- Phase 7 必须在 Editor 正常关闭且 `AssertClosedForBuild` 成功后开始。
- Phase 9 必须等待用户对 Phase 8 审阅包明确确认。

## Parallel execution

本功能禁止 subagent，并且资产依赖及 writer lease 要求串行；不安排并行写入任务。只读静态审计可在单一主会话内批量执行。
