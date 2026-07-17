# Quickstart: 正式 Floor 2/3 图与 Journey 总节奏冻结验证

## 1. Workspace preflight

```powershell
$Project = 'D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom'
Set-Location $Project
git branch --show-current
git rev-parse HEAD
git status --short --branch
git lfs status
```

Expected:

- Branch: `codex/run-level-authoring-baseline`.
- Base commit: `08b11ee1ce0f32afd36f033fc6d11e39099116e9`.
- Initial worktree and LFS status are clean.
- No Unreal Editor/Cmd/ShaderCompileWorker process occupies assets; this documentation slice does not launch Unreal.
- `.specify/extensions.yml` has no enabled `before_plan`/`after_plan` hook; disabled agent-context hook is skipped.

## 2. Canonical artifacts

- Requirements: [spec.md](./spec.md)
- Plan and boundaries: [plan.md](./plan.md)
- Decisions: [research.md](./research.md)
- Canonical Node/Edge/content/AP tables: [data-model.md](./data-model.md)
- Stable identities: [contracts/stable-identity-extension.md](./contracts/stable-identity-extension.md)
- Floor 2 graph: [contracts/floor2-graph-content-contract.md](./contracts/floor2-graph-content-contract.md)
- Floor 3 terminal graph: [contracts/floor3-terminal-graph-contract.md](./contracts/floor3-terminal-graph-contract.md)
- Journey pacing/readiness: [contracts/journey-pacing-production-readiness.md](./contracts/journey-pacing-production-readiness.md)

## 3. Static acceptance matrix

| Check | Floor 2 expected | Floor 3 expected |
|---|---|---|
| Nodes | 20 unique | 20 unique |
| Edges | 21 unique | 21 unique |
| Type counts | 4 Nav / 7 Encounter / 3 Event / 4 Treasure / 1 Shop / 1 Entrance | 4 Nav / 8 Encounter / 4 Event / 4 Treasure / 0 Shop / 0 Entrance |
| Reachability | 20/20 from Entry | 20/20 from Entry |
| First merge | A/B → Junction.02 | A/B → Junction.02 |
| Second merge | C/D → Key.01 | C/D → Core.01 |
| Dominance | Key dominates Guardian/Exit | Core dominates terminal chain |
| Camp | 4 Navigation nodes | 4 Navigation nodes |
| Terminal | Exit → Floor.Main.03 | Guardian has no outgoing Edge |
| Content slots | 15 | 16 |
| AP shortest/full | 8–9 / 14–15 | 10 / 16 |

Journey expected: `26–28 AP` shortest, `44–46 AP` full, 46 Production content slots across all three Floors.

## 4. Graph and identity audit

The canonical graph may be checked with a read-only parser or manually from the data-model tables. Required assertions:

```text
Floor2 Nodes=20 Edges=21 Reachable=20
Floor2 Key dominance: Guardian/Exit unreachable when Node.Key.01 is removed
Floor3 Nodes=20 Edges=21 Reachable=20
Floor3 Core dominance: Junction.03/Main.02/Main.03/Guardian unreachable when Node.Core.01 is removed
Floor3 Guardian outgoing=0; FloorEntrance count=0
```

Identity search:

```powershell
rg -n "Floor\.Main\.0[123]|Card\.Run\.MoltSeal|Credential\.Run\.MoltSeal|MoltCavern|VenomCore" `
  specs/009-formal-floor23-journey-pacing-freeze `
  Docs/WacomMap.md Docs/WacomDataAuthoring.md

rg -n "Journey\.Authoring|Floor\.Authoring\.01|DA_.*Debug|/Debug/" `
  specs/009-formal-floor23-journey-pacing-freeze/contracts `
  specs/009-formal-floor23-journey-pacing-freeze/data-model.md
```

The second search may find explicit forbidden-reference explanations in the readiness contract. It must not find a Production content table mapping to those assets.

## 5. Layout audit

For each Floor independently:

- Every MapPosition is finite.
- `0 <= X <= 1920` and `0 <= Y <= 1080`.
- No two node centers are identical.
- The listed defaults produce no `<48 px` proximity warning.

Floor 3 final positions are:

```text
Node.Core.01      (960,350)
Node.Junction.03  (960,270)
Node.Main.02      (960,200)
Node.Main.03      (960,130)
Node.Guardian.01  (960,60)
```

MapPosition remains mutable presentation and is not a stable identity.

## 6. AP and day audit

```text
Floor 1 shortest/full = 8–9 / 14–15
Floor 2 shortest/full = 8–9 / 14–15
Floor 3 shortest/full = 10 / 16
Journey shortest/full = 26–28 / 44–46
```

Only the Floor 1 and Floor 2 first successful Shop purchases create the 0–2 AP range.

Day model:

- Morning Planning consumes one of Morning's two AP.
- Normal Camp consumes/reserves Night AP, ends Night and skips Sunrise; ordinary content throughput is commonly 8–10 AP/day.
- Critical 26–28 AP therefore targets about 3 days.
- Full 44–46 AP targets about 5–6 days under normal Camp/recovery behavior.
- Full Night Exploration can expose up to 12 content AP/day, giving an aggressive lower bound of about 4 days.
- These are authoring targets, not a new forced Journey timer.

## 7. Terminal capability audit

Required statements must remain true across Spec and long-term Docs:

- `Floor.Main.03.Node.Guardian.01` is the frozen Journey success design trigger.
- The current runtime does not contain a Journey success state/event/result.
- Battle victory currently resolves the Encounter only.
- `bRunActive=false` remains Defeat semantics.
- FloorEntrance cannot represent completion because it requires a later TargetFloorId.
- Production activation remains blocked until a reusable WacomRun success contract and WacomApp handoff exist.

## 8. Cross-artifact and boundary validation

```powershell
$env:SPECIFY_FEATURE = '009-formal-floor23-journey-pacing-freeze'
$env:SPECIFY_FEATURE_DIRECTORY = 'specs/009-formal-floor23-journey-pacing-freeze'
& '.specify/scripts/powershell/check-prerequisites.ps1' -Json -RequireTasks -IncludeTasks
Remove-Item Env:SPECIFY_FEATURE, Env:SPECIFY_FEATURE_DIRECTORY

rg -n "\[NEEDS CLARIFICATION|\[FEATURE|\[DATE|\[###|\$ARGUMENTS|TXXX" `
  specs/009-formal-floor23-journey-pacing-freeze/spec.md `
  specs/009-formal-floor23-journey-pacing-freeze/plan.md `
  specs/009-formal-floor23-journey-pacing-freeze/tasks.md `
  specs/009-formal-floor23-journey-pacing-freeze/research.md `
  specs/009-formal-floor23-journey-pacing-freeze/data-model.md `
  specs/009-formal-floor23-journey-pacing-freeze/contracts

git diff --check
git diff --name-only | rg "^(Source|Config|Content)/"
git diff --name-status -- '*.uasset' '*.umap'
git lfs status
git status --short
```

Expected:

- Prerequisites resolve Spec 009 and find `tasks.md`.
- No unresolved template marker.
- No Source/Config/Content or binary change.
- No pending LFS object.

## 9. Skipped Unreal validation

| Validation | Status | Reason | Remaining risk |
|---|---|---|---|
| WacomEditor compile | Skipped | No source, Build.cs, Config or reflection change | None for this documentation slice |
| Automation | Skipped | No runtime/validator behavior changed | Static design is not yet encoded as assets |
| AssetRegistry / Blueprint compile | Skipped | No asset created or modified | Production asset slice must run both |
| PIE | Skipped | No Production Floor 2/3 world; Journey success runtime missing | Terminal flow and presentation remain blocked |
| Binary hashes | N/A | No binary migration or write | Git diff must remain free of Content changes |

## 10. Production readiness after this slice

- [x] Floor 1 graph approved.
- [x] Floor 2 graph approved.
- [x] Floor 3 terminal graph approved.
- [x] Persistent Credential framework implemented generically.
- [ ] Journey success runtime contract implemented and tested.
- [ ] All 46 Production definitions exist and pass Data Validation.
- [ ] Production map/package authority selected after AssetRegistry/reference/hash audit.
- [ ] Production DataAssets/worlds/scenes implemented and validated.

## 11. Validation record — 2026-07-17

- Branch/base preflight: PASS — `codex/run-level-authoring-baseline` at `08b11ee1ce0f32afd36f033fc6d11e39099116e9`.
- Initial worktree/LFS: PASS — clean.
- Unreal process audit: PASS — no related process.
- Spec Kit setup: PASS — feature pointer resolves `specs/009-formal-floor23-journey-pacing-freeze`; setup-plan returned the expected paths.
- Graph/type/reachability/dominance/layout/AP audit: PASS — Floor 2/3 均为 20 Node/21 Edge/20 reachable；类型为 `4/7/3/4/1/1` 与 `4/8/4/4/0/0`；Key/Core dominance、四处 Camp、Guardian terminal、坐标范围/无重合/无 `<48 px` proximity、`8–9 / 14–15`、`10 / 16` 和 `26–28 / 44–46` 全部通过。
- Identity/content audit: PASS — Floor 2 15 + Floor 3 16 = 31 个新增 Production IDs 非空唯一；WacomMap/DataAuthoring 全量覆盖；无 Production 内容映射 Authoring/Debug。
- Cross-artifact analysis: PASS — prerequisites 在显式 `SPECIFY_FEATURE`/`SPECIFY_FEATURE_DIRECTORY` 下定位 Spec 009；15 FR + 7 SC = 22 requirements、21 tasks、coverage 100%，0 ambiguity/duplication/Critical/High/Medium/Low，0 unmapped task，Constitution/terminology PASS。裸 prerequisites 命令仍会因长期分支名不符合数字 feature-branch 模式而拒绝，Quickstart 已固定支持的显式 feature 调用。
- Long-term Docs sync: PASS — `Docs/WacomMap.md`、`Docs/WacomDataAuthoring.md`、`Docs/TODO.md`、`Docs/Questions.md` 已同步；旧 Floor 2/3 缺图事实为 0；`Docs/Architecture.md` 与 `Docs/WacomRun.md` 无实现事实变化，无需修改。
- Placeholder/non-target audit: PASS — 无未解决模板标记；无 Source/Config/Content、`.uasset/.umap` 或 LFS 变更；`git diff --check` 通过。
- Unreal validation: intentionally skipped per §9.
- Changed files: 17 paths — 11 个 Spec 009 工件、4 个长期 Docs、`.specify/feature.json` 与 `AGENTS.md` 托管指针。
- Binary hashes: N/A — 未创建、迁移、加载或保存二进制 Package。
- Remaining blocker: Journey success state/event/result、总结/返回交接尚未实现；46 个 Production definitions 和资产权威审计尚未完成。
- Final commit: 见交接报告（即包含本文件的提交）。
