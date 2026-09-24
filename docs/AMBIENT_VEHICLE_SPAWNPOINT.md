# Справочник Ambient Vehicle Spawnpoint

## Область документа и границы подтверждения

Этот справочник фиксирует подтверждённую модель vanilla ambient vehicle spawning в текущем проекте и правила интерпретации его диагностики. Имена классов, методов, enum, resource paths, команды Workbench и log prefixes оставлены без перевода, чтобы их можно было сопоставить с кодом и журналами.

Подтверждение основано на текущем состоянии vanilla resources/scripts и наблюдениях в Workbench/Diag. Vanilla scripts и конфигурации могут измениться между версиями игры, поэтому при обновлении игры нужно повторно проверить описанные места, значения и порядок вызовов. Этот файл не заменяет исходные scripts и не обещает, что поведение сохранится в будущих версиях.

В репозитории есть два взаимоисключающих addon-проекта:

- `ME_Vehicle_Spawn` — production addon для editor-only placement checks, warning plugin, label display и визуального preview;
- `ME_Vehicle_Spawn_Test` — Test addon с runtime diagnostics, экспериментальными мирами и regression/autotest tooling.

Не подключайте оба addon одновременно. В Test есть runtime `modded class` overrides для vanilla-классов; одновременная загрузка с production может привести к конфликту overrides и смешать диагностические результаты. Production не следует описывать как standalone runtime spawning system: он наблюдает и подсказывает при работе в World Editor, но не заменяет vanilla spawn flow.

## Vanilla lifecycle

Ambient vehicle point не создаёт технику в момент размещения сущности. Упрощённая последовательность выглядит так:

1. **GameMode и prerequisites.** В мире должен быть активный GameMode с применимым `SCR_BaseGameMode`, а для обычного ambient flow должен быть установлен `EGameFlags.SpawnVehicles`. Для faction-aware точек нужен совместимый faction manager и доступная faction.
2. **Инициализация `SCR_AmbientVehicleSystem`.** World system создаётся и инициализируется в контексте мира. При инициализации `SCR_AmbientVehicleSpawnPointComponent` регистрирует себя через ambient system.
3. **Регистрация point.** Зарегистрированная точка появляется в списке ambient system. Это означает, что система её увидела и может включить в обход; это ещё не означает, что для неё найдётся prefab или что entity будет создана.
4. **`OnUpdatePoint` и `ProcessSpawnpoint`.** Ambient system периодически обрабатывает update point. В зависимости от timer, числа игроков, состояния системы и game flags вызывается `ProcessSpawnpoint` для очередной зарегистрированной точки. Guard по `EGameFlags.SpawnVehicles` может отключить point после `super.OnUpdatePoint(args)`.
5. **Faction и catalog selection.** При необходимости point разрешает свою faction, выбирает применимый `VEHICLE` catalog и фильтрует его по editable entity labels. Из результата vanilla code выбирает prefab и сохраняет его во внутреннем состоянии point, включая `m_sPrefab`.
6. **Проверка позиции и spawn.** Для выбранного prefab выполняется runtime flow проверки позиции/свободного пространства, после чего vanilla code пытается создать vehicle entity через `SpawnEntityPrefabEx` на transform точки. Точный результат зависит не только от каталога и геометрической проверки, но и от дальнейшего создания и инициализации entity.
7. **`GetOnVehicleSpawned`.** Callback ambient system вызывается для созданного vehicle. Подписка на `GetOnVehicleSpawned` — наиболее сильное наблюдение в текущей диагностике: callback с `Vehicle` означает, что vehicle entity была создана, в отличие от одного только выбранного `m_sPrefab`.

### Что означает состояние точки

| Стадия | Что подтверждает | Чего не подтверждает |
| --- | --- | --- |
| **registered spawn point** | `SCR_AmbientVehicleSpawnPointComponent` зарегистрирован в `SCR_AmbientVehicleSystem` и попал в его список | наличие GameMode flag, faction, catalog match, выбранного prefab или созданной entity |
| **selected prefab** | catalog selection дала prefab, обычно отражённый в `m_sPrefab` | свободное место, успешный `SpawnEntityPrefabEx`, корректную инициализацию vehicle или callback |
| **processed point** | `ProcessSpawnpoint` был вызван для точки и vanilla flow дошёл до её обработки | что filter дал кандидата или что vehicle появилась; обработка может завершиться без spawn |
| **successfully spawned vehicle** | создана vehicle entity и вызван `GetOnVehicleSpawned` с point и `Vehicle` | долгосрочную исправность техники, отсутствие последующих runtime ошибок или пригодность точки для всех других prefab |

## GameMode и `EGameFlags.SpawnVehicles`

`SCR_BaseGameMode.EOnInit()` применяет `m_eTestGameFlags` через `GetGame().SetGameFlags(...)`, пока flags не были получены другим способом. В подтверждённой текущей конфигурации значения имеют следующий смысл:

- `EGameFlags.SpawnVehicles = 2`;
- `EGameFlags.SpawnAI = 4`;
- `m_eTestGameFlags = 6` означает `SpawnVehicles | SpawnAI`.

Plain/Test world, использующий `GameMode_Plain.et` без override этого поля, получает базовое значение `m_eTestGameFlags = 0`. Поэтому наличие одной ambient spawn point и её успешная регистрация не включает vehicle spawning. В наблюдаемом `ME_MpTest` первый `OnUpdatePoint` входил с `enabled=1`, но после единственного `super.OnUpdatePoint(args)` система имела `enabled=0`: сработал vanilla guard `!GetGame().AreGameFlagsSet(EGameFlags.SpawnVehicles)`.

Чистый campaign baseline ведёт себя иначе: `SCR_GameModeCampaign` инициализирует 171 point; первый `OnUpdatePoint` имеет `enabled=1` и `spawnVehicles=1` до и после `super`, после чего вызывается `ProcessSpawnpoint`. Для сравнения используйте чистый `worlds/MP/CTI_Campaign_Eden.ent` в отдельной сессии. Не подменяйте его terrain-only `worlds/Eden/Eden.ent`: это другой контекст без нужного GameMode, и он не является campaign baseline.

Не добавляйте override native `Enable(bool)`. В текущем API такая декларация конфликтует с native declaration и ломает компиляцию. Нужные причины выключения выясняйте по GameMode, flags и runtime logs.

## Faction и catalog filtering

### Откуда берутся faction и catalog

`SCR_FactionAffiliationComponent` на spawn point задаёт default/current faction affiliation. `SCR_FactionManager`/`FactionManager` разрешает faction key в фактический объект faction. Если faction доступна, point использует faction-specific `VEHICLE` catalog через `SCR_Faction.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE)`. Для factionless point используется global `VEHICLE` catalog, предоставленный `SCR_EntityCatalogManagerComponent`.

Поэтому registered point с несовместимой faction не обязан исчезать из списка регистрации. Регистрация компонента и последующая возможность выбрать vehicle — разные стадии. Несовместимость проявляется при разрешении faction/catalog или позднее при фильтрации и spawn flow.

### Editable entity labels

Vanilla component фильтрует записи `SCR_EntityCatalogEntry` по полям:

- `m_aIncludedEditableEntityLabels` — labels, которые должны присутствовать у кандидата;
- `m_aExcludedEditableEntityLabels` — labels, исключающие кандидата;
- `m_bRequireAllIncludedLabels` — нужно ли требовать все включённые labels.

В текущем vanilla default `m_bRequireAllIncludedLabels` сериализуется как `0`. Это означает семантику **any included label**: при нескольких включённых labels кандидат может соответствовать любой из них, после чего применяется исключение. Если включённые labels пусты, point не добавляет ограничения по include и фактически рассматривает весь применимый catalog с учётом exclude.

Упрощённая модель фильтра:

1. взять faction-specific или global `VEHICLE` catalog;
2. оставить записи с подходящими include labels — любой label при `requireAll=false`, все labels при `requireAll=true`;
3. удалить записи с excluded labels;
4. если результат пуст, prefab выбрать нельзя.

Общее label из include и exclude — сильный признак противоречивой конфигурации, но его нельзя автоматически объявлять причиной пустого результата во всех случаях. При `m_bRequireAllIncludedLabels=true` один такой label делает требование невыполнимым. При default `false` candidate может соответствовать другому included label и не содержать excluded label, поэтому overlap сам по себе ещё не доказывает пустой результат. Для вывода `no vehicle matches` нужно учитывать фактический catalog и весь фильтр.

### Унаследованный `TRAIT_ARMED`

Базовый prefab

```text
Prefabs/Systems/AmbientVehicles/AmbientVehicleSpawnpoint_Base.et
```

уже задаёт `m_aExcludedEditableEntityLabels = { TRAIT_ARMED }`. Это исключение наследуется `AmbientVehicleSpawnpoint_US.et` и производными точками, даже если в локальном prefab его не видно как повторённое поле. Если point явно включает `TRAIT_ARMED`, нужно проверить `m_bRequireAllIncludedLabels` и фактический catalog: при `requireAll=true` такое требование несовместимо с inherited exclude; при `requireAll=false` наличие overlap не всегда опустошает результат.

### Когда вызывается `Update(SCR_Faction)`

Важное ограничение для диагностики: `SCR_AmbientVehicleSpawnPointComponent.Update(SCR_Faction)` вызывается из spawn flow, а не как одноразовая проверка при init. В текущем vanilla поведении он вызывается, когда affiliation отличается от `m_SavedFaction`, либо для factionless point, пока `m_sPrefab` ещё пуст. Поэтому label filter, который повторяет Test diagnostic, должен трактоваться как проверка spawn-time пути. Наличие point в editor и наличие записи в catalog до входа в этот путь не равны фактическому spawn.

## Terrain и свободное пространство

В editor/test diagnostics используется `SCR_WorldTools.FindEmptyTerrainPosition` как отдельная проверка, может ли рядом с transform точки быть найден подходящий свободный terrain position. В текущем preview вызов использует ограниченный поиск и trace flags для terrain/entities/ocean, а Shape рисуется в transform точки и окрашивается по результату.

Эта проверка отвечает на более узкий вопрос: удалось ли helper найти candidate position в заданном радиусе и с заданными ограничениями. Она не является копией всего runtime spawn flow. В частности, она не доказывает:

- что конкретный выбранный vehicle prefab поместится в найденной позиции;
- что runtime использует именно возвращённый helper position вместо transform точки;
- что все collision, physics, terrain, network и entity initialization checks пройдут;
- что `SpawnEntityPrefabEx` создаст entity и что созданная entity будет корректно работать.

Runtime пытается создать выбранный prefab через `SpawnEntityPrefabEx` на transform точки в рамках vanilla spawn flow. Поэтому зелёный `FindEmptyTerrainPosition`, Shape или editor envelope должны считаться только preflight/visual aid. Единственным наблюдением завершённого создания в этом справочнике является callback `GetOnVehicleSpawned` и связанное с ним runtime evidence.

## Production editor diagnostics

`ME_Vehicle_Spawn` проверяет конфигурацию мира до и во время работы в World Editor. Warning plugin и связанные component diagnostics могут проверять:

- наличие ровно одного применимого `SCR_BaseGameMode` и доступность его editable layer;
- наличие `m_eTestGameFlags` и включённого `EGameFlags.SpawnVehicles`;
- наличие `FactionManager`;
- доступность faction keys для размещённых или добавляемых points;
- доступность и непустоту global `VEHICLE` catalog для factionless points;
- label configuration и результат чтения применимого catalog;
- пересечения, static-object conflicts и clearance вокруг point;
- Shape, label display и conservative vehicle envelope preview.

Сфера становится серой при конфликте include/exclude, отсутствии подходящих кандидатов или недоступном каталоге. Над точкой отображаются отдельные строки ERROR для конфликта меток и пустого результата (с include/exclude), WARNING для недоступного каталога. Пересечения сфер и границ статических объектов показываются отдельными сообщениями с именами и координатами. Проверка границ объектов остаётся предварительной геометрической диагностикой.

Перед габаритами показана жёлтая стрелка направления локальной +Z с подписью угла 0–359 в формате `90 deg`. Стрелка расположена на 0,3 м выше максимальной выборки рельефа под ней; подпись — над стрелкой. Подсказки обновляются при перемещении/вращении точки и очищаются при удалении. Прозрачные сферы и габариты используют NOZWRITE с сохранением проверки глубины; габариты состоят из 12 треугольников, стрелка — из 3.

Команда `Check ambient vehicle spawning` собирает эти наблюдения для открытого мира. Drag-and-drop point может быть заблокирован, если базовые prerequisites явно отсутствуют; это защита от очевидно неполной editor-конфигурации, а не runtime simulation.

Shape/clearance preview, предупреждения overlap/static-object, catalog match, registered point и conservative envelope отвечают на разные диагностические вопросы. Ни один из них по отдельности или в совокупности не гарантирует runtime spawn, корректный transform, успешную инициализацию vehicle или отсутствие последующих runtime ошибок. Production остаётся editor-only tooling и не должен использоваться как замена runtime evidence из Test.

## Bounds pipeline и canonical aggregate

Для envelope preview применяется разделённый bounds pipeline:

1. `ME_Vehicle_Bounds_Toolkit` владеет измерением per-prefab AABB и проверкой своего VBT Candidate/Baseline;
2. `ME_Vehicle_Spawn_Test` потребляет проверенный VBT Candidate, применяет фактические faction/label filters и агрегирует bounds по faction и catalog labels;
3. production использует принятый aggregate snapshot для editor preview и не зависит от VBT.

Canonical aggregate resource имеет путь:

```text
{1C3AE4A8F2630BF7}Configs/Generated/ME_VehicleBoundsSnapshot.conf
```

Этот ресурс нужен для консервативного editor envelope и не является runtime collision model или доказательством spawn. Schema, generator workflow, Candidate/Baseline checks и проверка детерминированности описаны отдельно в `ME_Vehicle_Spawn_Test/VEHICLE_BOUNDS_REGRESSION.md`; здесь они не дублируются.

## Test diagnostics и evidence

`ME_Vehicle_Spawn_Test` сохраняет vanilla flow и добавляет наблюдение вокруг него. Основные prefixes в `script.log`:

- `[ME_DEBUG_AVSP]` — lifecycle и состояние `SCR_AmbientVehicleSpawnPointComponent`;
- `[ME_DEBUG_AVSP_POS]` — terrain/free-space probing и координаты;
- `[ME_DEBUG_AVSP_SYS]` — `SCR_AmbientVehicleSystem`, update points, registration, `ProcessSpawnpoint` и completed-spawn callback;
- `[ME_DEBUG_AVSP_GM]` — GameMode state transitions, game loop и player creation;
- `[ME_DEBUG_AVSP_WB]` — Workbench-only diagnostics, включая bounds snapshot generation/validation.

Дополнительные prefixes могут появляться для label, faction и ошибок; их следует читать вместе с основными lifecycle records, а не использовать как самостоятельное доказательство.

Для анализа журналов используйте:

- `script.log` — `Print`/`PrintFormat`, Test diagnostics и runtime observations;
- `error.log` — ошибки компиляции, загрузки ресурсов и runtime errors;
- `console.log` — полный engine context, полезный для отделения проблем окружения от ошибок addon.

Надёжная completed-spawn запись должна быть связана с `GetOnVehicleSpawned` и содержать созданную `Vehicle`, а не только `m_sPrefab`, candidate count или `ProcessSpawnpoint ENTER/EXIT`.

### Campaign baseline

Для контрольного сравнения откройте в чистой Workbench-сессии:

```text
worlds/MP/CTI_Campaign_Eden.ent
```

После полной загрузки campaign baseline подтверждался как `SCR_GameModeCampaign` с 171 зарегистрированной точкой; первый `OnUpdatePoint` видел `enabled=1` и `spawnVehicles=1`, после `super` значения сохранялись, затем запускался `ProcessSpawnpoint`.

Не используйте для этого сравнения:

- terrain-only `worlds/Eden/Eden.ent`, где нет нужного GameMode и ambient system стартует с `enabled=0` и нулём spawn points;
- campaign, загруженную поверх `ME_MpTest` или другого Test мира — это смешивает entities, GameMode и spawn points.

## Workbench validation checklist

1. Закройте сессию другого addon и загрузите только нужный `addon.gproj`.
2. Для production откройте `ME_Vehicle_Spawn/worlds/ME_TestWorld.ent`; для runtime diagnostics используйте отдельную Test-сессию и её fixture/world.
3. Дождитесь resource scan, зарегистрируйте/rebuild resources при необходимости и перезагрузите scripts.
4. Проверьте `error.log` до входа в Game mode: compile error делает последующие runtime observations недостоверными.
5. В Edit mode выполните `Check ambient vehicle spawning`; проверьте GameMode, `m_eTestGameFlags`, `EGameFlags.SpawnVehicles`, `FactionManager`, faction и catalog.
6. Проверьте, что point зарегистрирован, но не принимайте registration за spawn.
7. В Game mode дождитесь `OnUpdatePoint`/`ProcessSpawnpoint` и сопоставьте label, position и system logs.
8. Для подтверждения создания ищите `GetOnVehicleSpawned`/`VehicleSpawned`, а не только selected prefab, Shape или `status=processed`.
9. После наблюдения обязательно вызовите `wb_stop`, вернитесь в Edit mode и повторно проверьте `script.log` и `error.log`.
10. Для campaign comparison повторите шаги в чистой сессии с `worlds/MP/CTI_Campaign_Eden.ent`; не наслаивайте миры.

Markdown-изменения этого справочника не требуют Workbench или сборки addon. При изменении scripts, resources или fixture следуйте связанным workflow-документам.

## Границы достоверности

Полезно явно разделять наблюдение, допустимый вывод и недопустимую гарантию:

| Наблюдение | Допустимый вывод | Нельзя утверждать только на этом основании |
| --- | --- | --- |
| Point присутствует в `GetSpawnpoints()` | system зарегистрировал point | vehicle будет создана |
| `m_sPrefab` заполнен после catalog selection | выбран candidate prefab | prefab прошёл placement/spawn |
| `ProcessSpawnpoint` вызван | point была обработана update loop | обработка завершилась spawn |
| include/exclude filter вернул entries | есть catalog candidates для текущего filter context | конкретный runtime prefab обязательно создастся |
| `FindEmptyTerrainPosition` вернул `true` | helper нашёл подходящий candidate position в своих ограничениях | `SpawnEntityPrefabEx` создаст vehicle на transform точки |
| Shape зелёный или envelope не пересекает видимые объекты | editor preflight не нашёл соответствующую проблему | runtime collision, physics, network и initialization гарантированно пройдут |
| `GetOnVehicleSpawned` получил `Vehicle` | vehicle entity была создана этим ambient flow | она будет исправна во всех последующих кадрах и для всех условий |

Каноническая формулировка для документации и сообщений: editor diagnostics показывают отсутствие или наличие известных предпосылок и геометрических подозрений; runtime callback подтверждает конкретное завершённое создание. Ни registration, ни catalog match, ни `FindEmptyTerrainPosition`, ни Shape, ни conservative envelope не являются гарантией runtime spawn.

## Связанные документы

- [`docs/PROJECT_SPLIT.md`](PROJECT_SPLIT.md) — границы production/Test и состав addon-проектов;
- [`docs/PRODUCTION_WORKFLOW.md`](PRODUCTION_WORKFLOW.md) — production-проверка в Workbench;
- [`docs/TEST_DIAGNOSTICS.md`](TEST_DIAGNOSTICS.md) — значения flags, prefixes и campaign comparison;
- [`CLI_AUTOTESTS.md`](../CLI_AUTOTESTS.md) — оконные CLI autotests для `ME_Vehicle_Spawn_Test`;
- [`ME_Vehicle_Spawn_Test/VEHICLE_BOUNDS_REGRESSION.md`](../ME_Vehicle_Spawn_Test/VEHICLE_BOUNDS_REGRESSION.md) — bounds Candidate/Baseline и canonical aggregate workflow;
- [`TEST_TO_PROD_TRANSFER.md`](../TEST_TO_PROD_TRANSFER.md) — порядок переноса подтверждённых изменений и сверки документации.
