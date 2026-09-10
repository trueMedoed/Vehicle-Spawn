# Workshop description

Source text for the `ME_Vehicle_Spawn` Workshop page. Keep this file in sync with the published page. The Russian reference translation lives in [RU_DESCRIPTION.md](RU_DESCRIPTION.md) and is not published.

## ME_Vehicle_Spawn

An editor-only toolset that reports misconfigured ambient vehicle spawn points in the Arma Reforger World Editor. The vanilla component fails silently when a spawn point can never select a vehicle, which leaves an empty point and no trace in the log. This addon makes those cases visible while you edit, before you enter Game mode.

Nothing here changes runtime spawning. Every marker and message is an editor advisory, not a guarantee that a vehicle will spawn.

## Features

**Placement checks**

Dragging an ambient vehicle spawn point into a world is blocked with an explanation when its prerequisites are missing: no GameMode or several of them, a locked GameMode layer, a disabled Spawn Vehicles test flag, a missing FactionManager, or a faction the world cannot provide. For factionless points the global VEHICLE catalog is checked for readability and content.

**Check ambient vehicle spawning**

An explicit World Editor command that audits every point already placed in the open world. It reports points requiring faction keys absent from the FactionManager and factionless points whose global vehicle catalog is unavailable or empty.

**Clearance visualization**

Each point shows a sphere covering the area the vanilla code searches for free ground. Colour reflects the result of that search. The sphere follows the point as you move it.

**Conflict warnings**

Overlapping spawn areas are flagged, as are collisions between an area and ordinary static objects, based on their bounds. One marker is created per object even when several areas touch it.

**Vehicle envelope preview**

For a selected point, a box shows the space the vehicle will actually occupy. The size is aggregated from all catalog vehicles matching the point's label filter and is deliberately conservative, following the largest match. The box is tinted with the faction colour of the point, or neutral yellow when no faction is assigned.

**Filter labels**

The configured labels of a point are drawn above it: included labels each in their own colour, excluded labels in red, and `ALL` when a list is empty. The text always faces the camera.

## Requirements and limitations

- Requires the base game only.
- The demo world `worlds/ME_TestWorld.ent` ships with the addon and contains prepared valid and invalid cases.
- Do not enable this addon together with `ME_Vehicle_Spawn_Test`; both provide `modded class` overrides for the same vanilla classes and will conflict.
