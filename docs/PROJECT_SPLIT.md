# Project split

## Addons

| Addon | Назначение | GUID |
|---|---|---|
| `ME_Vehicle_Spawn` | production editor tooling | `6A30CC9A0322E1B9` |
| `ME_Vehicle_Spawn_Test` | diagnostics and experiments | `B7E4D91C6A2F5083` |

Оба проекта сохраняют dependency `58D0FB3206B6F859` на base game. ID и TITLE совпадают с именами каталогов. Addons нельзя подключать одновременно: Test содержит runtime `modded class` overrides, а production должен оставаться tooling-only.

## Production contents

- `Scripts/Game/Components/Locations/ME_DebugAmbientVehicleSpawnPointComponent.c` с editor-only marker-визуализацией, предпросмотром габаритов и метками фильтра;
- `Scripts/Game/Systems/ME_DebugAmbientVehicleSystem.c`;
- `Scripts/Game/Faction/ME_DebugFactionCatalogInitialization.c` для editor-only доступа к каталогу;
- `Scripts/Game/Configs/ME_VehicleBoundsSnapshot.c` и `ME_VehicleBoundsSnapshotHelper.c`;
- `Configs/Generated/ME_VehicleBoundsSnapshot.conf` с агрегатами габаритов и его `.meta`;
- `Scripts/WorkbenchGame/WorldEditor/ME_AmbientVehicleSpawnPointWarningPlugin.c`;
- `worlds/ME_TestWorld.ent`, его `.meta` и `worlds/ME_TestWorld_Layers/`.

Production не содержит diagnostic ambient system/base game mode overrides, автотестов, генератора агрегатов и экспериментальных миров Test. Локализация плагина не используется: сообщения заданы строками в коде. `resourceDatabase.rdb` намеренно не копируется из Test.

## Test contents

Test сохраняет полный текущий runtime diagnostic набор, экспериментальные миры и Workbench-managed database. Переименование каталога не должно менять незакоммиченные файлы.

## GUID и metadata

Каждый addon имеет собственный GUID. `.ent` и `.layer` следует редактировать через Workbench. Production resources нужно зарегистрировать/rebuild в Workbench, чтобы database была создана заново; старый `resourceDatabase.rdb` не переносится. `.meta` демонстрационного мира должна иметь `Name`, соответствующий фактическому resource path.

Цвет Shape показывает только edit-world clearance preflight и не является гарантией runtime spawn.
