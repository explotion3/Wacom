# Preview Seed Command Contract

## Entry point

```text
WacomSeedFormalFloor1PreviewBootstrap
```

The command takes no arguments. Unknown or duplicate arguments cannot be silently ignored because argument parsing is not exposed.

## Required external workflow

Before invocation:

1. Close Editor for the source compile gate.
2. Build default Unity `WacomEditor Win64 Development`.
3. Start the exact `run` endpoint for the target worktree/branch and call `AssertReady`.
4. Acquire one writer lease with both full package names:

```text
/Game/Wacom/Run/Preview/GM_WacomRunFloorPreview
/Game/Wacom/Maps/Run/L_Run_Floor_Main_01
```

5. Record Git/LFS status and pre-hashes.
6. The user enters the named command once in that verified Editor.

No subagent, builder command, manually started old MCP server, or wildcard package scope is permitted.

## Internal transaction

The command performs:

```text
Preflight both package authorities
  -> First pass: create Preview Blueprint if absent
  -> compile/validate Preview Blueprint
  -> load existing map
  -> validate Spec 015 scene contract
  -> set Preview GameMode override if needed
  -> add exact PlayerStart if missing
  -> validate map contract
  -> save only changed target packages
  -> wait for writes
  -> reload/inspect both targets
  -> Second pass with mutation enabled
  -> require 0 created / 0 modified / 0 saved / 0 failed
  -> emit report and success log
```

Preflight failure before first save produces no saved package. A later save/reload failure is reported as a failed transaction; the tool does not auto-delete, restore, or clean binary files.

## Reporting

The report exposes:

- exact package manifest;
- first- and second-pass created/modified/existing/saved/failed counts;
- package-scoped diagnostics;
- final exit code and failure category;
- no hidden save list.

The Editor command log includes a concise summary. The external writer audit remains the authority for actual package saves.

## Idempotence

Success requires the second internal pass to produce:

```text
Created=0 Modified=0 Saved=0 Failed=0
```

An already-correct first invocation also produces these values on both passes.
