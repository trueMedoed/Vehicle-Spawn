//! Diagnostics for ambient vehicle spawn points that are configured so that no vehicle can ever
//! be selected. The vanilla component returns silently when the label filter yields no candidates,
//! which leaves an empty spawn point and no trace in the log. This mod repeats the vanilla filter
//! after the base call and reports the empty result as an error, separating a self-contradictory
//! label setup from a filter that simply found nothing in the catalog.

modded class SCR_AmbientVehicleSpawnPointComponent
{
	ref array<ref DebugTextWorldSpace> m_aME_EditorFilterWarnings = {};
	// Vertical offset above category labels and spacing between diagnostic lines.
	static const float ME_EDITOR_EDITABLE_LABEL_CONFLICT_WARNING_OFFSET = 1.5;
	static const float ME_EDITOR_FILTER_WARNING_LINE_SPACING = 0.75;
	// Compact filter-status marker near a point without a matching vehicle.
	static const float ME_EDITOR_FILTER_STATUS_OFFSET = 2.5;
	static const float ME_EDITOR_FILTER_STATUS_FONT_SIZE = 1.0;
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
	protected ref array<string> m_aME_EditorStaticObjectConflictDescriptions = {};
	// Cached editor probe result for tinting the mesh preview without scanning every frame.
	protected bool m_bME_EditorHologramChecked;
	protected bool m_bME_EditorHologramPlacementError;

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
	//! Converts the complete filtered catalog result to unique prefab paths without changing spawn state.
	bool ME_GetEditorVehicleEnvelopeCandidatePaths(out array<string> prefabPaths, out string reason)
	{
		prefabPaths = {};
		array<SCR_EntityCatalogEntry> entries;
		if (!ME_GetEditorVehicleEnvelopeCandidates(entries, reason))
			return false;

		foreach (SCR_EntityCatalogEntry entry : entries)
		{
			string prefabPath = entry.GetPrefab();
			if (prefabPath.IsEmpty())
			{
				reason = "empty_catalog_prefab";
				prefabPaths = {};
				return false;
			}

			if (prefabPaths.Contains(prefabPath))
			{
				reason = string.Format("duplicate_catalog_prefab path=%1", prefabPath);
				prefabPaths = {};
				return false;
			}

			prefabPaths.Insert(prefabPath);
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
	//! Passenger-capacity traits share one comma-separated text when both are included.
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

		// Both passenger-capacity traits share the default colour and one text instead of overlapping labels.
		bool mergePassengerTraits = m_aIncludedEditableEntityLabels.Contains(EEditableEntityLabel.TRAIT_PASSENGERS_SMALL)
			&& m_aIncludedEditableEntityLabels.Contains(EEditableEntityLabel.TRAIT_PASSENGERS_LARGE);
		string passengerTraitText;
		if (mergePassengerTraits)
		{
			foreach (EEditableEntityLabel passengerLabel: m_aIncludedEditableEntityLabels)
			{
				if (passengerLabel != EEditableEntityLabel.TRAIT_PASSENGERS_SMALL && passengerLabel != EEditableEntityLabel.TRAIT_PASSENGERS_LARGE)
					continue;
				if (!passengerTraitText.IsEmpty())
					passengerTraitText += ", ";
				passengerTraitText += typename.EnumToString(EEditableEntityLabel, passengerLabel);
			}
		}

		array<string> labelTexts = {};
		array<int> labelColors = {};
		bool passengerTraitTextInserted = false;
		foreach (EEditableEntityLabel includedLabel: m_aIncludedEditableEntityLabels)
		{
			bool isPassengerTrait = includedLabel == EEditableEntityLabel.TRAIT_PASSENGERS_SMALL
				|| includedLabel == EEditableEntityLabel.TRAIT_PASSENGERS_LARGE;
			if (mergePassengerTraits && isPassengerTrait)
			{
				if (passengerTraitTextInserted)
					continue;
				passengerTraitTextInserted = true;
				labelTexts.Insert(passengerTraitText);
			}
			else
			{
				labelTexts.Insert(typename.EnumToString(EEditableEntityLabel, includedLabel));
			}
			labelColors.Insert(ME_GetEditorVehicleCategoryIncludedLabelColor(includedLabel).PackToInt());
		}

		int labelCount = labelTexts.Count();
		float offset = -0.5 * ME_EDITOR_VEHICLE_CATEGORY_LABEL_SPACING * (labelCount - 1);
		for (int labelIndex = 0; labelIndex < labelCount; labelIndex++)
		{
			vector labelTransform[4];
			for (int labelTransformIndex = 0; labelTransformIndex < 4; labelTransformIndex++)
				labelTransform[labelTransformIndex] = includedTransform[labelTransformIndex];
			labelTransform[3] = labelTransform[3] + transform[0] * offset;
			m_aME_EditorVehicleCategoryIncludedLabels.Insert(DebugTextWorldSpace.CreateInWorld(GetGame().GetWorld(), labelTexts[labelIndex], textFlags, labelTransform, ME_EDITOR_VEHICLE_CATEGORY_LABEL_FONT_SIZE, labelColors[labelIndex], backgroundColor, 1000));
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
	//! Tests whether an entity's oriented model bounds intersects an editor spawn-area sphere.
	//!
	//! This is a broad-phase editor warning and is not a guaranteed runtime spawn failure.
	protected bool ME_DoBoundsIntersectEditorSpawnArea(IEntity entity, vector mins, vector maxs, vector origin)
	{
		vector transform[4];
		entity.GetTransform(transform);
		vector offset = origin - transform[3];
		vector closestPoint = transform[3];
		// Project onto scaled world axes, then clamp in model coordinates.
		for (int i = 0; i < 3; i++)
		{
			vector axis = transform[i];
			float lengthSquared = vector.Dot(axis, axis);
			if (lengthSquared <= 0.000001)
				continue;
			float coordinate = vector.Dot(offset, axis) / lengthSquared;
			coordinate = Math.Clamp(coordinate, mins[i], maxs[i]);
			closestPoint += axis * coordinate;
		}
		vector delta = closestPoint - origin;
		return vector.Dot(delta, delta) <= SPAWNING_RADIUS * SPAWNING_RADIUS;
	}

	//------------------------------------------------------------------------------------------------
	//! Creates one shared marker for an object, even when several spawn areas intersect its bounds.
	//!
	//! The wireframe displays the same oriented local model bounds used by the intersection check.
	static void ME_AddEditorStaticObjectMarker(IEntity entity, vector mins, vector maxs)
	{
		if (s_ME_EditorStaticObjectMarkerEntities.Contains(entity))
			return;

		// Transform the same local bounds used by the intersection test into world space.
		ShapeFlags flags = ShapeFlags.WIREFRAME | ShapeFlags.NOZWRITE;
		s_ME_EditorStaticObjectMarkerEntities.Insert(entity);
		Shape marker = Shape.Create(ShapeType.BBOX, Color.RED, flags, mins, maxs);
		vector transform[4];
		entity.GetTransform(transform);
		marker.SetMatrix(transform);
		s_ME_EditorStaticObjectMarkerShapes.Insert(marker);
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
		entity.GetBounds(mins, maxs);
		vector origin = owner.GetOrigin();
		if (!ME_DoBoundsIntersectEditorSpawnArea(entity, mins, maxs, origin))
			return true;

		m_iME_EditorStaticObjectConflictCount++;
		string objectName = entity.GetName();
		if (objectName.IsEmpty())
			objectName = entity.Type().ToString();
		m_aME_EditorStaticObjectConflictDescriptions.Insert(string.Format("object=%1 coordinates=%2", objectName, entity.GetOrigin()));
		ME_AddEditorStaticObjectMarker(entity, mins, maxs);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Scans this point's area for static physics/bounds editor warnings.
	protected void ME_RefreshEditorStaticObjectConflicts(IEntity owner)
	{
		m_iME_EditorStaticObjectConflictCount = 0;
		m_aME_EditorStaticObjectConflictDescriptions.Clear();
		BaseWorld world = owner.GetWorld();
		if (world)
			world.QueryEntitiesBySphere(owner.GetOrigin(), SPAWNING_RADIUS, ME_CollectEditorStaticObjectConflict);
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuilds all registered editor diagnostics so every point reflects current overlaps and static objects.
	void ME_RefreshAllEditorDiagnostics()
	{
		ME_ClearEditorStaticObjectMarkers();
		foreach (SCR_AmbientVehicleSpawnPointComponent spawnPoint: s_ME_EditorSpawnPoints)
		{
			if (!spawnPoint)
				continue;

			IEntity owner = spawnPoint.GetOwner();
			if (owner)
				spawnPoint.ME_RefreshEditorDiagnostics(owner);
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

	//------------------------------------------------------------------------------------------------
	//! Probes the vanilla empty-terrain search and refreshes the point's preview tint and placement warnings.
	//!
	//! Static physics/bounds markers are an editor advisory and do not alter the vanilla probe result.
	//! \param[in] owner Spawn point entity whose origin and world are tested
	void ME_RefreshEditorDiagnostics(IEntity owner)
	{
		vector origin = owner.GetOrigin();
		BaseWorld world = owner.GetWorld();
		vector candidate;
		bool found = SCR_WorldTools.FindEmptyTerrainPosition(candidate, origin, SPAWNING_RADIUS, SPAWNING_RADIUS, 2, TraceFlags.ENTS | TraceFlags.OCEAN, world);
		SCR_AmbientVehicleSpawnPointComponent overlappingPoint;
		ME_FindOverlappingEditorSpawnPoint(origin, world, overlappingPoint);
		ME_RefreshEditorStaticObjectConflicts(owner);
		ME_RefreshEditorEditableLabelConflictWarning(owner, overlappingPoint, found);
		// Area overlaps and static bounds are advisory; only failed clearance makes the preview red.
		m_bME_EditorHologramPlacementError = !found;
		m_bME_EditorHologramChecked = true;
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
		ME_RefreshAllEditorDiagnostics();
		ME_UpdateEditorVehicleCategoryLabel();
	}

	//------------------------------------------------------------------------------------------------
	//! Refreshes every editor probe after a spawn point is moved.
	//! \param[in] owner Spawn point entity
	//! \param[in,out] mat Spawn point transform matrix
	//! \param[in] src Spawn point entity source
	override void _WB_SetTransform(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		super._WB_SetTransform(owner, mat, src);
		ME_RefreshAllEditorDiagnostics();
		ME_RefreshEditorVehicleCategoryLabel(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Unregisters the point, clears its labels, and refreshes remaining points on deletion.
	//! \param[in] owner Spawn point entity
	override void OnDelete(IEntity owner)
	{
		ME_UnregisterEditorDebugSpawnPoint();
		ME_ClearEditorVehicleCategoryLabel();
		ME_ClearEditorEditableLabelConflictWarning();
		super.OnDelete(owner);
		ME_RefreshAllEditorDiagnostics();
	}

	//------------------------------------------------------------------------------------------------
	//! Unregisters the point before Workbench removes it and refreshes remaining points.
	//! \param[in] owner Spawn point entity
	//! \param[in] src Spawn point entity source
	override void _WB_OnDelete(IEntity owner, IEntitySource src)
	{
		ME_UnregisterEditorDebugSpawnPoint();
		ME_ClearEditorVehicleCategoryLabel();
		ME_ClearEditorEditableLabelConflictWarning();
		super._WB_OnDelete(owner, src);
		ME_RefreshAllEditorDiagnostics();
	}

	//------------------------------------------------------------------------------------------------
	//! Releases all editor diagnostic text objects.
	void ME_ClearEditorEditableLabelConflictWarning()
	{
		for (int warningIndex = 0; warningIndex < m_aME_EditorFilterWarnings.Count(); warningIndex++)
			m_aME_EditorFilterWarnings[warningIndex] = null;

		m_aME_EditorFilterWarnings.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! Shows separate filter and intersection messages; returns true for catalog or filter problems.
	bool ME_RefreshEditorEditableLabelConflictWarning(IEntity owner, SCR_AmbientVehicleSpawnPointComponent overlappingPoint, bool clearanceFound)
	{
		ME_ClearEditorEditableLabelConflictWarning();
		if (!owner)
			return false;

		array<EEditableEntityLabel> conflictingLabels = ME_GetConflictingEditableEntityLabels();
		array<SCR_EntityCatalogEntry> entries;
		string catalogReason;
		bool catalogAvailable = ME_GetEditorVehicleEnvelopeCandidates(entries, catalogReason);
		bool noCandidates = catalogAvailable && entries.IsEmpty();
		bool filterWarning = !conflictingLabels.IsEmpty() || !catalogAvailable || noCandidates;
		if (!filterWarning)
		{
			if (clearanceFound && !overlappingPoint && m_iME_EditorStaticObjectConflictCount == 0)
				return false;
		}

		vector transform[4];
		owner.GetTransform(transform);
		transform[3] = transform[3] + Vector(0, 8 + ME_EDITOR_EDITABLE_LABEL_CONFLICT_WARNING_OFFSET, 0);

		string warningText;
		if (!conflictingLabels.IsEmpty() && m_bRequireAllIncludedLabels)
		{
			warningText = string.Format(
				"ERROR: conflicting labels [%1] are both included and excluded. requireAll=true: these requirements are impossible; no vehicle can match this filter.",
				ME_EditableEntityLabelsToString(conflictingLabels)
			);
		}
		else if (!conflictingLabels.IsEmpty())
		{
			warningText = string.Format(
				"ERROR: conflicting labels [%1] are both included and excluded. requireAll=false: these labels are excluded, but other included labels may still leave candidates.",
				ME_EditableEntityLabelsToString(conflictingLabels)
			);
		}

		array<string> warningLines = {};
		if (!warningText.IsEmpty())
			warningLines.Insert(warningText);
		if (noCandidates)
		{
			warningLines.Insert("ERROR: no vehicle candidates match this point's catalog and label filter.");
			warningLines.Insert(string.Format("include=[%1] exclude=[%2]",
				ME_EditableEntityLabelsToString(m_aIncludedEditableEntityLabels),
				ME_EditableEntityLabelsToString(m_aExcludedEditableEntityLabels)));
		}
		else if (!catalogAvailable)
			warningLines.Insert(string.Format("WARNING: vehicle candidates could not be checked (%1).", catalogReason));

		// Geometry messages do not change the filter status or preview colour.
		if (overlappingPoint)
		{
			IEntity overlappingOwner = overlappingPoint.GetOwner();
			if (overlappingOwner)
			{
				warningLines.Insert("WARNING: spawn areas overlap; recheck simultaneous placement.");
				warningLines.Insert(string.Format("overlapping point=%1 coordinates=%2", overlappingOwner.GetName(), overlappingOwner.GetOrigin()));
			}
		}

		if (!clearanceFound)
			warningLines.Insert("ERROR: no empty terrain position found; recheck placement.");

		if (m_iME_EditorStaticObjectConflictCount > 0)
		{
			warningLines.Insert("WARNING: spawn area intersects static object bounds; recheck placement.");
			foreach (string objectDescription : m_aME_EditorStaticObjectConflictDescriptions)
				warningLines.Insert(objectDescription);
		}

		const DebugTextFlags textFlags = DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA;
		const int backgroundColor = Color.FromRGBA(0, 0, 0, 178).PackToInt();
		// Keep filter failures visible at the point even when no matching vehicle preview exists.
		if (filterWarning)
		{
			string statusText;
			int statusColor = Color.FromRGBA(255, 48, 48, 255).PackToInt();
			if (noCandidates)
				statusText = "ERROR: NO VEHICLE CANDIDATES";
			else if (!conflictingLabels.IsEmpty())
				statusText = "ERROR: LABEL CONFLICT";
			else
			{
				statusText = "WARNING: CATALOG UNAVAILABLE";
				statusColor = Color.FromRGBA(255, 190, 0, 255).PackToInt();
			}

			vector statusTransform[4];
			owner.GetTransform(statusTransform);
			statusTransform[3] = statusTransform[3] + Vector(0, ME_EDITOR_FILTER_STATUS_OFFSET, 0);
			m_aME_EditorFilterWarnings.Insert(DebugTextWorldSpace.CreateInWorld(
				GetGame().GetWorld(), statusText, textFlags, statusTransform,
				ME_EDITOR_FILTER_STATUS_FONT_SIZE, statusColor, backgroundColor, 1000));
		}
		// Stack separate messages above the labels, with the first diagnostic at the top.
		vector warningOrigin = transform[3];
		int warningColor = Color.FromRGBA(255, 48, 48, 255).PackToInt();
		if (conflictingLabels.IsEmpty() && !noCandidates && clearanceFound)
			warningColor = Color.FromRGBA(255, 190, 0, 255).PackToInt();
		for (int warningIndex = 0; warningIndex < warningLines.Count(); warningIndex++)
		{
			transform[3] = warningOrigin + Vector(0, (warningLines.Count() - 1 - warningIndex) * ME_EDITOR_FILTER_WARNING_LINE_SPACING, 0);
			m_aME_EditorFilterWarnings.Insert(DebugTextWorldSpace.CreateInWorld(
				GetGame().GetWorld(),
				warningLines[warningIndex],
				textFlags,
				transform,
				ME_EDITOR_VEHICLE_CATEGORY_LABEL_FONT_SIZE,
				warningColor,
				backgroundColor,
				1000
			));
		}
		return filterWarning;
	}

}
