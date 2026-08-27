StringTable {
 ItemClassName "CustomStringTableItem"
 Items {
  CustomStringTableItem "{A101000000000001}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_Title"
   Target_en_us "Ambient vehicle spawn points"
   Target_ru_ru "Точки появления техники"
  }
  CustomStringTableItem "{A101000000000002}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_DropNoGameMode"
   Target_en_us "The point was not created because this world has no SCR_BaseGameMode. Create or configure exactly one editable GameMode, then enable Spawn Vehicles."
   Target_ru_ru "Точка не создана: в этом мире нет SCR_BaseGameMode. Создайте или настройте ровно один редактируемый GameMode, затем включите Spawn Vehicles."
  }
  CustomStringTableItem "{A101000000000003}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_DropMultipleGameModes"
   Target_en_us "The point was not created because multiple SCR_BaseGameMode entities were found. Configure exactly one editable GameMode, then enable Spawn Vehicles."
   Target_ru_ru "Точка не создана: найдено несколько SCR_BaseGameMode. Настройте ровно один редактируемый GameMode, затем включите Spawn Vehicles."
  }
  CustomStringTableItem "{A101000000000004}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_DropGameModeLayerLocked"
   Target_en_us "The point was not created because the only GameMode is on a locked layer or inside a locked parent layer and cannot be configured. Create another world with an editable GameMode."
   Target_ru_ru "Точка не создана: единственный GameMode находится на заблокированном слое или внутри заблокированного родительского слоя и не может быть настроен. Создайте другой мир с редактируемым GameMode."
  }
  CustomStringTableItem "{A101000000000005}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_DropTestGameFlagsUnavailable"
   Target_en_us "The point was not created because m_eTestGameFlags is unavailable on the only SCR_BaseGameMode. Use an editable GameMode that exposes Test Game Flags."
   Target_ru_ru "Точка не создана: m_eTestGameFlags недоступен у единственного SCR_BaseGameMode. Используйте редактируемый GameMode с доступными Test Game Flags."
  }
  CustomStringTableItem "{A101000000000006}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_DropSpawnVehiclesDisabled"
   Target_en_us "The point was not created because Spawn Vehicles is disabled in the only GameMode's Test Game Flags / m_eTestGameFlags."
   Target_ru_ru "Точка не создана: Spawn Vehicles выключен в Test Game Flags / m_eTestGameFlags единственного GameMode."
  }
  CustomStringTableItem "{A101000000000007}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_CheckWorldEditorUnavailable"
   Target_en_us "The World Editor API is unavailable, so ambient vehicle spawning cannot be checked."
   Target_ru_ru "World Editor API недоступен, поэтому проверить появление техники нельзя."
  }
  CustomStringTableItem "{A101000000000008}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_CheckNoGameMode"
   Target_en_us "Ambient vehicle spawning needs a GameMode to apply Test Game Flags. Add or configure a GameMode derived from SCR_BaseGameMode."
   Target_ru_ru "Для появления техники нужен GameMode, чтобы применить Test Game Flags. Добавьте или настройте GameMode, производный от SCR_BaseGameMode."
  }
  CustomStringTableItem "{A101000000000009}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_CheckMultipleGameModes"
   Target_en_us "Multiple SCR_BaseGameMode entities were found. Ambient vehicle spawning configuration is ambiguous; configure the intended GameMode before using ambient vehicle spawn points."
   Target_ru_ru "Найдено несколько SCR_BaseGameMode. Настройка появления техники неоднозначна; перед использованием точек появления настройте нужный GameMode."
  }
  CustomStringTableItem "{A10100000000000A}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_CheckGameModeLayerLocked"
   Target_en_us "The only GameMode is on a locked layer or inside a locked parent layer and cannot be configured. Create another world with an editable GameMode."
   Target_ru_ru "Единственный GameMode находится на заблокированном слое или внутри заблокированного родительского слоя и не может быть настроен. Создайте другой мир с редактируемым GameMode."
  }
  CustomStringTableItem "{A10100000000000B}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_CheckTestGameFlagsUnavailable"
   Target_en_us "Test Game Flags / m_eTestGameFlags is unavailable on the only SCR_BaseGameMode. Use an editable GameMode that exposes Test Game Flags."
   Target_ru_ru "Test Game Flags / m_eTestGameFlags недоступен у единственного SCR_BaseGameMode. Используйте редактируемый GameMode с доступными Test Game Flags."
  }
  CustomStringTableItem "{A10100000000000C}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_CheckSpawnVehiclesDisabled"
   Target_en_us "Ambient vehicle spawning is disabled for the current GameMode. Select the GameMode and enable Spawn Vehicles in Test Game Flags / m_eTestGameFlags. EGameFlags.SpawnVehicles = 2; 6 also enables SpawnAI."
   Target_ru_ru "Появление техники выключено для текущего GameMode. Выберите GameMode и включите Spawn Vehicles в Test Game Flags / m_eTestGameFlags. EGameFlags.SpawnVehicles = 2; значение 6 также включает SpawnAI."
  }
  CustomStringTableItem "{A10100000000000D}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_CheckLabelCatalogUnavailable"
   Target_en_us "The label filter could not be checked reliably because the vehicle catalog is unavailable or its context is ambiguous. This is a warning only and does not block the point. After editing labels in Inspector, run 'Check ambient vehicle spawning' again."
   Target_ru_ru "Label-фильтр нельзя надёжно проверить: каталог техники недоступен или его контекст неоднозначен. Это только предупреждение и не блокирует точку. После изменения labels в Inspector снова запустите «Check ambient vehicle spawning»."
  }
  CustomStringTableItem "{A10100000000000E}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_CheckLabelSourceArraysUnavailable"
   Target_en_us "The label filter could not be checked reliably because the serialized included/excluded arrays are unavailable on the component. This is a warning only and does not block the point."
   Target_ru_ru "Label-фильтр нельзя надёжно проверить: сериализованные included/excluded arrays недоступны у компонента. Это только предупреждение и не блокирует точку."
  }
  CustomStringTableItem "{A10100000000000F}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_CheckLabelFilterEmptyExcludedAll"
   Target_en_us "The label filter is confirmed empty: excluded labels remove every candidate found by the included labels. This is a warning only and does not block the point."
   Target_ru_ru "Подтверждён пустой результат label-фильтра: исключающие labels удаляют всех кандидатов, найденных по included labels. Это только предупреждение и не блокирует точку."
  }
  CustomStringTableItem "{A101000000000010}" {
   Id "MEVehicleSpawn_WB_AmbientSpawnWarning_CheckLabelFilterEmpty"
   Target_en_us "The label filter returned no candidates. The specific cause is not confirmed because the source catalog is also empty or the labels do not match available entities. This is a warning only."
   Target_ru_ru "Label-фильтр не дал кандидатов. Конкретная причина не подтверждена: исходный каталог по included labels также пуст или labels не соответствуют доступным сущностям. Это только предупреждение."
  }
 }
}
