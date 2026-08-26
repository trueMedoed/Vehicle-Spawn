# Production addon

`ME_Vehicle_Spawn` содержит editor-only marker-визуализацию, preview controller, Workbench warning plugin и локализацию для ambient vehicle spawn points. Runtime diagnostic overrides сюда не входят.

## Требования

Демонстрационный мир `ME_SpawnVehicles.ent` должен запускаться с GameMode, содержащим `SCR_FactionManager` и включённым `EGameFlags.SpawnVehicles`. В мире должна присутствовать ambient vehicle spawn point.

## Проверка

Откройте меню/действие `Check ambient vehicle spawning` в Edit mode. Проверьте локализованные preflight guards и сферу clearance. После перемещения точки сфера должна следовать за ней без дубликатов. Затем войдите в Game mode и проверьте базовый spawn flow.

Shape — визуальная проверка edit-world clearance, а не обещание runtime spawn.

Production нельзя загружать одновременно с `ME_Vehicle_Spawn_Test`.
