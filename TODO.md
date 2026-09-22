# TODO

## В работе

- [ ] Для голограммы добавить указатель в виде стрелки, показывающий направление, куда «смотрит» техника, и перемещающийся при вращении техники вокруг оси.
- [ ] Определить назначение snapshot’а `editable entity labels`:
  - [ ] диагностический snapshot для проверки фильтрации ambient vehicle spawn points;
  - [ ] полный справочник labels всех vehicle prefab’ов base game.
- [ ] Выбрать реализацию генератора snapshot’а:
  - [ ] для диагностики — fixture world с существующими spawn points;
  - [ ] для полного справочника — прямое чтение и разбор prefab sources.
- [ ] Добиться запуска `ME_GenerateEditableEntityLabelsSnapshotPlugin` из Workbench.
- [ ] Проверить, что `ME_EditableEntityLabelsSnapshot.conf` создаётся, перечитывается и проходит round-trip validation.
- [ ] Сравнить полученный snapshot с данными, которые возвращает ambient vehicle filtering.

## Завершено

- [x] Добавлена инструментированная test-архитектура для ambient vehicle system.
- [x] Подтверждено, что `ME_MpTest` достигает `GAME`, создаёт локального игрока и инициализирует ambient system.
- [x] Найден vanilla guard `EGameFlags.SpawnVehicles`, из-за которого ambient system отключается в `ME_MpTest`.
- [x] Установлено, что `m_eTestGameFlags 6` означает `SpawnVehicles | SpawnAI`.
- [x] Подтверждено, что `GameMode_Plain` не устанавливает `SpawnVehicles`.
- [x] Проверен чистый baseline `CTI_Campaign_Eden`: 171 spawn point и включённый vehicle spawning.
- [x] Добавлена схема `ME_EditableEntityLabelsSnapshot`.
- [x] Добавлен генератор snapshot’а и диагностические Workbench-плагины.
- [x] Подтверждено, что текущий генератор компилируется.
- [x] Зафиксировано, что запуск генератора через опробованные Workbench menu actions пока не работает.

## Техническое обслуживание

- [ ] После завершения Workbench-диагностики удалить временные `Scripts/WorkbenchGame/EnfusionMCP/` через `wb_cleanup`.
- [ ] Проверить изменения в test addon и обновить `resourceDatabase.rdb` только средствами Workbench.
- [ ] Провести финальную проверку test addon.
- [ ] Перенести проверенные изменения в production addon только после отдельного подтверждения.
