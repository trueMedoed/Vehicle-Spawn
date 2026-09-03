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

//! Diagnostics for ambient vehicle spawn points whose label configuration cannot select a vehicle.
//! The override preserves vanilla spawning: super.Update(faction) selects the prefab, then this code
//! repeats the catalog filter only to diagnose an empty result during the spawn-time callback.
//! Editor-only registry, overlap, static-object, and Shape checks are advisory visual diagnostics;
//! they neither replace nor guarantee the result of runtime spawning.
//! Диагностика точек появления ambient-техники, конфигурация меток которых не может выбрать технику.
//! Override сохраняет ванильное появление: super.Update(faction) выбирает префаб, после чего этот код
//! повторяет фильтрацию каталога только для диагностики пустого результата при callback во время появления.
//! Реестр редактора, проверки пересечений и статических объектов, а также Shape-визуализация —
//! рекомендательные визуальные диагностики; они не заменяют и не гарантируют результат runtime-появления.

modded class SCR_AmbientVehicleSpawnPointComponent
{
	ref Shape m_ME_EditorSpawnAreaShape;
	ref Shape m_ME_EditorVehicleEnvelopeShape;
	ref Shape m_ME_EditorVehicleEnvelopeFillShape;
	static ref array<SCR_AmbientVehicleSpawnPointComponent> s_ME_EditorSpawnPoints = {};
	static ref array<IEntity> s_ME_EditorStaticObjectMarkerEntities = {};
	static ref array<ref Shape> s_ME_EditorStaticObjectMarkerShapes = {};
	static SCR_AmbientVehicleSpawnPointComponent s_ME_ActiveEditorVehicleEnvelopePreview;
	protected vector m_vME_EditorVehicleEnvelopeLocalMins;
	protected vector m_vME_EditorVehicleEnvelopeLocalMaxs;
	protected bool m_bME_EditorVehicleEnvelopePreviewActive;
	protected int m_iME_EditorStaticObjectConflictCount;

	//------------------------------------------------------------------------------------------------
	//! Suppresses repeated runtime faction-manager mismatch messages for this spawn point until its required key resolves again.
	//! Подавляет повторяющиеся runtime-сообщения о несовпадении с FactionManager для этой точки, пока её требуемый ключ снова не разрешится.
	protected bool m_bME_FactionManagerMismatchLogged;

	//------------------------------------------------------------------------------------------------
	//! Formats the factions currently available from a FactionManager for diagnostic output.
	//!
	//! \param[in] factionManager Active faction manager to inspect
	//! \return Comma-separated faction keys, or "<none>" when the manager has no factions
	//! Форматирует фракции, доступные в текущем FactionManager, для диагностического вывода.
	//!
	//! \param[in] factionManager Активный менеджер фракций для проверки
	//! \return Ключи фракций через запятую либо "<none>", если в менеджере нет фракций
	protected string ME_GetFactionManagerKeys(FactionManager factionManager)
	{
		array<Faction> factions = {};
		factionManager.GetFactionsList(factions);

		string result;
		foreach (Faction managerFaction: factions)
		{
			if (!managerFaction)
				continue;

			if (!result.IsEmpty())
				result += ", ";

			result += managerFaction.GetFactionKey();
		}

		if (result.IsEmpty())
			return "<none>";

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Logs a faction key that the active FactionManager cannot resolve for this spawn point once per unresolved period.
	//! This is diagnostic-only and does not alter vanilla faction, catalog, prefab, or spawning decisions.
	//!
	//! \param[in] faction Vanilla faction passed to Update
	//! Диагностически записывает ключ фракции этой точки, который активный FactionManager не может разрешить, один раз за период отсутствия.
	//! Это только диагностика и она не меняет ванильные решения о фракции, каталоге, префабе или появлении.
	//!
	//! \param[in] faction Ванильная фракция, переданная в Update
	protected void ME_LogFactionManagerMismatch(SCR_Faction faction)
	{
		IEntity owner = GetOwner();
		SCR_FactionAffiliationComponent affiliation = SCR_FactionAffiliationComponent.Cast(owner.FindComponent(SCR_FactionAffiliationComponent));
		if (!affiliation)
		{
			m_bME_FactionManagerMismatchLogged = false;
			return;
		}

		FactionKey defaultKey = affiliation.GetDefaultFactionKey();
		FactionKey currentKey = affiliation.GetAffiliatedFactionKey();
		FactionKey requiredKey = defaultKey;
		if (requiredKey.IsEmpty())
			requiredKey = currentKey;

		if (requiredKey.IsEmpty())
		{
			m_bME_FactionManagerMismatchLogged = false;
			return;
		}

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return;

		if (factionManager.GetFactionByKey(requiredKey))
		{
			m_bME_FactionManagerMismatchLogged = false;
			return;
		}

		if (m_bME_FactionManagerMismatchLogged)
			return;

		string passedKey = "<none>";
		if (faction)
			passedKey = faction.GetFactionKey();

		PrintFormat("[ME_DEBUG_AVSP_FACTION] entity=%1 coords=%2 requiredKey=%3 defaultKey=%4 currentKey=%5 passedKey=%6 availableKeys=%7", owner.GetName(), owner.GetOrigin(), requiredKey, defaultKey, currentKey, passedKey, ME_GetFactionManagerKeys(factionManager));
		m_bME_FactionManagerMismatchLogged = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Formats editable entity labels as a readable comma-separated list for diagnostic output.
	//!
	//! \param[in] labels Labels to format; may be null or empty
	//! \return Enum names joined by ", ", or "<none>" when no labels are present
	//! Форматирует метки редактируемых сущностей в читаемый список через запятую для диагностики.
	//!
	//! \param[in] labels Метки для форматирования; могут быть null или пустыми
	//! \return Имена enum через ", " либо "<none>", когда меток нет
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
	//! Collects labels requested in IncludedEditableEntityLabels that are also rejected by
	//! ExcludedEditableEntityLabels. An overlap is a configuration contradiction; it proves an empty
	//! filter result only when all included labels are required.
	//!
	//! \return Labels present in both lists, or an empty array when no overlap exists
	//! Собирает метки из IncludedEditableEntityLabels, одновременно отклоняемые
	//! ExcludedEditableEntityLabels. Пересечение является противоречием конфигурации; оно доказывает
	//! пустой результат фильтра только когда требуются все включённые метки.
	//!
	//! \return Метки из обоих списков либо пустой массив при отсутствии пересечений
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
	//! Collects the vehicle-catalog candidates that vanilla Update() would filter for this point in the World Editor.
	//! This diagnostic does not select a prefab, spawn an entity, or change the editable world.
	//!
	//! \param[out] entries Filtered vehicle catalog entries
	//! \param[out] reason Stable reason when candidates cannot be read safely
	//! \return True when entries contain the complete applicable catalog filter result
	//! Собирает кандидаты vehicle-каталога, которые ванильный Update() отфильтровал бы для этой точки в World Editor.
	//! Эта диагностика не выбирает префаб, не создаёт сущность и не изменяет редактируемый мир.
	//!
	//! \param[out] entries Отфильтрованные записи vehicle-каталога
	//! \param[out] reason Стабильная причина, когда кандидаты нельзя безопасно прочитать
	//! \return True, когда entries содержит полный результат применимого фильтра каталога
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
			SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
			if (!catalogManager)
			{
				reason = "global_catalog_manager_unavailable";
				return false;
			}

			entityCatalog = catalogManager.GetEntityCatalogOfType(EEntityCatalogType.VEHICLE);
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
	//! Converts this point's complete editor catalog result into canonical prefab paths without mutating spawn state.
	//! Преобразует полный результат editor-каталога этой точки в канонические пути prefab без изменения состояния появления.
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
	//! Lets super.Update(faction) select the vanilla vehicle prefab, then repeats the catalog filter
	//! solely to log an empty candidate set. SpawnVehicle() invokes this at spawn time when faction
	//! state changes or no faction has selected a prefab; this override does not select a prefab.
	//!
	//! \param[in] faction Faction whose vehicle catalog vanilla selection uses; null uses the global catalog
	//! Выполняет выбор ванильного префаба через super.Update(faction), затем повторяет фильтрацию каталога
	//! исключительно для журнала при пустом наборе кандидатов. SpawnVehicle() вызывает метод во время
	//! появления при изменении фракции либо когда для отсутствующей фракции ещё не выбран префаб.
	//!
	//! \param[in] faction Фракция, чей каталог использует ванильный выбор; null использует общий каталог
	override protected void Update(SCR_Faction faction)
	{
		ME_LogFactionManagerMismatch(faction);
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
	//! Adds this spawn point to the editor-only registry if it is not already registered.
	//! The registry exists only for advisory Shape, overlap, and static-object scans.
	//! Добавляет эту точку в реестр только для редактора, если она ещё не зарегистрирована.
	//! Реестр существует только для рекомендательных проверок Shape, пересечений и статических объектов.
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
	//! Tests whether an entity's world-space AABB intersects this point's editor spawn-area sphere.
	//! This broad-phase advisory test does not predict or replace the runtime free-space search.
	//!
	//! \param[in] mins Minimum corner of the entity's world-space AABB
	//! \param[in] maxs Maximum corner of the entity's world-space AABB
	//! \param[in] origin Center of this spawn point's editor spawn area
	//! \return True when the AABB touches or intersects the spawn-area sphere
	//! Проверяет пересечение мировой AABB сущности со сферой области появления этой точки в редакторе.
	//! Эта рекомендательная broad-phase-проверка не предсказывает и не заменяет runtime-поиск свободного места.
	//!
	//! \param[in] mins Минимальный угол мировой AABB сущности
	//! \param[in] maxs Максимальный угол мировой AABB сущности
	//! \param[in] origin Центр области появления этой точки в редакторе
	//! \return True, когда AABB касается или пересекает сферу области появления
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
	//! Creates one shared editor-only marker for a static object, even if several spawn areas intersect it.
	//! Marker size follows object bounds while remaining a visual advisory rather than another spawn-area probe.
	//!
	//! \param[in] entity Static entity represented by the marker
	//! \param[in] mins Minimum corner of its world-space AABB
	//! \param[in] maxs Maximum corner of its world-space AABB
	//! Создаёт один общий marker только для редактора для статического объекта, даже если его пересекают несколько областей появления.
	//! Размер marker следует границам объекта, оставаясь визуальной рекомендацией, а не второй проверкой области появления.
	//!
	//! \param[in] entity Статическая сущность, представленная marker
	//! \param[in] mins Минимальный угол её мировой AABB
	//! \param[in] maxs Максимальный угол её мировой AABB
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
	//! Evaluates a broad-phase query entity as a possible static physics/bounds conflict for editor display.
	//! The callback always continues scanning and does not affect runtime spawning.
	//!
	//! \param[in] entity Queried entity to evaluate
	//! \return True to continue querying subsequent entities
	//! Оценивает сущность из broad-phase-запроса как возможный конфликт статической физики/границ для отображения в редакторе.
	//! Callback всегда продолжает сканирование и не влияет на runtime-появление.
	//!
	//! \param[in] entity Проверяемая сущность из запроса
	//! \return True, чтобы продолжить запрос следующих сущностей
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
	//! Scans this point's editor spawn area for static physics/bounds advisory conflicts.
	//! The scan updates diagnostic markers and does not alter vanilla runtime spawning.
	//!
	//! \param[in] owner Spawn point entity whose world and origin are scanned
	//! Сканирует область появления этой точки в редакторе на рекомендательные конфликты статической физики/границ.
	//! Сканирование обновляет диагностические markers и не изменяет ванильное runtime-появление.
	//!
	//! \param[in] owner Сущность точки появления, чьи мир и позиция сканируются
	protected void ME_RefreshEditorStaticObjectConflicts(IEntity owner)
	{
		m_iME_EditorStaticObjectConflictCount = 0;
		BaseWorld world = owner.GetWorld();
		if (world)
			world.QueryEntitiesBySphere(owner.GetOrigin(), SPAWNING_RADIUS, ME_CollectEditorStaticObjectConflict);
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuilds every registered editor-only shape after clearing shared static-object markers.
	//! This visual refresh is advisory and does not invoke or replace runtime spawning.
	//! Перестраивает каждую зарегистрированную форму только для редактора после очистки общих marker статических объектов.
	//! Это визуальное обновление рекомендательное и не вызывает и не заменяет runtime-появление.
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
	//! Finds the first registered spawn point whose editor spawn area touches this point's area.
	//! This is an editor-only overlap advisory and is not a runtime spawn decision.
	//!
	//! \param[in] origin Origin to compare against registered points
	//! \param[in] world World containing the origin
	//! \param[out] overlappingPoint First touching or intersecting point, if one exists
	//! \return True when another point is within two spawning radii
	//! Находит первую зарегистрированную точку появления, чья область в редакторе касается области этой точки.
	//! Это рекомендательная проверка пересечения только для редактора, а не решение runtime-появления.
	//!
	//! \param[in] origin Позиция для сравнения с зарегистрированными точками
	//! \param[in] world Мир, содержащий эту позицию
	//! \param[out] overlappingPoint Первая касающаяся или пересекающаяся точка, если она найдена
	//! \return True, когда другая точка находится в пределах двух радиусов появления
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
	//! Releases this point's editor-only spawn-area Shape.
	//! Освобождает Shape области появления этой точки, используемую только редактором.
	void ME_ClearEditorDebugShape()
	{
		m_ME_EditorSpawnAreaShape = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Probes the vanilla empty-terrain search and refreshes advisory editor Shapes for area overlaps and static objects.
	//! These visual checks do not alter or guarantee the result of the vanilla runtime spawning probe.
	//!
	//! \param[in] owner Spawn point entity whose origin and world are inspected
	//! Проверяет ванильный поиск свободного места и обновляет рекомендательные Shape редактора для пересечений областей и статических объектов.
	//! Эти визуальные проверки не изменяют и не гарантируют результат ванильной runtime-проверки появления.
	//!
	//! \param[in] owner Сущность точки появления, чьи позиция и мир проверяются
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
	//! Releases this point's cached editor-only vehicle-envelope Shape and bounds.
	//! Освобождает кэшированные Shape и границы vehicle-envelope этой точки только для редактора.
	void ME_ClearEditorVehicleEnvelopePreview()
	{
		m_ME_EditorVehicleEnvelopeShape = null;
		m_ME_EditorVehicleEnvelopeFillShape = null;
		m_bME_EditorVehicleEnvelopePreviewActive = false;
	}

	//------------------------------------------------------------------------------------------------
	//! Releases the one active editor-only vehicle-envelope preview, if any.
	//! Освобождает единственный активный preview vehicle-envelope только для редактора, если он есть.
	static void ME_ClearActiveEditorVehicleEnvelopePreview()
	{
		if (!s_ME_ActiveEditorVehicleEnvelopePreview)
			return;

		s_ME_ActiveEditorVehicleEnvelopePreview.ME_ClearEditorVehicleEnvelopePreview();
		s_ME_ActiveEditorVehicleEnvelopePreview = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuilds the active conservative vehicle envelope at this point's current editor origin,
	//! applying only its yaw to local X/Z while preserving local Y, with a translucent fill
	//! and contrasting outline for Workbench visibility.
	//! Перестраивает активный консервативный vehicle-envelope в текущей editor-позиции этой точки,
	//! применяя только её yaw к локальным X/Z и сохраняя локальный Y, с полупрозрачной заливкой
	//! и контрастной рамкой для видимости в Workbench.
	void ME_RefreshEditorVehicleEnvelopePreview()
	{
		m_ME_EditorVehicleEnvelopeShape = null;
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
			corners[cornerIndex] = origin + Vector(
				localCorner[0] * yawCos + localCorner[2] * yawSin,
				localCorner[1],
				localCorner[2] * yawCos - localCorner[0] * yawSin
			);
		}

		vector fillPoints[] = {
			corners[0], corners[1], corners[3], corners[0], corners[3], corners[2],
			corners[4], corners[6], corners[7], corners[4], corners[7], corners[5],
			corners[0], corners[2], corners[6], corners[0], corners[6], corners[4],
			corners[1], corners[5], corners[7], corners[1], corners[7], corners[3],
			corners[0], corners[4], corners[5], corners[0], corners[5], corners[1],
			corners[2], corners[3], corners[7], corners[2], corners[7], corners[6]
		};

		Color fillColor = Color.FromRGBA(255, 215, 0, 48);
		m_ME_EditorVehicleEnvelopeFillShape = Shape.CreateTris(fillColor.PackToInt(), ShapeFlags.TRANSP | ShapeFlags.DOUBLESIDE, fillPoints, 36);

		vector linePoints[] = {
			corners[0], corners[1], corners[1], corners[3], corners[3], corners[2], corners[2], corners[0],
			corners[4], corners[5], corners[5], corners[7], corners[7], corners[6], corners[6], corners[4],
			corners[0], corners[4], corners[1], corners[5], corners[2], corners[6], corners[3], corners[7]
		};

		Color outlineColor = Color.FromRGBA(255, 48, 48, 255);
		m_ME_EditorVehicleEnvelopeShape = Shape.CreateLines(outlineColor.PackToInt(), ShapeFlags.DOUBLESIDE, linePoints, 24);
	}

	//------------------------------------------------------------------------------------------------
	//! Activates this point as the sole owner of a validated conservative local vehicle envelope.
	//! Активирует эту точку единственным владельцем проверенного консервативного локального vehicle-envelope.
	void ME_ShowEditorVehicleEnvelopePreview(vector localMins, vector localMaxs)
	{
		ME_ClearActiveEditorVehicleEnvelopePreview();
		m_vME_EditorVehicleEnvelopeLocalMins = localMins;
		m_vME_EditorVehicleEnvelopeLocalMaxs = localMaxs;
		m_bME_EditorVehicleEnvelopePreviewActive = true;
		s_ME_ActiveEditorVehicleEnvelopePreview = this;
		ME_RefreshEditorVehicleEnvelopePreview();
	}


	//!
	//! \param[in] owner Spawn point entity being initialized
	//! \param[in,out] mat Spawn point transform matrix
	//! \param[in] src Spawn point entity source
	//! Регистрирует точку и обновляет рекомендательные Shape только для редактора при её инициализации Workbench.
	//!
	//! \param[in] owner Инициализируемая сущность точки появления
	//! \param[in,out] mat Матрица преобразования точки появления
	//! \param[in] src Источник сущности точки появления
	override void _WB_OnInit(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		super._WB_OnInit(owner, mat, src);
		ME_RegisterEditorDebugSpawnPoint();
		ME_RefreshAllEditorDebugShapes();
	}

	//------------------------------------------------------------------------------------------------
	//! Refreshes every editor-only advisory Shape after Workbench changes a spawn point transform.
	//!
	//! \param[in] owner Moved spawn point entity
	//! \param[in,out] mat Updated spawn point transform matrix
	//! \param[in] src Spawn point entity source
	//! Обновляет каждую рекомендательную Shape только для редактора после изменения Workbench преобразования точки появления.
	//!
	//! \param[in] owner Перемещённая сущность точки появления
	//! \param[in,out] mat Обновлённая матрица преобразования точки появления
	//! \param[in] src Источник сущности точки появления
	override void _WB_SetTransform(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		super._WB_SetTransform(owner, mat, src);
		ME_RefreshAllEditorDebugShapes();
		if (s_ME_ActiveEditorVehicleEnvelopePreview == this)
			ME_RefreshEditorVehicleEnvelopePreview();
	}

	//------------------------------------------------------------------------------------------------
	//! Unregisters the point, releases its editor-only Shape, and refreshes the remaining advisory Shapes during entity deletion.
	//!
	//! \param[in] owner Spawn point entity being deleted
	//! Удаляет точку из реестра, освобождает её Shape только для редактора и обновляет оставшиеся рекомендательные Shape при удалении сущности.
	//!
	//! \param[in] owner Удаляемая сущность точки появления
	override void OnDelete(IEntity owner)
	{
		ME_UnregisterEditorDebugSpawnPoint();
		ME_ClearEditorDebugShape();
		ME_ClearEditorVehicleEnvelopePreview();
		if (s_ME_ActiveEditorVehicleEnvelopePreview == this)
			s_ME_ActiveEditorVehicleEnvelopePreview = null;
		super.OnDelete(owner);
		ME_RefreshAllEditorDebugShapes();
	}

	//------------------------------------------------------------------------------------------------
	//! Unregisters the point before Workbench removes it, releases its editor-only Shape, and refreshes remaining advisory Shapes.
	//!
	//! \param[in] owner Spawn point entity being removed
	//! \param[in] src Spawn point entity source
	//! Удаляет точку из реестра до её удаления Workbench, освобождает её Shape только для редактора и обновляет оставшиеся рекомендательные Shape.
	//!
	//! \param[in] owner Удаляемая сущность точки появления
	//! \param[in] src Источник сущности точки появления
	override void _WB_OnDelete(IEntity owner, IEntitySource src)
	{
		ME_UnregisterEditorDebugSpawnPoint();
		ME_ClearEditorDebugShape();
		ME_ClearEditorVehicleEnvelopePreview();
		if (s_ME_ActiveEditorVehicleEnvelopePreview == this)
			s_ME_ActiveEditorVehicleEnvelopePreview = null;
		super._WB_OnDelete(owner, src);
		ME_RefreshAllEditorDebugShapes();
	}
}