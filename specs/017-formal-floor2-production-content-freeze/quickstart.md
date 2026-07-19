# Quickstart: 正式 Floor 2 Production 内容合同冻结

## 1. Working context

- Worktree: `D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom`
- Branch: `codex/formal-floor2-production-content-freeze`
- Base/merge-base: `e7b627a4197a2bbf41bbd69e49ccc8ec1ed750f6`
- Feature: `specs/017-formal-floor2-production-content-freeze/`
- Scope: Markdown、`.specify/feature.json`、`AGENTS.md` managed pointer only
- Commit gate: 用户审阅并明确确认前，不 stage、不 commit

开始时目标 worktree 的 Git/LFS 均干净。检测到的 UnrealEditor-Cmd 属于 `enemy-scene-component-rewrite` worktree，不占用本 worktree；本轮不连接、不关闭、不操作该 Editor。

## 2. Frozen totals

```text
4 Enemy + 4 Behavior + 12 Part + 7 Encounter
+ 3 Event + 4 Pickup + 1 Shop + 12 Card
= 47 future DataAssets
```

```text
Intent count: 5 + 6 + 7 + 8 = 26
Event choices: 3 + 3 + 4 = 10
Encounter HP: 21 / 36 / 42 / 36 / 34 / 57 / 70
```

## 3. Reward and AP audit

必经：ScaleScout 2 + EliteMolter 5 + CavernGuardian 4 = 11 Parts。

| Route | Added Parts | Reward cards |
|---|---:|---:|
| A + C | StoneScaleGuard 3 + BridgeSentinel 3 | 17 |
| B + C | HatcheryAmbush 4 + BridgeSentinel 3 | 18 |
| A + D | StoneScaleGuard 3 + VenomHunter 3 | 17 |
| B + D | HatcheryAmbush 4 + VenomHunter 3 | 18 |
| Full exploration | all seven Encounters | 24 |

击倒选择不增加 AP。Floor 2 保持：

```text
Shortest progression: 8 AP; 9 with first successful shop purchase
Full exploration:     14 AP; 15 with first successful shop purchase
```

## 4. Static validation commands

```powershell
& '.specify/scripts/powershell/check-prerequisites.ps1' -Json -RequireTasks -IncludeTasks
```

```powershell
rg -n "Debug|Authoring|Test\.|BadgeDisplayTests|TrainingWarrior|DA_Character" specs/017-formal-floor2-production-content-freeze Docs/WacomData.md Docs/WacomDataAuthoring.md Docs/WacomBattle.md Docs/WacomMap.md
```

允许的命中只能位于禁止引用/非目标/已知问题说明，不能出现在 47-package manifest 或 Production 引用表。

```powershell
git diff --check
git status --short --branch
git lfs status
git lfs fsck
```

还必须执行：

- 47-package 分类、package 与 stable ID 唯一性检查。
- 4 Enemy/12 Part/4 Behavior/26 Intent 和 HP/EXP/D/I/R/target 检查。
- 7 Encounter slot order、HP 与最多两敌人检查。
- 12 Card、4 Pickup、3 Event/10 Choice、1 Shop schema 值检查。
- 12/12 Part Aid/Destroy 同 Archetype 映射、legacy 0 检查。
- 路线 `17/18/17/18/24`、AP `8–9 / 14–15` 检查。
- Spec Kit 跨工件只读分析与 Markdown link/range audit。

## 5. Skipped Unreal validation

| Validation | Status | Reason / future gate |
|---|---|---|
| WacomEditor compile | Skipped by design | 没有 C++、Build.cs、Config、UHT 或 binary change |
| Automation | Skipped by design | 没有 runtime/DataAsset/test fixture change |
| AssetRegistry / failed-load | Skipped by design | 47 个未来资产尚未创建 |
| Builder / MCP writer | Forbidden this round | 本轮不写 Content，不取得 writer lease |
| Blueprint compile | Skipped by design | 没有 Blueprint change |
| PIE | Skipped by design | 没有 Floor 2 map/Host/Production Journey，静态合同无法通过 PIE 增加证据 |

运行时回归面为零；Production readiness 仍被 47 资产播种/真实校验、Floor 2 Floor/map/Host、完整 Journey 和跨层 handoff 阻塞。

## 6. Known external issue

`DA_Character_BugGirl` 的既有 StarterDeck 污染不属于本轮 47-package manifest。按用户确认：不修改资产、不提交、不削弱 validator，也不把 Production closure 误报为通过。未来 Floor 2 资产验证仍应让该外部问题保持可见，但必须把它与 MoltCavern 自身 manifest/closure 结果分开报告。

## 7. Delivery gate

文档完成后先向用户报告 changed files、计数、静态验证、跳过项与风险。只有用户明确确认后才提交：

```text
docs(content): freeze floor2 production content
```

不 merge main，不 push，不删除旧分支或 worktree。

## 8. Validation record — 2026-07-19

| Check | Result |
|---|---|
| Spec Kit prerequisites | PASS；research/data-model/contracts/quickstart/tasks 全部可发现 |
| Manifest | PASS；47/47 rows 可解析，编号 1–47 连续，package/stable ID 无重复 |
| Type counts | PASS；Card 12、Encounter 7、Behavior 4、Enemy 4、Part 12、Event 3、Pickup 4、Shop 1 |
| Content tables | PASS；26 Intent、12 Part、7 Encounter、10 Event Choice |
| Encounter HP | PASS；`21/36/42/36/34/57/70` |
| Reward/AP | PASS；`17/18/17/18/24`，`8–9 / 14–15` |
| Schema audit | PASS；Effect/Target/Rarity/Keyword、RunEvent condition/effect、pressure 与 `CardLocation.Draw` 均存在于 live schema |
| Forbidden references | PASS；manifest 与 Production 引用表零 Debug/Authoring/Test/BadgeDisplayTests/TrainingWarrior/Character dependency；命中仅在禁止说明 |
| Markdown links/placeholders | PASS；零 broken relative link，零 template/clarification placeholder |
| Spec Kit analyze | PASS；20 FR、9 SC 与 33 tasks 覆盖一致，零 Critical/High/Medium finding |
| `git diff --check` / whitespace | PASS |
| Range audit | PASS；tracked changes 仅 9 个允许的文本/指针文件；Spec 017 为 `.gitignore` 下 13 个待后续 `git add -f` 的文档文件；零 Source/Config/Content/binary change |
| Git LFS | PASS；`git lfs status` 无 staged object，`git lfs fsck OK` |

没有运行 WacomEditor、Automation、AssetRegistry、Builder、Blueprint 或 PIE，原因见 §5。用户于 2026-07-19 明确确认文档内容无问题并授权提交；提交范围仍只允许本节记录的文本和 Spec Kit 指针文件。
