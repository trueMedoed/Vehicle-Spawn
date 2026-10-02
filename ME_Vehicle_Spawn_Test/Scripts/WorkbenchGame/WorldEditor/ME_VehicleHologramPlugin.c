//! Switches the selected point's mesh example and clears all previews on game-mode transitions.
//! Переключает пример выбранной точки и удаляет все preview при смене игрового режима.
[WorkbenchPluginAttribute(name: "Next vehicle hologram", description: "Next catalog candidate for the selected ambient point.", wbModules: { "WorldEditor" }, category: "ME_Vehicle_Spawn/Preview")]
class ME_VehicleHologramPlugin : WorldEditorPlugin
{
 //! Resolves the currently selected ambient point for switching its preview.
 //! Находит выделенную ambient-точку для переключения её предпросмотра.
 static SCR_AmbientVehicleSpawnPointComponent ME_GetSelectedPoint()
 {
  WorldEditor editor = Workbench.GetModule(WorldEditor);
  if (!editor) return null;
  WorldEditorAPI api = editor.GetApi();
  if (!api || !api.GetSelectedEntity()) return null;
  IEntity entity = api.SourceToEntity(api.GetSelectedEntity());
  if (!entity) return null;
  return SCR_AmbientVehicleSpawnPointComponent.Cast(entity.FindComponent(SCR_AmbientVehicleSpawnPointComponent));
 }

 //! Advances the active point's candidate.
 //! Переключает кандидата активной точки.
 override void Run()
 {
  SCR_AmbientVehicleSpawnPointComponent point = ME_GetSelectedPoint();
  if (point) point.ME_NextHologram();
 }
 //! Removes transient meshes before gameplay.
 //! Удаляет временные меши перед игрой.
 override void OnGameModeStarted(string worldName, string gameMode, bool playFromCameraPos, vector cameraPosition, vector cameraAngles)
 {
  SCR_AmbientVehicleSpawnPointComponent.ME_ClearAllHolograms();
 }
 //! Resets preview state after gameplay.
 //! Сбрасывает preview после игры.
 override void OnGameModeEnded()
 {
  SCR_AmbientVehicleSpawnPointComponent.ME_ClearAllHolograms();
 }
}

//! Enables or disables vehicle mesh previews.
//! Включает или отключает предпросмотр мешей техники.
[WorkbenchPluginAttribute(name: "Toggle vehicle hologram", description: "Show or hide vehicle examples at all loaded ambient points.", wbModules: { "WorldEditor" }, category: "ME_Vehicle_Spawn/Preview")]
class ME_VehicleHologramTogglePlugin : WorldEditorPlugin
{
 //! Toggles only transient visualization.
 //! Переключает только временную визуализацию.
 override void Run()
 {
  SCR_AmbientVehicleSpawnPointComponent.s_ME_HologramEnabled = !SCR_AmbientVehicleSpawnPointComponent.s_ME_HologramEnabled;
  SCR_AmbientVehicleSpawnPointComponent.ME_ClearAllHolograms();
 }
}
