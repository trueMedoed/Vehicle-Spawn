# Test diagnostics

## SpawnVehicles flag

В vanilla ambient system первый `OnUpdatePoint` может иметь `enabled=1`, но при отсутствии `EGameFlags.SpawnVehicles` базовый код выключает update point после `super`. Значения флагов:

- `EGameFlags.SpawnVehicles = 2`;
- `EGameFlags.SpawnAI = 4`;
- `m_eTestGameFlags = 6` означает оба флага.

`SCR_BaseGameMode.EOnInit()` устанавливает `m_eTestGameFlags`. Plain mode с базовым значением `0` не включает SpawnVehicles; campaign baseline включает его.

## Log prefixes

Фильтруйте `script.log` по следующим префиксам:

- `[ME_DEBUG_AVSP]` — spawn-point component;
- `[ME_DEBUG_AVSP_POS]` — terrain position probing;
- `[ME_DEBUG_AVSP_SYS]` — ambient vehicle system;
- `[ME_DEBUG_AVSP_GM]` — game mode and player lifecycle.

Используйте `error.log` для compile/runtime errors, а `console.log` — для полного engine context.

## Сравнение

Сравнивайте Test только с чистым `worlds/MP/CTI_Campaign_Eden.ent` в отдельной Workbench-сессии. Не используйте terrain-only `worlds/Eden/Eden.ent` и не открывайте campaign поверх Test: это смешивает game mode, entities и spawn points.

Ожидаемый campaign baseline: `SCR_GameModeCampaign`, 171 spawn points, `spawnVehicles=1` и `enabled=1` до и после `super`, затем вызовы `ProcessSpawnpoint`.

Не добавляйте override `Enable(bool)`: native declaration конфликтует с таким modded override.

Shape marker — только edit-world clearance preflight и не гарантия того, что runtime действительно создаст vehicle.
