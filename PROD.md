# Production addon

`ME_Vehicle_Spawn` содержит editor-only marker-визуализацию, предпросмотр габаритов техники, метки фильтра и Workbench warning plugin для ambient vehicle spawn points. Расширенная runtime-инструментация Test сюда не переносится; существующая диагностика пустого результата фильтра в Update сохранена.

## Требования

Демонстрационный мир `worlds/ME_TestWorld.ent` должен запускаться с GameMode, содержащим `SCR_FactionManager` и включённым `EGameFlags.SpawnVehicles`. В мире должна присутствовать ambient vehicle spawn point.

## Проверка

Откройте меню/действие `Check ambient vehicle spawning` в Edit mode. Проверьте preflight guards и сферу clearance. После перемещения точки сфера должна следовать за ней без дубликатов. Затем войдите в Game mode и проверьте базовый spawn flow.

Shape — визуальная проверка edit-world clearance, а не обещание runtime spawn.

Production нельзя загружать одновременно с `ME_Vehicle_Spawn_Test`.

## Перенос от 2026-09-24

Перенесены проверенные в Test серые сферы, раздельные сообщения о фильтре и пересечениях с именами/координатами, стрелка над рельефом и подпись deg. Исправлены число треугольников габаритов и запись глубины прозрачных фигур. Пользователь подтвердил финальную проверку Test и отдельную проверку Prod в Workbench 2026-09-24. Чек-лист: docs/PRODUCTION_TRANSFER_PLAN.md.


## Справочники каталогов в production

В Configs/Generated добавлен ME_EditableEntityLabelsSnapshot.conf (схема 2, каталоги CIV/FIA/US/USSR игры 1.8.0.13) с классами сериализации. ME_VehicleBoundsSnapshot.conf и его reader перенесены на схему 5 с группами фракций; production GUID габаритов сохранён. Инструкция: [VEHICLE_CATALOG_REFERENCE.md](ME_Vehicle_Spawn/VEHICLE_CATALOG_REFERENCE.md). Справочник меток помогает интерпретировать include/exclude, но не заменяет текущий каталог и не гарантирует runtime-спавн. Пользователь подтвердил проверку переноса в production Workbench 24.09.2026.


Production обновлён: пересечения со статическими объектами проверяются по локальным границам модели с мировым transform (OBB); препятствия отмечены ориентированным красным каркасом. Пересечение OBB — WARNING с просьбой перепроверить размещение. Неудачный поиск свободной позиции — отдельный ERROR, пересечение сфер точек остаётся ERROR. Это редакторская диагностика, не гарантия результата runtime-спавна. Проверка текущего переноса в production Workbench подтверждена пользователем 24.09.2026.
