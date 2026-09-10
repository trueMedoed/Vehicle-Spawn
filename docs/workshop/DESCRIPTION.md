# Workshop page text

Source text for the two Workshop fields of `ME_Vehicle_Spawn`. Both blocks below are stored exactly as published, including the `•` and `-` bullets, because the Workshop page has no Markdown formatting. Copy them verbatim; do not convert them to Markdown lists.

The Russian reference translation lives in [RU_DESCRIPTION.md](RU_DESCRIPTION.md) and is not published.

## Summary

Workbench diagnostics for ambient vehicle spawn points: configuration warnings, visual spawn-area checks, overlap detection, and static-object conflict markers.

## Description

Ambient Vehicle Spawn Diag adds Workbench diagnostics for vanilla ambient vehicle spawn points in Arma Reforger.

It is a mission-making and configuration-validation tool. The addon does not replace the vanilla ambient vehicle spawning system and does not add a separate vehicle spawning system.

Features
• Prevents placement of an ambient vehicle spawn point when the current world configuration cannot support it.
• Checks that exactly one editable GameMode is available.
• Warns when the GameMode layer is locked.
• Checks that Spawn Vehicles is enabled in the GameMode Test Game Flags.
• Shows the spawn area in the World Editor:
  - green: an empty terrain position was found;
  - red: no valid terrain position was found, or another ambient spawn area overlaps it.
• Marks static physics objects whose bounds intersect a spawn area.
• Shows a translucent box for the selected spawn point with the space the vehicle will occupy, sized by the largest catalog vehicle that matches the point's label filter, tinted with the faction colour, and updated when the point is moved or rotated.
• Reports an error when configured included/excluded entity labels leave no eligible vehicle in the catalog.
• Draws the configured labels of a point above it: included labels each in their own colour, excluded labels in red, and ALL when a list is empty.
• Includes worlds/ME_TestWorld.ent with valid and intentionally invalid examples for testing the diagnostics.
• Checks that a FactionManager is present in the world.
• Validates the faction affiliation of incoming ambient vehicle spawn-point prefabs.
• Prevents placement when a spawn point requires a faction unavailable in the world's FactionManager.

Important notes
• This addon is designed for vanilla Arma Reforger ambient vehicle spawn points.
• It has been tested only with vanilla game content.
• Compatibility with other mods is not guaranteed, especially mods that override SCR_AmbientVehicleSpawnPointComponent, SCR_AmbientVehicleSystem, or World Editor placement callbacks.
• Some layers in ME_TestWorld intentionally contain conflicts and invalid configurations. They are demonstration cases, not ready-to-use mission content.

How to use
1. Open your world in Workbench.
2. Configure exactly one editable GameMode.
3. Enable Spawn Vehicles in Test Game Flags.
4. Add a FactionManager to the world and configure the required factions.
5. Place a vanilla Ambient Vehicle Spawnpoint prefab.
6. Use the visual indicators and warning dialogs to correct placement or configuration issues.
7. For a manual check of the current world, open Workbench Plugins and run “Check ambient vehicle spawning”.
