# Preview PIE Acceptance Contract

## Gate

PIE acceptance is a mandatory pre-commit gate owned by the user. Automation and asset validation do not waive it.

Acceptance becomes stale when any of these changes afterward:

- `Source/WacomApp/` runtime code participating in Preview or return flow;
- either Preview binary package;
- the Floor Definition or any scene binding/interaction asset used by the tested path.

When stale, rerun relevant automation and the complete checklist before commit.

## One-pass checklist

Open `/Game/Wacom/Maps/Run/L_Run_Floor_Main_01` directly and start PIE.

1. Startup log names `Journey.Preview.Floor.Main.01` and `Floor.Main.01`.
2. No `MissingJourney`, `DescriptorFloorMismatch`, or `SceneBindingSnapshotInvalid` appears.
3. Player starts at Entry and becomes Anchored.
4. Mouse look, W/S, Exploration HUD, and first-person hand work.
5. M map opens/closes and its buttons can be clicked.
6. ESC menu opens/closes and its buttons can be clicked.
7. Complete at least one path traversal.
8. Complete one battle and return to Run.
9. Complete or close at least one Event, Reward, or Shop interaction and return.
10. After each return, mouse look, W/S, M, ESC, HUD, and hand still work.
11. Reach Exit; it offers no interaction, floor transition, or Journey Summary.
12. Stop PIE and confirm the Preview GameMode hash is unchanged. If the user intentionally adjusted and saved the map, record explicit acceptance of the new map hash, rerun focused asset/scene-binding validation, and confirm that accepted hash stays stable.

## Failure handling

- Report the first failing step and relevant logs.
- Keep all changes uncommitted.
- Close the target Editor before any C++ rebuild.
- Apply the smallest contract-consistent correction.
- Rerun affected automation and the complete PIE checklist.

## Commit authorization

Only the user's explicit statement that all checklist items are normal authorizes staging/committing. It does not authorize merge or push.
