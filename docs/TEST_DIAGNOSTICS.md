# Test diagnostics

## SpawnVehicles flag

В vanilla ambient system первый `OnUpdatePoint` может иметь `enabled=1`, но при отсутствии `EGameFlags.SpawnVehicles` базовый код выключает update point после `super`. Значения флагов:

- `EGameFlags.SpawnVehicles = 2`;
- `EGameFlags.SpawnAI = 4`;
- `m_eTestGameFlags = 6` означает оба флага.

`SCR_BaseGameMode.EOnInit()` устанавливает `m_eTestGameFlags`. Plain mode с базовым значением `0` не включает SpawnVehicles; campaign baseline включает его.

## Log prefixes

Фильтруйте `script.log` по следующим префиксам:

- `[ME_DEBUG_AVSP]` — spawn-point component;
- `[ME_DEBUG_AVSP_POS]` — terrain position probing;
- `[ME_DEBUG_AVSP_SYS]` — ambient vehicle system;
- `[ME_DEBUG_AVSP_GM]` — game mode and player lifecycle.

Используйте `error.log` для compile/runtime errors, а `console.log` — для полного engine context.

## Сравнение

Сравнивайте Test только с чистым `worlds/MP/CTI_Campaign_Eden.ent` в отдельной Workbench-сессии. Не используйте terrain-only `worlds/Eden/Eden.ent` и не открывайте campaign поверх Test: это смешивает game mode, entities и spawn points.

Ожидаемый campaign baseline: `SCR_GameModeCampaign`, 171 spawn points, `spawnVehicles=1` и `enabled=1` до и после `super`, затем вызовы `ProcessSpawnpoint`.

Не добавляйте override `Enable(bool)`: native declaration конфликтует с таким modded override.

Shape marker — только edit-world clearance preflight и не гарантия того, что runtime действительно создаст vehicle.

## Массовая проверка меток ambient-точек (Test)

В Edit mode откройте World Editor Plugins → ME_Vehicle_Spawn/Diagnostics → Audit all ambient vehicle spawn points. Выделять точку, создавать персонажа и запускать Game mode не требуется. Проверяется текущий открытый мир, доступный через GetEditorEntityCount/GetEditorEntity; незагруженные миры не проверяются. Для ванильной кампании открывайте сценарий (например worlds/MP/CTI_Campaign_Eden.ent), а не только terrain.

Лог фильтруется по `[ME_DEBUG_AVSP_AUDIT]`. Между STARTED и FINISHED каждая проблема содержит имя точки, координаты, фракцию, include/exclude и requireAll. Команда при каждом запуске повторяет диагностику независимо от логирования визуальных подсказок.

- ERROR / conflicting_labels: конфликт include/exclude; при requireAll=false другие включённые метки ещё могут дать кандидатов.
- ERROR / no_vehicle_candidates: каталог прочитан, но штатный фильтр не вернул кандидатов.
- UNAVAILABLE: каталог, фракция или менеджер недоступны; причина указана в reason, результат не считается успешным.
- FINISHED: scanned — число уникальных точек; passed — без ошибок, предупреждений и недоступности; errorPoints — точки с ошибками; unavailablePoints — точки с неполной проверкой каталога или геометрии. unresolvedEntities выводится отдельно в COVERAGE и означает сущности редактора, которые не удалось разрешить. errorPoints и unavailablePoints могут пересекаться.
- result: PASS, WARNINGS_FOUND, ERRORS_FOUND, NO_POINTS либо INCOMPLETE относится только к найденным точкам; INCOMPLETE означает недоступность хотя бы одной проверки. ERRORS_FOUND имеет приоритет над WARNINGS_FOUND. Полнота обхода указана отдельно в coverage: COMPLETE_LOADED_ENTITIES (все выданные API сущности разрешены), UNVERIFIED (остались неразрешённые source), MISSED_POINTS (среди них обнаружен ambient-компонент). Ни один статус не распространяется на незагруженные миры.

Проверяется текущая настроенная фракция и фильтр. Команда дополнительно проверяет пересечения сферы точки с AABB статических физических объектов. GameMode prerequisites и реальный runtime spawn не проверяются. Каталоги инициализируются существующим editor-helper; техника не создаётся, мир не сохраняется.

Первую версию пользователь проверил на тестовом мире и CTI_Campaign_Cain.ent: scanned=62, passed=57, errorPoints=5, unavailablePoints=0, unresolvedEntities=3371. Уточнённая диагностика успешно выполнена на Cain 2026-09-24 в 09:31:17 (script.log сессии logs_2026-09-24_09-21-27): missingSources=0, unresolvedPointSources=0, продолжительность около 0,43 с. Пользователь независимо насчитал в редакторе 62 AmbientVehicleSpawnPoint — число совпало с аудитом. Для новой сверки SOURCE_CHECK повторная проверка в Workbench ещё требуется: перезагрузить скрипты Test; запустить команду на ME_TestWorld с корректной точкой, конфликтом меток и APC_ARMED; сверить координаты и сводку; повторить запуск после исправления фильтра. Затем проверить полноценный ванильный сценарий. На большом мире измерить время выполнения: первая версия выполняется синхронно.

Диагностика полноты: UNRESOLVED_CLASS группирует неразрешённые сущности по классам; UNRESOLVED_SOURCE показывает первые 10 прочих source и все source с обнаруженным ambient-компонентом, включая ID, слой, subscene и prefab. COVERAGE разделяет missingSources, unresolvedPointSources и unresolvedOtherSources. Поиск компонента проходит предков prefab; отсутствие точного класса не считается доказательством отсутствия точки (например, производного компонента). Поэтому UNVERIFIED не скрывается. На Cain неразрешённые source распределились так: GenericEntity=1843, SCR_DestructibleBuildingEntity=1285, StaticModelEntity=99, GameEntity=81, Building=32, LightEntity=30, SCR_IndestructibleEnvironmentalEntity=1. Причина отсутствия игровых сущностей не установлена; ручная сверка подтвердила количество точек только на этой карте.

SOURCE_CHECK проверяет HasAmbientPointSource на всех уникальных точках, независимо найденных по игровому компоненту. MATCH означает matched=knownPoints и missed=0; MISMATCH сопровождается строками SOURCE_MISMATCH с координатами и ID; при отсутствии точек — NOT_TESTED. Даже MATCH не доказывает отсутствие иных пропущенных точек. FINISHED теперь содержит scope=inspected_points и только результат проверки найденных точек; полнота остаётся отдельной строкой COVERAGE. Ручное число 62 не зашито в код.

Проверка SOURCE_CHECK подтверждена: script.log сессии logs_2026-09-24_09-21-27, запуск 09:40:10.353–09:40:10.776 (около 0,42 с). MATCH: knownPoints=62, matched=62, missed=0. FINISHED: ERRORS_FOUND, scanned=62, passed=57, errorPoints=5, unavailablePoints=0. Поиск в source распознал все известные точки Cain; результат совпадает с ручным подсчётом пользователя. Общая coverage остаётся UNVERIFIED для 3371 прочих source; это не отменяет успешную сверку известных точек и не является утверждением о полноте других карт.

### Предупреждения о статических объектах

WARNING / static_object_bounds_intersection с action=RECHECK_PLACEMENT означает: перепроверьте размещение вручную. Лог содержит координаты точки, имя (либо класс), ID, координаты и AABB объекта. Используется та же проверка сфера/AABB и отбор статической физики, что у визуальных маркеров; динамические объекты и другие ambient-точки исключены. Касание тоже считается пересечением. Это предварительная проверка габаритов, а не доказательство столкновения с точной геометрией или невозможности спавна. Аудит не перестраивает маркеры.

FINISHED: warningPoints — число точек с такими предупреждениями; staticObjectIntersections — число пар точка–объект (общий объект может учитываться для нескольких точек). Счётчики errorPoints, warningPoints и unavailablePoints могут пересекаться; passed не включает предупреждения. Геометрия проверяется даже при ошибке или недоступности каталога. Проверка новой версии ожидается: Standart8/ConcretePanel, свободная точка и Cain; после удаления пересечения предупреждение должно исчезнуть при повторном запуске.

В Test маркер статического объекта теперь представляет собой красный каркас точной мировой AABB из GetWorldBounds, используемой в проверке сфера/AABB. Прежняя сфера с ограниченным радиусом удалена: она не отражала проверяемые границы. Проверка глубины сохранена, заливки нет. Текст над точкой: WARNING: spawn area intersects static object bounds; recheck placement. Геометрический критерий и радиус сферы точки не изменены. Требуется визуальная проверка каркасов в Workbench на примерах Cain (здание, дерево, куча мусора).

### Уточнение: ориентированные границы объектов

Текущая версия Test заменяет описанную выше мировую AABB на OBB: GetBounds даёт локальные габариты модели, GetTransform задаёт её мировые оси, положение и масштаб. И визуальная проверка, и аудит используют общий расчёт ближайшей точки OBB к центру сферы. Каркас получает те же локальные границы и матрицу объекта через SetMatrix. WARNING остаётся предварительной проверкой габаритов модели, а не точной физической поверхности. В аудите boundsType=OBB, localBoundsMin/localBoundsMax заданы в координатах модели. Радиус сферы не изменён. Расчёт предполагает ортогональные оси трансформации (поворот и масштаб без сдвиговой деформации). Требуется проверить компиляцию и повёрнутое здание Cain в Workbench; затем повторить аудит — количество предупреждений может уменьшиться.

### Ошибка поиска свободной позиции

В Test над точкой и в аудите добавлен ERROR при отрицательном результате FindEmptyTerrainPosition с теми же параметрами, что задают красную сферу: радиусы SPAWNING_RADIUS, высота 2, TraceFlags.ENTS | TraceFlags.OCEAN. Сообщение: no empty terrain position found; recheck placement; в аудите reason=no_empty_terrain_position. Пересечения OBB остаются отдельными WARNING: они не доказывают, что именно этот объект стал причиной неудачного поиска. errorPoints включает ошибки фильтра и clearance без повторного подсчёта точки; clearanceErrorPoints — число точек с неудачным поиском позиции. WARNING и ERROR могут присутствовать одновременно. Пересечение сфер разных точек уже имеет отдельный ERROR над точкой; оно не добавлено в массовый аудит этой правкой. Серый цвет при проблемах фильтра сохраняет приоритет. Требуется повторная проверка Workbench: зелёная сфера с OBB-пересечением остаётся WARNING, отрицательный clearance даёт дополнительный ERROR.

### Подтверждённая рабочая версия аудита

2026-09-24 10:16:40.396, script.log сессии logs_2026-09-24_09-21-27: SOURCE_CHECK MATCH 62/62; FINISHED scanned=62, passed=17, errorPoints=17, warningPoints=43, unavailablePoints=0, staticObjectIntersections=169, clearanceErrorPoints=12. Пять точек имеют ошибки фильтра, ещё 12 — неудачный clearance; предупреждения могут сопутствовать ошибкам. Пользователь подтвердил визуальную проверку ориентированных каркасов и сообщений ERROR/WARNING. Предыдущие указания о необходимости проверки этих изменений выполнены для Cain. Результат не распространяется автоматически на остальные сценарии; общая полнота неразрешённых source остаётся отдельной диагностикой.
