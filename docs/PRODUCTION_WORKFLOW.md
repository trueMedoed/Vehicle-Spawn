# Production workflow

1. Закройте Workbench-сессии с Test addon и откройте `ME_Vehicle_Spawn/addon.gproj`.
2. Дождитесь resource scan. Зарегистрируйте production resources и выполните rebuild resource database; не копируйте `resourceDatabase.rdb` из Test.
3. Перезагрузите scripts.
4. Откройте `worlds/MP/MpTest/ME_MpTest_BasicSpawnVehicles.ent` через World Editor.
5. В слоях проверьте `SCR_BaseGameMode`, `m_eTestGameFlags` с `SpawnVehicles`, `SCR_FactionManager` и ambient vehicle spawn point.
6. В Edit mode откройте `Check ambient vehicle spawning`. Проверьте локализованные guards и editor sphere. Цвет Shape — clearance preflight, не гарантия runtime spawn.
7. Переместите spawn point и убедитесь, что preview sphere следует за точкой и не дублируется.
8. Войдите в Game mode и проверьте базовый ambient spawn flow.
9. Остановите Game mode, проверьте `error.log` и при использовании EnfusionMCP выполните `wb_cleanup` для production addon.

Для диагностики runtime используйте отдельную чистую Workbench-сессию с `ME_Vehicle_Spawn_Test`; не загружайте два addon одновременно.
