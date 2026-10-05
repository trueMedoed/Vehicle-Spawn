#ifdef WORKBENCH
//! Mesh-only vehicle previews for all loaded ambient points; never spawns vehicle prefabs.
modded class SCR_AmbientVehicleSpawnPointComponent
{
 // Use the game's translucent preview material; ParametricMaterialInstanceComponent changes only its color.
 protected static const ResourceName ME_HOLOGRAM_TINT_MATERIAL = "{58F07022C12D0CF5}Assets/Editor/PlacingPreview/Preview.emat";
 static bool s_ME_HologramEnabled = true;
 protected SCR_BasePreviewEntity m_ME_Hologram;
 protected ref Resource m_ME_HologramResource;
 protected string m_ME_HologramPrefab;
 protected string m_ME_HologramCaption;
 protected int m_ME_HologramIndex;
 protected ref array<string> m_ME_HologramPaths;
 protected string m_ME_HologramTypeSignature;
 protected string m_ME_HologramSnapshotFaction;
 protected int m_ME_HologramBestIndex;
 protected bool m_ME_HologramSnapshotSelected;
 protected string m_ME_HologramSelectionNote;
 protected bool m_ME_HologramFactionFallback;
 protected string m_ME_HologramFallbackType;
 protected string m_ME_HologramFallbackReason;
 protected bool m_ME_HologramPartial;
 protected float m_ME_HologramTimer;
 protected bool m_ME_HologramTimerInitialized;
 protected bool m_ME_HologramTransformValid;
 protected float m_ME_HologramTop;
 protected bool m_ME_HologramBoundsValid;
 protected vector m_ME_HologramBoundsAxisX;
 protected vector m_ME_HologramBoundsAxisZ;
 protected vector m_ME_HologramBoundsMins;
 protected vector m_ME_HologramBoundsMaxs;
 protected static const float ME_HOLOGRAM_OVERLAP_EPSILON = 0.25;
 protected ref DebugTextWorldSpace m_ME_HologramFactionTagText;
 protected string m_ME_HologramFactionTag;
 protected int m_ME_HologramFactionColor;
 protected vector m_ME_HologramFactionTagPosition;
 protected vector m_ME_HologramFactionTagOwnerOrigin;
 protected static const float ME_HOLOGRAM_FACTION_TAG_FONT_SIZE = 0.7;
 protected bool m_ME_HologramTintApplied;
 protected int m_ME_HologramAppliedColor;
 protected vector m_ME_HologramLastOrigin;
 protected vector m_ME_HologramLastAngles;
 protected string m_ME_HologramFailedPrefab;
 protected string m_ME_LastHologramStatus;

 //! Reports actionable preview issues once per state, with coordinates for unnamed points.
 protected void ME_LogHologramStatus(string status)
 {
  if (status == m_ME_LastHologramStatus) return;
  m_ME_LastHologramStatus = status;
  IEntity owner = GetOwner();
  if (owner) PrintFormat("[ME_VEHICLE_HOLOGRAM] entity=%1 coordinates=%2 %3", owner.GetName(), owner.GetOrigin(), status);
 }

 //! Releases the preview hierarchy, faction tag, and resource reference.
 void ME_ClearHologram()
 {
  if (m_ME_Hologram) delete m_ME_Hologram;
  m_ME_Hologram = null;
  m_ME_HologramFactionTagText = null;
  m_ME_HologramResource = null;
  m_ME_HologramPrefab = string.Empty;
  m_ME_HologramCaption = string.Empty;
  m_ME_HologramPartial = false;
  m_ME_HologramTransformValid = false;
  m_ME_HologramBoundsValid = false;
  m_ME_HologramTintApplied = false;
  m_ME_HologramFactionFallback = false;
  m_ME_HologramFallbackType = string.Empty;
  m_ME_HologramFallbackReason = string.Empty;
 }

 //! Applies one advisory color to every mesh preview in this point's hierarchy.
 protected void ME_TintHologramHierarchy(SCR_BasePreviewEntity preview, int color)
 {
  if (!preview) return;
  ParametricMaterialInstanceComponent material = ParametricMaterialInstanceComponent.Cast(preview.FindComponent(ParametricMaterialInstanceComponent));
  if (material) material.SetColor(color);
  array<SCR_BasePreviewEntity> children = preview.GetPreviewChildren();
  if (!children) return;
  foreach (SCR_BasePreviewEntity child : children)
   ME_TintHologramHierarchy(child, color);
 }

 //! White means no detected problem; overlapping preview bounds are red, while area-only and static-bounds warnings stay white.
 protected int ME_GetHologramTintColor()
 {
  if (m_ME_HologramFactionFallback || !m_bME_EditorHologramChecked || m_bME_EditorHologramPlacementError)
   return Color.RED;
  array<EEditableEntityLabel> conflictingLabels = ME_GetConflictingEditableEntityLabels();
  if (!conflictingLabels.IsEmpty()) return Color.RED;
  if (ME_HasOverlappingHologramBounds()) return Color.RED;
  return Color.FromRGBA(255, 255, 255, 255).PackToInt();
 }

 //! Tests one horizontal separating axis of two oriented preview bounds.
 protected bool ME_DoHologramBoundsOverlapOnAxis(SCR_AmbientVehicleSpawnPointComponent other, vector axis)
 {
  float ownX = vector.Dot(m_ME_HologramBoundsAxisX, axis);
  float ownZ = vector.Dot(m_ME_HologramBoundsAxisZ, axis);
  float ownCenter = 0.5 * ((m_ME_HologramBoundsMins[0] + m_ME_HologramBoundsMaxs[0]) * ownX + (m_ME_HologramBoundsMins[2] + m_ME_HologramBoundsMaxs[2]) * ownZ);
  float ownRadius = 0.5 * ((m_ME_HologramBoundsMaxs[0] - m_ME_HologramBoundsMins[0]) * Math.AbsFloat(ownX) + (m_ME_HologramBoundsMaxs[2] - m_ME_HologramBoundsMins[2]) * Math.AbsFloat(ownZ));
  float otherX = vector.Dot(other.m_ME_HologramBoundsAxisX, axis);
  float otherZ = vector.Dot(other.m_ME_HologramBoundsAxisZ, axis);
  float otherCenter = 0.5 * ((other.m_ME_HologramBoundsMins[0] + other.m_ME_HologramBoundsMaxs[0]) * otherX + (other.m_ME_HologramBoundsMins[2] + other.m_ME_HologramBoundsMaxs[2]) * otherZ);
  float otherRadius = 0.5 * ((other.m_ME_HologramBoundsMaxs[0] - other.m_ME_HologramBoundsMins[0]) * Math.AbsFloat(otherX) + (other.m_ME_HologramBoundsMaxs[2] - other.m_ME_HologramBoundsMins[2]) * Math.AbsFloat(otherZ));
  return Math.Min(ownCenter + ownRadius, otherCenter + otherRadius) - Math.Max(ownCenter - ownRadius, otherCenter - otherRadius) > ME_HOLOGRAM_OVERLAP_EPSILON;
 }

 //! Detects a visible preview-bounds conflict without treating spawn-area contact as a failed runtime spawn.
 protected bool ME_HasOverlappingHologramBounds()
 {
  if (!m_ME_HologramBoundsValid) return false;
  IEntity owner = GetOwner();
  if (!owner) return false;
  foreach (SCR_AmbientVehicleSpawnPointComponent other : s_ME_EditorSpawnPoints)
  {
   if (!other || other == this || !other.m_ME_HologramBoundsValid || !other.m_ME_Hologram) continue;
   IEntity otherOwner = other.GetOwner();
   if (!otherOwner || otherOwner.GetWorld() != owner.GetWorld()) continue;
   if (Math.Min(m_ME_HologramBoundsMaxs[1], other.m_ME_HologramBoundsMaxs[1]) - Math.Max(m_ME_HologramBoundsMins[1], other.m_ME_HologramBoundsMins[1]) <= ME_HOLOGRAM_OVERLAP_EPSILON) continue;
   if (!ME_DoHologramBoundsOverlapOnAxis(other, m_ME_HologramBoundsAxisX)) continue;
   if (!ME_DoHologramBoundsOverlapOnAxis(other, m_ME_HologramBoundsAxisZ)) continue;
   if (!ME_DoHologramBoundsOverlapOnAxis(other, other.m_ME_HologramBoundsAxisX)) continue;
   if (!ME_DoHologramBoundsOverlapOnAxis(other, other.m_ME_HologramBoundsAxisZ)) continue;
   return true;
  }
  return false;
 }

 //! Recolors the existing preview only when its advisory result changes.
 void ME_UpdateHologramTint()
 {
  if (!m_ME_Hologram) return;
  int color = ME_GetHologramTintColor();
  if (m_ME_HologramTintApplied && color == m_ME_HologramAppliedColor) return;
  ME_TintHologramHierarchy(m_ME_Hologram, color);
  m_ME_HologramAppliedColor = color;
  m_ME_HologramTintApplied = true;
 }

 //! Clears every registered editor preview on mode changes or plugin commands.
 static void ME_ClearAllHolograms()
 {
  foreach (SCR_AmbientVehicleSpawnPointComponent point : s_ME_EditorSpawnPoints)
  {
   if (!point) continue;
   point.ME_ClearHologram();
   point.m_ME_HologramFailedPrefab = string.Empty;
   point.m_ME_HologramTimerInitialized = false;
  }
 }

 //! Advances the planning example without modifying point configuration.
 void ME_NextHologram()
 {
  m_ME_HologramIndex++;
  ME_ClearHologram();
  m_ME_HologramFailedPrefab = string.Empty;
  if (SCR_Global.IsEditMode() && s_ME_HologramEnabled)
  {
   ME_RebuildHologram(GetOwner());
   ME_RefreshHologramFactionTag(GetOwner());
  }
 }

 //! Chooses a snapshot representative from the filtered catalog, or a red same-faction example when candidates are absent or unavailable.
 protected void ME_RebuildHologram(IEntity owner)
 {
  array<string> paths;
  string reason;
  bool candidateQueryAvailable = ME_GetEditorVehicleEnvelopeCandidatePaths(paths, reason);
  bool factionFallback = !candidateQueryAvailable || paths.IsEmpty();
  string fallbackCause;
  if (factionFallback)
  {
   if (candidateQueryAvailable) fallbackCause = "filter_empty";
   else fallbackCause = reason;
  }
  string fallbackFactionKey;
  string fallbackType;
  if (factionFallback)
  {
   SCR_FactionAffiliationComponent affiliation = SCR_FactionAffiliationComponent.Cast(owner.FindComponent(SCR_FactionAffiliationComponent));
   if (affiliation)
   {
    fallbackFactionKey = affiliation.GetDefaultFactionKey();
    if (fallbackFactionKey.IsEmpty()) fallbackFactionKey = affiliation.GetAffiliatedFactionKey();
   }
   array<string> preferredTypes = {};
   if (m_aIncludedEditableEntityLabels)
   {
    foreach (EEditableEntityLabel includedLabel : m_aIncludedEditableEntityLabels)
    {
     string includedName = typename.EnumToString(EEditableEntityLabel, includedLabel);
     if (includedName.Contains("VEHICLE_") && !preferredTypes.Contains(includedName)) preferredTypes.Insert(includedName);
    }
   }
   string fallbackPrefab;
   string fallbackReason;
   if (ME_VehicleBoundsSnapshotHelper.ME_GetFactionFallbackPreviewPrefab(fallbackFactionKey, preferredTypes, fallbackPrefab, fallbackType, fallbackReason))
    paths.Insert(fallbackPrefab);
   else
   {
    if (!reason.IsEmpty()) reason += "; ";
    reason += fallbackReason;
   }
  }
  if (paths.IsEmpty())
  {
   ME_ClearHologram();
   m_ME_HologramFailedPrefab = string.Empty;
   m_ME_HologramPaths = null;
   m_ME_HologramSnapshotFaction = string.Empty;
   m_ME_HologramTypeSignature = string.Empty;
   m_ME_HologramCaption = "Vehicle preview unavailable / " + reason;
   ME_LogHologramStatus("status=UNAVAILABLE reason=" + reason);
   return;
  }
  paths.Sort();

  string factionKey;
  array<string> vehicleTypes;
  array<SCR_EntityCatalogEntry> entries;
  string selectionReason;
  bool selectionAvailable;
  if (factionFallback)
   factionKey = fallbackFactionKey;
  else
   selectionAvailable = ME_GetEditorVehicleAggregateSelection(factionKey, vehicleTypes, entries, selectionReason);
  string typeSignature;
  if (m_aIncludedEditableEntityLabels)
  {
   foreach (EEditableEntityLabel includedLabel : m_aIncludedEditableEntityLabels)
   {
    string labelName = typename.EnumToString(EEditableEntityLabel, includedLabel);
    if (labelName.Contains("VEHICLE_")) typeSignature += labelName + ";";
   }
  }

  bool changed = !m_ME_HologramPaths || m_ME_HologramPaths.Count() != paths.Count() || m_ME_HologramSnapshotFaction != factionKey || m_ME_HologramTypeSignature != typeSignature || m_ME_HologramFactionFallback != factionFallback || m_ME_HologramFallbackReason != fallbackCause;
  if (!changed)
  {
   for (int i = 0; i < paths.Count(); i++)
   {
    if (paths[i] != m_ME_HologramPaths[i]) { changed = true; break; }
   }
  }
  if (changed)
  {
   ME_ClearHologram();
   m_ME_HologramFailedPrefab = string.Empty;
   m_ME_HologramFactionFallback = factionFallback;
   m_ME_HologramFallbackType = fallbackType;
   m_ME_HologramFallbackReason = fallbackCause;
   m_ME_HologramPaths = paths;
   m_ME_HologramSnapshotFaction = factionKey;
   m_ME_HologramTypeSignature = typeSignature;
   m_ME_HologramIndex = 0;
   m_ME_HologramBestIndex = 0;
   m_ME_HologramSnapshotSelected = false;
   m_ME_HologramSelectionNote = "Example";

   if (factionFallback)
    ME_LogHologramStatus(string.Format("status=FALLBACK reason=%1 faction=%2 type=%3 prefab=%4", fallbackCause, factionKey, fallbackType, paths[0]));
   else if (selectionAvailable && vehicleTypes && !vehicleTypes.IsEmpty())
   {
    array<string> selectedTypes = {};
    if (m_aIncludedEditableEntityLabels)
    {
     foreach (EEditableEntityLabel label : m_aIncludedEditableEntityLabels)
     {
      string labelName = typename.EnumToString(EEditableEntityLabel, label);
      if (labelName.Contains("VEHICLE_") && vehicleTypes.Contains(labelName) && !selectedTypes.Contains(labelName))
       selectedTypes.Insert(labelName);
     }
    }
    if (selectedTypes.IsEmpty()) selectedTypes.Copy(vehicleTypes);

    string snapshotPrefab;
    string snapshotType;
    string snapshotReason;
    if (ME_VehicleBoundsSnapshotHelper.ME_GetLargestFootprintPrefab(factionKey, selectedTypes, paths, snapshotPrefab, snapshotType, snapshotReason))
    {
     m_ME_HologramBestIndex = paths.Find(snapshotPrefab);
     m_ME_HologramSnapshotSelected = true;
     m_ME_HologramSelectionNote = "Largest " + snapshotType;
    }
    else
    {
     // Preserve type preference when the snapshot representative was excluded by another label.
     if (!snapshotType.IsEmpty())
     {
      selectedTypes.Clear();
      selectedTypes.Insert(snapshotType);
     }
     array<string> matchingPaths = {};
     foreach (SCR_EntityCatalogEntry catalogEntry : entries)
     {
      array<EEditableEntityLabel> candidateLabels = {};
      catalogEntry.GetEditableEntityLabels(candidateLabels);
      foreach (EEditableEntityLabel candidateLabel : candidateLabels)
      {
       string candidateType = typename.EnumToString(EEditableEntityLabel, candidateLabel);
       if (selectedTypes.Contains(candidateType) && !matchingPaths.Contains(catalogEntry.GetPrefab()))
       {
        matchingPaths.Insert(catalogEntry.GetPrefab());
        break;
       }
      }
     }
     if (!matchingPaths.IsEmpty())
     {
      matchingPaths.Sort();
      m_ME_HologramBestIndex = paths.Find(matchingPaths[0]);
     }
     if (snapshotReason != "snapshot_largest_prefab_filtered_out")
      ME_LogHologramStatus(string.Format("status=SELECTION_FALLBACK reason=%1 type=%2", snapshotReason, snapshotType));
    }
   }
   else if (!factionFallback && !selectionReason.IsEmpty()) ME_LogHologramStatus("status=SELECTION_FALLBACK reason=" + selectionReason);
  }

  m_ME_HologramIndex = m_ME_HologramIndex % paths.Count();
  int selectedIndex = (m_ME_HologramBestIndex + m_ME_HologramIndex) % paths.Count();
  string path = paths[selectedIndex];
  if (path == m_ME_HologramPrefab && m_ME_Hologram)
  {
   ME_SetHologramCaption(path, paths.Count());
   ME_PositionHologram(owner);
   ME_UpdateHologramTint();
   return;
  }
  // Retry unavailable prefab data only after the candidate set changes or an explicit Next/Toggle action.
  if (path == m_ME_HologramFailedPrefab) return;
  ME_ClearHologram();
  m_ME_HologramFactionFallback = factionFallback;
  m_ME_HologramFallbackType = fallbackType;
  m_ME_HologramFallbackReason = fallbackCause;
  m_ME_HologramResource = Resource.Load(path);
  if (!m_ME_HologramResource || !m_ME_HologramResource.IsValid())
  {
   m_ME_HologramFailedPrefab = path;
   m_ME_HologramCaption = "Vehicle preview: resource unavailable / " + path;
   ME_LogHologramStatus("status=UNAVAILABLE reason=resource_load_failed prefab=" + path);
   return;
  }
  IEntitySource source = SCR_BaseContainerTools.FindEntitySource(m_ME_HologramResource);
  if (!source)
  {
   m_ME_HologramFailedPrefab = path;
   m_ME_HologramCaption = "Vehicle preview: entity source unavailable";
   ME_LogHologramStatus("status=UNAVAILABLE reason=entity_source_unavailable prefab=" + path);
   return;
  }
  array<ref SCR_BasePreviewEntry> previewEntries = {};
  SCR_PrefabPreviewEntity.GetPreviewEntries(source, previewEntries);
  bool partial;
  foreach (SCR_BasePreviewEntry previewEntry : previewEntries)
  {
   if (previewEntry.m_Shape == EPreviewEntityShape.PREFAB)
   {
    previewEntry.m_Shape = EPreviewEntityShape.MESH;
    previewEntry.m_Mesh = ResourceName.Empty;
    partial = true;
   }
  }
  if (previewEntries.IsEmpty())
  {
   m_ME_HologramFailedPrefab = path;
   m_ME_HologramCaption = "Vehicle preview: no mesh entries";
   ME_LogHologramStatus("status=UNAVAILABLE reason=no_mesh_entries prefab=" + path);
   return;
  }
  EntitySpawnParams params = new EntitySpawnParams();
  params.TransformMode = ETransformMode.WORLD;
  Math3D.AnglesToMatrix(owner.GetYawPitchRoll(), params.Transform);
  params.Transform[3] = owner.GetOrigin();
  m_ME_Hologram = SCR_BasePreviewEntity.SpawnPreview(previewEntries, "{A96835FEA7B04418}Prefabs/Editor/ME_VehicleHologram.et", owner.GetWorld(), params, ME_HOLOGRAM_TINT_MATERIAL, EPreviewEntityFlag.IGNORE_TERRAIN);
  if (!m_ME_Hologram)
  {
   m_ME_HologramCaption = "Vehicle preview: creation failed";
   ME_LogHologramStatus("status=UNAVAILABLE reason=preview_creation_failed prefab=" + path);
   return;
  }
  m_ME_Hologram.SetFlags(EntityFlags.EDITOR_ONLY, true);
  m_ME_Hologram.ClearFlags(EntityFlags.TRACEABLE, true);
  m_ME_HologramPrefab = path;
  m_ME_HologramFailedPrefab = string.Empty;
  m_ME_HologramPartial = partial;
  ME_SetHologramCaption(path, paths.Count());
  ME_PositionHologram(owner);
  ME_UpdateHologramTint();
  if (m_ME_LastHologramStatus.StartsWith("status=UNAVAILABLE")) m_ME_LastHologramStatus = string.Empty;
  if (partial) ME_LogHologramStatus("status=PARTIAL reason=nested_prefab_mesh_unavailable prefab=" + path);
 }

 //! Refreshes the candidate count even if the chosen prefab stays the same.
 protected void ME_SetHologramCaption(string path, int count)
 {
  array<string> parts = {};
  path.Split("/", parts, true);
  if (m_ME_HologramFactionFallback)
  {
   string fallbackStatus = "FILTER HAS NO MATCH";
   if (m_ME_HologramFallbackReason != "filter_empty") fallbackStatus = "CANDIDATES COULD NOT BE CHECKED";
   m_ME_HologramCaption = string.Format("Faction %1 example (%2): %3 / %4", m_ME_HologramSnapshotFaction, m_ME_HologramFallbackType, parts[parts.Count() - 1], fallbackStatus);
   if (m_ME_HologramPartial) m_ME_HologramCaption += " / partial preview";
   return;
  }
  string label = "Example";
  if (m_ME_HologramIndex == 0 && m_ME_HologramSnapshotSelected) label = m_ME_HologramSelectionNote;
  m_ME_HologramCaption = string.Format("%1 %2/%3: %4 (not a spawn prediction)", label, m_ME_HologramIndex + 1, count, parts[parts.Count() - 1]);
  if (m_ME_HologramPartial) m_ME_HologramCaption += " / partial preview";
  if (!m_ME_HologramSnapshotSelected) m_ME_HologramCaption += " / snapshot fallback";
 }

 //! Finds the lowest and highest transformed mesh corners, including preview children.
 protected void ME_MeasureHologramVerticalBounds(IEntity entity, inout bool found, inout float bottom, inout float top)
 {
  if (entity.GetVObject())
  {
   vector mins, maxs;
   entity.GetBounds(mins, maxs);
   vector matrix[4];
   entity.GetWorldTransform(matrix);
   for (int i = 0; i < 8; i++)
   {
    vector corner = mins;
    if (i & 1) corner[0] = maxs[0];
    if (i & 2) corner[1] = maxs[1];
    if (i & 4) corner[2] = maxs[2];
    corner = corner.Multiply4(matrix);
    if (!found || corner[1] < bottom) bottom = corner[1];
    if (!found || corner[1] > top) top = corner[1];
    found = true;
   }
  }
  IEntity child = entity.GetChildren();
  while (child)
  {
   ME_MeasureHologramVerticalBounds(child, found, bottom, top);
   child = child.GetSibling();
  }
 }

 //! Projects every visible preview mesh into the point's horizontal axes and world height.
 protected void ME_MeasureHologramProjectedBounds(IEntity entity, vector axisX, vector axisZ, inout bool found, inout vector mins, inout vector maxs)
 {
  if (entity.GetVObject())
  {
   vector localMins, localMaxs;
   entity.GetBounds(localMins, localMaxs);
   vector matrix[4];
   entity.GetWorldTransform(matrix);
   for (int i = 0; i < 8; i++)
   {
    vector corner = localMins;
    if (i & 1) corner[0] = localMaxs[0];
    if (i & 2) corner[1] = localMaxs[1];
    if (i & 4) corner[2] = localMaxs[2];
    corner = corner.Multiply4(matrix);
    vector projected = Vector(vector.Dot(corner, axisX), corner[1], vector.Dot(corner, axisZ));
    if (!found)
    {
     mins = projected;
     maxs = projected;
     found = true;
    }
    else
    {
     for (int axis = 0; axis < 3; axis++)
     {
      mins[axis] = Math.Min(mins[axis], projected[axis]);
      maxs[axis] = Math.Max(maxs[axis], projected[axis]);
     }
    }
   }
  }
  IEntity child = entity.GetChildren();
  while (child)
  {
   ME_MeasureHologramProjectedBounds(child, axisX, axisZ, found, mins, maxs);
   child = child.GetSibling();
  }
 }

 //! Places the visual bottom at terrain/marker height without simulating suspension.
 protected void ME_PositionHologram(IEntity owner)
 {
  if (!m_ME_Hologram) return;
  m_ME_HologramBoundsValid = false;
  vector transform[4];
  Math3D.AnglesToMatrix(owner.GetYawPitchRoll(), transform);
  transform[3] = owner.GetOrigin();
  m_ME_Hologram.SetWorldTransform(transform);
  m_ME_Hologram.Update();
  bool found;
  float bottom;
  float top;
  ME_MeasureHologramVerticalBounds(m_ME_Hologram, found, bottom, top);
  if (!found) return;
  vector origin = owner.GetOrigin();
  float surface = owner.GetWorld().GetSurfaceY(origin[0], origin[2]);
  float support = Math.Max(origin[1], surface);
  float lift = support - bottom + 0.02;
  transform[3] = transform[3] + Vector(0, lift, 0);
  m_ME_Hologram.SetWorldTransform(transform);
  m_ME_Hologram.Update();
  vector angles = owner.GetYawPitchRoll();
  float yaw = angles[0] * Math.DEG2RAD;
  m_ME_HologramBoundsAxisX = Vector(Math.Cos(yaw), 0, -Math.Sin(yaw));
  m_ME_HologramBoundsAxisZ = Vector(Math.Sin(yaw), 0, Math.Cos(yaw));
  ME_MeasureHologramProjectedBounds(m_ME_Hologram, m_ME_HologramBoundsAxisX, m_ME_HologramBoundsAxisZ, m_ME_HologramBoundsValid, m_ME_HologramBoundsMins, m_ME_HologramBoundsMaxs);
  m_ME_HologramTop = top + lift;
  m_ME_HologramLastOrigin = origin;
  m_ME_HologramLastAngles = owner.GetYawPitchRoll();
  m_ME_HologramTransformValid = true;
  ME_RefreshHologramFactionTag(owner);
 }

 //! Resolves the configured point faction and its game-defined color for the editor caption.
 protected void ME_GetHologramFactionTag(IEntity owner, out string tag, out int color)
 {
  tag = "FACTION UNKNOWN";
  color = Color.FromRGBA(224, 224, 224, 255).PackToInt();
  SCR_FactionAffiliationComponent affiliation = SCR_FactionAffiliationComponent.Cast(owner.FindComponent(SCR_FactionAffiliationComponent));
  if (!affiliation) return;
  FactionKey factionKey = affiliation.GetDefaultFactionKey();
  if (factionKey.IsEmpty()) factionKey = affiliation.GetAffiliatedFactionKey();
  if (factionKey.IsEmpty()) return;
  tag = factionKey;
  FactionManager factionManager = GetGame().GetFactionManager();
  if (!factionManager) return;
  Faction faction = factionManager.GetFactionByKey(factionKey);
  if (!faction) return;
  Color factionColor = faction.GetFactionColor();
  if (factionColor) color = factionColor.PackToInt();
 }

 //! Keeps one persistent, slightly larger faction tag just above each preview instead of creating text every frame.
 protected void ME_RefreshHologramFactionTag(IEntity owner)
 {
  if (!owner) return;
  vector origin = owner.GetOrigin();
  vector position = origin + Vector(0, 2.5, 0);
  if (m_ME_Hologram && m_ME_HologramTransformValid) position[1] = m_ME_HologramTop + 0.35;
  string tag;
  int color;
  ME_GetHologramFactionTag(owner, tag, color);
  if (m_ME_HologramFactionTagText && tag == m_ME_HologramFactionTag && color == m_ME_HologramFactionColor
   && position[0] == m_ME_HologramFactionTagPosition[0] && position[1] == m_ME_HologramFactionTagPosition[1] && position[2] == m_ME_HologramFactionTagPosition[2])
   return;
  m_ME_HologramFactionTagText = null;
  vector transform[4];
  owner.GetTransform(transform);
  transform[3] = position;
  m_ME_HologramFactionTagText = DebugTextWorldSpace.CreateInWorld(owner.GetWorld(), tag, DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA, transform, ME_HOLOGRAM_FACTION_TAG_FONT_SIZE, color, Color.FromRGBA(0, 0, 0, 178).PackToInt(), 1000);
  m_ME_HologramFactionTag = tag;
  m_ME_HologramFactionColor = color;
  m_ME_HologramFactionTagPosition = position;
  m_ME_HologramFactionTagOwnerOrigin = origin;
 }

 //! Keeps preview updates active for every loaded editor point.
 override int _WB_GetAfterWorldUpdateSpecs(IEntity owner, IEntitySource src)
 {
  return super._WB_GetAfterWorldUpdateSpecs(owner, src) | EEntityFrameUpdateSpecs.CALL_ALWAYS;
 }

 //! Builds previews and persistent faction tags for all points; the detailed caption remains selection-only.
 override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
 {
  super._WB_AfterWorldUpdate(owner, timeSlice);
  if (!SCR_Global.IsEditMode() || !s_ME_HologramEnabled)
  {
   if (m_ME_Hologram || m_ME_HologramFactionTagText) ME_ClearHologram();
   return;
  }
  WorldEditor editor = Workbench.GetModule(WorldEditor);
  WorldEditorAPI api;
  if (editor) api = editor.GetApi();
  IEntity selected;
  if (api)
  {
   IEntitySource selectedSource = api.GetSelectedEntity();
   if (selectedSource) selected = api.SourceToEntity(selectedSource);
  }

  if (!m_ME_HologramTimerInitialized)
  {
   m_ME_HologramTimer = -Math.RandomFloat01();
   m_ME_HologramTimerInitialized = true;
  }
  m_ME_HologramTimer += timeSlice;
  if (m_ME_HologramTimer >= 1)
  {
   m_ME_HologramTimer = -4;
   ME_RebuildHologram(owner);
   ME_RefreshHologramFactionTag(owner);
  }
  if (m_ME_Hologram)
  {
   vector origin = owner.GetOrigin();
   vector angles = owner.GetYawPitchRoll();
   if (!m_ME_HologramTransformValid || origin[0] != m_ME_HologramLastOrigin[0] || origin[1] != m_ME_HologramLastOrigin[1] || origin[2] != m_ME_HologramLastOrigin[2] || angles[0] != m_ME_HologramLastAngles[0] || angles[1] != m_ME_HologramLastAngles[1] || angles[2] != m_ME_HologramLastAngles[2])
   {
    ME_PositionHologram(owner);
    ME_UpdateHologramTint();
   }
  }
  vector tagOrigin = owner.GetOrigin();
  if (m_ME_HologramFactionTagText && (tagOrigin[0] != m_ME_HologramFactionTagOwnerOrigin[0] || tagOrigin[1] != m_ME_HologramFactionTagOwnerOrigin[1] || tagOrigin[2] != m_ME_HologramFactionTagOwnerOrigin[2]))
   ME_RefreshHologramFactionTag(owner);
  if (selected == owner)
  {
   vector origin = owner.GetOrigin();
   float factionTagY = origin[1] + 2.5;
   if (m_ME_Hologram && m_ME_HologramTransformValid) factionTagY = m_ME_HologramTop + 0.35;
   if (!m_ME_HologramCaption.IsEmpty())
   {
    float captionY = Math.Max(origin[1] + 5, factionTagY + 1.1);
    string caption = m_ME_HologramCaption;
    int captionColor = Color.RED;
    if (m_ME_Hologram && !m_ME_HologramFactionFallback)
    {
     caption += " / visual ground offset; suspension not simulated";
     captionColor = Color.YELLOW;
    }
    DebugTextWorldSpace.Create(owner.GetWorld(), caption, DebugTextFlags.ONCE | DebugTextFlags.CENTER, origin[0], captionY, origin[2], 12, captionColor, Color.BLACK);
    if (m_ME_Hologram && ME_HasOverlappingHologramBounds())
     DebugTextWorldSpace.Create(owner.GetWorld(), "WARNING: vehicle previews overlap", DebugTextFlags.ONCE | DebugTextFlags.CENTER, origin[0], captionY + 0.75, origin[2], 12, Color.RED, Color.BLACK);
   }
  }
 }

 //! Releases the transient preview before entity deletion.
 override void OnDelete(IEntity owner)
 {
  ME_ClearHologram();
  super.OnDelete(owner);
 }

 //! Clears previews before editor deletion as well.
 override void _WB_OnDelete(IEntity owner, IEntitySource src)
 {
  ME_ClearHologram();
  super._WB_OnDelete(owner, src);
 }
}
#endif
