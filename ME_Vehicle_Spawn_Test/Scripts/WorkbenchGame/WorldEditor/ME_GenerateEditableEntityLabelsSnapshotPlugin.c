//! Test-only generator for editable entity labels snapshot from editor vehicle catalogs.
//! Генератор только для Test для snapshot меток editable entity из editor vehicle catalogs.

//------------------------------------------------------------------------------------------------
//! Generates deterministic snapshot of vehicle editable entity labels from faction catalogs.
//! Генерирует детерминированный snapshot меток editable entity техники из faction-каталогов.
[WorkbenchPluginAttribute(name: "Generate entity labels snapshots", description: "Generates and reload-validates the canonical vehicle entity labels snapshot.", wbModules: { "WorldEditor" }, category: "ME_Vehicle_Spawn/Entity Labels")]
class ME_GenerateEditableEntityLabelsSnapshotPlugin : WorldEditorPlugin
{
	protected const string SNAPSHOT_PATH = "Configs/Generated/ME_EditableEntityLabelsSnapshot.conf";

	//------------------------------------------------------------------------------------------------
	//! Generates and reload-validates the entity labels snapshot from editor catalogs.
	//! Генерирует и проверяет после перезагрузки snapshot меток сущностей из editor-каталогов.
	protected bool m_bGenerationRunning;

	//------------------------------------------------------------------------------------------------
	//! Starts one cooperative Workbench task so resource builds can finish between validation attempts.
	//! Запускает одну кооперативную задачу Workbench, позволяя сборке завершаться между попытками проверки.
	override void Run()
	{
		if (m_bGenerationRunning)
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] labels_snapshot status=BUSY");
			return;
		}
		m_bGenerationRunning = true;
		thread ME_RunGeneration();
	}

	//------------------------------------------------------------------------------------------------
	//! Runs generation and releases the repeated-launch guard when it returns.
	//! Выполняет генерацию и снимает защиту от повторного запуска после возврата.
	void ME_RunGeneration()
	{
		ME_Generate();
		m_bGenerationRunning = false;
	}

	//------------------------------------------------------------------------------------------------
	//! Collects, saves, and validates the catalog snapshot, reporting its final status.
	//! Собирает, сохраняет и проверяет snapshot каталога, сообщая итоговый статус.
	void ME_Generate()
	{
		PrintFormat("[ME_DEBUG_AVSP_WB] labels_snapshot status=STARTED");
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

		PrintFormat("[ME_DEBUG_AVSP_WB] labels_snapshot status=SUCCESS scopes=%1 labels=%2 prefabLabelLinks=%3", totalScopes, totalLabels, totalPrefabs);
	}

	//------------------------------------------------------------------------------------------------
	//! Creates the snapshot from faction vehicle catalogs.
	//! Создаёт snapshot из faction-каталогов техники.
	ME_EditableEntityLabelsSnapshot ME_CreateLabelsSnapshot(string gameVersion, out string reason)
	{
		reason = "";
		ME_EditableEntityLabelsSnapshot snapshot = new ME_EditableEntityLabelsSnapshot();
		snapshot.m_iSchemaVersion = 2;
		snapshot.m_sGeneratorVersion = "ME_labels_generator_v3";
		snapshot.m_sGameVersion = gameVersion;
		snapshot.m_aScopes = {};

		if (!GetGame() || !SCR_Global.IsEditMode())
		{
			reason = "open_world_in_edit_mode_required";
			return null;
		}
		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
		{
			reason = "faction_manager_unavailable";
			return null;
		}

		array<Faction> factions = {};
		factionManager.GetFactionsList(factions);
		array<string> scopeKeys = {};
		int factionCatalogCount = 0;
		foreach (Faction faction : factions)
		{
			SCR_Faction scrFaction = SCR_Faction.Cast(faction);
			if (!scrFaction)
				continue;
			string factionKey = scrFaction.GetFactionKey();
			if (factionKey.IsEmpty() || scopeKeys.Contains(factionKey))
			{
				reason = string.Format("invalid_or_duplicate_faction_key key=%1", factionKey);
				return null;
			}
			scopeKeys.Insert(factionKey);
			if (!scrFaction.ME_EnsureEditorCatalogsInitialized())
			{
				reason = string.Format("faction_catalog_initialization_failed key=%1", factionKey);
				return null;
			}
			SCR_EntityCatalog factionCatalog = scrFaction.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE);
			if (!factionCatalog)
			{
				PrintFormat("[ME_DEBUG_AVSP_WB] labels_snapshot scope=%1 status=SKIPPED reason=no_vehicle_catalog", factionKey);
				continue;
			}
			ME_EditableEntityLabelsSnapshotScope factionScope = ME_CollectCatalogLabels(factionKey, factionCatalog, reason);
			if (!factionScope)
				return null;
			snapshot.m_aScopes.Insert(factionScope);
			factionCatalogCount++;
		}
		if (factionCatalogCount == 0)
		{
			reason = "no_faction_vehicle_catalogs";
			return null;
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
			if (!entry)
			{
				reason = "null_catalog_entry";
				return null;
			}
			ResourceName prefab = entry.GetPrefab();
			if (prefab.IsEmpty())
			{
				reason = "empty_prefab_path";
				return null;
			}


			array<EEditableEntityLabel> labels = {};
			entry.GetEditableEntityLabels(labels);
			array<string> labelNames = {};
			foreach (EEditableEntityLabel labelEnum : labels)
			{
				string enumName = typename.EnumToString(EEditableEntityLabel, labelEnum);
				if (enumName.IsEmpty())
				{
					reason = string.Format("unknown_label scope=%1 prefab=%2", scopeKey, prefab);
					return null;
				}
				if (!labelNames.Contains(enumName))
					labelNames.Insert(enumName);
			}
			if (labelNames.IsEmpty())
				labelNames.Insert("__NO_LABELS__");
			foreach (string labelName : labelNames)
			{
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
		array<string> scopeKeys = {};
		map<string, ref ME_EditableEntityLabelsSnapshotScope> scopes = new map<string, ref ME_EditableEntityLabelsSnapshotScope>();
		foreach (ME_EditableEntityLabelsSnapshotScope scope : snapshot.m_aScopes)
		{
			scopeKeys.Insert(scope.m_sScopeKey);
			scopes.Insert(scope.m_sScopeKey, scope);
		}
		scopeKeys.Sort();
		snapshot.m_aScopes.Clear();
		foreach (string scopeKey : scopeKeys)
			snapshot.m_aScopes.Insert(scopes.Get(scopeKey));
		foreach (ME_EditableEntityLabelsSnapshotScope scope : snapshot.m_aScopes)
		{
			array<string> labelNames = {};
			map<string, ref ME_EditableEntityLabelsSnapshotLabel> labels = new map<string, ref ME_EditableEntityLabelsSnapshotLabel>();
			foreach (ME_EditableEntityLabelsSnapshotLabel label : scope.m_aLabels)
			{
				labelNames.Insert(label.m_sLabelName);
				labels.Insert(label.m_sLabelName, label);
			}
			labelNames.Sort();
			scope.m_aLabels.Clear();
			foreach (string labelName : labelNames)
			{
				ME_EditableEntityLabelsSnapshotLabel label = labels.Get(labelName);
				scope.m_aLabels.Insert(label);
				array<string> paths = {};
				map<string, ref ME_EditableEntityLabelsSnapshotPrefab> prefabs = new map<string, ref ME_EditableEntityLabelsSnapshotPrefab>();
				foreach (ME_EditableEntityLabelsSnapshotPrefab prefab : label.m_aPrefabs)
				{
					paths.Insert(prefab.m_sPrefabPath);
					prefabs.Insert(prefab.m_sPrefabPath, prefab);
				}
				paths.Sort();
				label.m_aPrefabs.Clear();
				foreach (string path : paths)
					label.m_aPrefabs.Insert(prefabs.Get(path));
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Derives a safe container name from the prefab filename without its extension.
	//! Получает допустимое имя контейнера из имени файла prefab без расширения.
	string ME_GetPrefabContainerName(string path)
	{
		int start = 0;
		int end = path.Length();
		for (int i = 0; i < path.Length(); i++)
		{
			string character = path.Substring(i, 1);
			if (character == "/")
				start = i + 1;
			if (character == ".")
				end = i;
		}
		if (end <= start)
			end = path.Length();
		string name;
		string allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
		for (int i = start; i < end; i++)
		{
			string character = path.Substring(i, 1);
			if (allowed.Contains(character))
				name += character;
			else
				name += "_";
		}
		if (name.IsEmpty())
			return "Prefab";
		if ("0123456789".Contains(name.Substring(0, 1)))
			name = "Prefab_" + name;
		return name;
	}

	//------------------------------------------------------------------------------------------------
	//! Names nested containers deterministically for readable Config Editor groups.
	//! Детерминированно именует вложенные контейнеры для читаемых групп в Config Editor.
	bool ME_NameContainers(BaseContainer container, ME_EditableEntityLabelsSnapshot snapshot, bool validateOnly = false)
	{
		BaseContainerList scopes = container.GetObjectArray("m_aScopes");
		if (!scopes || scopes.Count() != snapshot.m_aScopes.Count())
			return false;
		for (int si = 0; si < scopes.Count(); si++)
		{
			BaseContainer scope = scopes.Get(si);
			if (!scope)
				return false;
			if (validateOnly && scope.GetName() != snapshot.m_aScopes[si].m_sScopeKey)
				return false;
			if (!validateOnly)
				scope.SetName(snapshot.m_aScopes[si].m_sScopeKey);
			BaseContainerList labels = scope.GetObjectArray("m_aLabels");
			if (!labels || labels.Count() != snapshot.m_aScopes[si].m_aLabels.Count())
				return false;
			for (int li = 0; li < labels.Count(); li++)
			{
				BaseContainer label = labels.Get(li);
				if (!label)
					return false;
				if (validateOnly && label.GetName() != snapshot.m_aScopes[si].m_aLabels[li].m_sLabelName)
					return false;
				if (!validateOnly)
					label.SetName(snapshot.m_aScopes[si].m_aLabels[li].m_sLabelName);
				BaseContainerList prefabs = label.GetObjectArray("m_aPrefabs");
				if (!prefabs || prefabs.Count() != snapshot.m_aScopes[si].m_aLabels[li].m_aPrefabs.Count())
					return false;
				array<string> usedNames = {};
				for (int pi = 0; pi < prefabs.Count(); pi++)
				{
					BaseContainer prefab = prefabs.Get(pi);
					if (!prefab)
						return false;
					string baseName = ME_GetPrefabContainerName(snapshot.m_aScopes[si].m_aLabels[li].m_aPrefabs[pi].m_sPrefabPath);
					string containerName = baseName;
					int suffix = 2;
					while (usedNames.Contains(containerName))
					{
						containerName = string.Format("%1_%2", baseName, suffix);
						suffix++;
					}
					usedNames.Insert(containerName);
					if (validateOnly && prefab.GetName() != containerName)
						return false;
					if (!validateOnly)
						prefab.SetName(containerName);
				}
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Flattens every persisted field and array count for exact round-trip comparison.
	//! Собирает все сохраняемые поля и размеры массивов для точного сравнения после перезагрузки.
	bool ME_GetSnapshotFields(ME_EditableEntityLabelsSnapshot snapshot, out array<string> fields)
	{
		fields = {};
		if (!snapshot || !snapshot.m_aScopes)
			return false;
		fields.Insert(snapshot.m_iSchemaVersion.ToString());
		fields.Insert(snapshot.m_sGeneratorVersion);
		fields.Insert(snapshot.m_sGameVersion);
		fields.Insert(snapshot.m_aScopes.Count().ToString());
		foreach (ME_EditableEntityLabelsSnapshotScope scope : snapshot.m_aScopes)
		{
			if (!scope)
				return false;
			fields.Insert(scope.m_sScopeKey);
			// Empty arrays may be omitted from the serialized config and reload as null.
			// Пустые массивы могут отсутствовать в конфиге и загружаться как null.
			if (!scope.m_aLabels)
			{
				fields.Insert("0");
				continue;
			}
			fields.Insert(scope.m_aLabels.Count().ToString());
			foreach (ME_EditableEntityLabelsSnapshotLabel label : scope.m_aLabels)
			{
				if (!label)
					return false;
				fields.Insert(label.m_sLabelName);
				if (!label.m_aPrefabs)
				{
					fields.Insert("0");
					continue;
				}
				fields.Insert(label.m_aPrefabs.Count().ToString());
				foreach (ME_EditableEntityLabelsSnapshotPrefab prefab : label.m_aPrefabs)
				{
					if (!prefab)
						return false;
					fields.Insert(prefab.m_sPrefabPath);
				}
			}
		}
		return true;
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

		if (!ME_NameContainers(container, snapshot))
		{
			reason = "container_naming_failed";
			return false;
		}
		if (!BaseContainerTools.SaveContainer(container, ResourceName.Empty, absolutePath))
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

		// Register new files before rebuilding; Workbench owns the meta file and GUID.
		// Регистрируем новые файлы перед сборкой; meta-файл и GUID создаёт Workbench.
		MetaFile meta = resourceManager.GetMetaFile(absolutePath);
		if (!meta)
		{
			if (!resourceManager.RegisterResourceFile(absolutePath, false))
			{
				reason = "resource_registration_failed";
				return false;
			}
			if (!resourceManager.WaitForFile(absolutePath, 5000))
			{
				reason = "resource_registration_wait_failed";
				return false;
			}
			meta = resourceManager.GetMetaFile(absolutePath);
		}
		if (!meta)
		{
			reason = "resource_meta_unavailable";
			return false;
		}
		ResourceName snapshotResource = meta.GetResourceID();
		if (snapshotResource.IsEmpty())
		{
			reason = "registered_resource_id_unavailable";
			return false;
		}
		// Build the PC configuration declared in the Workbench-generated meta file.
		// Собираем конфигурацию PC, объявленную в созданном Workbench метафайле.
		resourceManager.RebuildResourceFile(absolutePath, "PC", false);
		if (!resourceManager.WaitForFile(absolutePath, 5000))
		{
			reason = "resource_wait_failed";
			return false;
		}

		// WaitForFile acknowledges registration, not completion of an asynchronous rebuild.
		// WaitForFile подтверждает регистрацию, а не завершение асинхронной сборки.
		for (int attempt = 0; attempt < 20; attempt++)
		{
			Sleep(250);
			if (ME_ValidateSnapshotResource(snapshot, snapshotResource, reason))
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Loads the resource anew and compares every field; each attempt releases its local resource references.
	//! Заново загружает ресурс и сравнивает все поля; каждая попытка освобождает локальные ссылки на ресурс.
	bool ME_ValidateSnapshotResource(ME_EditableEntityLabelsSnapshot snapshot, ResourceName snapshotResource, out string reason)
	{
		reason = "";
		Resource loaded = BaseContainerTools.LoadContainer(snapshotResource);
		BaseContainer loadedContainer;
		if (loaded)
			loadedContainer = loaded.GetResource().ToBaseContainer();
		if (!loadedContainer)
		{
			reason = "round_trip_load_failed";
			return false;
		}

		if (!ME_NameContainers(loadedContainer, snapshot, true))
		{
			reason = "round_trip_container_names_or_counts_mismatch";
			return false;
		}

		ME_EditableEntityLabelsSnapshot reloadedSnapshot = ME_EditableEntityLabelsSnapshot.Cast(BaseContainerTools.CreateInstanceFromContainer(loadedContainer));
		if (!reloadedSnapshot)
		{
			reason = "round_trip_cast_failed";
			return false;
		}

		array<string> expected;
		array<string> actual;
		if (!ME_GetSnapshotFields(snapshot, expected) || !ME_GetSnapshotFields(reloadedSnapshot, actual))
		{
			reason = "round_trip_invalid_structure";
			return false;
		}
		if (expected.Count() != actual.Count())
		{
			reason = string.Format("round_trip_field_count_mismatch expected=%1 actual=%2 expectedScopes=%3 actualScopes=%4", expected.Count(), actual.Count(), snapshot.m_aScopes.Count(), reloadedSnapshot.m_aScopes.Count());
			for (int compareIndex = 0; compareIndex < expected.Count() && compareIndex < actual.Count(); compareIndex++)
			{
				if (expected[compareIndex] != actual[compareIndex])
				{
					reason += string.Format(" firstDifference=%1 expectedValue=%2 actualValue=%3", compareIndex, expected[compareIndex], actual[compareIndex]);
					break;
				}
			}
			return false;
		}
		for (int fieldIndex = 0; fieldIndex < expected.Count(); fieldIndex++)
		{
			if (expected[fieldIndex] != actual[fieldIndex])
			{
				reason = string.Format("round_trip_field_mismatch index=%1 expected=%2 actual=%3", fieldIndex, expected[fieldIndex], actual[fieldIndex]);
				return false;
			}
		}

		return true;
	}
}
