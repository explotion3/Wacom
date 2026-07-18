# Contract: Unreal MCP Writer and Binary Asset Safety

## Fixed identity

```text
Role: run
Endpoint: ue_wacom_run
Port: 8140
ThreadId: 019f69c0-56e1-7280-b959-6b796da49af0
ProjectRoot: D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom
ExpectedBranch: codex/formal-floor1-production-scene-baseline
```

Before the first Editor start, the worktree must have its own compiled `WacomEditor` binaries. Start is forbidden until:

```text
ReleaseWriter (if any)
normal Editor close
AssertClosedForBuild
default Unity WacomEditor build
Start with exact Role/ProjectRoot/ExpectedBranch
AssertReady
```

## Group allowlists

### Floor

```text
/Game/Wacom/Data/Map/Production/DA_Floor_Main_01
```

### EnemyHosts

```text
/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/BrushSnake/BP_EnemyHost_BrushSnake_Graybox
/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/MoltGuard/BP_EnemyHost_MoltGuard_Graybox
/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/RootStalker/BP_EnemyHost_RootStalker_Graybox
/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/ShallowGuardian/BP_EnemyHost_ShallowGuardian_Graybox
```

### Scene

```text
/Game/Wacom/Run/SceneActors/Graybox/BP_WacomRunFloorEntranceMarker_Graybox
/Game/Wacom/Maps/Run/L_Run_Floor_Main_01
```

The writer lease must list the complete selected group allowlist using full extensionless `/Game/...` package paths. A full run uses all seven paths in one declared allowlist but still saves in dependency order.

## Mutation procedure

For each group:

1. Confirm Git/LFS state and record hashes of existing read-only dependencies.
2. `AssertReady` immediately before mutation.
3. `AcquireWriter` with the exact ThreadId and group package allowlist.
4. Invoke only the named seed command in the verified Editor session.
5. Save only allowlisted packages.
6. Inspect command report, Editor log, Git status, binary paths, LFS status, and hashes while the writer is still held.
7. Fail closed if any unexpected package becomes dirty or saved. Do not auto-clean it.
8. Record writer audit JSON and then `ReleaseWriter`.

## Forbidden lifecycle operations

- No branch/HEAD change while Editor is alive.
- No C++ compile while Editor is alive.
- No hand-started legacy `unreal-mcp:8000` server.
- No direct save outside the package allowlist.
- No builder or editor command other than the feature's seed/inspect/validation commands.
- No merge of binary conflicts.

## Required evidence

- AssertReady identity JSON: uproject, PID, process start time, SessionId, port owner, branch, HEAD.
- Acquire/Release writer audit JSON.
- Exact created/saved package list per group.
- Git path and Git LFS status after each group.
- SHA-256 for all seven created files and byte-stability hashes for read-only formal dependencies selected for audit.
- Second-run report showing zero create/save and unchanged seven hashes.
- Final normal Editor close and `AssertClosedForBuild` result.
