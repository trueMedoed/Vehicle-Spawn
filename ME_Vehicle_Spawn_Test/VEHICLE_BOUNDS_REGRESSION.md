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

Перед записью canonical aggregate resource генератор строго проверяет grouped schema v2 VBT (`faction → basic vehicle type → prefab`):

- schema version `2`, generator version и fixture identity VBT;
- совпадение `m_sGameVersion` с текущей сборкой игры;
- непустые, уникальные и лексикографически отсортированные faction groups;
- непустые, уникальные и лексикографически отсортированные basic-type groups внутри каждой faction;
- непустые и отсортированные canonical prefab paths внутри каждой type group;
- глобальную уникальность prefab: один canonical prefab встречается ровно один раз и получает faction/basic type только из родительских групп;
- конечные и упорядоченные `mins`/`maxs` каждой prefab entry;
- точное общее количество `146` prefab;
- точное наличие каждого выбранного Test catalog prefab;
- строгое совпадение его catalog faction и единственной basic classification с родительскими VBT groups:
  - `VEHICLE_CAR`;
  - `VEHICLE_HELICOPTER`;
  - `VEHICLE_AIRPLANE`;
  - `VEHICLE_APC`;
  - `VEHICLE_TRUCK`;
  - `VEHICLE_TURRET`.

Повторяющиеся в catalog entry экземпляры одной и той же basic label сначала семантически дедуплицируются. Ноль или несколько разных basic classifications приводят к `FAIL`. Lookup выполняется только по точному canonical `ResourceName`, полученному из `SCR_EntityCatalogEntry.GetPrefab()`, и сохраняет parent faction/type context для проверки membership.

### Test aggregate

Test сохраняет aggregate schema v5 в единственный canonical resource:

```text
{1C3AE4A8F2630BF7}Configs/Generated/ME_VehicleBoundsSnapshot.conf
```

| Файл | Назначение | Кто изменяет |
| --- | --- | --- |
| `Configs/Generated/ME_VehicleBoundsSnapshot.conf` | Canonical aggregate для preview/regression | Генератор после успешной полной проверки |
| `Scripts/WorkbenchGame/WorldEditor/ME_GenerateVehicleBoundsSnapshotPlugin.c` | VBT-backed агрегация, валидация, сериализация и reload-validation | Разработчик |
| `worlds/TestCases/ME_VehicleBoundsSnapshot.ent` | Минимальная Test-сцена с ambient spawn-point filters | Разработчик через Workbench |

Отдельного staged resource нет: историю и review изменений обеспечивает Git. Существующий filename, GUID и `.meta` canonical resource должны сохраняться.

Генератор использует существующий Test resolver `ME_GetEditorVehicleAggregateSelection(...)` и фактические labels каждого `SCR_EntityCatalogEntry`. В aggregate входят все labels, имя которых содержит `VEHICLE_`, включая traits, например `TRAIT_VEHICLE_REARMING`. VBT хранит только шесть basic classifications, поэтому trait labels берутся из каталога, а их bounds — из соответствующей VBT per-prefab записи.

Для каждой aggregate-группы сохраняются:

- faction key один раз в объекте faction-группы;
- label/type техники только во вложенной aggregate-записи;
- объединённые локальные `mins` и `maxs`;
- количество уникальных prefab-кандидатов;
- canonical prefab path, определивший каждый из шести экстремумов.

Корневой `m_aFactions` отсортирован лексикографически по `m_sFactionKey`, а `m_aEntries` внутри каждой группы — по `m_sVehicleType`. Имена сериализованных faction-контейнеров равны `<FactionKey>` (например, `CIV`), а имена вложенных entry-контейнеров — только `<VehicleType>` (например, `VEHICLE_CAR` или `TRAIT_VEHICLE_REARMING`). Faction names уникальны глобально, entry names уникальны внутри своей faction-группы; фиксированный список faction keys не используется.

После записи генератор reload-validates оба уровня имён, порядок и полное field-by-field содержимое schema v5. При равных extrema provenance выбирается лексикографически.

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

### 2. Создать canonical Test aggregate snapshot

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

Успешный запуск сохраняет и reload-validates canonical resource, затем завершается строкой:

```text
[ME_DEBUG_AVSP_WB] bounds_snapshot status=PASS source=VBT resource=Configs/Generated/ME_VehicleBoundsSnapshot.conf count=17 memberships=150
```

Перед итоговой строкой выводится одна `bounds_snapshot_aggregate` запись для каждой группы с count, bounds и шестью provenance paths.

### 3. Проверить изменения через Git

Просмотрите Git diff единственного aggregate resource:

```text
ME_Vehicle_Spawn_Test/Configs/Generated/ME_VehicleBoundsSnapshot.conf
```

Для каждой изменённой группы проверьте:

1. имя faction-контейнера `<FactionKey>`, единственный `m_sFactionKey` группы и имя вложенного entry-контейнера `<VehicleType>`;
2. candidate count;
3. `mins` и `maxs`;
4. все шесть provenance prefab paths;
5. ожидаемость изменений каталогов, фильтров или VBT bounds;
6. наличие trait-групп, если соответствующие catalog labels существуют.

`PASS` означает, что записанный canonical snapshot внутренне согласован с текущим grouped VBT Candidate v2 и Test filters и успешно загружен обратно с теми же именами и полями. VBT Candidate перед агрегацией отдельно проверяется на трёхуровневый порядок, глобальную prefab uniqueness и точное совпадение parent faction/basic type с Test catalog selection. Принятие или отклонение получившегося Git diff остаётся явным действием разработчика.

### 4. Проверить canonical aggregate

1. Повторно запустите Test generator без промежуточных изменений.
2. Убедитесь, что второй запуск не создаёт нового Git diff.
3. Проверьте preview/warning plugins на том же canonical aggregate resource.
4. Если изменение отклонено, восстановите canonical `.conf` через Git; не создавайте второй staged resource.
5. Не заменяйте и не копируйте `.meta`: canonical resource сохраняет filename и GUID `1C3AE4A8F2630BF7`.

## Значение `FAIL`

Проверки входных данных и aggregate-модели завершают работу до записи canonical resource. Ошибка сохранения или reload-validation выводит `reason=...` уже после попытки записи; в этом случае проверьте diff и восстановите canonical `.conf` через Git. Основные категории:

- VBT Candidate отсутствует, не десериализуется, не соответствует grouped schema v2 или имеет неверную metadata;
- версия игры VBT Candidate устарела;
- VBT hierarchy не отсортирована, содержит пустую группу, недопустимый basic type, неверные bounds, duplicate prefab или не ровно `146` entries;
- canonical prefab из Test catalog отсутствует в VBT;
- parent faction membership не совпадает;
- catalog entry не имеет ровно одной семантически уникальной basic classification либо она не совпадает с parent VBT type;
- Test spawn point или его aggregate selection недоступны;
- candidate counts, reverse coverage, bounds или provenance не прошли проверку;
- canonical resource не сохранился, потерял читаемые faction/entry names или не прошёл двухуровневую field-by-field reload-validation.

Не обходите такие проверки fallback-логикой и не принимайте частичный результат.

## Проверка детерминированности

После изменения VBT Candidate, Test filters или генератора выполните Test generator два раза без промежуточных изменений.

После второго запуска:

- canonical `.conf` не должен получать новый Git diff;
- faction names должны остаться уникальными и соответствовать `<FactionKey>`, а entry names — быть уникальными внутри группы и соответствовать `<VehicleType>`;
- порядок faction groups и entries внутри каждой группы должен остаться лексикографическим;
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
7. Убедитесь, что canonical aggregate сохраняет GUID `1C3AE4A8F2630BF7`, а staged resource отсутствует.
8. Если Workbench запускался через EnfusionMCP, удалите временные handler scripts:

   ```text
   wb_cleanup(modDir: "C:\Users\Phil\Documents\GitHub\Mods\Vehicle Spawn\ME_Vehicle_Spawn_Test")
   ```

`resourceDatabase.rdb` является Workbench-managed metadata; не редактируйте его вручную.
