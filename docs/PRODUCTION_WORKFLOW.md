# Production workflow

1. Закройте Workbench-сессии с Test addon и откройте `ME_Vehicle_Spawn/addon.gproj`.
2. Дождитесь resource scan. Зарегистрируйте production resources и выполните rebuild resource database; не копируйте `resourceDatabase.rdb` из Test.
3. Перезагрузите scripts.
4. Откройте `worlds/ME_TestWorld.ent` через World Editor.
5. В слоях проверьте `SCR_BaseGameMode`, `m_eTestGameFlags` с `SpawnVehicles`, `SCR_FactionManager` и ambient vehicle spawn point.
6. В Edit mode откройте `Check ambient vehicle spawning`. Проверьте guards и editor sphere. Цвет Shape — clearance preflight, не гарантия runtime spawn.
7. Переместите и поверните spawn point: сфера, габариты, стрелка над рельефом и подпись deg должны обновляться без дубликатов. Проверьте серую сферу и отдельные строки при конфликте меток/пустом каталожном результате, сообщения пересечений с другой точкой и статическим объектом. Исправьте причины и удалите точку: старые подсказки должны исчезнуть, соседние — обновиться. Смените ракурс: сфера внутри габаритов не исчезает, посторонних полигонов нет.
8. Войдите в Game mode и проверьте базовый ambient spawn flow.
9. Остановите Game mode, проверьте `error.log` и при использовании EnfusionMCP выполните `wb_cleanup` для production addon.

Для диагностики runtime используйте отдельную чистую Workbench-сессию с `ME_Vehicle_Spawn_Test`; не загружайте два addon одновременно.


## Справочники каталогов в production

В Configs/Generated добавлен ME_EditableEntityLabelsSnapshot.conf (схема 2, каталоги CIV/FIA/US/USSR игры 1.8.0.13) с классами сериализации. ME_VehicleBoundsSnapshot.conf и его reader перенесены на схему 5 с группами фракций; production GUID габаритов сохранён. Инструкция: [VEHICLE_CATALOG_REFERENCE.md](../ME_Vehicle_Spawn/VEHICLE_CATALOG_REFERENCE.md). Справочник меток помогает интерпретировать include/exclude, но не заменяет текущий каталог и не гарантирует runtime-спавн. Пользователь подтвердил проверку переноса в production Workbench 24.09.2026.


Production обновлён: пересечения со статическими объектами проверяются по локальным границам модели с мировым transform (OBB); препятствия отмечены ориентированным красным каркасом. Пересечение OBB — WARNING с просьбой перепроверить размещение. Неудачный поиск свободной позиции — отдельный ERROR, пересечение сфер точек остаётся ERROR. Это редакторская диагностика, не гарантия результата runtime-спавна. Проверка текущего переноса в production Workbench подтверждена пользователем 24.09.2026.
