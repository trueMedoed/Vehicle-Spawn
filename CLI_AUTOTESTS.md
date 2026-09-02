# CLI-автотесты ambient vehicle spawn point

Этот документ описывает повторяемый запуск тестового аддона `ME_Vehicle_Spawn_Test` через оконный Diag executable Arma Reforger.

## Что проверяется

Тесты находятся в:

```text
ME_Vehicle_Spawn_Test/Scripts/Game/Autotest/ME_Vehicle_Spawn/ME_AmbientVehicleSpawnPointAutotest.c
```

Каждый suite загружает отдельный fixture-мир из `ME_Vehicle_Spawn_Test/worlds/CliAutotest/`. Эти ресурсы изолированы от ручного мира `ME_TestWorld` и legacy-ресурсов `worlds/TestCases`.

| Suite | Fixture | Ожидаемое состояние |
| --- | --- | --- |
| `ME_TEST_AmbientVehicleSpawnPointConfiguredSuite` | `ME_CLI_AmbientVehicle_Configured` | GameMode=1, FactionManager=1, registered spawn points=1, SpawnVehicles=true |
| `ME_TEST_AmbientVehicleSpawnPointSpawnVehiclesDisabledSuite` | `ME_CLI_AmbientVehicle_NoSpawnVehicles` | 1, 1, 0, false |
| `ME_TEST_AmbientVehicleSpawnPointMissingGameModeSuite` | `ME_CLI_AmbientVehicle_NoGameMode` | 0, 0, 0, false |
| `ME_TEST_AmbientVehicleSpawnPointMissingFactionManagerSuite` | `ME_CLI_AmbientVehicle_NoFactionManager` | 1, 0, 1, true |
| `ME_TEST_AmbientVehicleSpawnPointIncompatibleFactionManagerSuite` | `ME_CLI_AmbientVehicle_IncompatibleFactionManager` | 1, 1, 1, true |

`registered spawn points` — это результат `SCR_AmbientVehicleSystem.GetSpawnpoints()`, а не общий обход сущностей мира.

Несовместимая faction точки не отменяет её регистрацию: `SCR_AmbientVehicleSpawnPointComponent` регистрируется при наличии `SCR_FactionAffiliationComponent`; совместимость faction влияет на последующий подбор техники. Поэтому последний fixture ожидает одну зарегистрированную FIA-точку, а не ноль.

## Перед запуском

1. Не редактируйте `resourceDatabase.rdb` вручную. При изменении `.ent` или `.layer` предпочтительно сохранить ресурс через Workbench, чтобы metadata и resource database были актуальны.
2. Проверьте test addon:

   ```text
   mod_validate(projectPath: "C:\Users\Phil\Documents\GitHub\Mods\Vehicle Spawn\ME_Vehicle_Spawn_Test")
   git diff --check
   ```

3. Если перед этим использовался EnfusionMCP Workbench bridge, удалите временные handler scripts перед публикацией:

   ```text
   wb_cleanup(modDir: "C:\Users\Phil\Documents\GitHub\Mods\Vehicle Spawn\ME_Vehicle_Spawn_Test")
   ```

## Запуск одного suite

Запускайте каждый suite отдельным **оконным** процессом. Не добавляйте `-headless` или параметры скрытия окна.

В PowerShell:

```powershell
$exe = "D:\SteamLibrary\steamapps\common\Arma Reforger\ArmaReforgerSteamDiag.exe"
$root = "C:\Users\Phil\Documents\GitHub\Mods\Vehicle Spawn"
$suite = "ME_TEST_AmbientVehicleSpawnPointConfiguredSuite"
$args = "-addonsDir `"$root`" -addons ME_Vehicle_Spawn_Test -autotest $suite"

$process = Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe) `
  -ArgumentList $args -Wait -PassThru
$process.Refresh()
$process.ExitCode
```

`-addonsDir` и `-addons ME_Vehicle_Spawn_Test` обязательны: один `-gproj` не обеспечивает надёжную доступность локального test addon после смены сценария.

Код процесса `0` не означает, что assertions прошли. Авторитетный результат находится в `autotest.log`.

## Запуск всех suite-ов

```powershell
$exe = "D:\SteamLibrary\steamapps\common\Arma Reforger\ArmaReforgerSteamDiag.exe"
$root = "C:\Users\Phil\Documents\GitHub\Mods\Vehicle Spawn"
$suites = @(
  "ME_TEST_AmbientVehicleSpawnPointConfiguredSuite",
  "ME_TEST_AmbientVehicleSpawnPointSpawnVehiclesDisabledSuite",
  "ME_TEST_AmbientVehicleSpawnPointMissingGameModeSuite",
  "ME_TEST_AmbientVehicleSpawnPointMissingFactionManagerSuite",
  "ME_TEST_AmbientVehicleSpawnPointIncompatibleFactionManagerSuite"
)

foreach ($suite in $suites) {
  $args = "-addonsDir `"$root`" -addons ME_Vehicle_Spawn_Test -autotest $suite"
  $process = Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe) `
    -ArgumentList $args -Wait -PassThru
  $process.Refresh()
  "${suite}: $($process.ExitCode)"
}
```

Цикл всё равно создаёт отдельный процесс для каждого suite.

## Где смотреть результаты

Каждый запуск создаёт новый каталог:

```text
C:\Users\Phil\Documents\My Games\ArmaReforger\logs\logs_YYYY-MM-DD_HH-MM-SS\
```

Проверяйте:

- `autotest.log` — итог suite: строка `✅ ... SUCCESS` обязательна;
- `script.log` — компиляция test addon, сообщения autotest runner и runtime exceptions;
- `error.log` — ошибки загрузки ресурсов, компиляции и runtime exceptions.

Не должно быть:

```text
Invalid -autotest parameter value
Wrong GUID/name for resource
Missing Component Class
```

`ME_CLI_AmbientVehicle_NoFactionManager` намеренно загружается без `SCR_FactionManager`. В текущем vanilla runtime это вызывает диагностические сообщения и VM exceptions в зависящих от manager системах (`SCR_EntityCatalogManagerComponent`, deploy menu, character identity manager). Они подтверждают отсутствие требуемой зависимости, но не отменяют ожидаемый successful autotest assertion этого fixture.

Ошибки вида `SCR_FilterCategory` constructor, устаревшие API warnings и resource/UI warnings могут происходить до загрузки fixture и относятся к окружению Diag/base game; отделяйте их от ошибок test addon по времени и stack trace.

## Изменение fixture-ресурсов

- У каждого `.ent.meta` должен быть собственный GUID, а `Name` должен точно совпадать с путём fixture-мира.
- В `GetWorldFile()` используйте GUID конкретного `.ent`, а не GUID аддона из `addon.gproj`.
- Не меняйте ручной диагностический мир `ME_TestWorld` и не используйте его слои для CLI fixtures.
- `ME_TEST_AmbientVehicleSpawnFlagsComponent` устанавливает runtime-флаг `EGameFlags.SpawnVehicles` в fixture-мирах, которым он нужен; сериализованного `m_eTestGameFlags` для CLI было недостаточно.
- GameMode fixture-миры содержат `SCR_LoadoutManager` и `SCR_MapEntity`, чтобы базовый menu-spawn runtime не завершался `SCR_RoleSelectionMenu::InitMapPlain` exception.
