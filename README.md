# Vehicle Spawn

Репозиторий разделён на два взаимоисключающих addon-проекта:

- `ME_Vehicle_Spawn` — production addon с editor-only подсказками и preview для ambient vehicle spawn points.
- `ME_Vehicle_Spawn_Test` — диагностический addon со всеми runtime override, логированием и экспериментальными мирами.

Не подключайте оба addon в одну игровую конфигурацию: оба могут содержать `modded class` для vanilla-классов, что создаёт конфликт override.

## Production

Демонстрационный мир: `ME_Vehicle_Spawn/worlds/ME_TestWorld.ent`.

Краткий workflow:

1. Откройте production `addon.gproj` в Workbench и дождитесь resource scan.
2. Зарегистрируйте/rebuild resources и перезагрузите scripts.
3. Откройте демонстрационный мир, проверьте GameMode, FactionManager, флаг Spawn Vehicles и ambient spawn point.
4. В Edit mode используйте `Check ambient vehicle spawning`, затем войдите в Game mode.

В Edit mode над загруженными точками отображаются полупрозрачные голограммы примеров техники и цветные метки фракций. Белая голограмма означает, что предварительная проверка не нашла проблемы; красная указывает на проблему фильтра, каталога, поиска свободного места или пересечение объёмных границ самих показанных голограмм. Простое пересечение областей появления и границ статических объектов остаётся предупреждением без смены цвета. Команды `Next vehicle hologram` и `Toggle vehicle hologram` находятся в Plugins → `[ME] Vehicle Spawn/Preview`.

Голограмма — только подсказка редактора, а не прогноз конкретной техники или гарантия runtime spawn.

Подробности: `docs/PROJECT_SPLIT.md` и `docs/PRODUCTION_WORKFLOW.md`.

Каноническое описание vanilla lifecycle, catalog filtering и границ editor preview: [`docs/AMBIENT_VEHICLE_SPAWNPOINT.md`](docs/AMBIENT_VEHICLE_SPAWNPOINT.md).

Тексты для страницы мода в Workshop хранятся в `docs/workshop/`: `DESCRIPTION.md` — публикуемое описание, `RU_DESCRIPTION.md` — его русский перевод для внутренней сверки, `CHANGELOG.md` — история версий. Обновляйте их вместе с публикацией новой версии.

## Test

Открывайте `ME_Vehicle_Spawn_Test` отдельно от production. Диагностические значения, log prefixes и сравнение с campaign baseline описаны в `docs/TEST_DIAGNOSTICS.md`.

Vehicle-bounds workflow разделён по ответственности:

1. `ME_Vehicle_Bounds_Toolkit` измеряет prefab и проверяет свой per-prefab Candidate/Baseline.
2. `ME_Vehicle_Spawn_Test` читает проверенный VBT Candidate, применяет реальные ambient spawn-point filters и генерирует единственный canonical aggregate snapshot для preview.
3. Генератор reload-validates этот resource, а изменения aggregate payload и их история проверяются через Git без отдельного staged-файла.

VBT является единственным владельцем per-prefab regression. Test владеет aggregate filter/preview contract. Production `ME_Vehicle_Spawn` не зависит от VBT.

Подробный процесс описан в `ME_Vehicle_Spawn_Test/VEHICLE_BOUNDS_REGRESSION.md`.


## Справочники каталогов в production

В Configs/Generated находится ME_EditableEntityLabelsSnapshot.conf (схема 2, каталоги CIV/FIA/US/USSR игры 1.8.0.13) и ME_VehicleBoundsSnapshot.conf (схема 6, включая prefab с наибольшей площадью для каждого типа техники). GUID production-конфига сохранён. Инструкция: [VEHICLE_CATALOG_REFERENCE.md](ME_Vehicle_Spawn/VEHICLE_CATALOG_REFERENCE.md). Справочники помогают читать фильтры и выбирать пример голограммы, но не заменяют текущий каталог и не гарантируют runtime spawn. Пользователь подтвердил проверку схемы 6 в Prod Workbench 05.10.2026.
