# План: миграция Test aggregate snapshot на canonical resource

## Цель и границы
- Изменять только `ME_Vehicle_Spawn_Test`; `ME_Vehicle_Spawn` production не трогать.
- Оставить один ресурс: `Configs/Generated/ME_VehicleBoundsSnapshot.conf`.
- Генератор должен писать и reload-validate именно canonical resource с GUID `{1C3AE4A8F2630BF7}`.
- Удалить staged `.conf` и `.meta`; `resourceDatabase.rdb` не редактировать вручную.

## Порядок реализации
1. В `ME_GenerateVehicleBoundsSnapshotPlugin.c` заменить `STAGED_PATH/STAGED_RESOURCE` на canonical path/resource, переименовать `ME_SaveAndValidateStagedSnapshot` и связанные сообщения/описания на canonical.
2. Сохранить текущую последовательность: загрузить/проверить VBT Candidate, собрать selection spawn points, отсортировать entries, выполнить полную aggregate validation, затем сохранить canonical и дождаться resource rebuild.
3. Расширить сериализацию entry-container names:
   - после сортировки entries формировать имя `<FactionKey>_<VehicleType>`;
   - добавить safe-name validator для faction/type и итогового имени (непустые, допустимые символы для serialized container name, без whitespace/разделителей/управляющих символов, с детерминированным ASCII-safe контрактом);
   - отдельно проверять collision итоговых имен, даже если `(factionKey, vehicleType)` уже уникальны;
   - валидировать соответствие числа сериализованных entries и ожидаемых names до SaveContainer.
4. В `ME_ValidateAggregateSnapshot`/новом validator сохранить проверки duplicate composite keys, membership count/reverse coverage, bounds и provenance; дополнительно проверять, что каждая пара производит ровно ожидаемое safe-name и что сортировка entries согласуется с именами.
5. В canonical save/reload методе:
   - `BaseContainerTools.CreateContainerFromInstance(snapshot)`;
   - `GetAbsolutePath(CANONICAL_PATH, ...)`;
   - установить names `<FactionKey>_<VehicleType>`;
   - `SaveContainer(..., CANONICAL_RESOURCE, ...)`;
   - `RebuildResourceFile`, `WaitForFile`, затем `Resource.Load(CANONICAL_RESOURCE)`;
   - сравнить schema/generator/count и каждое поле каждой entry, включая bounds с существующим tolerance и provenance;
   - после reload повторно прогнать schema/entry/name validation.
6. Не менять loader: `ME_VehicleBoundsSnapshotHelper.c` уже читает `{1C3AE4A8F2630BF7}Configs/Generated/ME_VehicleBoundsSnapshot.conf`. Не менять runtime component, production scripts или production snapshot.
7. Обновить `ME_Vehicle_Spawn_Test/VEHICLE_BOUNDS_REGRESSION.md` и корневой `README.md`: canonical — единственный generated snapshot, generator overwrite + reload-validation, manual acceptance больше не является copy staged→published; описать удаления staged artifacts, name contract, collision/safe-name failures, deterministic double-run и preview/warning verification.

## Точная миграция metadata/resources
1. До запуска Workbench сохранить/зафиксировать текущий canonical payload как исходный reference; проверить Git diff.
2. В Workbench зарегистрировать/rebuild Test resources, но не открывать/править `resourceDatabase.rdb` вручную.
3. Удалить из Test working tree `Configs/Generated/ME_VehicleBoundsSnapshot_Staged.conf` и `.meta`.
4. Исправить canonical `.meta` так, чтобы `Name` был ровно `{1C3AE4A8F2630BF7}Configs/Generated/ME_VehicleBoundsSnapshot.conf`; не переносить staged GUID/path в canonical metadata.
5. В Workbench выполнить resource rebuild/register для canonical resource и дождаться завершения; Workbench должен сам обновить managed database/metadata при необходимости.
6. Если Workbench генерирует canonical `.meta` заново, принять только результат с canonical filename/path/GUID; staged identity не восстанавливать.
7. Проверить, что loader GUID, canonical `.meta`, canonical `.conf` и Workbench resource info согласованы; `resourceDatabase.rdb` проверять только как Workbench output.
8. После генератора не принимать никакую отдельную staged-копию и не менять production metadata.

## Детерминированность и проверки
- Запустить генератор дважды без изменений между запусками.
- Оба запуска должны дать `status=PASS source=VBT resource=Configs/Generated/ME_VehicleBoundsSnapshot.conf` (новое имя поля вместо `stage`), одинаковые counts/diagnostics.
- После второго запуска canonical `.conf` не должен получить новый diff; names и порядок entries должны совпадать.
- Проверить, что имена, например `CIV_VEHICLE_CAR`, присутствуют в serialized `m_aEntries`, уникальны и соответствуют полям.
- Проверить Test-only Workbench validation и `git diff --check`; production tree/доступность/metadata не изменены.
- Открыть `worlds/TestCases/ME_VehicleBoundsSnapshot.ent`, выбрать существующие точки и выполнить preview: PASS при полном canonical coverage, UNVERIFIABLE/без частичного envelope при invalid/missing data.
- Запустить `Check ambient vehicle spawning`: предупреждения должны остаться корректными для configured/missing/empty cases и не перестать загружать canonical snapshot.
- Проверить `script.log` и `error.log`, затем отдельно выполнить addon validation для Test; после EnfusionMCP удалить временные handler scripts через `wb_cleanup`.

## Критерии завершения
- В репозитории нет staged `.conf/.meta`; canonical `.conf/.meta` имеют только canonical identity.
- Loader продолжает использовать `{1C3AE4A8F2630BF7}` без изменений.
- Генерация, serialization-name validation и reload-validation проходят.
- Два последовательных запуска byte/diff-deterministic.
- Preview/warnings работают на canonical resource.
- Ни один файл `ME_Vehicle_Spawn` production и никакой production dependency graph не изменён.
