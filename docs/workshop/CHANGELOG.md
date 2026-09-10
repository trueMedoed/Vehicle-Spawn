# Changelog

История версий `ME_Vehicle_Spawn`, опубликованных в Workshop.

Записи упорядочены от новых к старым. Заголовок записи — номер версии Workshop и дата публикации. Номер версии присваивает Workshop, поля версии в `addon.gproj` нет, поэтому с кодом версию связывает git-тег `vX.Y.Z` на коммите, который был опубликован. Английский блок каждой записи — текст, отправленный в Workshop; русский блок ниже него служит внутренней сверкой и не публикуется.

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
