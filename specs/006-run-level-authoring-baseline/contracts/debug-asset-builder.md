# Contract: Run Exploration Debug Asset Builder

## Command

```text
-run=WacomBuildRunExplorationDebugAssets
```

The old `-run=WacomBuildRunExplorationAssets` command is removed without a wrapper or redirect.

## Builder-owned write set

- `/Game/Wacom/Data/Map/DA_Journey_Debug`
- `/Game/Wacom/Data/Map/DA_Floor_Debug_01`
- `/Game/Wacom/Debug/GameModes/GM_WacomRunDebug`
- `/Game/Wacom/Maps/Debug/L_RunExploration_Debug`
- actors inside the Debug World carrying the project Debug generated ownership marker

## Explicit forbidden write set

- `/Game/Wacom/Maps/L_Exploration`
- `/Game/Wacom/Data/Map/Authoring/*`
- `/Game/Wacom/Core/GameModes/GM_Wacom`
- `/Game/Wacom/Core/Player/BP_WacomPlayerCharacter`
- `/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathSegmentActor`
- `/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathBranchTargetActor`
- `/Game/Wacom/Run/Path/Blueprints/BP_WacomRunMapNodeAnchorActor`
- any hand-authored art, material, card or UI asset

The builder may load forbidden-write assets only as dependencies. It must not compile, save, mark dirty or reconstruct them.

## Build behavior

1. Validate required shared Blueprint assets exist and inherit the expected native classes.
2. Create/update Debug Journey and Floor data deterministically.
3. Create/update the dedicated Debug GameMode and point it to Debug Journey.
4. Load/create only the Debug map.
5. Rebuild only Debug-owned node/path/branch/host actors.
6. Ensure exactly one Scene Descriptor referencing Debug Floor.
7. Run the shared scene validator.
8. Save only the builder-owned write set if validation succeeds.

If a dependency or validation step fails, the command returns failure and reports the exact asset/diagnostic. It never repairs a shared dependency.

## Idempotence and integrity

- Two consecutive builds produce identical logical graph, actor counts, identities and references.
- No duplicate descriptor, anchor, path, branch or input/config mapping is created.
- Tests compute SHA-256 for the forbidden write set before and after the build; every hash must remain equal.
- The Debug map and new Debug GameMode are tracked with project Git LFS rules.
