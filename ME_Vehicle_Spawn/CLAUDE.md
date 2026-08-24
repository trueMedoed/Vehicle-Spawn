# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

This is an Arma Reforger / Enfusion addon. The project root contains `addon.gproj` (addon ID `MEVehicleSpawn`) and has a base-game dependency. Runtime code is Enforce Script under `Scripts/Game/`; resources are authored for Arma Reforger Workbench.

## Commands and validation

This repository has no package manager, command-line build wrapper, linter, automated test suite, or single-test command.

- Develop and validate the addon with **Arma Reforger Tools / Workbench**: open `addon.gproj`, open `worlds/MP/MpTest/ME_MpTest.ent`, then enter game mode to compile scripts and exercise the test setup.
- Inspect the Workbench/runtime log for the diagnostic prefixes `[ME_DEBUG_AVSP]`, `[ME_DEBUG_AVSP_POS]`, and `[ME_DEBUG_AVSP_SYS]`.
- Use Git for repository inspection, e.g. `git status` and `git diff`.

## Architecture

The current addon is diagnostic instrumentation for the base-game ambient vehicle spawning feature, not a standalone spawning system.

- `Scripts/Game/Systems/ME_DebugAmbientVehicleSystem.c` mods `SCR_AmbientVehicleSystem`. It preserves base behavior by calling `super` while logging edit-mode enablement, initialization, spawn-point registration, and the before/after state of each `ProcessSpawnpoint` call.
- `Scripts/Game/Components/Locations/ME_DebugAmbientVehicleSpawnPointComponent.c` mods `SCR_AmbientVehicleSpawnPointComponent`. After the base component initializes, it logs terrain-position probing and ambient-system/spawn-point registration state.
- `worlds/MP/MpTest/ME_MpTest.ent` is a subscene inheriting the base-game `MpTest` world. Its `ME_MpTest_Layers/default.layer` places the base-game US ambient-vehicle spawn-point prefab used to exercise the instrumented classes.

## Editing resources and overrides

- Keep `super` calls in the modded overrides unless the intended change explicitly replaces base-game behavior; the existing overrides are intended to observe, not alter, the ambient spawning flow.
- Retain the existing log-prefix families when adding related diagnostics so Workbench output remains filterable.
- The test world and layer refer to base-game resources by GUID. Prefer editing `.ent` and `.layer` resources in Workbench rather than manually changing serialization or generated resource metadata.
- Treat `resourceDatabase.rdb` as Workbench-managed metadata; do not hand-edit it unless there is a specific, verified reason.
