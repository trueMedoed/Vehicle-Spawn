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
6. Если Workbench запускался через EnfusionMCP, очистите production-аддон по процедуре ниже.
7. Сверьте документацию с фактическим состоянием production, если перенос менял состав файлов, путь демонстрационного мира или пользовательское поведение.
8. Проверьте `git diff --check`, `git diff` и `git status` перед коммитом.

## Сверка документации

Документация не проверяется автоматически, поэтому расхождения накапливаются молча: к 10 сентября три файла ссылались на демонстрационный мир, которого в production нет, а описания обещали локализацию, удалённую ещё в августе.

Проверяйте эти файлы, когда перенос затрагивает состав аддона или видимое поведение:

- `README.md` — путь демонстрационного мира и краткий workflow;
- `PROD.md` — состав аддона и порядок проверки;
- `docs/PRODUCTION_WORKFLOW.md` — шаги проверки в Workbench;
- `docs/PROJECT_SPLIT.md` — перечень файлов production;
- `docs/workshop/DESCRIPTION.md` и `docs/workshop/RU_DESCRIPTION.md` — описание для страницы мода и его русский перевод.

Быстрая проверка ссылок на ресурсы, упомянутые в документации:

```bash
git ls-tree -r --name-only HEAD -- ME_Vehicle_Spawn/
```

Сравните вывод с путями в файлах выше. При публикации новой версии добавьте запись в `docs/workshop/CHANGELOG.md`.

## Очистка production-аддона после запуска через EnfusionMCP

`wb_launch` копирует handler-скрипты в `<аддон>/Scripts/WorkbenchGame/EnfusionMCP/`, чтобы они компилировались вместе с модом. Workbench регистрирует их в `<аддон>/resourceDatabase.rdb`. `wb_cleanup` удаляет только сами `.c`-файлы, поэтому в `.rdb` остаются висячие записи `EMCP_WB_*`. Ни файлы, ни эти записи не должны попадать в коммит или в Workshop.

1. Вызовите `wb_cleanup` с путём к production-аддону.
2. Остановите процесс `ArmaReforgerWorkbenchSteamDiag.exe`, иначе он перезапишет `resourceDatabase.rdb` при выходе.
3. Восстановите базу: `git checkout -- ME_Vehicle_Spawn/resourceDatabase.rdb`.
4. Убедитесь, что записей не осталось: `grep -c EnfusionMCP ME_Vehicle_Spawn/resourceDatabase.rdb` должен вернуть `0`, а в `Scripts/WorkbenchGame/` должен остаться только `WorldEditor/`.

Восстановление из HEAD корректно, когда во время сессии не менялись ресурсы аддона. Если ресурсы менялись, сравните таблицы печатных строк вместо восстановления файла и убедитесь, что различия ограничены записями EnfusionMCP:

```bash
tr -c '[:print:]' '\n' < resourceDatabase.rdb | grep -E '.{6,}' | sort -u
```

## Пример: проверка FactionManager

После успешной проверки в Test переносится в production-плагин только функциональная часть проверки:

- результат `NO_FACTION_MANAGER`;
- счётчик найденных `FactionManager`;
- проверка `FactionManager.Cast(entity)` в существующем обходе editor-entities;
- блокировка при отсутствии менеджера после проверок GameMode;
- сообщения для drag-and-drop и ручной команды;
- `Run()` с вызовом `CheckOpenWorld()` без preview-controller.

`CheckOpenWorld()` требуется для команды **Check ambient vehicle spawning**: она проверяет уже размещённые точки. Перетаскивание новой точки использует `OnWorldEditWindowDataDropped()` и `CanCreateAmbientSpawnPoint()` независимо от этого метода.
