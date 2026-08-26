# Vehicle Spawn

Репозиторий разделён на два взаимоисключающих addon-проекта:

- `ME_Vehicle_Spawn` — production addon с editor-only подсказками и preview для ambient vehicle spawn points.
- `ME_Vehicle_Spawn_Test` — диагностический addon со всеми runtime override, логированием и экспериментальными мирами.

Не подключайте оба addon в одну игровую конфигурацию: оба могут содержать `modded class` для vanilla-классов, что создаёт конфликт override.

## Production

Демонстрационный мир: `ME_Vehicle_Spawn/worlds/MP/MpTest/ME_MpTest_BasicSpawnVehicles.ent`.

Краткий workflow:

1. Откройте production `addon.gproj` в Workbench и дождитесь resource scan.
2. Зарегистрируйте/rebuild resources и перезагрузите scripts.
3. Откройте демонстрационный мир, проверьте GameMode, FactionManager, флаг Spawn Vehicles и ambient spawn point.
4. В Edit mode используйте `Check ambient vehicle spawning`, затем войдите в Game mode.

Цвет Shape — только edit-world clearance preflight. Он не гарантирует будущий runtime spawn.

Подробности: `docs/PROJECT_SPLIT.md` и `docs/PRODUCTION_WORKFLOW.md`.

## Test

Открывайте `ME_Vehicle_Spawn_Test` отдельно от production. Диагностические значения, log prefixes и сравнение с campaign baseline описаны в `docs/TEST_DIAGNOSTICS.md`.
