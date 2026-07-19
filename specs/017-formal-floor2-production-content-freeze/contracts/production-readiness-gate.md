# Contract: Floor 2 Production readiness gate

## 1. Documentation freeze result

Spec 017 完成后只代表以下内容已冻结：

- 47 个未来 package/class/stable ID；
- Enemy/Behavior/Part/Intent 与 Encounter 结构和初始数值；
- 12 Card、4 Pickup、3 Event、1 Shop 的规则字段；
- Part reward 映射、路线经济、奖励产量与 AP 不变量；
- stable/tunable 字段边界与禁止依赖。

它不代表任何 DataAsset、Floor、map、Host、Journey 或 runtime Golden Path 已交付。

## 2. Future asset implementation gates

未来 47 资产轮必须：

1. 从集成后的最新 main 新开安全分支或复用已审计 worktree；
2. 提供 exact-manifest、seed-missing-only、inspect-first 的 `WacomEditor` 制作服务；
3. 在 mutation 前用 Unreal MCP `run` role、准确 worktree/branch/HEAD 和完整 47-package allowlist 取得 writer lease；
4. 按可验证依赖小组串行创建，不覆盖已存在正确 class 的资产；
5. 运行通用 Data Validation、FormalProduction Part profile、exact structure、真实加载和 failed-load；
6. 运行 AssetRegistry class/count、forbidden dependency closure、哈希、双跑 `0 created / 0 saved` 与 Git LFS fsck；
7. 证明三张 Shop 外部卡只读且哈希不变；
8. 用户验收前不把调参字段交给 seed-only 工具覆盖。

## 3. Future scene gates

47 资产完成后仍需独立完成：

- `DA_Floor_Main_02`、Production world、Anchor/Path/BranchTarget/Host 与 Exit；
- Floor 2 Enemy Host、交互范围、人工 Transform 和 scene binding；
- `DA_Journey_Main_01` 与 Floor 1→2→3 的 FloorId-to-world handoff；
- Builder/validator 幂等、Blueprint compile、AssetRegistry 和用户 Golden Path PIE；
- Preview/release blocker 清理。

不得在 Level Blueprint、Exit marker 或临时 GameMode 中硬编码跨层 travel。

## 4. Forbidden shortcuts

- 不运行会覆盖人工调参的全量内容、卡牌、材质或 DreamShader rebuild。
- 不复制 Debug/Authoring/Test/TrainingWarrior/Character 内容作为空壳 Production 资产。
- 不把文档表格、package 存在或单独 AssetRegistry online 误当成真实规则/PIE 验证。
- 不削弱 Production dependency validator 来隐藏 `DA_Character_BugGirl` 或其它外部污染。
- 不对同路径 `.uasset/.umap` 做 Git 合并；冲突由集成会话确定权威版本或在新基线上重放。

## 5. Current known blocker state

- Floor 2 内容设计 blocker：由 Spec 017 关闭。
- Floor 2 47 DataAsset：未创建，Ready for implementation after integration/review。
- Floor 2 Floor/map/Host：未创建，依赖内容资产与独立场景计划。
- Production Journey/跨层 handoff：未创建。
- Floor 3 内容设计与资产：继续独立保留。
- BugGirl StarterDeck 污染：已知外部问题，保持 validator 可见，不在本轮修复。
