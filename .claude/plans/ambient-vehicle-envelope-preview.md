# Context

В `ME_Vehicle_Spawn_Test` уже есть проверенный snapshot консервативных локальных границ техники (`Configs/Generated/ME_VehicleBoundsSnapshot.conf`), но он пока не используется визуально в Workbench. Нужен явный **Test-only** preview для одной выбранной точки ambient-спавна: один aggregate/worst-case wireframe Bounding Box по всем prefab-кандидатам её текущего каталога. Частичный результат нельзя показывать: при невалидном snapshot или неполном покрытии preview очищается и помечается как `UNVERIFIABLE`.

`ME_Vehicle_Spawn` (Prod) не изменять.

## Implementation

1. Расширить `ME_Vehicle_Spawn_Test/Scripts/Game/Components/Locations/ME_DebugAmbientVehicleSpawnPointComponent.c` отдельным lifecycle для envelope, не затрагивая существующую сферу spawn area:
   - добавить отдельное владение Shape, cached local min/max и статическую ссылку на единственного активного владельца preview;
   - добавить public Test-only методы, чтобы очистить активный preview и показать уже проверенный envelope на выбранной точке;
   - строить 8 углов local AABB и 12 рёбер (24 endpoint) через `Shape.CreateLines`, с визуально отличимым полупрозрачным жёлтым/оранжевым стилем;
   - переводить углы только смещением от `owner.GetOrigin()`: snapshot заведомо yaw-independent и консервативен, поэтому yaw точки не должен сужать envelope;
   - при `_WB_SetTransform()` перестраивать envelope только для активной точки; при `OnDelete()` и `_WB_OnDelete()` освобождать его и сбрасывать active owner. Сохранить существующие `super` и логику сфер/реестра;
   - добавить двуязычные English/Russian комментарии для каждого нового класса/метода согласно `CLAUDE.md`.

2. Создать Test-only плагин `ME_Vehicle_Spawn_Test/Scripts/WorkbenchGame/WorldEditor/ME_AmbientVehicleEnvelopePreviewPlugin.c` с явной командой `Preview ambient vehicle envelope`:
   - в `Run()` первым действием очищать прежний активный preview, чтобы не оставалась устаревшая рамка;
   - через `WorldEditorAPI` требовать ровно одну выделенную entity и получать с неё `SCR_AmbientVehicleSpawnPointComponent`; при любом другом selection завершаться без Shape;
   - получать путь каждого применимого candidate только готовым безопасным API `ME_GetEditorVehicleEnvelopeCandidatePaths(out array<string>, out string)` — не вызывать runtime `Update`, не выбирать prefab и не создавать технику;
   - загружать опубликованный snapshot (не `_Staged.conf`) с тем же typed-container подходом, что использует генератор: `Resource.Load`, `ToBaseContainer`, `BaseContainerTools.CreateInstanceFromContainer`, `ME_VehicleBoundsSnapshot.Cast`.

3. В плагине валидировать snapshot до отрисовки:
   - проверить доступность ресурса/контейнера/schema, `m_iSchemaVersion == 1`, ожидаемые generator/fixture identities и непустой массив entries;
   - построить lookup по canonical `m_sPrefab`, отклоняя пустые либо дублирующиеся записи, пустые fixture names и некорректные/inverted local bounds;
   - потребовать ровно одну snapshot-запись для **каждого** candidate path выбранной точки. При отсутствии хотя бы одной записи не рисовать частичный box;
   - агрегировать component-wise minimum `m_vLocalMins` и maximum `m_vLocalMaxs` по всем покрытым кандидатам, затем передать итоговые bounds в component для отрисовки.

4. Стабилизировать диагностику плагина без перевода runtime strings:
   - при любой ошибке после начальной очистки писать `[ME_DEBUG_AVSP_WB] status=UNVERIFIABLE operation=ambient_vehicle_envelope_preview reason=<...> entity=<...>`;
   - для отсутствующей записи включать canonical prefab path (`reason=snapshot_entry_missing prefab=<...>`);
   - при успехе писать `status=PASS` с entity, количеством candidates, local min/max и `edgeCount=12`.
   - Оставить `ME_AmbientVehiclePrefabBoundsDiagnosticPlugin.c` самостоятельной текстовой диагностикой; не переиспользовать и не оживлять закомментированный `ME_AmbientVehicleSpawnPointPreviewController.c`.

## Verification

1. Открыть `ME_Vehicle_Spawn_Test/addon.gproj` в Workbench, дождаться регистрации нового WorkbenchGame script и выполнить Reload Scripts/Plugins. Убедиться в отсутствии ошибок компиляции в актуальном `error.log`; отдельно подтвердить, что `Shape.CreateLines` компилируется с использованной сигнатурой.
2. Открыть `worlds/TestCases/ME_VehicleBoundsSnapshot.ent` либо сконфигурированную fixture-точку, выбрать одну ambient vehicle spawn point и вызвать `Plugins → Preview ambient vehicle envelope`. Проверить появление одного wireframe box и `[ME_DEBUG_AVSP_WB] status=PASS` в `script.log`.
3. Передвинуть активную точку: рамка должна обновиться на новой позиции. Удалить активную точку: рамка должна исчезнуть.
4. Проверить selection errors (нет selection, две entity, не-spawn point): рамка должна очищаться, а log содержать `status=UNVERIFIABLE`.
5. В контролируемой Test-only проверке вызвать случай с отсутствующей snapshot entry и подтвердить, что частичный box не появляется, старый box очищен, а лог содержит `snapshot_entry_missing`; затем восстановить исходный snapshot.
6. Войти в Game mode для финальной проверки компиляции и вернуться через `wb_stop`; просмотреть `error.log` и `script.log`. Выполнить `mod_validate` для Test addon и `git diff --check`. Не редактировать `resourceDatabase.rdb` вручную.
