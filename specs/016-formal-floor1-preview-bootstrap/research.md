# Research: Floor 1 Preview 启动层

## Decision 1 — Continue on the Spec 015 branch

**Decision**: Keep `D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom` and `codex/formal-floor1-production-scene-baseline` as a continuous Spec 015→016 chain.

**Rationale**: The user prioritizes D-drive capacity and wants to reuse hydration, Binaries, Intermediate, Saved, and DDC. The live dirty map is user-owned PIE tuning that must be preserved. Main-only commits are unrelated enemy UI/backpack slices and are not Preview dependencies.

**Alternatives considered**:

- Wait for Spec 015 integration and create a new worktree: cleanest independent history, rejected by the user because of disk pressure.
- Merge/rebase latest main into the old branch: rejected because it complicates binary authority and violates the established integration workflow.

## Decision 2 — Use a virtual resolver rather than special-case PlayerController

**Decision**: `AWacomGameMode::ResolveJourneyDefinitionForNewRun()` is the single non-reflection seam; PlayerController consumes the resolved pointer once.

**Rationale**: Base/Authoring/Debug behavior stays identical, while a subclass can resolve Preview inputs without PlayerController knowing Descriptor or Preview rules.

**Alternatives considered**:

- Detect map/package/Actor label in PlayerController: brittle scene special-case.
- Set `DefaultJourneyDefinition` dynamically in BeginPlay: ordering relative to PlayerController BeginPlay is not a safe contract.
- Change `URunSession` to accept a Floor: expands the Run contract and invents a second initialization model.

## Decision 3 — Construct one transient Journey from the exact Descriptor Floor

**Decision**: Preview creates one `UWacomJourneyDefinition` with GameMode outer and `RF_Transient`, adds the exact Descriptor Floor pointer, configured character, stable Preview identity, and no success terminal.

**Rationale**: It reuses the current Journey/Run validation and graph truth without saving an incomplete Production asset or rebuilding the Floor graph.

**Alternatives considered**:

- Author a one-Floor Production Journey: violates frozen three-floor identity and terminal contract.
- Copy Floor nodes/edges into a transient duplicate: unnecessary second graph authority.
- Reuse Authoring/Debug Journey: Floor mismatch and forbidden identity leakage.

## Decision 4 — Editor PIE is a hard runtime boundary

**Decision**: resolution requires `WITH_EDITOR` and `World->WorldType == EWorldType::PIE`; every other build/world type returns null.

**Rationale**: A Preview GameMode accidentally selected by a packaged map must fail visibly instead of becoming an undocumented release startup path.

**Alternatives considered**:

- `#if WITH_EDITOR` only: Editor standalone/Game worlds would still pass.
- `GIsEditor` or port/process checks: too broad and not tied to the actual world lifecycle.

## Decision 5 — Cache and guard Descriptor identity

**Decision**: first valid resolution caches the transient Journey, Descriptor, Floor pointer, and FloorId. Later resolution returns the same Journey only if all identity facts still match; otherwise it fails closed and does not rebuild.

**Rationale**: Replacing the Run's static graph authority during a live world would make the Session and scene registry inconsistent.

**Alternatives considered**:

- Rebuild on drift: silently invalidates the active Session.
- Cache without revalidation: hides live authoring changes and stale pointers.

## Decision 6 — Copy presentation configuration from `GM_Wacom`, not its Journey

**Decision**: the Preview Blueprint copies `PlayerControllerClass`, `DefaultPawnClass`, `DefaultCharacter`, `BattleHUDClass`, `ExplorationHUDClass`, and `JourneySummaryScreenClass`, but forces `DefaultJourneyDefinition=null`.

**Rationale**: It preserves the known playable UI/input/avatar setup while making the C++ Preview resolver the sole Journey source.

**Alternatives considered**:

- Derive Blueprint from `GM_Wacom_C`: Blueprint inheritance would also inherit the wrong Authoring Journey and obscure the parent contract.
- Hand-author six fields: error-prone drift from the project's configured shell.

## Decision 7 — Update only World Settings and one plain PlayerStart

**Decision**: the map mutation changes `DefaultGameMode` and creates one plain `APlayerStart` at the unique Entry Anchor transform when absent. Existing scene actors and transforms are inspect-only.

**Rationale**: Those are the two missing startup facts; the Spec 015 map already owns the complete Floor scene contract.

**Alternatives considered**:

- Level Blueprint startup: one-off behavior and hidden runtime authority.
- Teleport pawn after BeginPlay: too late for a clean spawn and overlaps lifecycle.
- Re-run Spec 015 builder: forbidden and would overwrite manual tuning.

## Decision 8 — Seed command is strict, two-package, and internally idempotent

**Decision**: one command preflights both targets, applies the minimum changes, saves only changed allowlisted packages, reloads/inspects, then executes a second pass requiring zero create/save.

**Rationale**: The current map is binary and manually tuned; fail-closed scope and idempotence are required to protect it.

**Alternatives considered**:

- Manual Blueprint/World Settings edits: possible but difficult to audit consistently.
- General-purpose builder: too broad and risks unrelated package writes.

## Decision 9 — PIE is the final commit gate

**Decision**: no commit is created until the user explicitly confirms the consolidated PIE checklist. Any later runtime or binary change invalidates the gate.

**Rationale**: The feature exists to make a full local interaction loop usable; compile and AssetRegistry cannot prove camera/input/CommonUI return behavior.
