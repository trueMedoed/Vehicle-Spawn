# Production workflow

1. Закройте Workbench-сессии с Test addon и откройте `ME_Vehicle_Spawn/addon.gproj`.
2. Дождитесь resource scan. Зарегистрируйте production resources и выполните rebuild resource database; не копируйте `resourceDatabase.rdb` из Test.
3. Перезагрузите scripts.
4. Откройте `worlds/ME_TestWorld.ent` через World Editor.
5. В слоях проверьте `SCR_BaseGameMode`, `m_eTestGameFlags` с `SpawnVehicles`, `SCR_FactionManager` и ambient vehicle spawn point.
6. В Edit mode откройте Plugins → `[ME] Vehicle Spawn` → `Check ambient vehicle spawning` и проверьте guards. На всех загруженных точках должны появиться голограммы техники с меткой фракции. Обычная голограмма белая; при проблеме фильтра, каталога, поиска свободной позиции или пересечении объёмных границ показанных голограмм — красная. Пересечение только областей двух точек и статических границ остаётся WARNING и не окрашивает голограмму. В тестовом мире проверьте пару `<130,1,195>` и `<129.815,1,196.442>`: после обновления голограмм (до пяти секунд) обе UH1H должны быть красными, а при выделении видна отдельная строка `WARNING: vehicle previews overlap`.
7. Переместите и поверните spawn point: голограмма и метка фракции должны обновиться без дубликатов. Если включены `TRAIT_PASSENGERS_SMALL` и `TRAIT_PASSENGERS_LARGE`, они должны отображаться одной строкой через запятую. Проверьте отдельные строки при конфликте меток и пустом результате, сообщения о пересечениях с другой точкой и статическим объектом. `Next vehicle hologram` переключает пример только выделенной точки; `Toggle vehicle hologram` скрывает и восстанавливает все примеры. После удаления точки, смены мира и входа в Game mode временные голограммы должны исчезнуть.
8. Войдите в Game mode и проверьте базовый ambient spawn flow.
9. Остановите Game mode, проверьте `error.log` и при использовании EnfusionMCP выполните `wb_cleanup` для production addon.

Для диагностики runtime используйте отдельную чистую Workbench-сессию с `ME_Vehicle_Spawn_Test`; не загружайте два addon одновременно.


## Справочники каталогов в production

ME_EditableEntityLabelsSnapshot.conf остаётся справочником схемы 2 для каталогов CIV/FIA/US/USSR игры 1.8.0.13. ME_VehicleBoundsSnapshot.conf обновлён до схемы 6; его production GUID сохранён. Инструкция: [VEHICLE_CATALOG_REFERENCE.md](../ME_Vehicle_Spawn/VEHICLE_CATALOG_REFERENCE.md). Справочники не заменяют текущий каталог и не гарантируют runtime spawn. Пользователь подтвердил проверку схемы 6 в production Workbench 05.10.2026.


Пересечения со статическими объектами проверяются по локальным границам модели с мировым transform (OBB); препятствия отмечены ориентированным красным каркасом. Пересечение OBB и областей двух точек — WARNING с просьбой перепроверить размещение. Неудачный поиск свободной позиции — отдельный ERROR. Это редакторская диагностика, не гарантия результата runtime spawn. Пользователь подтвердил голограммы, уровни предупреждений и форматирование пассажирских меток в production Workbench 05.10.2026.
