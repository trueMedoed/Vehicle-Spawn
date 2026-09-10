# Контекст

Пользователь уточнил визуальную семантику Test-only editor-preview vehicle envelope: цвет faction должен применяться к полупрозрачной заливке, а рамка должна быть нейтральной и не конкурировать с цветами фракций. Дополнительно рамка сейчас не отображается. Предварительная проверка показала корректные bounds, corner geometry и Shape lifecycle; наиболее вероятная причина — depth occlusion линий, расположенных на тех же поверхностях, что и заливка.

## Реализация

1. Изменить только `ME_Vehicle_Spawn_Test/Scripts/Game/Components/Locations/ME_DebugAmbientVehicleSpawnPointComponent.c`.
   - Сохранить существующее разрешение `SCR_FactionAffiliationComponent`, затем `GetDefaultFactionKey()`, fallback `GetAffiliatedFactionKey()`, и `FactionManager.GetFactionByKey()` с текущим красным fallback при отсутствии faction.
   - Переназначить сохранённый resolved faction color: использовать его для `fillColor` в `Shape.CreateTris()`, сохранив полупрозрачный alpha и заливку как отдельный смысл envelope.
   - Для `Shape.CreateLines()` использовать отдельный нейтральный opaque цвет (светло-серый/белый), не зависящий от faction.
   - Для outline добавить подтверждённый `ShapeFlags.NOZBUFFER` к существующему `ShapeFlags.DOUBLESIDE`, чтобы линия не скрывалась собственной прозрачной заливкой, terrain или другой геометрией. Не добавлять неподтверждённые параметры толщины/offset и не менять точки.
   - Не менять bounds/filtering API, corner ordering, yaw transform, shape lifecycle, регистрацию точек, labels и runtime diagnostics.
   - Сохранить bilingual English/Russian документацию для изменённой логики согласно правилам проекта.

2. Проверить все существующие пути preview:
   - Workbench `_WB_OnInit()`;
   - ручной preview plugin;
   - `_WB_SetTransform()` после перемещения;
   - очистка при удалении.
   Цвета должны пересоздаваться без утечки цвета предыдущей faction, а невалидный snapshot/selection по-прежнему не должен создавать envelope.

## Проверка

1. Прочитать diff и выполнить `git diff --check`; убедиться, что изменён только Test-компонент и не затронуты production или `resourceDatabase.rdb`.
2. Запустить `mod_validate` для `ME_Vehicle_Spawn_Test`.
3. В Workbench открыть `ME_Vehicle_Spawn_Test/worlds/MP/MpTest/ME_MpTest.ent`, reload scripts и проверить свежие `error.log`/`script.log`.
4. Включить preview для точек US, USSR, CIV и FIA и убедиться, что fill получает цвет faction с прежней прозрачностью, а outline остаётся нейтральным, opaque и видимым; отдельно проверить точку без resolved faction с красным fill fallback.
5. Проверить перемещение, удаление и ручной plugin preview; убедиться, что геометрия, yaw и границы не изменились.
6. Если Workbench bridge недоступен, не заявлять runtime/visual success: завершить static checks и удалить временные `Scripts/WorkbenchGame/EnfusionMCP/` handlers через `wb_cleanup`, если они были установлены.

## Ограничения

- Production addon не изменять: это Test-only визуальная диагностика, пока пользователь отдельно не запросит перенос.
- Не добавлять новые конфиги, resourceDatabase entries или runtime behavior.
- Не менять существующие bounds/filtering API, кроме переназначения уже разрешённого faction color на fill и visibility flags outline.
