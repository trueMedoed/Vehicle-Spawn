# Diagnostic addon

`ME_Vehicle_Spawn_Test` — сохранённый диагностический проект. В нём остаются runtime `modded class` для ambient vehicle system и base game mode, диагностические логи, текущая точка и экспериментальные миры.

Миры:

- `worlds/MP/MpTest/ME_MpTest.ent` — минимальный тест с диагностической instrumentation.
- `worlds/MP/MpTest/ME_MpTest_BasicSpawnVehicles.ent` — исходный демонстрационный вариант.

Открывайте этот addon отдельно от production: runtime overrides в обоих addon не должны компилироваться вместе.

Изменения в scripts, worlds и Workbench data сохраняются в Test-проекте; `resourceDatabase.rdb` остаётся его Workbench-managed metadata.
