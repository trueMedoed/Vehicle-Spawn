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

Vehicle-bounds workflow разделён по ответственности:

1. `ME_Vehicle_Bounds_Toolkit` измеряет prefab и проверяет свой per-prefab Candidate/Baseline.
2. `ME_Vehicle_Spawn_Test` читает проверенный VBT Candidate, применяет реальные ambient spawn-point filters и создаёт aggregate staged snapshot для preview.
3. После изучения staged/published diff aggregate payload принимается вручную с сохранением published filename, GUID и `.meta`.

VBT является единственным владельцем per-prefab regression. Test владеет aggregate filter/preview contract. Production `ME_Vehicle_Spawn` не зависит от VBT.

Подробный процесс описан в `ME_Vehicle_Spawn_Test/VEHICLE_BOUNDS_REGRESSION.md`.
