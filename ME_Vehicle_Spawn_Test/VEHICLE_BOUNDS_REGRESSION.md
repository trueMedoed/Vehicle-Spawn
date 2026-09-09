# Регрессионная проверка границ техники

Этот документ описывает разделённый workflow vehicle bounds для `ME_Vehicle_Bounds_Toolkit` и `ME_Vehicle_Spawn_Test`.

## Ответственность addon-проектов

- `ME_Vehicle_Bounds_Toolkit` (VBT) — единственный владелец измерений и регрессии **per-prefab bounds**. Он содержит эталонную сцену со всеми prefab, создаёт Candidate и сравнивает его со своим вручную принятым Baseline.
- `ME_Vehicle_Spawn_Test` — потребитель VBT Candidate. Он применяет реальные фильтры ambient spawn points и создаёт только **aggregate snapshot** по `faction + label` для проверки editor preview.
- `ME_Vehicle_Spawn` production — использует вручную принятый aggregate snapshot и не зависит от VBT.

Test не измеряет prefab, не содержит собственной per-prefab schema, Candidate/Baseline или marker fixture. Зависимость от VBT объявлена только в `ME_Vehicle_Spawn_Test/addon.gproj`.

## Контракт данных

### VBT Candidate

Test загружает зарегистрированный ресурс:

```text
{0F8D7A7D004E2D06}Configs/Generated/ME_VBT_VehicleBoundsPerPrefabCandidate.conf
```

Перед записью aggregate staged resource генератор строго проверяет:

- schema, generator version и fixture identity VBT;
- совпадение `m_sGameVersion` с текущей сборкой игры;
- непустые, уникальные и лексикографически отсортированные canonical prefab paths;
- конечные и упорядоченные `mins`/`maxs`;
- отсортированные уникальные faction keys и basic vehicle types;
- точное наличие каждого выбранного catalog prefab;
- соответствие faction и всех шести basic classifications:
  - `VEHICLE_CAR`;
  - `VEHICLE_HELICOPTER`;
  - `VEHICLE_AIRPLANE`;
  - `VEHICLE_APC`;
  - `VEHICLE_TRUCK`;
  - `VEHICLE_TURRET`.

Lookup выполняется только по точному canonical `ResourceName`, полученному из `SCR_EntityCatalogEntry.GetPrefab()`.

### Test aggregate

Test сохраняет aggregate schema v4 в два независимых ресурса:

| Файл | Назначение | Кто изменяет |
| --- | --- | --- |
| `Configs/Generated/ME_VehicleBoundsSnapshot_Staged.conf` | Результат текущего запуска для проверки | Генератор перезаписывает автоматически |
| `Configs/Generated/ME_VehicleBoundsSnapshot.conf` | Вручную принятый aggregate для preview/regression | Только человек после проверки изменений |
| `Scripts/WorkbenchGame/WorldEditor/ME_GenerateVehicleBoundsSnapshotPlugin.c` | VBT-backed агрегация, валидация, сериализация и reload-validation | Разработчик |
| `worlds/TestCases/ME_VehicleBoundsSnapshot.ent` | Минимальная Test-сцена с ambient spawn-point filters | Разработчик через Workbench |

Published и staged `.conf` имеют разные `.meta` и GUID. При принятии нового aggregate заменяйте только payload published `.conf`; не копируйте и не заменяйте `.meta`.

Генератор использует существующий Test resolver `ME_GetEditorVehicleAggregateSelection(...)` и фактические labels каждого `SCR_EntityCatalogEntry`. В aggregate входят все labels, имя которых содержит `VEHICLE_`, включая traits, например `TRAIT_VEHICLE_REARMING`. VBT хранит только шесть basic classifications, поэтому trait labels берутся из каталога, а их bounds — из соответствующей VBT per-prefab записи.

Для каждой aggregate-группы сохраняются:

- faction key и label;
- объединённые локальные `mins` и `maxs`;
- количество уникальных prefab-кандидатов;
- canonical prefab path, определивший каждый из шести экстремумов.

При равных extrema provenance выбирается лексикографически. Записи и сериализованные контейнеры создаются детерминированно.

## Обычный workflow

### 1. Обновить и проверить VBT Candidate

1. Откройте `ME_Vehicle_Bounds_Toolkit/addon.gproj` в Workbench.
2. Откройте `Worlds/ME_VBT_VehicleBoundsFixture.ent`.
3. Выполните через `Plugins > ME Vehicle Bounds Toolkit`:
   1. `VBT: Preflight fixture names`;
   2. `VBT: Validate fixture coverage`;
   3. `VBT: Generate bounds snapshot`.
4. Проверьте сообщения `[ME_VBT_WB]` в `script.log`.
5. Изучите Candidate/Baseline diff VBT. Не переходите к Test при необъяснённом `DIFF` или любом `FAIL`.
6. Если изменение ожидаемо, примите VBT Baseline по инструкции VBT, сохранив filename, GUID и `.meta` Baseline.
7. Повторите генерацию VBT и получите `snapshot_compare status=PASS`.

Источник данных для следующего этапа:

```text
ME_Vehicle_Bounds_Toolkit/Configs/Generated/ME_VBT_VehicleBoundsPerPrefabCandidate.conf
```

### 2. Создать Test aggregate staged snapshot

1. Запустите Workbench с `ME_Vehicle_Spawn_Test/addon.gproj` и доступным addon `ME_Vehicle_Bounds_Toolkit`.
2. Откройте:

   ```text
   worlds/TestCases/ME_VehicleBoundsSnapshot.ent
   ```

3. Дождитесь полной загрузки мира и каталогов.
4. Выполните:

   ```text
   Plugins → ME_Vehicle_Spawn → Vehicle Bounds → Generate bounds snapshots
   ```

5. Проверьте свежие `script.log` и `error.log` в:

   ```text
   C:\Users\Phil\Documents\My Games\ArmaReforgerWorkbench\logs\logs_YYYY-MM-DD_HH-MM-SS\
   ```

Успешный запуск завершается строкой:

```text
[ME_DEBUG_AVSP_WB] bounds_snapshot status=PASS source=VBT stage=Configs/Generated/ME_VehicleBoundsSnapshot_Staged.conf count=... memberships=...
```

Перед итоговой строкой выводится одна `bounds_snapshot_aggregate` запись для каждой группы с count, bounds и шестью provenance paths.

### 3. Проверить staged versus published

Просмотрите Git diff:

```text
ME_Vehicle_Spawn_Test/Configs/Generated/ME_VehicleBoundsSnapshot_Staged.conf
ME_Vehicle_Spawn_Test/Configs/Generated/ME_VehicleBoundsSnapshot.conf
```

Для каждой изменённой группы проверьте:

1. faction и label;
2. candidate count;
3. `mins` и `maxs`;
4. все шесть provenance prefab paths;
5. ожидаемость изменений каталогов, фильтров или VBT bounds;
6. наличие trait-групп, если соответствующие catalog labels существуют.

`PASS` означает, что staged snapshot внутренне согласован с текущим VBT Candidate и Test filters. Он не означает автоматического принятия отличий от published snapshot.

### 4. Вручную принять aggregate

1. Принимайте staged payload только после полного изучения diff.
2. Замените содержимое:

   ```text
   Configs/Generated/ME_VehicleBoundsSnapshot.conf
   ```

   проверенным содержимым:

   ```text
   Configs/Generated/ME_VehicleBoundsSnapshot_Staged.conf
   ```

3. Сохраните published filename, GUID и существующий `.meta`.
4. Дайте Workbench rebuild/reload ресурса.
5. Повторно запустите Test generator и проверьте preview/warning plugins на принятом aggregate.
6. Не добавляйте автоматическое копирование staged в published: принятие должно оставаться явным ручным действием.

## Значение `FAIL`

При любой ненадёжности генератор завершает работу до записи staged resource и выводит `reason=...`. Основные категории:

- VBT Candidate отсутствует, не десериализуется или имеет неверную metadata;
- версия игры VBT Candidate устарела;
- canonical prefab из Test catalog отсутствует в VBT;
- faction membership не совпадает;
- basic vehicle classifications не совпадают;
- Test spawn point или его aggregate selection недоступны;
- candidate counts, reverse coverage, bounds или provenance не прошли проверку;
- staged resource не сохранился или не прошёл field-by-field reload-validation.

Не обходите такие проверки fallback-логикой и не принимайте частичный результат.

## Проверка детерминированности

После изменения VBT Candidate, Test filters или генератора выполните Test generator два раза без промежуточных изменений.

После второго запуска:

- staged `.conf` не должен получать новый Git diff;
- итоговые group и membership counts должны совпадать;
- aggregate diagnostics должны совпадать;
- оба запуска должны завершиться `status=PASS source=VBT`.

## Проверка перед commit или публикацией

1. Подтвердите VBT `snapshot_compare status=PASS` и детерминированность его Candidate.
2. Подтвердите два последовательных Test `bounds_snapshot status=PASS source=VBT`.
3. Проверьте свежие `script.log` и `error.log`.
4. Запустите validation отдельно для Test и VBT.
5. Проверьте whitespace:

   ```bash
   git diff --check
   ```

6. Убедитесь, что production `ME_Vehicle_Spawn` и его dependency graph не изменены.
7. Убедитесь, что published aggregate `.meta` не изменён и staged не был принят автоматически.
8. Если Workbench запускался через EnfusionMCP, удалите временные handler scripts:

   ```text
   wb_cleanup(modDir: "C:\Users\Phil\Documents\GitHub\Mods\Vehicle Spawn\ME_Vehicle_Spawn_Test")
   ```

`resourceDatabase.rdb` является Workbench-managed metadata; не редактируйте его вручную.
