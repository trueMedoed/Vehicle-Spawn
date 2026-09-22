# Claude Code Instructions для Vehicle Spawn проекта

## Работа с Workbench

**КРИТИЧЕСКИ ВАЖНО**: Перед любой работой с Workbench (`wb_launch`, `wb_connect`, любые `wb_*` инструменты) ОБЯЗАТЕЛЬНО прочитай `.claude/workbench-debugging.md` — там полная процедура запуска и диагностики.

## Структура проекта

- `ME_Vehicle_Spawn_Test/` — основной мод
- `ME_Vehicle_Spawn_Test/Scripts/Game/` — игровые скрипты
- `ME_Vehicle_Spawn_Test/Scripts/WorkbenchGame/` — плагины для Workbench
- `.claude/` — инструкции и документация для работы с Claude Code

## Важные правила

1. **Всегда проверяй процессы Workbench перед запуском** — не должно быть запущенных ArmaReforgerWorkbenchSteamDiag.exe
2. **Используй таймаут 15 секунд** при запуске `wb_launch`
3. **Если что-то не работает** — первым делом проверь логи компиляции, обычно там ошибка в коде

## Диагностика проблем

- Workbench зависает → см. `.claude/workbench-debugging.md`
- Плагин не появляется в меню → проверь логи компиляции
- Bridge не отвечает → ошибка компиляции блокирует EnfusionMCP handler addon

## Коммиты и документация

- Коммиты делаю только по явной просьбе пользователя
- Changelog и версии связаны с git-тегами
- Русское описание мода в отдельном файле
