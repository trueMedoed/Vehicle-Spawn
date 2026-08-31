# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Communication

- Communicate with the user in Russian unless they explicitly request another language.

## Claude project data

- Keep all Claude Code project data, settings, and worktrees for this repository under `C:\Users\Phil\Documents\GitHub\Mods\.claude\Vehicle Spawn`, not in the repository-local `.claude` directory.

## Script documentation

- When creating a script, add a brief description of what the script does.
- Add comments describing the purpose of each function.
- Keep script, class, and enum descriptions, as well as comments for every supported method, in English and duplicate them in Russian.
- Preserve the original English text and add the Russian block immediately beside it, directly after the English documentation block.
- Apply this bilingual documentation rule to the main addon and to test addon projects.
- Do not translate runtime strings, diagnostic `Print`/`PrintFormat` messages, localization keys, or historical commented-out code unless explicitly requested.

## Project

This is an Arma Reforger / Enfusion addon. The project root contains `addon.gproj` (addon ID `MEVehicleSpawn`) and has a base-game dependency. Runtime code is Enforce Script under `Scripts/Game/`; resources are authored for Arma Reforger Workbench.

For the required workflow for transferring verified changes from the test addon to production, see [TEST_TO_PROD_TRANSFER.md](TEST_TO_PROD_TRANSFER.md).

## Commands and validation

This repository has no package manager, command-line build wrapper, linter, automated test suite, or single-test command.

- Develop and validate the addon with **Arma Reforger Tools / Workbench**: open `addon.gproj`, open `worlds/MP/MpTest/ME_MpTest.ent`, then enter game mode to compile scripts and exercise the test setup. For the vanilla Eden ambient-vehicle baseline, open `worlds/MP/CTI_Campaign_Eden.ent`; do not use terrain-only `worlds/Eden/Eden.ent`, which has no game mode and initializes the ambient system with `enabled=0` and zero spawn points.
- To restart Workbench cleanly through EnfusionMCP: stop the existing `ArmaReforgerWorkbenchSteamDiag.exe` process, launch Workbench with `wb_launch` and `addon.gproj`, then verify the bridge using `wb_connect`. Do not assume that a launch or reload completed merely because its call returned.
- Open a test world with `wb_open_resource`, e.g. `wb_open_resource(path: "worlds/MP/CTI_Campaign_Eden.ent")`; this is the reliable world-opening operation. Campaign Eden has many entities: after a successful call, wait 5 seconds for the world to load before entering Game mode or querying editor state. After the required Game-mode observation/log capture, always call `wb_stop` to return Workbench to edit mode.
- `wb_open_resource` currently exposes no caller-configurable timeout; its bridge call may report its built-in 10-second timeout while the editor continues loading. Confirm actual loading from Workbench state/logs after the 5-second wait instead of immediately retrying or layering another world into the current session.
- Workbench Net API is permanently enabled. If the EnfusionMCP bridge cannot connect, first check Game/Workbench script compilation: a Game-script error prevents the EnfusionMCP handler addon from loading and can leave the bridge unavailable even while the Workbench process and its Net API port are running. Only after compilation is clean investigate Workbench startup or handler-addon loading; do not ask to enable Net API.
- Inspect the Workbench/runtime log for the diagnostic prefixes `[ME_DEBUG_AVSP]`, `[ME_DEBUG_AVSP_POS]`, `[ME_DEBUG_AVSP_SYS]`, and `[ME_DEBUG_AVSP_GM]`.
- Workbench creates one timestamped log directory per launch at `C:\Users\Phil\Documents\My Games\ArmaReforgerWorkbench\logs\logs_YYYY-MM-DD_HH-MM-SS\`. Use `script.log` for diagnostic `Print` output, `error.log` for compilation/runtime errors, and `console.log` for complete engine context.
- Use Git for repository inspection, e.g. `git status` and `git diff`.

## Reference sources

- Vanilla scripts: `https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff`
- Arma Reforger Script API: `https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/`

## Architecture

The current addon is diagnostic instrumentation for the base-game ambient vehicle spawning feature, not a standalone spawning system.

- `Scripts/Game/Systems/ME_DebugAmbientVehicleSystem.c` mods `SCR_AmbientVehicleSystem`. It preserves base behavior by calling `super` while logging edit-mode enablement, initialization, spawn-point registration, and the before/after state of each `ProcessSpawnpoint` call.
- `Scripts/Game/GameMode/ME_DebugBaseGameMode.c` mods `SCR_BaseGameMode` and logs state transitions, game-loop start, and player creation with `[ME_DEBUG_AVSP_GM]`.
- `Scripts/Game/Components/Locations/ME_DebugAmbientVehicleSpawnPointComponent.c` mods `SCR_AmbientVehicleSpawnPointComponent`. After the base component initializes, it logs terrain-position probing and ambient-system/spawn-point registration state.
- `worlds/MP/MpTest/ME_MpTest.ent` is a subscene inheriting the base-game `MpTest` world. Its `ME_MpTest_Layers/default.layer` places the base-game US ambient-vehicle spawn-point prefab used to exercise the instrumented classes.

## Confirmed diagnostic result and next hook

`ME_MpTest` достигает `GAME` с `IsRunning()=true` и `IsMaster()=true`, создаёт локального игрока и инициализирует включённую ambient-систему с одной точкой. Первый `OnUpdatePoint` фиксирует `enabled=1, spawnVehicles=0`; после единственного `super.OnUpdatePoint(args)` система имеет `enabled=0`. Это подтверждает ванильный guard `!GetGame().AreGameFlagsSet(EGameFlags.SpawnVehicles)`.

Источник флага определён: `EGameFlags.SpawnVehicles = 2`, а `SpawnAI = 4`, поэтому `m_eTestGameFlags 6` в `Prefabs/MP/Modes/Conflict/GameMode_Campaign.et` означает `SpawnVehicles | SpawnAI`. В Workbench `SCR_BaseGameMode.EOnInit()` вызывает `GetGame().SetGameFlags(m_eTestGameFlags, false)`, пока флаги не получены. `GameMode_Plain.et`, используемый `ME_MpTest`, не переопределяет это поле; базовый `[Attribute("0", ...)]` задаёт `0`, так что `SpawnVehicles` не устанавливается. Не добавлять override `Enable(bool)`: он конфликтует с native-декларацией и ломает компиляцию.

В чистой `CTI_Campaign_Eden` `SCR_AmbientVehicleSystem` также выводит `InitInfo=... {}`: `WorldSystemInfo.ToString()` не раскрывает конфигурацию update-point, поэтому по этой строке нельзя делать выводы о различиях регистрации. Чистый baseline подтверждён: `SCR_GameModeCampaign` инициализирует 171 точку, а первый `OnUpdatePoint` имеет `enabled=1, spawnVehicles=1`; после `super` оба значения остаются `1`, и сразу вызывается `ProcessSpawnpoint` (индексы 0, 1, 2…). Для нового сравнения обязательно открывать campaign в чистой сессии по процедуре выше; не загружать её поверх `ME_MpTest`, иначе spawn-point и game-mode сущности смешиваются.

## Editing resources and overrides

- Keep `super` calls in the modded overrides unless the intended change explicitly replaces base-game behavior; the existing overrides are intended to observe, not alter, the ambient spawning flow.
- Retain the existing log-prefix families when adding related diagnostics so Workbench output remains filterable.
- The test world and layer refer to base-game resources by GUID. Prefer editing `.ent` and `.layer` resources in Workbench rather than manually changing serialization or generated resource metadata.
- Treat `resourceDatabase.rdb` as Workbench-managed metadata; do not hand-edit it unless there is a specific, verified reason.
