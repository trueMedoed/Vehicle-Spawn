# Проблема генерации snapshot editable entity labels

## Контекст

Нужен воспроизводимый способ получить список всех editable entity labels для vehicle prefab'ов из editor catalogs (global и faction). Результат должен сохраняться в отдельный конфиг `Configs/Generated/ME_EditableEntityLabelsSnapshot.conf` в test addon `ME_Vehicle_Spawn_Test`.

## Зачем это нужно

Текущая проверка spawn point'ов работает только с выбранной в World Editor сущностью `AmbientVehicleSpawnPoint`. Нужен детерминированный snapshot всех vehicle labels из catalogs, чтобы:
- Проверять фильтрацию spawn point'ов независимо от выбора в редакторе
- Иметь стабильный baseline для регрессионных тестов
- Понимать, какие labels присвоены каким prefab'ам в разных faction catalogs

## Что уже есть

1. **Схема snapshot** — `ME_EditableEntityLabelsSnapshot.c` с четырьмя классами:
   - `ME_EditableEntityLabelsSnapshotPrefab` — путь к prefab, catalog index, entity name
   - `ME_EditableEntityLabelsSnapshotLabel` — имя label, массив prefab'ов с этим label
   - `ME_EditableEntityLabelsSnapshotScope` — ключ scope (global или faction key), массив labels
   - `ME_EditableEntityLabelsSnapshot` — корневой объект с schema version, generator version, game version, массивом scopes

2. **Плагин-генератор** — `ME_GenerateEditableEntityLabelsSnapshotPlugin.c`:
   - Наследует `WorldEditorPlugin`
   - В `Run()` получает game version, создаёт snapshot, сохраняет и валидирует
   - `ME_CreateLabelsSnapshot()` собирает данные из global catalog и всех faction catalogs
   - `ME_CollectCatalogLabels()` итерируется по `SCR_EntityCatalog.GetEntityList()`, для каждого entry вызывает `GetEditableEntityLabels()`, конвертирует enum через `typename.EnumToString()`
   - `ME_SortSnapshot()` обеспечивает детерминированный порядок: scopes по ключу, labels по имени, prefabs по пути
   - `ME_SaveAndValidateSnapshot()` сериализует через `BaseContainerTools`, вызывает `ResourceManager.RebuildResourceFile()`, перечитывает и проверяет round-trip

3. **Catalog helpers** — существующие методы из `ME_DebugFactionCatalogInitialization.c`:
   - `ME_GetEditorInstance()` — получить `SCR_EntityCatalogManagerComponent`
   - `ME_GetEditorGlobalVehicleCatalog()` — получить global vehicle catalog
   - `ME_EnsureEditorCatalogsInitialized()` — инициализировать faction catalogs

## Текущая проблема

Плагин скомпилирован без ошибок, но **не запускается**.

**Что пробовали:**
1. Запустить через `wb_execute_action` с путём `Tools,ME_Vehicle_Spawn/Entity Labels,Generate entity labels snapshots` — вернул `false`
2. Исправили путь на `Plugins,ME_Vehicle_Spawn/Entity Labels,Generate entity labels snapshots` — операция прервана

**Возможные причины:**
- Плагин не регистрируется в Workbench UI из-за неправильной конфигурации `WorkbenchPluginAttribute`
- Catalogs недоступны в Edit mode, нужен активный Game instance
- Нужно использовать fixture world pattern: загрузить test world, войти в Game mode, там вызвать генератор

## План action (подтверждённый из исходного плана)

Исходный план указывает на **"fixture world pattern (Approach A)"** как подтверждённый подход. Approach B (чтение через BaseContainer) провалился, потому что `Resource.Load().GetResource().ToBaseContainer()` возвращает корневой entity container, а не component structure.

Fixture world pattern означает:
1. Создать/использовать test world
2. Войти в Game mode
3. В Game mode spawn'ить entities из catalog (или использовать существующие spawn points)
4. Читать runtime `SCR_EditableEntityComponent` с живых entities
5. Собрать labels и сохранить snapshot

**НО:** Моя текущая реализация НЕ использует fixture world. Она пытается читать labels напрямую из `SCR_EntityCatalogEntry.GetEditableEntityLabels()` в Edit mode.

## Открытый вопрос

**Можно ли читать labels из catalog entries напрямую, или обязательно нужно spawn'ить entities?**

API search показал, что метод `SCR_EntityCatalogEntry.GetEditableEntityLabels(notnull out array<EEditableEntityLabel>)` существует. Это противоречит требованию плана использовать fixture world pattern.

Нужно проверить:
1. Доступны ли editor catalogs в Edit mode через `ME_GetEditorGlobalVehicleCatalog()`?
2. Возвращает ли `GetEditableEntityLabels()` правильные данные без spawn'а entity?
3. Или labels резолвятся только после создания entity в Game mode?
