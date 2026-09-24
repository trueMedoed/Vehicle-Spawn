# Changelog

История версий `ME_Vehicle_Spawn`, опубликованных в Workshop.

Записи упорядочены от новых к старым. Заголовок записи — номер версии Workshop и дата публикации. Номер версии присваивает Workshop, поля версии в `addon.gproj` нет, поэтому с кодом версию связывает git-тег `vX.Y.Z` на коммите, который был опубликован. Английский блок каждой записи — текст, отправленный в Workshop; русский блок ниже него служит внутренней сверкой и не публикуется.

## 1.0.5 | 2026-09-24

Проверка production и публикация версии 1.0.5 подтверждены пользователем 2026-09-24. Обновлено изображение мода.

### English

- Improved obstacle checks to account for object rotation, reducing false intersection warnings near rotated buildings and other objects.
- Replaced red obstacle spheres with wireframe boxes showing the oriented model bounds used by the checks.
- Intersections with static object bounds now appear as warnings to recheck placement. Failure to find an empty terrain position produces a separate error; overlapping spawn areas remain errors.
- Updated the vehicle bounds reference with readable faction groups and source prefabs for each bound.
- Added ME_EditableEntityLabelsSnapshot.conf, a faction-and-label reference listing vehicle prefabs from Arma Reforger 1.8.0.13 catalogs, plus a guide to interpreting spawn-point filters. This reference does not guarantee runtime spawning and may differ from modded catalogs.
- Updated the Workshop description with instructions for inspecting existing Conflict scenarios in World Editor and a GitHub link to the source code, test project, and audit reports.

### Русский

- Проверка препятствий теперь учитывает поворот объектов, уменьшая число ложных предупреждений рядом с повёрнутыми зданиями и другими объектами.
- Красные сферы препятствий заменены каркасами, показывающими ориентированные границы моделей, используемые при проверке.
- Пересечения с границами статических объектов теперь отображаются как предупреждения с просьбой перепроверить размещение. Если свободная позиция не найдена, выводится отдельная ошибка; пересечения областей появления двух точек остаются ошибками.
- Обновлён справочник габаритов техники: читаемые группы по фракциям и указание prefab, определивших границы.
- Добавлен ME_EditableEntityLabelsSnapshot.conf — справочник prefab техники по фракциям и меткам из каталогов Arma Reforger 1.8.0.13, а также инструкция по интерпретации фильтров точек. Справочник не гарантирует появление техники и может отличаться от каталогов с модификациями.
- В описание Workshop добавлены инструкция по проверке существующих сценариев Conflict в World Editor и ссылка на GitHub с исходным кодом, тестовым проектом и отчётами аудита.

## 1.0.4 | 2026-09-24

Проверка production и публикация версии 1.0.4 подтверждены пользователем 2026-09-24.

### English

- Spawn-area spheres now turn grey when labels conflict, no vehicle matches the filter, or the catalog cannot be checked.
- Filter errors appear as separate messages above the spawn point. Empty results also show the included and excluded labels.
- Added on-point error messages for overlapping spawn areas and intersections with static object bounds, including names and coordinates.
- Added a yellow direction arrow above the terrain, with a heading label in degrees (for example, 90 deg). The arrow and label follow point movement and rotation.
- Fixed stray polygons in the vehicle bounds preview and spheres disappearing inside translucent bounds when changing the viewing angle.

### Русский

- Сферы области появления становятся серыми при конфликте меток, отсутствии подходящей техники или невозможности проверить каталог.
- Ошибки фильтра отображаются отдельными сообщениями над точкой. При пустом результате также показаны включающие и исключающие метки.
- Добавлены сообщения над точкой о пересечениях областей появления и границ статических объектов, с именами и координатами.
- Добавлена жёлтая стрелка направления над рельефом с подписью угла в градусах (например, 90 deg). Стрелка и подпись обновляются при перемещении и повороте точки.
- Исправлены посторонние полигоны предпросмотра габаритов и исчезновение сфер внутри полупрозрачных габаритов при смене ракурса.

## 1.0.3 | 2026-09-10

### English

Reduced the size of the vehicle label text shown above spawn points in the 3D view, along with the spacing between labels.
The labels no longer take up excessive screen space when the camera is moved close to a point.

### Русский

Уменьшены размер шрифта меток техники над точками появления и интервал между метками. Метки больше не занимают лишнее место на экране при близкой камере.

## 1.0.2 | 2026-09-07

### English

- Added a translucent 3D preview showing the maximum vehicle size supported by each ambient vehicle spawn point.
- The preview uses the assigned faction's color and updates automatically when the point is moved or rotated.
- Added color-coded labels showing which vehicle types are allowed or excluded:
  - Cars — green
  - Trucks — cyan
  - APCs — orange
  - Helicopters — purple
  - Excluded types — red
- Added clearer warnings for incorrectly configured spawn points, including their names and coordinates.
- The editor now prevents placement of factionless spawn points when the current GameMode has no global vehicles available.
- Spawn points assigned to unavailable factions are now detected and reported.
- Valid faction-specific spawn points continue to work normally.
- Runtime vehicle spawning behavior has not been changed.

### Русский

- Добавлен полупрозрачный трёхмерный предпросмотр, показывающий максимальный размер техники, поддерживаемый точкой появления ambient-техники.
- Предпросмотр использует цвет назначенной фракции и обновляется автоматически при перемещении или повороте точки.
- Добавлены цветные метки, показывающие, какие типы техники разрешены или исключены:
  - легковые — зелёный
  - грузовики — голубой
  - БТР — оранжевый
  - вертолёты — фиолетовый
  - исключённые типы — красный
- Добавлены более понятные предупреждения о некорректно настроенных точках появления, включая их имена и координаты.
- Редактор теперь запрещает размещение точек без фракции, когда у текущего GameMode нет доступной глобальной техники.
- Точки появления, назначенные недоступным фракциям, теперь обнаруживаются, и о них сообщается.
- Корректные точки появления с указанной фракцией продолжают работать как прежде.
- Поведение появления техники во время игры не изменено.

## 1.0.1 | 2026-08-31

### English

- Added FactionManager validation for worlds using ambient vehicle spawn points.
- Added faction compatibility checks for incoming ambient vehicle spawn-point prefabs.
- Placement is now blocked when a spawn point requires a faction unavailable in the world's FactionManager.
- Added an explicit World Editor command for manually checking ambient vehicle spawn-point configuration.
- Improved error messages for missing or incompatible faction configuration.

### Русский

- Добавлена проверка FactionManager для миров, использующих точки появления ambient-техники.
- Добавлена проверка совместимости фракций для добавляемых префабов точек появления ambient-техники.
- Размещение теперь блокируется, если точке появления нужна фракция, недоступная в FactionManager мира.
- Добавлена отдельная команда World Editor для ручной проверки настройки точек появления ambient-техники.
- Улучшены сообщения об ошибках при отсутствующей или несовместимой настройке фракций.
