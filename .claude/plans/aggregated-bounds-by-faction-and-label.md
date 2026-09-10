# План: агрегированные bounds по faction key и vehicle type label

## Цель и границы

Заменить production snapshot с 146 записей `prefab -> bounds` на небольшой статический snapshot, где bounds агрегированы по паре `faction key + реальная vehicle-type label`. Production остаётся только runtime/editor reader и preview; fixture, marker-компонент, Workbench generator и staged-файл остаются в `ME_Vehicle_Spawn_Test` и не переносятся в production.

## Что подтверждено исследованием

- Production reader находится в `ME_Vehicle_Spawn/Scripts/Game/Configs/ME_VehicleBoundsSnapshotHelper.c`.
- Production schema находится в `ME_Vehicle_Spawn/Scripts/Game/Configs/ME_VehicleBoundsSnapshot.c`.
- Production envelope получает точный vanilla catalog result через `GetFullFilteredEntityListWithLabels` в `ME_Vehicle_Spawn/Scripts/Game/Components/Locations/ME_DebugAmbientVehicleSpawnPointComponent.c`, затем сейчас превращает его в prefab paths и агрегирует per-prefab entries.
- Оба текущих ресурса содержат 146 `ME_VehicleBoundsSnapshotEntry`; test snapshot не является отдельным источником истины.
- Test-only generator `ME_Vehicle_Spawn_Test/Scripts/WorkbenchGame/WorldEditor/ME_GenerateVehicleBoundsSnapshotPlugin.c` работает через fixture roots и marker-компонент `ME_Vehicle_Spawn_Test/Scripts/Game/Components/Locations/ME_VehicleBoundsFixtureMarkerComponent.c`; эти механизмы не должны попадать в production.
- Текущая фильтрация уже делегирует vanilla semantics: included/excluded labels и `m_bRequireAllIncludedLabels` передаются в `GetFullFilteredEntityListWithLabels`; пустой include означает полный каталог. Базовая диагностика подтверждает, что `TRAIT_ARMED` может быть унаследован в excluded и что пересечение include/exclude доказывает пустой результат только при `requireAll=true`.
- Надёжные faction keys: `US`, `USSR`, `CIV`, `FIA`; ключ берётся из affiliation: default key, затем affiliated/current key. Truck пока нельзя считать существующим типом без инвентаризации catalog labels.

## Предлагаемая compact schema

В `ME_Vehicle_Spawn/Scripts/Game/Configs/ME_VehicleBoundsSnapshot.c` заменить prefab entry на две вложенные записи:

```text
ME_VehicleBoundsSnapshot {
 m_iSchemaVersion 3
 m_sGeneratorVersion "catalog-aggregate-v3"
 m_sFixtureIdentity "ME_VehicleBoundsSnapshot"
 m_aFactions {
  ME_VehicleBoundsFactionEntry "..." {
   m_sFactionKey "US"
   m_aVehicleTypes {
    ME_VehicleBoundsTypeEntry "..." {
     m_sVehicleType "VEHICLE_CAR"
     m_vLocalMins ...
     m_vLocalMaxs ...
    }
   }
  }
 }
}
```

Практические решения:

1. Хранить `m_sFactionKey` и `m_sVehicleType` как `string`, а не Enforce enum. Это устойчивее для config serialization и позволяет добавить реально найденный новый label без изменения enum/schema-кода. Значение должно быть точным `typename.EnumToString(EEditableEntityLabel, label)`.
2. Группировать сначала по faction, затем по type: faction key повторяется один раз, а не в каждой type-записи. Ожидаемый размер — примерно 4 faction groups × число реально присутствующих vehicle types, а не 146 prefab records.
3. Не хранить prefab paths в production payload. `m_vLocalMins/maxs` — компонентный min/max union по всем fixture/catalog vehicles, попавшим в эту faction/type группу.
4. В root добавить при необходимости `m_aKnownVehicleTypes` не нужно: список типов выводится из entries; неизвестная label должна быть ошибкой генерации/валидации, а не silently ignored.
5. Уникальность: одна запись на faction key и type label; сортировка `m_aFactions` по key и nested types по label делает output детерминированным.

Если nested array окажется нестабильным в конкретной версии `BaseContainerTools`, fallback — плоская array `ME_VehicleBoundsSnapshotEntry { m_sFactionKey, m_sVehicleType, mins, maxs }`. Это менее компактно, но проще для Enforce serialization и lookup. Не менять schema до runtime reload-test.

## Helper API и reader

В `ME_Vehicle_Spawn/Scripts/Game/Configs/ME_VehicleBoundsSnapshotHelper.c`:

- Поднять schema/generator constants до version 3 / `catalog-aggregate-v3`; сохранить fixture identity только как provenance metadata, не использовать её как runtime dependency.
- Заменить prefab coverage validation на:
  - `ME_LoadSnapshot(out snapshot, out reason)`;
  - `ME_ValidateFactionGroups(...)`: root не null, arrays не null/empty;
  - `ME_ValidateFactionEntry(...)`: non-empty key, unique faction key;
  - `ME_ValidateTypeEntry(...)`: non-empty type, unique type per faction, finite ordered bounds;
  - lookup `ME_FindFactionEntry(snapshot, factionKey)` и `ME_FindTypeEntry(factionEntry, typeLabel)`;
  - общий `ME_UnionBounds(...)` с теми же NaN/order/absolute-limit checks.
- Добавить публичный основной метод примерно:
  `static bool ME_GetValidatedAggregateBounds(string factionKey, array<string> selectedTypeLabels, out vector mins, out vector maxs, out string reason)`.
  Он требует непустой faction key и непустой resolved type set, находит каждую группу, объединяет bounds, валидирует результат; отсутствие faction/type entry — hard failure с устойчивой причиной.
- Добавить `ME_GetValidatedBoundsForFilteredCatalog(string factionKey, array<SCR_EntityCatalogEntry> entries, out vector mins, out vector maxs, out string reason)` либо оставить catalog-to-label derivation в component. Предпочтительнее helper принимает уже полученные уникальные type names, чтобы production schema helper не зависел от catalog API.
- Не сохранять/не использовать prefab path lookup и не делать per-prefab completeness check.

## Selection semantics в component

В `ME_Vehicle_Spawn/Scripts/Game/Components/Locations/ME_DebugAmbientVehicleSpawnPointComponent.c` минимально изменить только envelope path:

1. Сохранить `ME_GetEditorVehicleEnvelopeCandidates`: faction resolution и точный вызов vanilla `GetFullFilteredEntityListWithLabels(entries, included, excluded, requireAll)`.
2. Добавить helper, который из **итогового** `entries` собирает set recognized type labels:
   - для каждой `SCR_EntityCatalogEntry` получить все editable labels;
   - выбрать все labels, присутствующие в агрегированном schema, либо в заранее валидированном known-type set;
   - deduplicate labels, сортировать строки для детерминированных логов;
   - если catalog entry не имеет ни одного vehicle-type label — вернуть `unsupported_vehicle_type_label` вместо пропуска bounds.
3. `ME_GetEditorVehicleEnvelopeCandidatePaths` больше не нужен для preview; удалить/оставить только если ещё есть вызывающие test-only инструменты. Production не должен зависеть от prefab coverage.
4. Resolve faction key тем же правилом, что уже используется в `ME_GetEditorVehicleEnvelopeCandidates`: affiliation default key, затем affiliated key. Для faction-resolved catalog ключ должен быть фактическим `SCR_Faction.GetFactionKey()`, а не display name.
5. Для `include=[]` трактовать результат только через vanilla catalog result: это `ALL` (все записи faction catalog), затем union всех type labels, которые реально есть у результата, с применением `excluded` уже на этапе vanilla filter.
6. Для `include=[CAR]`, `requireAll=false` — vanilla result (CAR OR другие included labels), затем union всех type labels результата.
7. Для `include=[CAR, APC]`, `requireAll=true` — vanilla result (CAR AND APC), затем union только labels действительно оставшихся entries. Если пересечение невозможно, результат пустой и preview отключается с существующей диагностикой; не подменять его union отдельных include-групп.
8. `excluded` никогда не фильтровать вручную после агрегирования: он уже применён vanilla filter. Это важно для `TRAIT_ARMED`, overlap include/exclude и inherited prefab defaults.
9. Если included/excluded содержат `TRAIT_ARMED`, не считать его vehicle type. Он влияет на catalog result, но не должен становиться ключом bounds.
10. Если faction key не разрешён или schema не содержит нужную faction/type group, fail closed: не рисовать envelope и логировать одну стабильную причину. Не использовать bounds другой faction или global fallback для faction-specific catalog.

`ME_RefreshValidatedEditorVehicleEnvelopePreview()` будет вызывать: collect filtered entries -> resolve faction key -> derive selected type labels -> helper aggregate -> сохранить `m_v...LocalMins/maxs` -> текущий `ME_RefreshEditorVehicleEnvelopePreview()` без изменений геометрии.

## Generation / derivation strategy

Генератор остаётся test-only и меняется только в `ME_Vehicle_Spawn_Test/Scripts/WorkbenchGame/WorldEditor/ME_GenerateVehicleBoundsSnapshotPlugin.c` плюс test schema/helper и staged resource:

1. Использовать существующие spawn points для US/USSR/CIV/FIA и armed/non-armed вариантов, чтобы получить exact vanilla candidate sets. Не копировать fixture world, marker component или generator в production.
2. Для каждого fixture marker root сохранить не только prefab path, но и catalog metadata: faction key и все editable labels, полученные из соответствующего catalog entry. Не выводить type из имени prefab или directory path.
3. Для каждого `(factionKey, recognizedVehicleTypeLabel)` вычислять union fixture root world bounds, переводить в local coordinates тем же способом, что текущий `ME_CreateEntry`, и обновлять min/max по осям.
4. Типы определять по фактически встреченным labels из catalog. В первую очередь ожидать `VEHICLE_CAR`, `VEHICLE_APC`, `VEHICLE_HELICOPTER`; `VEHICLE_TRUCK` включать только если он реально присутствует в `GetEditableEntityLabels`/catalog. Если одна запись имеет несколько vehicle-type labels, её bounds включить в каждую соответствующую группу (консервативно и корректно для label query).
5. Проверять exact coverage не по 146 prefab entries, а по каждому candidate prefab: у него должен быть marker, faction context и минимум один supported vehicle type; затем проверять, что все ожидаемые group keys получили bounds. Это предотвращает ошибочное уменьшение snapshot из-за незамеченного типа.
6. Сохранять staged config, rebuild resource, reload и сравнивать metadata, group count, sorted keys и vectors с допуском `0.001`, сохраняя текущую защиту от трёх знаков serialization.
7. После проверки staged payload вручную/через безопасный resource transfer обновить production `ME_Vehicle_Spawn/Configs/Generated/ME_VehicleBoundsSnapshot.conf`; test fixtures и test-only staged file не переносить. На текущем запросе этот transfer не выполнять.

Для faction context генератор должен явно иметь mapping fixture spawn point -> `US/USSR/CIV/FIA`; не пытаться выводить faction из prefab path. Для каждого group полезно логировать count source prefabs и bounds, чтобы можно было проверить, что armed/non-armed исключения действительно дали ожидаемый union.

## Validation

Статическая:

- `ME_Vehicle_Spawn` production содержит только schema/helper/component и aggregated `.conf`; нет ссылок на marker, fixture world, generator или staged resource.
- `git diff --check`, `mod_validate` для production и test; проверить duplicate faction/type keys, finite/order bounds, schema/generator metadata.
- Проверить, что все реально доступные faction keys имеют groups и что truck либо присутствует с валидной группой, либо явно отсутствует и не считается поддержанным.

Workbench/runtime:

- Сначала скомпилировать test addon и проверить `error.log`; не запускать production transfer при compile error.
- Открыть dedicated fixture, выполнить generator, reload-validate staged config.
- Проверить envelope на точках: ALL, ALL except `TRAIT_ARMED`, CAR, APC, HELICOPTER, комбинация CAR+APC с `requireAll=false`, та же комбинация с `requireAll=true`, невозможное include/exclude overlap.
- Отдельно проверить US, USSR, CIV, FIA и отсутствие/неизвестный faction key; ожидание — fail closed без чужих bounds.
- Сравнить новый агрегированный envelope с union старого 146-entry snapshot для тех же filtered candidate sets. Это ключевая регрессия: размеры могут стать больше только из-за сознательной группировки, но не должны быть меньше старого union.
- В чистой сессии после открытия fixture/мира capture `script.log` и `error.log`; после game-mode observation всегда `wb_stop`.

## Минимальный diff

Production:

- `ME_Vehicle_Spawn/Scripts/Game/Configs/ME_VehicleBoundsSnapshot.c`: заменить две сериализуемые entry-модели и version metadata.
- `ME_Vehicle_Spawn/Scripts/Game/Configs/ME_VehicleBoundsSnapshotHelper.c`: заменить prefab lookup/coverage на group validation, label-set lookup и union.
- `ME_Vehicle_Spawn/Scripts/Game/Components/Locations/ME_DebugAmbientVehicleSpawnPointComponent.c`: заменить только candidate-path -> helper call на filtered-entry -> faction/type-label -> helper call; оставить shape math, registry, diagnostics и `super` без изменений.
- `ME_Vehicle_Spawn/Configs/Generated/ME_VehicleBoundsSnapshot.conf`: новый компактный payload.

Test:

- Аналогично обновить test schema/helper для проверки staged payload.
- Обновить только `ME_GenerateVehicleBoundsSnapshotPlugin.c` и staged generated resource; fixture world/marker остаются как есть.
- Не переносить тестовые `Scripts/WorkbenchGame`, marker component или fixture resources в production.

## Основные риски

1. **Enforce serialization nested arrays/GUIDs.** `ref array<ref ...>` и `BaseContainerProps(namingConvention: NC_MUST_HAVE_NAME)` должны round-trip корректно. Сначала тестировать staged reload; при проблеме перейти на плоскую schema.
2. **Enum names/version drift.** Поэтому type key — string, но generator обязан инвентаризировать реальные labels; не хардкодить TRUCK до подтверждения.
3. **Одна запись с несколькими type labels.** Включать bounds во все группы, иначе запрос по второму label даст неполный envelope.
4. **Разные faction catalogs.** Нельзя агрегировать только по type или использовать US как fallback: один и тот же prefab/type может иметь разный каталог и bounds.
5. **ALL и excluded.** Нельзя вычислять типы из included list: для пустого include и для TRAIT_ARMED нужны фактические filtered catalog entries.
6. **requireAll.** Нельзя делать независимый union по каждому include label; только результат единого vanilla filter.
7. **Unknown/untyped catalog entries.** Silent omission даст опасно маленький envelope; безопаснее fail closed и диагностировать coverage.
8. **Local coordinate convention.** Сохранить текущий yaw-only local AABB convention и rounding tolerance; не смешивать world bounds разных fixture orientations.
9. **Production/test divergence.** Сначала стабилизировать schema и reload в Test, затем отдельным осознанным transfer обновлять production resource/schema, не перенося test fixtures.
