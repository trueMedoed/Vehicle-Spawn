# Перенос изменений из Test в Prod

`ME_Vehicle_Spawn_Test` используется для разработки и проверки изменений. Не переносите экспериментальный код в `ME_Vehicle_Spawn`, пока он не скомпилирован и не проверен в Workbench.

## Порядок переноса

1. Убедитесь, что изменение находится в `ME_Vehicle_Spawn_Test` и проходит проверку в Workbench.
2. Сравните соответствующие файлы Test и Prod. Переносите только код, необходимый для подтверждённого поведения.
3. Не переносите test-only диагностику без отдельной необходимости:
   - временные `Print` / `PrintFormat`;
   - расширенные отладочные счётчики и логи с префиксом `[ME_DEBUG_AVSP_WB]`;
   - закомментированные эксперименты;
   - тестовые миры, слои и Workbench-managed `resourceDatabase.rdb`.
4. Для modded overrides сохраняйте `super`-вызовы, если изменение явно не заменяет базовое поведение.
5. После переноса откройте production `addon.gproj` в Workbench, перезагрузите скрипты и проверьте свежий `error.log`.
6. Проверьте `git diff --check`, `git diff` и `git status` перед коммитом.

## Пример: проверка FactionManager

После успешной проверки в Test переносится в production-плагин только функциональная часть проверки:

- результат `NO_FACTION_MANAGER`;
- счётчик найденных `FactionManager`;
- проверка `FactionManager.Cast(entity)` в существующем обходе editor-entities;
- блокировка при отсутствии менеджера после проверок GameMode;
- сообщения для drag-and-drop и ручной команды;
- `Run()` с вызовом `CheckOpenWorld()` без preview-controller.

`CheckOpenWorld()` требуется для команды **Check ambient vehicle spawning**: она проверяет уже размещённые точки. Перетаскивание новой точки использует `OnWorldEditWindowDataDropped()` и `CanCreateAmbientSpawnPoint()` независимо от этого метода.
