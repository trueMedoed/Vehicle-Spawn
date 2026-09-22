//! Test-only generator for editable entity labels snapshot from editor vehicle catalogs.
//! Генератор только для Test для snapshot меток editable entity из editor vehicle catalogs.

//------------------------------------------------------------------------------------------------
//! Generates deterministic snapshot of vehicle editable entity labels from global and faction catalogs.
//! Генерирует детерминированный snapshot меток editable entity техники из глобальных и faction-каталогов.
[WorkbenchPluginAttribute(name: "Generate entity labels snapshots", description: "Generates and reload-validates the canonical vehicle entity labels snapshot.", wbModules: { "WorldEditor" }, category: "ME_Vehicle_Spawn/Entity Labels")]
class ME_GenerateEditableEntityLabelsSnapshotPlugin : WorldEditorPlugin
{
	protected const string SNAPSHOT_PATH = "Configs/Generated/ME_EditableEntityLabelsSnapshot.conf";
	protected static const ResourceName SNAPSHOT_RESOURCE = "{CB8A49D70FEBE12A}Configs/Generated/ME_EditableEntityLabelsSnapshot.conf";
	protected const string GLOBAL_SCOPE_KEY = "__ME_GLOBAL_VEHICLE_CATALOG__";

	//------------------------------------------------------------------------------------------------
	//! Generates and reload-validates the entity labels snapshot from editor catalogs.
	//! Генерирует и проверяет после перезагрузки snapshot меток сущностей из editor-каталогов.
	override void Run()
	{
		Game game = GetGame();
		string gameVersion;
		if (game)
			gameVersion = game.GetBuildVersion();
		if (gameVersion.IsEmpty())
		{
			Print("[ME_DEBUG_AVSP_WB] labels_snapshot status=FAIL reason=game_version_unavailable");
			return;
		}

		string reason;
		ME_EditableEntityLabelsSnapshot snapshot = ME_CreateLabelsSnapshot(gameVersion, reason);
		if (!snapshot)
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] labels_snapshot status=FAIL reason=%1", reason);
			return;
		}

		if (!ME_SaveAndValidateSnapshot(snapshot, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] labels_snapshot status=FAIL reason=%1", reason);
			return;
		}

		int totalScopes = snapshot.m_aScopes.Count();
		int totalLabels = 0;
		int totalPrefabs = 0;
		foreach (ME_EditableEntityLabelsSnapshotScope scope : snapshot.m_aScopes)
		{
			totalLabels += scope.m_aLabels.Count();
			foreach (ME_EditableEntityLabelsSnapshotLabel label : scope.m_aLabels)
			{
				totalPrefabs += label.m_aPrefabs.Count();
			}
		}

		PrintFormat("[ME_DEBUG_AVSP_WB] labels_snapshot status=SUCCESS scopes=%1 labels=%2 prefabs=%3", totalScopes, totalLabels, totalPrefabs);
	}

	//------------------------------------------------------------------------------------------------
	//! Creates the snapshot from global and faction vehicle catalogs.
	//! Создаёт snapshot из глобальных и faction-каталогов техники.
	ME_EditableEntityLabelsSnapshot ME_CreateLabelsSnapshot(string gameVersion, out string reason)
	{
		reason = "";
		ME_EditableEntityLabelsSnapshot snapshot = new ME_EditableEntityLabelsSnapshot();
		snapshot.m_iSchemaVersion = 1;
		snapshot.m_sGeneratorVersion = "ME_labels_generator_v1";
		snapshot.m_sGameVersion = gameVersion;
		snapshot.m_aScopes = {};

		// Collect global catalog labels
		// Собрать метки глобального каталога
		SCR_EntityCatalog globalCatalog = SCR_EntityCatalogManagerComponent.ME_GetEditorGlobalVehicleCatalog(reason);
		if (!globalCatalog)
		{
			reason = "global_catalog_unavailable";
			return null;
		}

		ME_EditableEntityLabelsSnapshotScope globalScope = ME_CollectCatalogLabels(GLOBAL_SCOPE_KEY, globalCatalog, reason);
		if (!globalScope)
			return null;

		snapshot.m_aScopes.Insert(globalScope);

		// Collect faction catalog labels
		// Собрать метки faction-каталогов
		array<Faction> factions = {};
		int factionCount = GetGame().GetFactionManager().GetFactionsList(factions);
		foreach (Faction faction : factions)
		{
			SCR_Faction scrFaction = SCR_Faction.Cast(faction);
			if (!scrFaction)
				continue;

			if (!scrFaction.ME_EnsureEditorCatalogsInitialized())
				continue;

			SCR_EntityCatalog factionCatalog = scrFaction.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE);
			if (!factionCatalog)
				continue;

			string factionKey = scrFaction.GetFactionKey();
			ME_EditableEntityLabelsSnapshotScope factionScope = ME_CollectCatalogLabels(factionKey, factionCatalog, reason);
			if (!factionScope)
				continue;

			snapshot.m_aScopes.Insert(factionScope);
		}

		ME_SortSnapshot(snapshot);
		return snapshot;
	}

	//------------------------------------------------------------------------------------------------
	//! Collects all labels from a catalog into a snapshot scope.
	//! Собирает все метки из каталога в scope snapshot.
	ME_EditableEntityLabelsSnapshotScope ME_CollectCatalogLabels(string scopeKey, SCR_EntityCatalog catalog, out string reason)
	{
		reason = "";
		ME_EditableEntityLabelsSnapshotScope scope = new ME_EditableEntityLabelsSnapshotScope();
		scope.m_sScopeKey = scopeKey;
		scope.m_aLabels = {};

		map<string, ref ME_EditableEntityLabelsSnapshotLabel> labelMap = new map<string, ref ME_EditableEntityLabelsSnapshotLabel>();

		array<SCR_EntityCatalogEntry> entries = {};
		int entryCount = catalog.GetEntityList(entries);

		foreach (SCR_EntityCatalogEntry entry : entries)
		{
			ResourceName prefab = entry.GetPrefab();
			if (prefab.IsEmpty())
				continue;

			int catalogIndex = entry.GetCatalogIndex();
			string entityName = entry.GetEntityName();

			array<EEditableEntityLabel> labels = {};
			int labelCount = entry.GetEditableEntityLabels(labels);
			if (labelCount == 0)
				continue;

			foreach (EEditableEntityLabel labelEnum : labels)
			{
				string labelName = typename.EnumToString(EEditableEntityLabel, labelEnum);
				if (labelName.IsEmpty())
					continue;

				ME_EditableEntityLabelsSnapshotLabel labelGroup;
				if (!labelMap.Find(labelName, labelGroup))
				{
					labelGroup = new ME_EditableEntityLabelsSnapshotLabel();
					labelGroup.m_sLabelName = labelName;
					labelGroup.m_aPrefabs = {};
					labelMap.Insert(labelName, labelGroup);
				}

				// Check for duplicate prefab in this label group
				// Проверить дубликат prefab в этой label-группе
				bool isDuplicate = false;
				foreach (ME_EditableEntityLabelsSnapshotPrefab existing : labelGroup.m_aPrefabs)
				{
					if (existing.m_sPrefabPath == prefab)
					{
						isDuplicate = true;
						break;
					}
				}

				if (!isDuplicate)
				{
					ME_EditableEntityLabelsSnapshotPrefab prefabEntry = new ME_EditableEntityLabelsSnapshotPrefab();
					prefabEntry.m_sPrefabPath = prefab;
					prefabEntry.m_iCatalogIndex = catalogIndex;
					prefabEntry.m_sEntityName = entityName;
					labelGroup.m_aPrefabs.Insert(prefabEntry);
				}
			}
		}

		// Convert map to array
		// Преобразовать map в массив
		foreach (string labelName, ME_EditableEntityLabelsSnapshotLabel labelGroup : labelMap)
		{
			scope.m_aLabels.Insert(labelGroup);
		}

		return scope;
	}

	//------------------------------------------------------------------------------------------------
	//! Sorts snapshot scopes, labels, and prefabs deterministically.
	//! Детерминированно сортирует scope, метки и prefab snapshot.
	void ME_SortSnapshot(ME_EditableEntityLabelsSnapshot snapshot)
	{
		// Sort scopes by key
		// Сортировать scope по ключу
		array<ref ME_EditableEntityLabelsSnapshotScope> sortedScopes = {};
		foreach (ME_EditableEntityLabelsSnapshotScope scope : snapshot.m_aScopes)
		{
			sortedScopes.Insert(scope);
		}
		sortedScopes.Sort(true);
		snapshot.m_aScopes = sortedScopes;

		foreach (ME_EditableEntityLabelsSnapshotScope scope : snapshot.m_aScopes)
		{
			// Sort labels by name
			// Сортировать метки по имени
			array<ref ME_EditableEntityLabelsSnapshotLabel> sortedLabels = {};
			foreach (ME_EditableEntityLabelsSnapshotLabel label : scope.m_aLabels)
			{
				sortedLabels.Insert(label);
			}
			sortedLabels.Sort(true);
			scope.m_aLabels = sortedLabels;

			foreach (ME_EditableEntityLabelsSnapshotLabel label : scope.m_aLabels)
			{
				// Sort prefabs by path
				// Сортировать prefab по пути
				array<ref ME_EditableEntityLabelsSnapshotPrefab> sortedPrefabs = {};
				foreach (ME_EditableEntityLabelsSnapshotPrefab prefab : label.m_aPrefabs)
				{
					sortedPrefabs.Insert(prefab);
				}
				sortedPrefabs.Sort(true);
				label.m_aPrefabs = sortedPrefabs;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Saves snapshot, rebuilds resource, and validates round-trip.
	//! Сохраняет snapshot, пересобирает ресурс и проверяет round-trip.
	bool ME_SaveAndValidateSnapshot(ME_EditableEntityLabelsSnapshot snapshot, out string reason)
	{
		reason = "";
		Resource containerResource = BaseContainerTools.CreateContainerFromInstance(snapshot);
		string absolutePath;
		if (!containerResource || !Workbench.GetAbsolutePath(SNAPSHOT_PATH, absolutePath, false))
		{
			reason = "create_or_path_failed";
			return false;
		}

		BaseContainer container = containerResource.GetResource().ToBaseContainer();
		if (!container)
		{
			reason = "container_conversion_failed";
			return false;
		}

		if (!BaseContainerTools.SaveContainer(container, SNAPSHOT_RESOURCE, absolutePath))
		{
			reason = "save_container_failed";
			return false;
		}

		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		if (!resourceManager)
		{
			reason = "resource_manager_unavailable";
			return false;
		}

		resourceManager.RebuildResourceFile(absolutePath, "", false);
		if (!resourceManager.WaitForFile(absolutePath, 5000))
		{
			reason = "resource_wait_failed";
			return false;
		}

		Resource loaded = BaseContainerTools.LoadContainer(SNAPSHOT_RESOURCE);
		BaseContainer loadedContainer;
		if (loaded)
			loadedContainer = loaded.GetResource().ToBaseContainer();
		if (!loadedContainer)
		{
			reason = "round_trip_load_failed";
			return false;
		}

		ME_EditableEntityLabelsSnapshot reloadedSnapshot = ME_EditableEntityLabelsSnapshot.Cast(BaseContainerTools.CreateInstanceFromContainer(loadedContainer));
		if (!reloadedSnapshot)
		{
			reason = "round_trip_cast_failed";
			return false;
		}

		if (reloadedSnapshot.m_aScopes.Count() != snapshot.m_aScopes.Count())
		{
			reason = "round_trip_scope_count_mismatch";
			return false;
		}

		return true;
	}
}
