# Vehicle catalog references

Open these resources in Workbench Resource Browser under Configs/Generated:

- ME_EditableEntityLabelsSnapshot.conf: faction → label → vehicle prefabs. Schema 2, captured from Arma Reforger 1.8.0.13 catalogs (CIV, FIA, US, USSR).
- ME_VehicleBoundsSnapshot.conf: faction → vehicle type → conservative bounds and source prefabs. Schema 5, used by the editor preview.

To interpret a spawn point, use its faction catalog. With Require All Included Labels enabled, intersect the prefab lists for the included labels; otherwise, take their union. Remove prefabs carrying excluded labels. For example, VEHICLE_APC with TRAIT_ARMED excluded means APC candidates without the armed label in that faction catalog. An empty include list must be checked against the live catalog rather than interpreted as an empty candidate set.

The labels snapshot is a reference, not a replacement for the game's catalog filtering. Mods, catalog overrides and game updates may change the candidates. Catalog membership does not guarantee runtime spawning: placement, available space and game conditions still apply. The bounds are conservative aggregates, not the size of a guaranteed vehicle.