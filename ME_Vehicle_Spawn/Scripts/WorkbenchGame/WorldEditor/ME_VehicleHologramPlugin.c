//! Switches the selected point's mesh example and clears all previews on game-mode transitions.
[WorkbenchPluginAttribute(name: "Next vehicle hologram", description: "Next catalog candidate for the selected ambient point.", wbModules: { "WorldEditor" }, category: "[ME] Vehicle Spawn/Preview")]
class ME_VehicleHologramPlugin : WorldEditorPlugin
{
 //! Resolves the currently selected ambient point for switching its preview.
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
 override void Run()
 {
  SCR_AmbientVehicleSpawnPointComponent point = ME_GetSelectedPoint();
  if (point) point.ME_NextHologram();
 }
 //! Removes transient meshes before gameplay.
 override void OnGameModeStarted(string worldName, string gameMode, bool playFromCameraPos, vector cameraPosition, vector cameraAngles)
 {
  SCR_AmbientVehicleSpawnPointComponent.ME_ClearAllHolograms();
 }
 //! Resets preview state after gameplay.
 override void OnGameModeEnded()
 {
  SCR_AmbientVehicleSpawnPointComponent.ME_ClearAllHolograms();
 }
}

//! Enables or disables vehicle mesh previews.
[WorkbenchPluginAttribute(name: "Toggle vehicle hologram", description: "Show or hide vehicle examples at all loaded ambient points.", wbModules: { "WorldEditor" }, category: "[ME] Vehicle Spawn/Preview")]
class ME_VehicleHologramTogglePlugin : WorldEditorPlugin
{
 //! Toggles only transient visualization.
 override void Run()
 {
  SCR_AmbientVehicleSpawnPointComponent.s_ME_HologramEnabled = !SCR_AmbientVehicleSpawnPointComponent.s_ME_HologramEnabled;
  SCR_AmbientVehicleSpawnPointComponent.ME_ClearAllHolograms();
 }
}
