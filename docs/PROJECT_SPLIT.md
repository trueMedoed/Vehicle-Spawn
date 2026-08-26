# Project split

## Addons

| Addon | Назначение | GUID |
|---|---|---|
| `ME_Vehicle_Spawn` | production editor tooling | `6A30CC9A0322E1B9` |
| `ME_Vehicle_Spawn_Test` | diagnostics and experiments | `B7E4D91C6A2F5083` |

Оба проекта сохраняют dependency `58D0FB3206B6F859` на base game. ID и TITLE совпадают с именами каталогов. Addons нельзя подключать одновременно: Test содержит runtime `modded class` overrides, а production должен оставаться tooling-only.

## Production contents

- `ME_DebugAmbientVehicleSpawnPointComponent.c` с editor-only marker-визуализацией;
- `ME_AmbientVehicleSpawnPointWarningPlugin.c`;
- `ME_AmbientVehicleSpawnPointPreviewController.c`;
- четыре файла string table из `Strings/`;
- `ME_MpTest_BasicSpawnVehicles.ent`, его `.meta` и layer directory.

Production не содержит diagnostic ambient system/base game mode overrides, `ME_MpTest.ent`, TestCain и Cain Broken worlds. `resourceDatabase.rdb` намеренно не копируется.

## Test contents

Test сохраняет полный текущий runtime diagnostic набор, экспериментальные миры и Workbench-managed database. Переименование каталога не должно менять незакоммиченные файлы.

## GUID и metadata

Каждый addon имеет собственный GUID. String-table references и localization IDs production не изменяются. `.ent` и `.layer` следует редактировать через Workbench. Production resources нужно зарегистрировать/rebuild в Workbench, чтобы database была создана заново; старый `resourceDatabase.rdb` не переносится. `.meta` демонстрационного мира должна иметь `Name`, соответствующий фактическому resource path.

Цвет Shape показывает только edit-world clearance preflight и не является гарантией runtime spawn.
