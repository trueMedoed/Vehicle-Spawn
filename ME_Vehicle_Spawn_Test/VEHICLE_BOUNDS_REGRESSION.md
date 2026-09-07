# Регрессионная проверка границ техники

Этот документ описывает ручной процесс проверки границ каждого prefab техники после обновления Arma Reforger или изменения каталогов/fixture в `ME_Vehicle_Spawn_Test`.

## Зачем нужны два вида snapshot

Генератор создаёт два независимых представления одних измерений:

- aggregate snapshot группирует консервативные границы по `faction + vehicle type` и используется существующим editor-preview;
- per-prefab snapshot хранит отдельные границы каждого канонического prefab и позволяет заметить изменение модели, даже если она не определяла ни один экстремум aggregate envelope.

Per-prefab контур является только регрессионной проверкой Test addon. Он не участвует в runtime production addon.

## Основные файлы

| Файл | Назначение | Кто изменяет |
| --- | --- | --- |
| `Configs/Generated/ME_VehicleBoundsPerPrefabBaseline.conf` | Последняя вручную принятая эталонная версия каждого prefab | Только человек после проверки изменений |
| `Configs/Generated/ME_VehicleBoundsPerPrefabCandidate.conf` | Результат текущего запуска генератора | Генератор перезаписывает автоматически |
| `Configs/Generated/ME_VehicleBoundsSnapshot_Staged.conf` | Текущий aggregate-v4 snapshot для проверки и последующего переноса | Генератор перезаписывает автоматически |
| `Scripts/Game/Configs/ME_VehicleBoundsPerPrefabSnapshot.c` | Test-only схема per-prefab snapshot | Разработчик при осознанном изменении schema |
| `Scripts/WorkbenchGame/WorldEditor/ME_GenerateVehicleBoundsSnapshotPlugin.c` | Измерение, валидация, сериализация и сравнение | Разработчик |
| `worlds/TestCases/ME_VehicleBoundsSnapshot.ent` | Fixture со всеми измеряемыми prefab | Разработчик через Workbench |

У Baseline и Candidate разные `.meta` и GUID. При принятии новой Baseline заменяйте **только содержимое `.conf`**, но не копируйте и не заменяйте `.meta`.

## Что хранится в per-prefab snapshot

Корневая запись содержит:

- версию schema и генератора;
- identity fixture;
- версию игры из `GetGame().GetBuildVersion()`;
- признак доступности classification metadata.

Каждая запись prefab содержит:

- канонический resource path prefab;
- локальные `mins` и `maxs` относительно неповёрнутого marker root;
- отсортированный набор faction keys;
- отсортированный набор фактических vehicle-type labels: `VEHICLE_CAR`, `VEHICLE_HELICOPTER`, `VEHICLE_AIRPLANE`, `VEHICLE_APC`, `VEHICLE_TRUCK` или `VEHICLE_TURRET`.

Trait labels, например `TRAIT_VEHICLE_REARMING`, не являются per-prefab vehicle types. При этом существующая aggregate-v4 логика может сохранять такие labels как отдельные aggregate-группы; это намеренное сохранение её прежнего поведения.

## Обычный запуск проверки

1. Запустите Arma Reforger Workbench с:

   ```text
   ME_Vehicle_Spawn_Test/addon.gproj
   ```

2. Откройте ресурс:

   ```text
   worlds/TestCases/ME_VehicleBoundsSnapshot.ent
   ```

3. Дождитесь полной загрузки мира.

4. Выполните меню:

   ```text
   Plugins → ME_Vehicle_Spawn → Vehicle Bounds → Generate bounds snapshots
   ```

5. Откройте свежий каталог логов:

   ```text
   C:\Users\Phil\Documents\My Games\ArmaReforgerWorkbench\logs\logs_YYYY-MM-DD_HH-MM-SS\
   ```

6. Проверьте `script.log` по префиксу:

   ```text
   [ME_DEBUG_AVSP_WB]
   ```

7. Проверьте `error.log` на ошибки компиляции, ресурсов и выполнения Test addon.

Успешный запуск должен включать обе итоговые строки:

```text
[ME_DEBUG_AVSP_WB] bounds_snapshot status=PASS ...
[ME_DEBUG_AVSP_WB] bounds_per_prefab_compare status=PASS|DIFF ... candidate_written=1
```

## Значение результатов

### `PASS`

Candidate успешно создан, повторно загружен и проверен. Между Baseline и Candidate нет сравнимых semantic differences.

Разница `baseline_game_version` и `candidate_game_version` выводится как контекст и сама по себе не создаёт `DIFF`: сравнение разных версий игры является назначением механизма.

### `DIFF`

Candidate успешно записан и проверен, но обнаружены реальные различия. Это не ошибка генерации и не означает, что Baseline нужно заменить автоматически.

Перед итоговой строкой будут выведены детали:

```text
[ME_DEBUG_AVSP_WB] bounds_per_prefab_diff kind=ADDED prefab=...
[ME_DEBUG_AVSP_WB] bounds_per_prefab_diff kind=REMOVED prefab=...
[ME_DEBUG_AVSP_WB] bounds_per_prefab_diff kind=CHANGED prefab=... bounds=... factions=... vehicle_types=...
```

- `ADDED` — prefab появился в Candidate, но отсутствует в Baseline;
- `REMOVED` — prefab присутствует в Baseline, но больше не входит в Candidate;
- `CHANGED bounds=1` — изменились локальные границы;
- `CHANGED factions=1` — изменился набор фракций;
- `CHANGED vehicle_types=1` — изменился набор типов техники.

### `FAIL`

Произошла ошибка API, fixture coverage, модели, сериализации, reload-validation либо загрузки Baseline. Найдите `reason=...` в строке `bounds_per_prefab_compare status=FAIL` или `bounds_snapshot status=FAIL` и исправьте причину до принятия результатов.

Если Candidate уже был успешно записан, а последующая загрузка/проверка Baseline завершилась ошибкой, в сообщении будет `candidate_written=1`. Такой Candidate остаётся на диске для диагностики, но не должен приниматься до устранения `FAIL`.

## Как анализировать `DIFF`

Для каждого сообщения `bounds_per_prefab_diff`:

1. Убедитесь, что изменение соответствует ожидаемому обновлению игры, каталога или fixture.
2. Для `ADDED` проверьте, что новый prefab действительно должен входить в ambient vehicle catalog и имеет ровно один marker root в fixture.
3. Для `REMOVED` проверьте, что prefab действительно удалён или больше не должен входить в фильтр каталога, а marker root не был удалён случайно.
4. Для изменения bounds проверьте prefab в Workbench и убедитесь, что изменение геометрии/дочерних сущностей ожидаемо.
5. Для изменения factions или vehicle types проверьте catalog labels и faction catalogs.
6. Не принимайте Baseline при необъяснённом различии.

Генератор требует точного one-to-one покрытия между:

- каноническими prefab paths из отфильтрованных каталогов;
- marker roots в fixture;
- выполненными измерениями;
- записями Candidate.

Дубликат, отсутствующий или лишний marker приводит к `FAIL`, а не к неполному Candidate.

## Как вручную принять Candidate

Принимайте Candidate только после проверки всех различий и подтверждения нового поддерживаемого состояния игры/fixture.

1. Убедитесь, что итог генерации — `PASS` или ожидаемый `DIFF`, но не `FAIL`.
2. Просмотрите Git diff для:

   ```text
   ME_Vehicle_Spawn_Test/Configs/Generated/ME_VehicleBoundsPerPrefabCandidate.conf
   ```

3. Скопируйте содержимое Candidate `.conf` в:

   ```text
   ME_Vehicle_Spawn_Test/Configs/Generated/ME_VehicleBoundsPerPrefabBaseline.conf
   ```

4. Не изменяйте и не копируйте:

   ```text
   ME_VehicleBoundsPerPrefabBaseline.conf.meta
   ME_VehicleBoundsPerPrefabCandidate.conf.meta
   ```

5. Дайте Workbench обновить ресурс, затем повторно выполните генератор.
6. Убедитесь, что итоговое сравнение стало `status=PASS`, а Candidate по-прежнему проходит reload-validation.
7. Просмотрите итоговый Git diff и зафиксируйте Baseline вместе с осмысленным объяснением принятого изменения.

В генератор нельзя добавлять автоматическое копирование Candidate в Baseline. Разделение этих ресурсов специально делает принятие изменений явным действием человека.

Если semantic differences отсутствуют, обновлять Baseline только из-за другой `game_version` необязательно. Это можно сделать при формальном принятии новой поддерживаемой версии игры, чтобы Baseline отражала последний проверенный build.

## Legacy Baseline

Исходная Baseline импортирована из исторического v2 snapshot без повторного измерения. Поэтому она содержит:

```text
m_sGameVersion "unknown-legacy"
m_bClassificationMetadataAvailable 0
```

Для неё comparator полностью проверяет prefab path coverage и bounds, но явно пропускает factions и vehicle types:

```text
classification=skipped_legacy_metadata_unavailable
```

После первого ручного принятия современной Candidate Baseline получит известную версию игры и полные classification metadata. Последующие сравнения будут проверять bounds, factions и vehicle types.

## Проверка детерминированности

После изменения генератора, fixture или каталогов выполните команду генератора два раза без промежуточных изменений.

После второго запуска:

- Candidate не должен получать новый Git diff;
- aggregate staged snapshot не должен получать новый Git diff;
- итоговые counts и diagnostics должны совпадать.

Генератор сортирует prefab paths и classification arrays и назначает стабильные имена сериализованным entry containers. Поэтому одинаковое состояние fixture должно создавать байт-в-байт одинаковые `.conf`.

## Проверка перед commit или публикацией

1. Выполните генератор минимум два раза и подтвердите детерминированность.
2. Проверьте свежие `script.log` и `error.log`.
3. Запустите Test-addon validation:

   ```text
   mod_validate(projectPath: "C:\Users\Phil\Documents\GitHub\Mods\Vehicle Spawn\ME_Vehicle_Spawn_Test")
   ```

4. Проверьте whitespace:

   ```bash
   git diff --check
   ```

5. Убедитесь, что изменения относятся к `ME_Vehicle_Spawn_Test`, а production addon не был затронут случайно.
6. Если Workbench запускался через EnfusionMCP, удалите временные handler scripts:

   ```text
   wb_cleanup(modDir: "C:\Users\Phil\Documents\GitHub\Mods\Vehicle Spawn\ME_Vehicle_Spawn_Test")
   ```

Не редактируйте `resourceDatabase.rdb` вручную: это Workbench-managed metadata.
