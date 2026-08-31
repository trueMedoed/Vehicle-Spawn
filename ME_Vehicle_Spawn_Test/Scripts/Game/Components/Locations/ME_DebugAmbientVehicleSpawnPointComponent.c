/*
modded class SCR_AmbientVehicleSpawnPointComponent
{
	ref Shape m_ME_EditorSpawnAreaShape;

	override void OnPostInit(IEntity owner)
	{
		PrintFormat("[ME_DEBUG_AVSP] OnPostInit BEFORE super entity=%1 class=%2 coords=%3", owner.GetName(), owner.Type().ToString(), owner.GetOrigin());

		super.OnPostInit(owner);

		PrintFormat("[ME_DEBUG_AVSP] OnPostInit AFTER super entity=%1 class=%2 coords=%3", owner.GetName(), owner.Type().ToString(), owner.GetOrigin());

		SCR_AmbientVehicleSystem ambientVehicleSystem = SCR_AmbientVehicleSystem.GetInstance();
		if (!ambientVehicleSystem)
		{
			Print("[ME_DEBUG_AVSP] Ambient vehicle system unavailable");
			return;
		}

		array<SCR_AmbientVehicleSpawnPointComponent> spawnpoints = {};
		int spawnpointCount = ambientVehicleSystem.GetSpawnpoints(spawnpoints);
		PrintFormat("[ME_DEBUG_AVSP] Ambient vehicle system available spawnpoints=%1 listSize=%2 editModeEnabled=%3", spawnpointCount, spawnpoints.Count(), ambientVehicleSystem.ShouldBeEnabledInEditMode());
	}

	protected string ME_LabelsToString(array<EEditableEntityLabel> labels)
	{
		if (!labels || labels.IsEmpty())
			return "[]";

		string result = "[";
		for (int i = 0; i < labels.Count(); i++)
		{
			if (i > 0)
				result += ",";

			result += labels[i].ToString();
		}

		return result + "]";
	}

	protected SCR_Faction ME_GetSpawnpointFaction()
	{
		SCR_FactionAffiliationComponent affiliation = SCR_FactionAffiliationComponent.Cast(GetOwner().FindComponent(SCR_FactionAffiliationComponent));
		if (!affiliation)
			return null;

		SCR_Faction faction = SCR_Faction.Cast(affiliation.GetAffiliatedFaction());
		if (!faction)
			faction = SCR_Faction.Cast(affiliation.GetDefaultAffiliatedFaction());

		return faction;
	}

	protected int ME_LogLabelFilterForFaction(SCR_Faction faction)
	{
		SCR_EntityCatalog entityCatalog;
		string factionKey = "<none>";
		if (faction)
		{
			factionKey = faction.GetFactionKey();
			entityCatalog = faction.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE);
		}
		else
		{
			SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
			if (!catalogManager)
			{
				PrintFormat("[ME_DEBUG_AVSP_LABEL] entity=%1 coords=%2 faction=%3 catalog=unavailable include=%4 exclude=%5 requireAll=%6", GetOwner().GetName(), GetOwner().GetOrigin(), factionKey, ME_LabelsToString(m_aIncludedEditableEntityLabels), ME_LabelsToString(m_aExcludedEditableEntityLabels), m_bRequireAllIncludedLabels);
				return -1;
			}

			entityCatalog = catalogManager.GetEntityCatalogOfType(EEntityCatalogType.VEHICLE);
		}

		if (!entityCatalog)
		{
			PrintFormat("[ME_DEBUG_AVSP_LABEL] entity=%1 coords=%2 faction=%3 catalog=missing include=%4 exclude=%5 requireAll=%6", GetOwner().GetName(), GetOwner().GetOrigin(), factionKey, ME_LabelsToString(m_aIncludedEditableEntityLabels), ME_LabelsToString(m_aExcludedEditableEntityLabels), m_bRequireAllIncludedLabels);
			return -1;
		}

		array<SCR_EntityCatalogEntry> includeCandidates = {};
		array<SCR_EntityCatalogEntry> finalCandidates = {};
		entityCatalog.GetFullFilteredEntityListWithLabels(includeCandidates, m_aIncludedEditableEntityLabels, null, m_bRequireAllIncludedLabels);
		entityCatalog.GetFullFilteredEntityListWithLabels(finalCandidates, m_aIncludedEditableEntityLabels, m_aExcludedEditableEntityLabels, m_bRequireAllIncludedLabels);
		PrintFormat("[ME_DEBUG_AVSP_LABEL] entity=%1 coords=%2 faction=%3 include=%4 exclude=%5 requireAll=%6 includeCandidates=%7 finalCandidates=%8", GetOwner().GetName(), GetOwner().GetOrigin(), factionKey, ME_LabelsToString(m_aIncludedEditableEntityLabels), ME_LabelsToString(m_aExcludedEditableEntityLabels), m_bRequireAllIncludedLabels, includeCandidates.Count(), finalCandidates.Count());

		foreach (SCR_EntityCatalogEntry entry : finalCandidates)
		{
			array<EEditableEntityLabel> labels = {};
			entry.GetEditableEntityLabels(labels);
			PrintFormat("[ME_DEBUG_AVSP_LABEL] candidate prefab=%1 index=%2 name=%3 labels=%4 hasAPC=%5 hasCAR=%6", entry.GetPrefab(), entry.GetCatalogIndex(), entry.GetEntityName(), ME_LabelsToString(labels), entry.HasEditableEntityLabel(EEditableEntityLabel.VEHICLE_APC), entry.HasEditableEntityLabel(EEditableEntityLabel.VEHICLE_CAR));
		}

		return finalCandidates.Count();
	}

	int ME_LogLabelFilter()
	{
		return ME_LogLabelFilterForFaction(ME_GetSpawnpointFaction());
	}

	override protected void Update(SCR_Faction faction)
	{
		//ME_LogLabelFilterForFaction(faction);
		super.Update(faction);
		PrintFormat("[ME_DEBUG_AVSP_LABEL] Update selectedPrefab=%1 entity=%2 coords=%3", m_sPrefab, GetOwner().GetName(), GetOwner().GetOrigin());
	}

	void ME_ClearEditorDebugShape()
	{
		m_ME_EditorSpawnAreaShape = null;
	}

	void ME_RefreshEditorDebugShape(vector position, BaseWorld world)
	{
		ME_ClearEditorDebugShape();

		vector candidate;
		bool found = SCR_WorldTools.FindEmptyTerrainPosition(candidate, position, 5, 5, 2, TraceFlags.ENTS|TraceFlags.OCEAN, world);
		int color;
		string status;
		if (found)
		{
			color = Color.GREEN;
			status = "GREEN";
		}
		else
		{
			color = Color.RED;
			status = "RED";
		}

		Color colorValue = Color.FromInt(color);
		colorValue.SetA(0.375);
		ShapeFlags flags = ShapeFlags.TRANSP | ShapeFlags.DOUBLESIDE | ShapeFlags.NOOUTLINE;
		m_ME_EditorSpawnAreaShape = Shape.CreateSphere(colorValue.PackToInt(), flags, position, 5);

		PrintFormat("[ME_DEBUG_AVSP_EDITOR] entity=%1 position=%2 found=%3 candidate=%4 status=%5", GetOwner().GetName(), position, found, candidate, status);
	}

	override void _WB_OnInit(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		super._WB_OnInit(owner, mat, src);
		ME_RefreshEditorDebugShape(mat[3], owner.GetWorld());
	}

	override void _WB_SetTransform(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		super._WB_SetTransform(owner, mat, src);
		ME_RefreshEditorDebugShape(mat[3], owner.GetWorld());
	}

	override void OnDelete(IEntity owner)
	{
		ME_ClearEditorDebugShape();
		super.OnDelete(owner);
	}

	override void _WB_OnDelete(IEntity owner, IEntitySource src)
	{
		ME_ClearEditorDebugShape();
		super._WB_OnDelete(owner, src);
	}
}
*/


/*
modded class SCR_AmbientVehicleSpawnPointComponent
{
	override protected void Update(SCR_Faction faction)
	{
		Print("[ME_DEBUG] SCR_AmbientVehicleSpawnPointComponent::Update");
		
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
		
		super.Update(faction);
		
		//PrintFormat("[ME_DEBUG_AVSP_LABEL] Update selectedPrefab=%1 entity=%2 coords=%3", m_sPrefab, GetOwner().GetName(), GetOwner().GetOrigin());
	}

}
*/

//! Diagnostics for ambient vehicle spawn points that are configured so that no vehicle can ever
//! be selected. The vanilla component returns silently when the label filter yields no candidates,
//! which leaves an empty spawn point and no trace in the log. This mod repeats the vanilla filter
//! after the base call and reports the empty result as an error, separating a self-contradictory
//! label setup from a filter that simply found nothing in the catalog.
//! Диагностика точек появления техники, настроенных так, что выбор техники невозможен.
//! Встроенный компонент молча возвращает пустой результат фильтра меток; этот мод повторяет
//! фильтрацию после базового вызова и сообщает об ошибке, различая противоречивую настройку меток
//! и обычное отсутствие подходящих записей в каталоге.

modded class SCR_AmbientVehicleSpawnPointComponent
{
	ref Shape m_ME_EditorSpawnAreaShape;
	static ref array<SCR_AmbientVehicleSpawnPointComponent> s_ME_EditorSpawnPoints = {};
	static ref array<IEntity> s_ME_EditorStaticObjectMarkerEntities = {};
	static ref array<ref Shape> s_ME_EditorStaticObjectMarkerShapes = {};
	protected int m_iME_EditorStaticObjectConflictCount;

	//------------------------------------------------------------------------------------------------
	//! Formats editable entity labels as a readable comma-separated list for log output.
//! Форматирует метки редактируемых сущностей в удобный для журнала список через запятую.
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

		//Print("[ME_DEBUG] SCR_AmbientVehicleSpawnPointComponent::Update");

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
				//string errorMessage = string.Format("[ME_DEBUG_AVSP_ERROR] SCR_AmbientVehicleSpawnPointComponent: conflicting labels [%1] are present in both IncludedEditableEntityLabels and ExcludedEditableEntityLabels. Vehicle will not spawn. Entity=%2, coordinates=%3", ME_EditableEntityLabelsToString(conflictingLabels), pointName);
				//Print(errorMessage, LogLevel.ERROR);
				
				
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
				/*string errorMessage = string.Format("[ME_DEBUG_AVSP_ERROR] SCR_AmbientVehicleSpawnPointComponent: no vehicle matches the configured entity labels. Vehicle will not spawn. Included=[%1], Excluded=[%2], Entity=%3, coordinates=%4", ME_EditableEntityLabelsToString(m_aIncludedEditableEntityLabels), ME_EditableEntityLabelsToString(m_aExcludedEditableEntityLabels), pointName, pointPosition);
				Print(errorMessage, LogLevel.ERROR);*/
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
	//! Adds this spawn point to the editor-only registry once.
	void ME_RegisterEditorDebugSpawnPoint()
	{
		if (!s_ME_EditorSpawnPoints.Contains(this))
			s_ME_EditorSpawnPoints.Insert(this);
	}

	//------------------------------------------------------------------------------------------------
	//! Removes this spawn point from the editor-only registry.
	//! Удаляет эту точку появления из реестра, используемого только редактором.
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
	//! Освобождает все общие маркеры редактора для конфликтов со статической физикой и границами.
	static void ME_ClearEditorStaticObjectMarkers()
	{
		s_ME_EditorStaticObjectMarkerShapes.Clear();
		s_ME_EditorStaticObjectMarkerEntities.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! Tests whether an entity's world-space AABB intersects an editor spawn-area sphere.
	//! Проверяет, пересекается ли мировая AABB сущности со сферой области появления в редакторе.
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
		PrintFormat("[ME_DEBUG_AVSP_POS] entity=%1 origin=%2 staticCandidate=%3 candidateType=%4 candidateOrigin=%5 boundsMin=%6 boundsMax=%7 editorWarning=static_object_conflict", owner.GetName(), origin, entity.GetName(), entity.Type().ToString(), entity.GetOrigin(), mins, maxs);
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
	//! Перестраивает все зарегистрированные формы редактора, чтобы точки отражали текущие пересечения и статические объекты.
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

		string reason = "none";
		if (!found)
			reason = "no_empty_position";
		else if (overlap)
			reason = "overlapping_spawn_area";

		string editorWarning = "none";
		if (m_iME_EditorStaticObjectConflictCount > 0)
			editorWarning = "static_object_conflict";

		if (overlap)
		{
			IEntity overlappingOwner = overlappingPoint.GetOwner();
			if (found)
			{
				string editorWarningAndReason = string.Format("editorWarning=%1 reason=%2", editorWarning, reason);
				PrintFormat("[ME_DEBUG_AVSP_POS] entity=%1 origin=%2 searchRadius=%3 cylinderRadius=%4 cylinderHeight=2 traceFlags=ENTS|OCEAN found=1 candidate=%5 overlap=1 overlappingEntity=%6 overlappingOrigin=%7 staticConflictCount=%8 %9", owner.GetName(), origin, SPAWNING_RADIUS, SPAWNING_RADIUS, candidate, overlappingOwner.GetName(), overlappingOwner.GetOrigin(), m_iME_EditorStaticObjectConflictCount, editorWarningAndReason);
			}
			else
				PrintFormat("[ME_DEBUG_AVSP_POS] entity=%1 origin=%2 searchRadius=%3 cylinderRadius=%4 cylinderHeight=2 traceFlags=ENTS|OCEAN found=0 overlap=1 overlappingEntity=%5 overlappingOrigin=%6 staticConflictCount=%7 editorWarning=%8 reason=%9", owner.GetName(), origin, SPAWNING_RADIUS, SPAWNING_RADIUS, overlappingOwner.GetName(), overlappingOwner.GetOrigin(), m_iME_EditorStaticObjectConflictCount, editorWarning, reason);
		}
		else if (found)
			PrintFormat("[ME_DEBUG_AVSP_POS] entity=%1 origin=%2 searchRadius=%3 cylinderRadius=%4 cylinderHeight=2 traceFlags=ENTS|OCEAN found=1 candidate=%5 overlap=0 staticConflictCount=%6 editorWarning=%7 reason=%8", owner.GetName(), origin, SPAWNING_RADIUS, SPAWNING_RADIUS, candidate, m_iME_EditorStaticObjectConflictCount, editorWarning, reason);
		else
			PrintFormat("[ME_DEBUG_AVSP_POS] entity=%1 origin=%2 searchRadius=%3 cylinderRadius=%4 cylinderHeight=2 traceFlags=ENTS|OCEAN found=0 overlap=0 staticConflictCount=%5 editorWarning=%6 reason=%7", owner.GetName(), origin, SPAWNING_RADIUS, SPAWNING_RADIUS, m_iME_EditorStaticObjectConflictCount, editorWarning, reason);
	}

	//------------------------------------------------------------------------------------------------
	//! Registers and refreshes the editor probe when a spawn point is initialized.
	//! Регистрирует и обновляет проверочную область редактора при инициализации точки появления.
	//! \param[in] owner Spawn point entity
	//! \param[in,out] mat Spawn point transform matrix
	//! \param[in] src Spawn point entity source
	override void _WB_OnInit(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		super._WB_OnInit(owner, mat, src);
		ME_RegisterEditorDebugSpawnPoint();
		ME_RefreshAllEditorDebugShapes();
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
	}

	//------------------------------------------------------------------------------------------------
	//! Unregisters the point, clears its shape, and refreshes remaining points on deletion.
	//! \param[in] owner Spawn point entity
	override void OnDelete(IEntity owner)
	{
		ME_UnregisterEditorDebugSpawnPoint();
		ME_ClearEditorDebugShape();
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
		super._WB_OnDelete(owner, src);
		ME_RefreshAllEditorDebugShapes();
	}
}