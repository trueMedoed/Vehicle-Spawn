# Как работает ME Vehicle Spawn

## Краткая схема

Репозиторий содержит два взаимоисключающих addon-проекта:

```text
ME_Vehicle_Spawn
    ↓
editor-only диагностика и preview
для размещённых ambient vehicle spawn points

ME_Vehicle_Spawn_Test
    ↓
runtime-наблюдение, Workbench-диагностика
и экспериментальные regression-инструменты

Vanilla ambient vehicle system
    ↓
выбор prefab по faction/catalog/labels
    ↓
проверка позиции и попытка создания vehicle
```

`ME Vehicle Spawn` не заменяет vanilla spawning system и не реализует собственный менеджер создания техники. Production addon помогает обнаружить очевидные ошибки конфигурации в World Editor и показывает диагностический preview. Реальный runtime spawn по-прежнему выполняет игра.

Не подключайте `ME_Vehicle_Spawn` и `ME_Vehicle_Spawn_Test` одновременно: Test содержит runtime `modded class` overrides vanilla-классов, а одновременная загрузка двух addon может привести к конфликту overrides и недостоверным результатам диагностики.

## Production addon

Production находится в `ME_Vehicle_Spawn` и предназначен для работы с открытым миром в World Editor.

Его инструменты проверяют и визуализируют известные предпосылки ambient spawning:

- наличие применимого `SCR_BaseGameMode`;
- наличие `EGameFlags.SpawnVehicles`;
- наличие `FactionManager` и доступной faction;
- доступность global или faction-specific `VEHICLE` catalog;
- конфигурацию included/excluded editable entity labels;
- пересечения точки с видимыми объектами и clearance вокруг неё;
- conservative vehicle envelope для выбранного набора техники;
- диагностические labels и Shape preview.

Основная команда editor-проверки — `Check ambient vehicle spawning`. Она анализирует открытый мир и размещённые в нём spawn points. Некоторые проверки могут блокировать очевидно неполное размещение точки, но это не является симуляцией runtime spawn.

Демонстрационный мир production:

```text
ME_Vehicle_Spawn/worlds/ME_TestWorld.ent
```

## Что происходит в vanilla runtime

Ambient vehicle spawn point не создаёт технику в момент размещения сущности. Упрощённая последовательность выглядит так:

```text
GameMode и game flags
        ↓
SCR_AmbientVehicleSystem
        ↓
регистрация spawn point
        ↓
OnUpdatePoint / ProcessSpawnpoint
        ↓
faction и VEHICLE catalog
        ↓
include/exclude label filtering
        ↓
выбор prefab
        ↓
проверка позиции и SpawnEntityPrefabEx
        ↓
GetOnVehicleSpawned
```

### 1. GameMode и prerequisites

В мире должен быть активный GameMode с применимым `SCR_BaseGameMode`. Для обычного ambient vehicle flow должен быть установлен `EGameFlags.SpawnVehicles`.

Для faction-aware spawn point также нужны:

- совместимый `FactionManager`;
- разрешаемая faction;
- доступный faction-specific `VEHICLE` catalog.

### 2. Инициализация и регистрация

`SCR_AmbientVehicleSystem` инициализируется в контексте мира. При инициализации `SCR_AmbientVehicleSpawnPointComponent` регистрирует точку в ambient system.

Регистрация означает только то, что система увидела точку и добавила её в список update points. Она не подтверждает:

- наличие `SpawnVehicles` flag;
- доступность faction;
- наличие подходящего prefab;
- успешное создание vehicle.

### 3. Обработка точки

Ambient system периодически обрабатывает зарегистрированные точки. В зависимости от timer, состояния системы, числа игроков и game flags вызываются `OnUpdatePoint` и `ProcessSpawnpoint`.

Если `EGameFlags.SpawnVehicles` отсутствует, vanilla guard может отключить update point после `super.OnUpdatePoint(args)`. Поэтому включённая на входе точка ещё не гарантирует продолжение обработки.

### 4. Выбор catalog и prefab

Для faction-aware точки используется faction-specific `VEHICLE` catalog. Для factionless точки применяется global `VEHICLE` catalog.

Затем к записям catalog применяются editable entity labels:

- `m_aIncludedEditableEntityLabels` — обязательные included labels;
- `m_aExcludedEditableEntityLabels` — labels, исключающие кандидата;
- `m_bRequireAllIncludedLabels` — требовать все included labels или достаточно одного.

При текущем vanilla default `m_bRequireAllIncludedLabels = false`, поэтому несколько included labels обычно имеют семантику **any included label**. После include-фильтра из результата удаляются записи с excluded labels.

Упрощённо:

```text
применимый VEHICLE catalog
        ↓
include labels
        ↓
exclude labels
        ↓
список кандидатов
        ↓
выбранный prefab
```

У базового prefab ambient spawn point наследуется исключение `TRAIT_ARMED`. Это важно учитывать даже тогда, когда поле не видно как локально заданное в производном prefab.

### 5. Проверка позиции и создание

После выбора prefab vanilla flow проверяет возможность размещения техники и пытается создать entity через `SpawnEntityPrefabEx` на transform spawn point.

Успешный `FindEmptyTerrainPosition`, зелёный Shape, найденный catalog candidate или заполненный `m_sPrefab` сами по себе не доказывают, что vehicle была создана. Наиболее сильное подтверждение завершённого spawn — callback `GetOnVehicleSpawned` с созданной `Vehicle`.

## Что именно показывает production preview

Production preview отвечает на ограниченные editor-вопросы:

```text
есть ли известная проблема конфигурации точки;
есть ли подозрение на пересечение или отсутствие clearance;
какой набор техники связан с текущим filter context;
какой conservative envelope можно показать в редакторе.
```

Shape и envelope строятся как editor aids. Они не являются runtime collision model и не учитывают все проверки, которые выполняются при фактическом создании entity:

- полный placement и collision flow;
- physics и network initialization;
- runtime transform и поведение entity;
- успешное выполнение `SpawnEntityPrefabEx`;
- последующие runtime errors.

Поэтому корректная формулировка результата preview — «известные предпосылки и геометрические подозрения не обнаружены», а не «техника обязательно заспавнится».

## Bounds pipeline

Для conservative vehicle envelope используется отдельный pipeline:

```text
ME_Vehicle_Bounds_Toolkit
    ↓
per-prefab bounds Candidate/Baseline
    ↓
ME_Vehicle_Spawn_Test
    ↓
реальные faction/label filters ambient points
    ↓
aggregate bounds snapshot
    ↓
ME_Vehicle_Spawn
    ↓
editor envelope preview
```

Ответственность разделена следующим образом:

- `ME_Vehicle_Bounds_Toolkit` измеряет отдельные prefab’ы и владеет их per-prefab regression baseline;
- `ME_Vehicle_Spawn_Test` читает проверенный VBT Candidate, применяет реальные ambient filters и агрегирует bounds;
- production использует принятый aggregate snapshot для preview и не зависит от runtime spawning.

Канонический aggregate resource:

```text
{1C3AE4A8F2630BF7}Configs/Generated/ME_VehicleBoundsSnapshot.conf
```

Изменение envelope не означает изменение runtime collision и не является доказательством успешного spawn.

## Test addon и runtime diagnostics

`ME_Vehicle_Spawn_Test` сохраняет vanilla flow через `super` и добавляет наблюдение вокруг него.

Основные overrides:

- `ME_DebugAmbientVehicleSystem` — lifecycle ambient system, registration, update points, `ProcessSpawnpoint` и completed-spawn callback;
- `ME_DebugAmbientVehicleSpawnPointComponent` — инициализация точки, terrain-position probing и состояние регистрации;
- `ME_DebugBaseGameMode` — переходы GameMode, запуск game loop и создание игрока;
- `ME_DebugFactionCatalogInitialization` — диагностика инициализации faction catalogs.

Основные log prefixes:

- `[ME_DEBUG_AVSP]` — lifecycle и состояние spawn point;
- `[ME_DEBUG_AVSP_POS]` — terrain/free-space probing и координаты;
- `[ME_DEBUG_AVSP_SYS]` — ambient system, registration, update points и spawn callbacks;
- `[ME_DEBUG_AVSP_GM]` — GameMode, game loop и player lifecycle;
- `[ME_DEBUG_AVSP_WB]` — Workbench-only diagnostics и snapshot tooling.

Используйте:

- `script.log` — diagnostic `Print`/`PrintFormat`;
- `error.log` — ошибки компиляции, загрузки ресурсов и runtime;
- `console.log` — полный engine context.

Compile errors нужно проверять до входа в Game mode: после ошибки компиляции дальнейшие runtime-наблюдения могут быть недостоверны.

## Подтверждённые режимы и сравнение

### `ME_MpTest`

В `ME_MpTest` подтверждено, что:

- мир достигает `GAME`;
- `IsRunning() = true` и `IsMaster() = true`;
- создаётся локальный игрок;
- ambient system инициализируется с одной точкой;
- первый `OnUpdatePoint` входит с `enabled=1`, но после `super` получает `enabled=0`.

Причина — отсутствие `EGameFlags.SpawnVehicles`. `GameMode_Plain.et` не переопределяет базовое поле `m_eTestGameFlags`, поэтому его значение равно `0`.

### Campaign baseline

Для чистого сравнения используется отдельная Workbench-сессия с:

```text
worlds/MP/CTI_Campaign_Eden.ent
```

В подтверждённом baseline:

- используется `SCR_GameModeCampaign`;
- инициализируются 171 spawn point;
- первый `OnUpdatePoint` имеет `enabled=1` и `spawnVehicles=1` до и после `super`;
- затем запускается `ProcessSpawnpoint`.

Не используйте для этого сравнения terrain-only `worlds/Eden/Eden.ent` и не открывайте campaign поверх Test world: это смешивает GameMode, entities и spawn points.

Значения flags:

```text
EGameFlags.SpawnVehicles = 2
EGameFlags.SpawnAI       = 4
m_eTestGameFlags         = 6  → SpawnVehicles | SpawnAI
```

Не добавляйте override `Enable(bool)`: в текущем API он конфликтует с native declaration и ломает компиляцию.

## Editable entity labels snapshot

В Test addon добавлена экспериментальная схема `ME_EditableEntityLabelsSnapshot` и Workbench-инструменты для сохранения labels, сгруппированных по scope, label и prefab.

Цель snapshot:

- зафиксировать состав labels, участвующих в ambient filtering;
- проверять фильтрацию независимо от одной выбранной точки;
- иметь воспроизводимый материал для диагностики изменений catalog’ов.

Текущая реализация генератора пока не считается завершённой: код компилируется, но запуск через опробованные Workbench menu actions не был подтверждён. Кроме того, прямой доступ к runtime catalogs в Edit mode ограничен наличием активного Game context.

Для дальнейшей работы рассматриваются два подхода:

1. **Fixture world** — собрать labels через существующие spawn points и их runtime catalog filtering. Это проверенный для текущего проекта путь, но он покрывает только набор entries, выбранный fixture-конфигурацией.
2. **Прямое чтение prefab sources** — получить полный справочник prefab’ов независимо от spawn points. Этот вариант требует самостоятельного разбора `.et` resources и разрешения наследования.

До завершения этой проверки snapshot не следует использовать как полный справочник всех vehicle labels base game.

## Как читать результаты диагностики

| Наблюдение | Что оно подтверждает | Чего оно не подтверждает |
| --- | --- | --- |
| Point присутствует в списке ambient system | point зарегистрирован | vehicle будет создана |
| `m_sPrefab` заполнен | выбран candidate prefab | prefab успешно прошёл spawn |
| `ProcessSpawnpoint` вызван | point обработана update loop | обработка завершилась созданием |
| filter вернул entries | есть catalog candidates | конкретная vehicle будет создана |
| `FindEmptyTerrainPosition` вернул `true` | helper нашёл candidate position | `SpawnEntityPrefabEx` завершится успешно |
| Shape или envelope не показывают проблему | editor preflight не нашёл известного конфликта | runtime collision, physics и initialization пройдут |
| `GetOnVehicleSpawned` получил `Vehicle` | vehicle entity создана этим ambient flow | она будет исправна во всех последующих условиях |

## Рабочий процесс в Workbench

1. Загрузить только один addon-проект — production или Test.
2. Дождаться resource scan и проверить `error.log`.
3. При необходимости зарегистрировать/rebuild resources и перезагрузить scripts.
4. Для production открыть `ME_Vehicle_Spawn/worlds/ME_TestWorld.ent`.
5. Для runtime diagnostics использовать отдельный Test world.
6. В Edit mode выполнить `Check ambient vehicle spawning` и проверить GameMode, flags, faction и catalog.
7. В Game mode сопоставить `OnUpdatePoint`, `ProcessSpawnpoint` и catalog/filter logs.
8. Для подтверждения создания искать `GetOnVehicleSpawned` с `Vehicle`, а не только Shape, selected prefab или статус обработки.
9. После наблюдения вызвать `wb_stop` и повторно проверить `script.log` и `error.log`.
10. Campaign baseline запускать только в чистой сессии с `worlds/MP/CTI_Campaign_Eden.ent`.

## Итоговая модель

```text
Production:
  проверяет prerequisites и показывает editor preview

Vanilla:
  регистрирует point, фильтрует catalog,
  выбирает prefab и пытается создать vehicle

Test:
  наблюдает vanilla lifecycle и сохраняет evidence

VBT + Test aggregate:
  измеряют и группируют bounds для conservative preview

Runtime callback:
  единственный надёжный признак конкретного завершённого spawn
```

## Связанные документы

- [`AMBIENT_VEHICLE_SPAWNPOINT.md`](AMBIENT_VEHICLE_SPAWNPOINT.md) — подробный справочник vanilla lifecycle, catalog filtering и границ preview;
- [`TEST_DIAGNOSTICS.md`](TEST_DIAGNOSTICS.md) — значения flags, log prefixes и campaign comparison;
- [`PRODUCTION_WORKFLOW.md`](PRODUCTION_WORKFLOW.md) — production workflow в Workbench;
- [`PROJECT_SPLIT.md`](PROJECT_SPLIT.md) — границы production и Test addon;
- [`../ME_Vehicle_Spawn_Test/VEHICLE_BOUNDS_REGRESSION.md`](../ME_Vehicle_Spawn_Test/VEHICLE_BOUNDS_REGRESSION.md) — bounds Candidate/Baseline и aggregate workflow;
- [`../TEST_TO_PROD_TRANSFER.md`](../TEST_TO_PROD_TRANSFER.md) — перенос проверенных изменений из Test в production.
