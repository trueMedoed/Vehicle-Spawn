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

modded class SCR_AmbientVehicleSpawnPointComponent
{
	override protected void Update(SCR_Faction faction)
	{
		super.Update(faction);
		
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
		
		if (data.IsEmpty())
		{
			foreach (EEditableEntityLabel includedLabel: m_aIncludedEditableEntityLabels)
			{
				Print("includedLabel = " + includedLabel);
				Print( typename.EnumToString(EEditableEntityLabel, includedLabel) );
				
			}
			
			foreach (EEditableEntityLabel excludedLabel: m_aExcludedEditableEntityLabels)
			{
				Print("excludedLabel = " + excludedLabel);
				Print( typename.EnumToString(EEditableEntityLabel, excludedLabel) );
			}
			//Print("[ME_DEBUG] SCR_AmbientVehicleSpawnPointComponent::data.IsEmpty() = " + data.IsEmpty());
			Print("SCR_AmbientVehicleSpawnPointComponent: conflict between IncludedEditableEntityLabels and ExcludedEditableEntityLabels. Vehicle will not spawn.", LogLevel.ERROR);
			
			return;
		}

		m_sPrefab = (data.GetRandomElement().GetPrefab());
	}

}