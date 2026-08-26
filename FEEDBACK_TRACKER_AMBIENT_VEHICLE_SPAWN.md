# Feedback Tracker: встроенная проверка точек появления ambient-техники в World Editor

## Summary

Add native validation and clear localized feedback when placing ambient vehicle spawn points in World Editor.

Currently, an `AmbientVehicleSpawnpoint_*` prefab can be placed even when the active GameMode cannot spawn ambient vehicles. The point then remains in the world but never produces a vehicle, with no editor-side explanation. Mod authors must implement a Workbench plugin only to detect this configuration error.

## Current behaviour

Ambient vehicle spawning is controlled by `EGameFlags.SpawnVehicles`.

- `EGameFlags.SpawnVehicles = 2`.
- `SCR_BaseGameMode` applies `m_eTestGameFlags` during initialization.
- A basic GameMode inherits the default value `m_eTestGameFlags = 0`.
- The ambient vehicle system disables itself when `SpawnVehicles` is not enabled.

As a result, a user can place valid ambient vehicle spawn points in a world, enter Game mode, and see no spawned vehicles. This is particularly confusing because the point prefab itself has no visible configuration error.

## Expected behaviour

When an ambient vehicle spawn-point prefab is dropped into a world, World Editor should validate the GameMode configuration before creating it.

For a valid setup, the world must contain exactly one editable `SCR_BaseGameMode` with `EGameFlags.SpawnVehicles` enabled in `m_eTestGameFlags` / Test Game Flags.

If the setup is invalid, World Editor should cancel the drop and show a localized dialog explaining the specific reason.

## Validation rules

Perform the checks in this order:

1. World Editor API / current world is available.
2. There is exactly one entity derived from `SCR_BaseGameMode`.
3. The unique GameMode is on an editable layer.
   - Check its own layer and every parent layer.
   - A locked layer hierarchy must block the operation.
   - A hidden but unlocked layer must **not** block the operation.
4. `m_eTestGameFlags` is available on the GameMode source.
5. `m_eTestGameFlags` contains `EGameFlags.SpawnVehicles`.

## Suggested localized messages

### No GameMode

> The point was not created because this world has no SCR_BaseGameMode. Create or configure exactly one editable GameMode, then enable Spawn Vehicles.

### Multiple GameModes

> The point was not created because multiple SCR_BaseGameMode entities were found. Configure exactly one editable GameMode, then enable Spawn Vehicles.

### Locked GameMode layer

> The point was not created because the only GameMode is on a locked layer or inside a locked parent layer and cannot be configured. Create another world with an editable GameMode.

### Test Game Flags unavailable

> The point was not created because m_eTestGameFlags is unavailable on the only SCR_BaseGameMode. Use an editable GameMode that exposes Test Game Flags.

### Spawn Vehicles disabled

> The point was not created because Spawn Vehicles is disabled in the only GameMode's Test Game Flags / m_eTestGameFlags.

## Manual validation command

A World Editor plugin or menu command such as **Check ambient vehicle spawning** would also be useful. It should scan the already open world and report the same condition for existing ambient spawn points without changing the world.

This would help users diagnose old scenarios, imported worlds, and points placed before validation was added.

## Reproduction

1. Open a world using a GameMode derived from `SCR_BaseGameMode` with default `m_eTestGameFlags = 0`.
2. Place a vanilla `Prefabs/Systems/AmbientVehicles/AmbientVehicleSpawnpoint_*` prefab.
3. Enter Game mode.
4. Observe that the ambient vehicle system does not spawn vehicles.

## Technical reference

The relevant condition is the vanilla game flag check:

```c
GetGame().AreGameFlagsSet(EGameFlags.SpawnVehicles)
```

`EGameFlags.SpawnVehicles` has value `2`; setting `m_eTestGameFlags = 6` enables both `SpawnVehicles` and `SpawnAI`.

## Why this matters

The feature would prevent a common silent configuration failure, make the vanilla ambient vehicle workflow discoverable, and remove the need for mod authors to write editor-only guard plugins solely to explain a missing GameMode flag.
