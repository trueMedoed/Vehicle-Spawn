//! Diagnostics for ambient vehicle spawn points that are configured so that no vehicle can ever
//! be selected. The vanilla component returns silently when the label filter yields no candidates,
//! which leaves an empty spawn point and no trace in the log. This mod repeats the vanilla filter
//! after the base call and reports the empty result as an error, separating a self-contradictory
//! label setup from a filter that simply found nothing in the catalog.

modded class SCR_AmbientVehicleSpawnPointComponent
{
	ref Shape m_ME_EditorSpawnAreaShape;
	ref Shape m_ME_EditorVehicleEnvelopeFillShape;
	ref DebugTextWorldSpace m_ME_EditorVehicleCategoryExcludedLabel;
	ref array<ref DebugTextWorldSpace> m_aME_EditorVehicleCategoryIncludedLabels = {};
	// World-space font size shared by the excluded and included vehicle label texts, kept small enough to stay readable with a close camera.
	static const float ME_EDITOR_VEHICLE_CATEGORY_LABEL_FONT_SIZE = 0.5;
	// Horizontal distance between adjacent included vehicle label texts, scaled to the label font size.
	static const float ME_EDITOR_VEHICLE_CATEGORY_LABEL_SPACING = 1.0;
	// Bit value for the wheeled vehicle catalog category.
	static const int ME_EDITOR_VEHICLE_CATEGORY_WHEELED = 1;
	// Bit value for the helicopter vehicle catalog category.
	static const int ME_EDITOR_VEHICLE_CATEGORY_HELICOPTER = 2;
	static ref array<SCR_AmbientVehicleSpawnPointComponent> s_ME_EditorSpawnPoints = {};
	static ref array<IEntity> s_ME_EditorStaticObjectMarkerEntities = {};
	static ref array<ref Shape> s_ME_EditorStaticObjectMarkerShapes = {};
	protected int m_iME_EditorVehicleCategoryMask;
	protected int m_iME_EditorStaticObjectConflictCount;
	protected vector m_vME_EditorVehicleEnvelopeLocalMins;
	protected vector m_vME_EditorVehicleEnvelopeLocalMaxs;
	protected bool m_bME_EditorVehicleEnvelopePreviewActive;
	protected int m_iME_EditorVehicleEnvelopeFillColor;

	//------------------------------------------------------------------------------------------------
	//! Formats editable entity labels as a readable comma-separated list for log output.
	//! \param[in] labels Labels to format, may be null or empty
	//! \return Enum names joined by ", ", or "<none>" when there is nothing to list
	protected string ME_EditableEntityLabelsToString(array<EEditableEntityLabel> labels)
	{
		if (!labels || labels.IsEmpty())
			return "<none>";

		string result;
		foreach (EEditableEntityLabel label: labels)
		{
			if (result != "")
				result += ", ";

			result += typename.EnumToString(EEditableEntityLabel, label);
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
    //! Collects labels requested in IncludedEditableEntityLabels that are at the same time rejected
    //! by ExcludedEditableEntityLabels. Such a label can never be satisfied, so a non-empty result
    //! means the spawn point contradicts itself. Note that the exclusion may be inherited: the base
    //! prefab AmbientVehicleSpawnpoint_Base.et already excludes TRAIT_ARMED.
    //! \return Labels present in both lists, empty when the configuration is consistent
	protected array<EEditableEntityLabel> ME_GetConflictingEditableEntityLabels()
	{
		array<EEditableEntityLabel> conflictingLabels = {};
		foreach (EEditableEntityLabel includedLabel: m_aIncludedEditableEntityLabels)
		{
			if (m_aExcludedEditableEntityLabels.Contains(includedLabel))
				conflictingLabels.Insert(includedLabel);
		}

		return conflictingLabels;
	}
	
	//------------------------------------------------------------------------------------------------
    //! Picks the vehicle prefab for this spawn point and reports a configuration that can never
    //! produce one. Called from SpawnVehicle() when the affiliated faction changed or when there is
    //! no faction and no prefab has been chosen yet, so this runs at spawn time rather than on init.
    //! After the base call the vanilla label filter is repeated: an empty result is logged as an
    //! error, naming the conflicting labels when the include and exclude lists overlap and listing
    //! both lists otherwise. The conflicting-labels diagnosis is only conclusive while
    //! m_bRequireAllIncludedLabels is set, because then a single unsatisfiable label empties the
    //! result on its own; with the default "any included label" matching the empty result may have
    //! an unrelated cause.
    //! \param[in] faction Faction whose vehicle catalog is filtered, null falls back to the global catalog
	override protected void Update(SCR_Faction faction)
	{
		super.Update(faction);

		m_SavedFaction = faction;
		SCR_EntityCatalog entityCatalog;

		if (faction)
		{
			entityCatalog = faction.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE);
		}
		else
		{
			SCR_EntityCatalogManagerComponent comp = SCR_EntityCatalogManagerComponent.GetInstance();

			if (!comp)
				return;

			entityCatalog = comp.GetEntityCatalogOfType(EEntityCatalogType.VEHICLE);
		}

		if (!entityCatalog)
			return;

		array<SCR_EntityCatalogEntry> data = {};
		entityCatalog.GetFullFilteredEntityListWithLabels(data, m_aIncludedEditableEntityLabels, m_aExcludedEditableEntityLabels, m_bRequireAllIncludedLabels);

		if (data.IsEmpty())
		{
			array<EEditableEntityLabel> conflictingLabels = ME_GetConflictingEditableEntityLabels();
			string pointName = GetOwner().GetName();
		  	string pointInfo = string.Format("coordinates=%1", GetOwner().GetOrigin());
		
		 	if (!pointName.IsEmpty())
       			pointInfo = string.Format("Entity=%1, coordinates=%2", pointName, GetOwner().GetOrigin());

			if (!conflictingLabels.IsEmpty())
			{
				Print(
        			string.Format(
		                "[ME_DEBUG_AVSP_ERROR] SCR_AmbientVehicleSpawnPointComponent: conflicting labels [%1] are present in both IncludedEditableEntityLabels and ExcludedEditableEntityLabels. Vehicle will not spawn. Coordinates=%2",
		                ME_EditableEntityLabelsToString(conflictingLabels),
		                pointInfo
			        ),
			        LogLevel.ERROR
				);
			}
			else
			{
				Print(
        			string.Format(
		                "[ME_DEBUG_AVSP_ERROR] SCR_AmbientVehicleSpawnPointComponent: no vehicle matches the configured entity labels. Vehicle will not spawn. Included=[%1], Excluded=[%2], %3",
		                ME_EditableEntityLabelsToString(m_aIncludedEditableEntityLabels),
		                ME_EditableEntityLabelsToString(m_aExcludedEditableEntityLabels),
		                pointInfo
			        ),
			        LogLevel.ERROR
				);
			}

			return;
		}

		// No prefab is selected here: super.Update(faction) already performed the vanilla selection.
	}

	//------------------------------------------------------------------------------------------------
	//! Resolves the spawn point faction color used by the editor-only vehicle envelope fill.
	//! The fallback uses a neutral diagnostic yellow when no faction is assigned to this point.
	protected int ME_GetEditorVehicleEnvelopeFillColor()
	{
		const int fallbackColor = Color.FromRGBA(255, 215, 0, 255).PackToInt();
		IEntity owner = GetOwner();
		if (!owner)
			return fallbackColor;

		SCR_FactionAffiliationComponent affiliation = SCR_FactionAffiliationComponent.Cast(owner.FindComponent(SCR_FactionAffiliationComponent));
		if (!affiliation)
			return fallbackColor;

		FactionKey factionKey = affiliation.GetDefaultFactionKey();
		if (factionKey.IsEmpty())
			factionKey = affiliation.GetAffiliatedFactionKey();
		if (factionKey.IsEmpty())
			return fallbackColor;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return fallbackColor;

		SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
		if (!faction)
			return fallbackColor;

		return faction.GetFactionColor().PackToInt();
	}

	//------------------------------------------------------------------------------------------------
	//! Stores the resolved faction color before rebuilding the validated envelope.
	void ME_SetEditorVehicleEnvelopeFillColor()
	{
		m_iME_EditorVehicleEnvelopeFillColor = ME_GetEditorVehicleEnvelopeFillColor();
	}

	//------------------------------------------------------------------------------------------------
	//! Collects the vehicle-catalog candidates that vanilla Update() would filter for this point in the World Editor.
	//! This read-only diagnostic does not select a prefab, spawn an entity, or change runtime state.
	//!
	//! \param[out] entries Filtered vehicle catalog entries
	//! \param[out] reason Stable reason when candidates cannot be read safely
	//! \return True when entries contain the complete applicable catalog filter result
	bool ME_GetEditorVehicleEnvelopeCandidates(out array<SCR_EntityCatalogEntry> entries, out string reason)
	{
		entries = {};
		reason = "";

		IEntity owner = GetOwner();
		if (!owner)
		{
			reason = "owner_unavailable";
			return false;
		}

		SCR_Faction faction;
		SCR_FactionAffiliationComponent affiliation = SCR_FactionAffiliationComponent.Cast(owner.FindComponent(SCR_FactionAffiliationComponent));
		if (affiliation)
		{
			FactionKey factionKey = affiliation.GetDefaultFactionKey();
			if (factionKey.IsEmpty())
				factionKey = affiliation.GetAffiliatedFactionKey();

			if (!factionKey.IsEmpty())
			{
				FactionManager factionManager = GetGame().GetFactionManager();
				if (!factionManager)
				{
					reason = "faction_manager_unavailable";
					return false;
				}

				faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
				if (!faction)
				{
					reason = string.Format("faction_unavailable key=%1", factionKey);
					return false;
				}
			}
		}

		SCR_EntityCatalog entityCatalog;
		if (faction)
		{
			if (!faction.ME_EnsureEditorCatalogsInitialized())
			{
				reason = "faction_vehicle_catalog_initialization_unavailable";
				return false;
			}

			entityCatalog = faction.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE);
		}
		else
		{
			entityCatalog = SCR_EntityCatalogManagerComponent.ME_GetEditorGlobalVehicleCatalog(reason);
			if (!entityCatalog)
				return false;
		}

		if (!entityCatalog)
		{
			reason = "vehicle_catalog_unavailable";
			return false;
		}

		entityCatalog.GetFullFilteredEntityListWithLabels(entries, m_aIncludedEditableEntityLabels, m_aExcludedEditableEntityLabels, m_bRequireAllIncludedLabels);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Collects the complete vehicle catalog result that vanilla Update() would filter for this point in the World Editor.
	//! This read-only diagnostic does not select a prefab, spawn an entity, or change runtime state.
	//!
	//! \param[out] entries Filtered vehicle catalog entries
	//! \param[out] reason Stable reason when candidates cannot be read safely
	//! \return True when entries contain the complete applicable catalog filter result
	bool ME_GetEditorVehicleCategoryCandidates(out array<SCR_EntityCatalogEntry> entries, out string reason)
	{
		return ME_GetEditorVehicleEnvelopeCandidates(entries, reason);
	}

	//------------------------------------------------------------------------------------------------
	//! Collects the filtered vehicle catalog, its faction or global scope key, and unique VEHICLE_* label names.
	//!
	//! \param[out] factionKey Resolved faction key or the reserved global-catalog scope
	//! \param[out] vehicleTypeNames Unique vehicle-type label names present in the filtered result
	//! \param[out] entries Filtered vehicle catalog entries
	//! \param[out] reason Stable reason when the selection cannot be read safely
	//! \return True when the complete applicable catalog filter result is available
	bool ME_GetEditorVehicleAggregateSelection(out string factionKey, out array<string> vehicleTypeNames, out array<SCR_EntityCatalogEntry> entries, out string reason)
	{
		factionKey = "";
		vehicleTypeNames = {};
		if (!ME_GetEditorVehicleEnvelopeCandidates(entries, reason))
			return false;

		SCR_FactionAffiliationComponent affiliation = SCR_FactionAffiliationComponent.Cast(GetOwner().FindComponent(SCR_FactionAffiliationComponent));
		if (affiliation)
		{
			FactionKey resolvedFactionKey = affiliation.GetDefaultFactionKey();
			if (resolvedFactionKey.IsEmpty())
				resolvedFactionKey = affiliation.GetAffiliatedFactionKey();
			if (!resolvedFactionKey.IsEmpty())
				factionKey = resolvedFactionKey;
		}

		if (factionKey.IsEmpty())
			factionKey = ME_VehicleBoundsSnapshotHelper.ME_GLOBAL_VEHICLE_CATALOG_SCOPE;

		foreach (SCR_EntityCatalogEntry entry : entries)
		{
			array<EEditableEntityLabel> labels = {};
			entry.GetEditableEntityLabels(labels);
			foreach (EEditableEntityLabel label : labels)
			{
				string labelName = typename.EnumToString(EEditableEntityLabel, label);
				if (labelName.Contains("VEHICLE_") && !vehicleTypeNames.Contains(labelName))
					vehicleTypeNames.Insert(labelName);
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Classifies filtered vehicle prefab paths using the canonical wheeled and helicopter categories.
	//!
	//! \param[out] reason Stable reason when the catalog cannot be read safely
	//! \return Bit mask of supported vehicle categories
	protected int ME_GetEditorVehicleCategoryMask(out string reason)
	{
		reason = "";
		array<SCR_EntityCatalogEntry> entries;
		if (!ME_GetEditorVehicleCategoryCandidates(entries, reason))
			return 0;

		int categoryMask;
		foreach (SCR_EntityCatalogEntry entry: entries)
		{
			string prefabPath = entry.GetPrefab();
			if (prefabPath.Contains("Prefabs/Vehicles/Wheeled/"))
				categoryMask = categoryMask | ME_EDITOR_VEHICLE_CATEGORY_WHEELED;
			else if (prefabPath.Contains("Prefabs/Vehicles/Helicopters/"))
				categoryMask = categoryMask | ME_EDITOR_VEHICLE_CATEGORY_HELICOPTER;
		}

		return categoryMask;
	}

	//------------------------------------------------------------------------------------------------
	//! Formats configured labels as comma-separated enum names, using ALL when the list is empty.
	//!
	//! \param[in] labels Included or excluded labels configured on this spawn point
	//! \return Comma-separated label names, or ALL when none are configured
	protected string ME_GetEditorVehicleCategoryLabelLabels(array<EEditableEntityLabel> labels)
	{
		if (!labels || labels.IsEmpty())
			return "ALL";

		string result;
		foreach (EEditableEntityLabel label: labels)
		{
			if (!result.IsEmpty())
				result += ", ";

			result += typename.EnumToString(EEditableEntityLabel, label);
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Selects the editor-only color for one included vehicle label by its enum name.
	//!
	//! \param[in] label Included label configured on this spawn point
	//! \return Color for this included label
	protected Color ME_GetEditorVehicleCategoryIncludedLabelColor(EEditableEntityLabel label)
	{
		string labelName = typename.EnumToString(EEditableEntityLabel, label);
		if (labelName.Contains("_HELICOPTER"))
			return Color.FromRGBA(180, 80, 255, 255);
		if (labelName.Contains("_TRUCK"))
			return Color.FromRGBA(0, 200, 255, 255);
		if (labelName.Contains("_APC"))
			return Color.FromRGBA(255, 140, 0, 255);
		if (labelName.Contains("_CAR"))
			return Color.FromRGBA(80, 220, 100, 255);
		return Color.FromRGBA(224, 224, 224, 255);
	}

	//------------------------------------------------------------------------------------------------
	//! Clears this point's editor-only excluded and included category text objects.
	void ME_ClearEditorVehicleCategoryLabel()
	{
		m_ME_EditorVehicleCategoryExcludedLabel = null;
		for (int includedIndex = 0; includedIndex < m_aME_EditorVehicleCategoryIncludedLabels.Count(); includedIndex++)
			m_aME_EditorVehicleCategoryIncludedLabels[includedIndex] = null;
		m_aME_EditorVehicleCategoryIncludedLabels.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuilds the camera-facing configured-label display from the cached catalog mask.
	//!
	//! \param[in] owner Spawn point entity whose transform anchors the label
	void ME_RefreshEditorVehicleCategoryLabel(IEntity owner)
	{
		ME_ClearEditorVehicleCategoryLabel();
		if (!owner)
			return;

		switch (m_iME_EditorVehicleCategoryMask)
		{
			case ME_EDITOR_VEHICLE_CATEGORY_WHEELED:
			case ME_EDITOR_VEHICLE_CATEGORY_HELICOPTER:
			case ME_EDITOR_VEHICLE_CATEGORY_WHEELED | ME_EDITOR_VEHICLE_CATEGORY_HELICOPTER:
				break;
			default:
				return;
		}

		vector transform[4];
		owner.GetTransform(transform);
		transform[3] = transform[3] + Vector(0, 8, 0);
		const int backgroundColor = Color.FromRGBA(0, 0, 0, 178).PackToInt();
		const DebugTextFlags textFlags = DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA;

		if (m_aExcludedEditableEntityLabels && !m_aExcludedEditableEntityLabels.IsEmpty())
		{
			vector excludedTransform[4];
			for (int excludedTransformIndex = 0; excludedTransformIndex < 4; excludedTransformIndex++)
				excludedTransform[excludedTransformIndex] = transform[excludedTransformIndex];
			excludedTransform[3] = excludedTransform[3] - Vector(0, 0.5, 0);
			m_ME_EditorVehicleCategoryExcludedLabel = DebugTextWorldSpace.CreateInWorld(GetGame().GetWorld(), ME_GetEditorVehicleCategoryLabelLabels(m_aExcludedEditableEntityLabels), textFlags, excludedTransform, ME_EDITOR_VEHICLE_CATEGORY_LABEL_FONT_SIZE, Color.FromRGBA(255, 48, 48, 255).PackToInt(), backgroundColor, 1000);
		}

		vector includedTransform[4];
		for (int includedTransformIndex = 0; includedTransformIndex < 4; includedTransformIndex++)
			includedTransform[includedTransformIndex] = transform[includedTransformIndex];
		includedTransform[3] = includedTransform[3] + Vector(0, 0.5, 0);
		if (!m_aIncludedEditableEntityLabels || m_aIncludedEditableEntityLabels.IsEmpty())
		{
			m_aME_EditorVehicleCategoryIncludedLabels.Insert(DebugTextWorldSpace.CreateInWorld(GetGame().GetWorld(), ME_GetEditorVehicleCategoryLabelLabels(m_aIncludedEditableEntityLabels), textFlags, includedTransform, ME_EDITOR_VEHICLE_CATEGORY_LABEL_FONT_SIZE, Color.FromRGBA(255, 215, 0, 255).PackToInt(), backgroundColor, 1000));
			return;
		}

		float offset = -0.5 * ME_EDITOR_VEHICLE_CATEGORY_LABEL_SPACING * (m_aIncludedEditableEntityLabels.Count() - 1);
		foreach (EEditableEntityLabel includedLabel: m_aIncludedEditableEntityLabels)
		{
			vector labelTransform[4];
			for (int labelTransformIndex = 0; labelTransformIndex < 4; labelTransformIndex++)
				labelTransform[labelTransformIndex] = includedTransform[labelTransformIndex];
			labelTransform[3] = labelTransform[3] + transform[0] * offset;
			m_aME_EditorVehicleCategoryIncludedLabels.Insert(DebugTextWorldSpace.CreateInWorld(GetGame().GetWorld(), typename.EnumToString(EEditableEntityLabel, includedLabel), textFlags, labelTransform, ME_EDITOR_VEHICLE_CATEGORY_LABEL_FONT_SIZE, ME_GetEditorVehicleCategoryIncludedLabelColor(includedLabel).PackToInt(), backgroundColor, 1000));
			offset += ME_EDITOR_VEHICLE_CATEGORY_LABEL_SPACING;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Computes the catalog category mask and creates the editor-only labels.
	void ME_UpdateEditorVehicleCategoryLabel()
	{
		string reason;
		m_iME_EditorVehicleCategoryMask = ME_GetEditorVehicleCategoryMask(reason);
		ME_RefreshEditorVehicleCategoryLabel(GetOwner());
	}

	//------------------------------------------------------------------------------------------------
	//! Releases this point's cached editor-only vehicle-envelope fill Shape and bounds.
	void ME_ClearEditorVehicleEnvelopePreview()
	{
		m_ME_EditorVehicleEnvelopeFillShape = null;
		m_bME_EditorVehicleEnvelopePreviewActive = false;
	}

	//------------------------------------------------------------------------------------------------
	//! Calculates and validates the selected faction and vehicle-type aggregate envelope, then stores it for transform refreshes.
	void ME_RefreshValidatedEditorVehicleEnvelopePreview()
	{
		string factionKey;
		array<string> vehicleTypeNames;
		array<SCR_EntityCatalogEntry> entries;
		string reason;
		if (!ME_GetEditorVehicleAggregateSelection(factionKey, vehicleTypeNames, entries, reason))
			return;

		vector aggregateMins;
		vector aggregateMaxs;
		if (!ME_VehicleBoundsSnapshotHelper.ME_GetValidatedAggregateBounds(factionKey, vehicleTypeNames, aggregateMins, aggregateMaxs, reason))
			return;

		ME_ShowEditorVehicleEnvelopePreview(aggregateMins, aggregateMaxs);
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuilds the cached vehicle envelope at the current editor transform.
	void ME_RefreshEditorVehicleEnvelopePreview()
	{
		m_ME_EditorVehicleEnvelopeFillShape = null;
		if (!m_bME_EditorVehicleEnvelopePreviewActive)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		vector origin = owner.GetOrigin();
		vector transform[4];
		owner.GetTransform(transform);
		vector angles = Math3D.MatrixToAngles(transform);
		float yawRadians = angles[0] * Math.DEG2RAD;
		float yawSin = Math.Sin(yawRadians);
		float yawCos = Math.Cos(yawRadians);
		vector localCorners[8];
		localCorners[0] = Vector(m_vME_EditorVehicleEnvelopeLocalMins[0], m_vME_EditorVehicleEnvelopeLocalMins[1], m_vME_EditorVehicleEnvelopeLocalMins[2]);
		localCorners[1] = Vector(m_vME_EditorVehicleEnvelopeLocalMaxs[0], m_vME_EditorVehicleEnvelopeLocalMins[1], m_vME_EditorVehicleEnvelopeLocalMins[2]);
		localCorners[2] = Vector(m_vME_EditorVehicleEnvelopeLocalMins[0], m_vME_EditorVehicleEnvelopeLocalMaxs[1], m_vME_EditorVehicleEnvelopeLocalMins[2]);
		localCorners[3] = Vector(m_vME_EditorVehicleEnvelopeLocalMaxs[0], m_vME_EditorVehicleEnvelopeLocalMaxs[1], m_vME_EditorVehicleEnvelopeLocalMins[2]);
		localCorners[4] = Vector(m_vME_EditorVehicleEnvelopeLocalMins[0], m_vME_EditorVehicleEnvelopeLocalMins[1], m_vME_EditorVehicleEnvelopeLocalMaxs[2]);
		localCorners[5] = Vector(m_vME_EditorVehicleEnvelopeLocalMaxs[0], m_vME_EditorVehicleEnvelopeLocalMins[1], m_vME_EditorVehicleEnvelopeLocalMaxs[2]);
		localCorners[6] = Vector(m_vME_EditorVehicleEnvelopeLocalMins[0], m_vME_EditorVehicleEnvelopeLocalMaxs[1], m_vME_EditorVehicleEnvelopeLocalMaxs[2]);
		localCorners[7] = Vector(m_vME_EditorVehicleEnvelopeLocalMaxs[0], m_vME_EditorVehicleEnvelopeLocalMaxs[1], m_vME_EditorVehicleEnvelopeLocalMaxs[2]);

		vector corners[8];
		for (int cornerIndex = 0; cornerIndex < 8; cornerIndex++)
		{
			vector localCorner = localCorners[cornerIndex];
			corners[cornerIndex] = origin + Vector(localCorner[0] * yawCos + localCorner[2] * yawSin, localCorner[1], localCorner[2] * yawCos - localCorner[0] * yawSin);
		}

		vector fillPoints[] = {
			corners[0], corners[1], corners[3], corners[0], corners[3], corners[2],
			corners[4], corners[6], corners[7], corners[4], corners[7], corners[5],
			corners[0], corners[2], corners[6], corners[0], corners[6], corners[4],
			corners[1], corners[5], corners[7], corners[1], corners[7], corners[3],
			corners[0], corners[4], corners[5], corners[0], corners[5], corners[1],
			corners[2], corners[3], corners[7], corners[2], corners[7], corners[6]
		};
		Color fillColor = Color.FromInt(m_iME_EditorVehicleEnvelopeFillColor);
		fillColor.SetA(48.0 / 255.0);
		m_ME_EditorVehicleEnvelopeFillShape = Shape.CreateTris(fillColor.PackToInt(), ShapeFlags.TRANSP | ShapeFlags.DOUBLESIDE, fillPoints, 36);
	}

	//------------------------------------------------------------------------------------------------
	//! Stores this point's validated conservative local vehicle envelope and refreshes only this point's Shape.
	void ME_ShowEditorVehicleEnvelopePreview(vector localMins, vector localMaxs)
	{
		m_vME_EditorVehicleEnvelopeLocalMins = localMins;
		m_vME_EditorVehicleEnvelopeLocalMaxs = localMaxs;
		ME_SetEditorVehicleEnvelopeFillColor();
		m_bME_EditorVehicleEnvelopePreviewActive = true;
		ME_RefreshEditorVehicleEnvelopePreview();
	}

	//------------------------------------------------------------------------------------------------
	//! Adds this spawn point to the editor-only registry once.
	void ME_RegisterEditorDebugSpawnPoint()
	{
		if (!s_ME_EditorSpawnPoints.Contains(this))
			s_ME_EditorSpawnPoints.Insert(this);
	}

	//------------------------------------------------------------------------------------------------
	//! Removes this spawn point from the editor-only registry.
	void ME_UnregisterEditorDebugSpawnPoint()
	{
		for (int i = s_ME_EditorSpawnPoints.Count() - 1; i >= 0; i--)
		{
			if (s_ME_EditorSpawnPoints[i] == this)
				s_ME_EditorSpawnPoints.Remove(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Releases every shared editor-only marker for static physics/bounds conflicts.
	static void ME_ClearEditorStaticObjectMarkers()
	{
		s_ME_EditorStaticObjectMarkerShapes.Clear();
		s_ME_EditorStaticObjectMarkerEntities.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! Tests whether an entity's world-space AABB intersects an editor spawn-area sphere.
	//!
	//! This is a broad-phase editor warning and is not a guaranteed runtime spawn failure.
	protected bool ME_DoBoundsIntersectEditorSpawnArea(vector mins, vector maxs, vector origin)
	{
		vector closestPoint;
		for (int i = 0; i < 3; i++)
		{
			closestPoint[i] = origin[i];
			if (closestPoint[i] < mins[i])
				closestPoint[i] = mins[i];
			else if (closestPoint[i] > maxs[i])
				closestPoint[i] = maxs[i];
		}

		vector delta = closestPoint - origin;
		float distanceSquared = delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2];
		return distanceSquared <= SPAWNING_RADIUS * SPAWNING_RADIUS;
	}

	//------------------------------------------------------------------------------------------------
	//! Creates one shared marker for an object, even when several spawn areas intersect its bounds.
	//!
	//! The marker size follows the object's bounds but remains visible for small objects and bounded
	//! for large objects so it remains an editor warning rather than a second spawn-area visualization.
	static void ME_AddEditorStaticObjectMarker(IEntity entity, vector mins, vector maxs)
	{
		if (s_ME_EditorStaticObjectMarkerEntities.Contains(entity))
			return;

		vector center = (mins + maxs) * 0.5;
		vector extents = (maxs - mins) * 0.5;
		float radius = Math.Max(extents[0], Math.Max(extents[1], extents[2]));
		radius = Math.Max(radius, 0.5);
		radius = Math.Min(radius, SPAWNING_RADIUS);

		Color color = Color.FromInt(Color.RED);
		color.SetA(0.375);
		ShapeFlags flags = ShapeFlags.TRANSP | ShapeFlags.DOUBLESIDE | ShapeFlags.NOOUTLINE;
		s_ME_EditorStaticObjectMarkerEntities.Insert(entity);
		s_ME_EditorStaticObjectMarkerShapes.Insert(Shape.CreateSphere(color.PackToInt(), flags, center, radius));
	}

	//------------------------------------------------------------------------------------------------
	//! Evaluates a broad-phase query result as a static physics/bounds editor-warning candidate.
	//!
	//! \return True to continue querying further entities
	protected bool ME_CollectEditorStaticObjectConflict(IEntity entity)
	{
		IEntity owner = GetOwner();
		if (!entity || entity == owner || entity.GetWorld() != owner.GetWorld())
			return true;

		if (SCR_AmbientVehicleSpawnPointComponent.Cast(entity.FindComponent(SCR_AmbientVehicleSpawnPointComponent)))
			return true;

		Physics physics = entity.GetPhysics();
		if (!physics || physics.IsDynamic())
			return true;

		vector mins;
		vector maxs;
		entity.GetWorldBounds(mins, maxs);
		vector origin = owner.GetOrigin();
		if (!ME_DoBoundsIntersectEditorSpawnArea(mins, maxs, origin))
			return true;

		m_iME_EditorStaticObjectConflictCount++;
		ME_AddEditorStaticObjectMarker(entity, mins, maxs);
		
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Scans this point's area for static physics/bounds editor warnings.
	protected void ME_RefreshEditorStaticObjectConflicts(IEntity owner)
	{
		m_iME_EditorStaticObjectConflictCount = 0;
		BaseWorld world = owner.GetWorld();
		if (world)
			world.QueryEntitiesBySphere(owner.GetOrigin(), SPAWNING_RADIUS, ME_CollectEditorStaticObjectConflict);
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuilds all registered editor shapes so every point reflects current overlaps and static objects.
	void ME_RefreshAllEditorDebugShapes()
	{
		ME_ClearEditorStaticObjectMarkers();
		foreach (SCR_AmbientVehicleSpawnPointComponent spawnPoint: s_ME_EditorSpawnPoints)
		{
			if (!spawnPoint)
				continue;

			IEntity owner = spawnPoint.GetOwner();
			if (owner)
				spawnPoint.ME_RefreshEditorDebugShape(owner);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Finds the first registered spawn point whose editor area touches this point's area.
	//! \param[in] origin Origin to compare against
	//! \param[in] world World in which the origin exists
	//! \param[out] overlappingPoint First touching or intersecting point, if any
	//! \return True when another point is within two spawning radii
	bool ME_FindOverlappingEditorSpawnPoint(vector origin, BaseWorld world, out SCR_AmbientVehicleSpawnPointComponent overlappingPoint)
	{
		float maxDistance = 2 * SPAWNING_RADIUS;
		float maxDistanceSquared = maxDistance * maxDistance;

		foreach (SCR_AmbientVehicleSpawnPointComponent spawnPoint: s_ME_EditorSpawnPoints)
		{
			if (!spawnPoint || spawnPoint == this)
				continue;

			IEntity otherOwner = spawnPoint.GetOwner();
			if (!otherOwner || otherOwner.GetWorld() != world)
				continue;

			vector delta = otherOwner.GetOrigin() - origin;
			float distanceSquared = delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2];
			if (distanceSquared <= maxDistanceSquared)
			{
				overlappingPoint = spawnPoint;
				return true;
			}
		}

		return false;
	}

	void ME_ClearEditorDebugShape()
	{
		m_ME_EditorSpawnAreaShape = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Probes the vanilla empty-terrain search and displays its result and area-overlap warning.
	//!
	//! Static physics/bounds markers are an editor advisory and do not alter the vanilla probe result.
	//! \param[in] owner Spawn point entity whose origin and world are tested
	void ME_RefreshEditorDebugShape(IEntity owner)
	{
		ME_ClearEditorDebugShape();

		vector origin = owner.GetOrigin();
		BaseWorld world = owner.GetWorld();
		vector candidate;
		bool found = SCR_WorldTools.FindEmptyTerrainPosition(candidate, origin, SPAWNING_RADIUS, SPAWNING_RADIUS, 2, TraceFlags.ENTS | TraceFlags.OCEAN, world);
		SCR_AmbientVehicleSpawnPointComponent overlappingPoint;
		bool overlap = ME_FindOverlappingEditorSpawnPoint(origin, world, overlappingPoint);
		bool red = !found || overlap;
		int color = Color.GREEN;
		if (red)
			color = Color.RED;

		Color colorValue = Color.FromInt(color);
		colorValue.SetA(0.375);
		ShapeFlags flags = ShapeFlags.TRANSP | ShapeFlags.DOUBLESIDE | ShapeFlags.NOOUTLINE;
		m_ME_EditorSpawnAreaShape = Shape.CreateSphere(colorValue.PackToInt(), flags, origin, SPAWNING_RADIUS);
		ME_RefreshEditorStaticObjectConflicts(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Registers and refreshes the editor probe when a spawn point is initialized.
	//! \param[in] owner Spawn point entity
	//! \param[in,out] mat Spawn point transform matrix
	//! \param[in] src Spawn point entity source
	override void _WB_OnInit(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		super._WB_OnInit(owner, mat, src);
		ME_RegisterEditorDebugSpawnPoint();
		ME_RefreshAllEditorDebugShapes();
		ME_UpdateEditorVehicleCategoryLabel();
		ME_RefreshValidatedEditorVehicleEnvelopePreview();
	}

	//------------------------------------------------------------------------------------------------
	//! Refreshes every editor probe after a spawn point is moved.
	//! \param[in] owner Spawn point entity
	//! \param[in,out] mat Spawn point transform matrix
	//! \param[in] src Spawn point entity source
	override void _WB_SetTransform(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		super._WB_SetTransform(owner, mat, src);
		ME_RefreshAllEditorDebugShapes();
		if (m_bME_EditorVehicleEnvelopePreviewActive)
			ME_RefreshEditorVehicleEnvelopePreview();
		ME_RefreshEditorVehicleCategoryLabel(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Unregisters the point, clears its shape, and refreshes remaining points on deletion.
	//! \param[in] owner Spawn point entity
	override void OnDelete(IEntity owner)
	{
		ME_UnregisterEditorDebugSpawnPoint();
		ME_ClearEditorDebugShape();
		ME_ClearEditorVehicleEnvelopePreview();
		ME_ClearEditorVehicleCategoryLabel();
		super.OnDelete(owner);
		ME_RefreshAllEditorDebugShapes();
	}

	//------------------------------------------------------------------------------------------------
	//! Unregisters the point before Workbench removes it and refreshes remaining points.
	//! \param[in] owner Spawn point entity
	//! \param[in] src Spawn point entity source
	override void _WB_OnDelete(IEntity owner, IEntitySource src)
	{
		ME_UnregisterEditorDebugSpawnPoint();
		ME_ClearEditorDebugShape();
		ME_ClearEditorVehicleEnvelopePreview();
		ME_ClearEditorVehicleCategoryLabel();
		super._WB_OnDelete(owner, src);
		ME_RefreshAllEditorDebugShapes();
	}
}