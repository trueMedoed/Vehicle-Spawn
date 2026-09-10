# Workshop description

Source text for the `ME_Vehicle_Spawn` Workshop page. Keep the English section in sync with the published page; the Russian section is an internal reference and is not published.

## English

### ME_Vehicle_Spawn

An editor-only toolset that reports misconfigured ambient vehicle spawn points in the Arma Reforger World Editor. The vanilla component fails silently when a spawn point can never select a vehicle, which leaves an empty point and no trace in the log. This addon makes those cases visible while you edit, before you enter Game mode.

Nothing here changes runtime spawning. Every marker and message is an editor advisory, not a guarantee that a vehicle will spawn.

### Features

**Placement checks**

Dragging an ambient vehicle spawn point into a world is blocked with an explanation when its prerequisites are missing: no GameMode or several of them, a locked GameMode layer, a disabled Spawn Vehicles test flag, a missing FactionManager, or a faction the world cannot provide. For factionless points the global VEHICLE catalog is checked for readability and content.

**Check ambient vehicle spawning**

An explicit World Editor command that audits every point already placed in the open world. It reports points requiring faction keys absent from the FactionManager and factionless points whose global vehicle catalog is unavailable or empty.

**Clearance visualization**

Each point shows a sphere covering the area the vanilla code searches for free ground. Colour reflects the result of that search. The sphere follows the point as you move it.

**Conflict warnings**

Overlapping spawn areas are flagged, as are collisions between an area and ordinary static objects, based on their bounds. One marker is created per object even when several areas touch it.

**Vehicle envelope preview**

For a selected point, a box shows the space the vehicle will actually occupy. The size is aggregated from all catalog vehicles matching the point's label filter and is deliberately conservative, following the largest match. The box is tinted with the faction colour of the point, or neutral yellow when no faction is assigned.

**Filter labels**

The configured labels of a point are drawn above it: included labels each in their own colour, excluded labels in red, and `ALL` when a list is empty. The text always faces the camera.

### Requirements and limitations

- Requires the base game only.
- The demo world `worlds/ME_TestWorld.ent` ships with the addon and contains prepared valid and invalid cases.
- Do not enable this addon together with `ME_Vehicle_Spawn_Test`; both provide `modded class` overrides for the same vanilla classes and will conflict.

## Русский

Внутренний перевод для сверки. В Workshop публикуется только английский раздел.

### ME_Vehicle_Spawn

Набор инструментов редактора, сообщающий о некорректно настроенных точках появления ambient-техники в World Editor. Ванильный компонент завершается молча, когда точка не может выбрать технику: остаётся пустая точка и никаких следов в логе. Аддон делает такие случаи заметными во время редактирования, до входа в Game mode.

Runtime-появление не изменяется. Все маркеры и сообщения — рекомендации редактора, а не гарантия появления техники.

### Возможности

**Проверки при размещении**

Перетаскивание точки в мир блокируется с пояснением, если не выполнены предпосылки: нет GameMode или их несколько, слой GameMode заблокирован, выключен тестовый флаг Spawn Vehicles, отсутствует FactionManager либо требуется недоступная фракция. Для точек без фракции проверяется доступность и непустота глобального каталога VEHICLE.

**Команда Check ambient vehicle spawning**

Явная команда редактора, проверяющая все уже размещённые точки открытого мира. Сообщает о точках, требующих отсутствующие в FactionManager ключи фракций, и о точках без фракции, чей глобальный каталог недоступен или пуст.

**Визуализация свободного места**

У каждой точки отображается сфера области, в которой ванильный код ищет свободную землю. Цвет отражает результат поиска. Сфера следует за точкой при перемещении.

**Предупреждения о конфликтах**

Отмечаются пересечения областей появления между собой, а также столкновения области с обычными статическими объектами по их габаритам. На объект создаётся один маркер, даже если его задевают несколько областей.

**Предпросмотр габаритов техники**

Для выбранной точки рамка показывает место, которое реально займёт техника. Размер агрегируется по всей технике каталога, подходящей под фильтр метки точки, и рассчитывается консервативно — по самому крупному варианту. Рамка окрашивается в цвет фракции точки либо в нейтральный жёлтый, если фракция не задана.

**Метки фильтра**

Настроенные метки точки отображаются над ней: включающие — каждая своим цветом, исключающие — красным, `ALL` — когда список пуст. Текст всегда развёрнут к камере.

### Требования и ограничения

- Требуется только базовая игра.
- Демонстрационный мир `worlds/ME_TestWorld.ent` входит в аддон и содержит подготовленные корректные и некорректные случаи.
- Не подключайте аддон вместе с `ME_Vehicle_Spawn_Test`: оба содержат `modded class` для одних и тех же ванильных классов и конфликтуют.
