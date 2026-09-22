# Архитектура генератора labels snapshot

## Краткий план

Сейчас labels-плагин пытается читать runtime entity catalogs напрямую, но это не работает в World Editor — catalog manager существует только в game mode.

Есть два способа исправить:

**A. Fixture world (простой, как bounds-плагин):**
- Открываем существующий test world с 7 spawn points
- Каждый spawn point уже умеет отдавать свои отфильтрованные vehicles с labels
- Собираем labels со всех 7 spawn points
- ❌ Покрывает не все vehicles, а только те, что попадают под фильтры этих 7 spawn points
- ✅ Быстро работает, проверенный подход

**B. Прямое чтение prefabs (сложный, универсальный):**
- Берём список всех vehicle prefabs из VBT Candidate
- Для каждого prefab читаем его `.et` файл напрямую
- Извлекаем labels из prefab definition
- ✅ Покрывает все vehicles base game
- ❌ Сложнее код, нужно парсить prefab structure

**Вопрос:** зачем нужен этот snapshot — для проверки spawn points или как полный справочник всех vehicles?

---

## Проблема

Изначальный план предлагал собирать editable entity labels напрямую из runtime entity catalogs через `SCR_EntityCatalogManagerComponent` и `SCR_Faction`. Но это не работает в World Editor:

1. `SCR_EntityCatalogManagerComponent.GetInstance()` возвращает `null` в edit mode — компонент существует только в runtime game mode с инициализированной сущностью game mode.
2. `SCR_Faction.Init()` вызывается только когда `!IsEditMode()`, поэтому в редакторе `m_aEntityCatalogs` остаётся заполненным, но `m_mEntityCatalogs` (map для быстрого доступа) не инициализирован.
3. Попытка инициализировать faction catalogs через `ME_EnsureEditorCatalogsInitialized()` работает только если в открытом world есть runtime `Game` с `FactionManager`.

## Рабочий образец: bounds-плагин

Bounds-плагин (`ME_GenerateVehicleBoundsSnapshotPlugin`) успешно работает, используя **fixture world подход**:

1. **Требует открытый world**: плагин наследует `WorldEditorPlugin` (не `WorkbenchPlugin`) и работает только с открытым `.ent` world.
2. **Использует WorldEditorAPI**: перечисляет pre-placed spawn point entities через `api.FindEntityByName()`.
3. **Делегирует catalog-работу spawn points**: каждый `SCR_AmbientVehicleSpawnPointComponent` имеет метод `ME_GetEditorVehicleAggregateSelection()`, который:
   - Находит свою faction через runtime `FactionManager.GetFactionByKey()`
   - Вызывает `faction.ME_EnsureEditorCatalogsInitialized()` для инициализации catalog map
   - Возвращает отфильтрованные `SCR_EntityCatalogEntry` через `GetFullFilteredEntityListWithLabels()`
4. **Fixture world содержит runtime Game**: открытый world имеет game mode entity, поэтому `GetGame()`, `FactionManager`, и faction catalogs работают.

## Решение для labels-плагина

### Подход A: Fixture world (как bounds-плагин)

Использовать тот же fixture world `worlds/TestCases/ME_VehicleBoundsSnapshot.ent` с pre-placed spawn points:

**Плюсы:**
- Гарантированно работает — проверенный bounds-плагином подход
- Переиспользует существующий fixture world
- Получает точно те же catalog entries, что и bounds-плагин
- Labels собираются через ту же `GetEditableEntityLabels()` на уже отфильтрованных entries

**Минусы:**
- Fixture world содержит только 7 spawn points с конкретными комбинациями include/exclude labels
- Не покрывает все возможные vehicle prefabs — только те, что выбираются этими 7 комбинациями фильтров
- Если base game добавит новый vehicle prefab, который не попадает ни под один из 7 фильтров, он не появится в snapshot

**Реализация:**
1. Изменить `WorkbenchPlugin` → `WorldEditorPlugin`
2. Проверить, что world открыт через `Workbench.GetModule(WorldEditor)`
3. Перечислить те же 7 spawn point names, что и bounds-плагин
4. Для каждого вызвать `ME_GetEditorVehicleAggregateSelection()` для получения faction key и filtered entries
5. Собрать все labels через `GetEditableEntityLabels()` и дедуплицировать по `(scope, label, prefab)`

### Подход B: Прямое чтение prefab sources

Обойти catalogs полностью и читать vehicle prefabs напрямую из filesystem:

**Плюсы:**
- Не зависит от fixture world
- Покрывает **все** vehicle prefabs base game
- Более универсальный — не привязан к конфигурации spawn points

**Минусы:**
- Нужно перечислить все vehicle prefabs — использовать VBT Candidate как источник списка
- Нужно напрямую парсить `.et` prefab через `Resource.Load()` и читать `SCR_EditableEntityUIInfo`
- Более сложная реализация — требует безопасного чтения BaseContainer для `m_aAuthoredLabels` и `m_aAutoLabels`
- Не использует runtime catalog filtering — нужно самостоятельно определять faction membership

**Реализация:**
1. Загрузить VBT Candidate как источник полного списка vehicle prefabs с их faction/type membership
2. Для каждого prefab path:
   - `Resource.Load()` prefab source
   - Найти `SCR_EditableEntityComponent` в BaseContainer
   - Прочитать `m_UIInfo` → `SCR_EditableEntityUIInfo`
   - Извлечь resolved labels из runtime enum (если prefab можно instantiate в редакторе)
   - Безопасно попытаться прочитать raw `m_aAuthoredLabels`/`m_aAutoLabels` arrays
3. Использовать VBT faction/type как scope для группировки

## Рекомендация

**Использовать Подход A (fixture world)** по следующим причинам:

1. **Совместимость с bounds-плагином**: оба плагина работают с одним fixture world и одинаковым набором spawn points, что упрощает debugging и понимание.
2. **Проверенная надёжность**: bounds-плагин уже использует этот подход и работает стабильно.
3. **Меньше кода**: делегируем catalog initialization и filtering существующему `ME_GetEditorVehicleAggregateSelection()`.
4. **Покрытие достаточное**: 7 spawn points с комбинациями `AllExceptArmed` / `All` для US/USSR/FIA/CIV фракций покрывают практически все base-game vehicles. Если потребуется полное покрытие, можно добавить spawn points в fixture world.

### Ограничения Подхода A

Fixture world подход **не** даст полного списка всех vehicle prefabs, если:
- Base game содержит vehicle с редкой комбинацией labels, не попадающей ни под один из 7 фильтров
- Нужен snapshot именно **всех** prefabs независимо от того, используются ли они ambient spawn system

Если цель — **полный каталог всех vehicle labels base game**, тогда нужен Подход B.

## Цель labels snapshot

**Уточнить у пользователя:**

Какая конечная цель этого snapshot?

1. **Diagnostic/validation для ambient spawn points**: проверить, что configured include/exclude labels корректно фильтруют vehicle prefabs → Подход A достаточен
2. **Полный reference всех vehicle editable entity labels base game**: независимый справочник всех labels для любого vehicle prefab → нужен Подход B
3. **Что-то ещё?**

От ответа зависит выбор архитектуры.
