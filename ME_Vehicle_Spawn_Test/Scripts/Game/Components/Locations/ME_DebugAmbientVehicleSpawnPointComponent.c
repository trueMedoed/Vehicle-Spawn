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

//------------------------------------------------------------------------------------------------
//! Diagnostics for ambient vehicle spawn points that are configured so that no vehicle can ever
//! be selected. The vanilla component returns silently when the label filter yields no candidates,
//! which leaves an empty spawn point and no trace in the log. This mod repeats the vanilla filter
//! after the base call and reports the empty result as an error, separating a self-contradictory
//! label setup from a filter that simply found nothing in the catalog.

modded class SCR_AmbientVehicleSpawnPointComponent
{
	ref Shape m_ME_EditorSpawnAreaShape;
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
	//! Removes the previous editor-only shape for this spawn point.
	void ME_ClearEditorDebugShape()
	{
		m_ME_EditorSpawnAreaShape = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Probes the vanilla empty-terrain search area and displays its result in the editor.
	//! \param[in] owner Spawn point entity whose origin and world are tested
	void ME_RefreshEditorDebugShape(IEntity owner)
	{
		ME_ClearEditorDebugShape();

		vector origin = owner.GetOrigin();
		BaseWorld world = owner.GetWorld();
		vector candidate;
		bool found = SCR_WorldTools.FindEmptyTerrainPosition(candidate, origin, SPAWNING_RADIUS, SPAWNING_RADIUS, 2, TraceFlags.ENTS | TraceFlags.OCEAN, world);

		int color = Color.RED;
		if (found)
			color = Color.GREEN;

		Color colorValue = Color.FromInt(color);
		colorValue.SetA(0.375);
		ShapeFlags flags = ShapeFlags.TRANSP | ShapeFlags.DOUBLESIDE | ShapeFlags.NOOUTLINE;
		m_ME_EditorSpawnAreaShape = Shape.CreateSphere(colorValue.PackToInt(), flags, origin, SPAWNING_RADIUS);

		if (found)
			PrintFormat("[ME_DEBUG_AVSP_POS] entity=%1 origin=%2 searchRadius=%3 cylinderRadius=%4 cylinderHeight=2 traceFlags=ENTS|OCEAN found=1 candidate=%5", owner.GetName(), origin, SPAWNING_RADIUS, SPAWNING_RADIUS, candidate);
		else
			PrintFormat("[ME_DEBUG_AVSP_POS] entity=%1 origin=%2 searchRadius=%3 cylinderRadius=%4 cylinderHeight=2 traceFlags=ENTS|OCEAN found=0", owner.GetName(), origin, SPAWNING_RADIUS, SPAWNING_RADIUS);
	}

	//------------------------------------------------------------------------------------------------
	//! Refreshes the editor probe when a spawn point is initialized.
	//! \param[in] owner Spawn point entity
	//! \param[in,out] mat Spawn point transform matrix
	//! \param[in] src Spawn point entity source
	override void _WB_OnInit(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		super._WB_OnInit(owner, mat, src);
		ME_RefreshEditorDebugShape(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Refreshes the editor probe after a spawn point is moved.
	//! \param[in] owner Spawn point entity
	//! \param[in,out] mat Spawn point transform matrix
	//! \param[in] src Spawn point entity source
	override void _WB_SetTransform(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		super._WB_SetTransform(owner, mat, src);
		ME_RefreshEditorDebugShape(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Releases the editor probe shape when the spawn point is deleted.
	//! \param[in] owner Spawn point entity
	override void OnDelete(IEntity owner)
	{
		ME_ClearEditorDebugShape();
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Releases the editor probe shape before the spawn point is removed from Workbench.
	//! \param[in] owner Spawn point entity
	//! \param[in] src Spawn point entity source
	override void _WB_OnDelete(IEntity owner, IEntitySource src)
	{
		ME_ClearEditorDebugShape();
		super._WB_OnDelete(owner, src);
	}
}