//! Test-only generator for a deterministic ambient-vehicle bounds snapshot staged resource.
//! Генератор только для Test детерминированного staged-ресурса snapshot границ ambient-техники.

//------------------------------------------------------------------------------------------------
//! Measures marked roots in the dedicated fixture without creating, moving, or deleting world entities.
//! Stores an orientation-aligned local box with independent horizontal X/Z extrema for yaw-only preview.
//! Измеряет помеченные корни в выделенной fixture без создания, перемещения или удаления сущностей мира.
//! Сохраняет ориентированный по осям локальный box с независимыми горизонтальными экстремумами X/Z для yaw-only preview.
[WorkbenchPluginAttribute(name: "Generate ambient vehicle bounds snapshot", description: "Measures the dedicated fixture and reload-validates a staged vehicle-bounds snapshot.", wbModules: { "WorldEditor" })]
class ME_GenerateVehicleBoundsSnapshotPlugin : WorldEditorPlugin
{
	protected const string US_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_US";
	protected const string USSR_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_USSR";
	protected const string US_ARMED_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_US_Armed";
	protected const string USSR_ARMED_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_USSR_Armed";
	protected const string CIV_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_CIV";
	protected const string FIA_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_FIA";
	protected const string FIA_ARMED_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_FIA_Armed";
	protected const string FIXTURE_IDENTITY = "ME_VehicleBoundsSnapshot";
	protected const string STAGED_PATH = "Configs/Generated/ME_VehicleBoundsSnapshot_Staged.conf";
	protected static const ResourceName STAGED_RESOURCE = "{1C3AE4A8F2630BF7}Configs/Generated/ME_VehicleBoundsSnapshot_Staged.conf";

	//------------------------------------------------------------------------------------------------
	//! Generates and reload-validates a staged snapshot from every marked root in the currently open dedicated fixture.
	//! Генерирует и reload-валидирует staged snapshot по каждому помеченному корню в открытой выделенной fixture.
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=world_editor_unavailable");
			return;
		}

		WorldEditorAPI api = worldEditor.GetApi();
		if (!api)
		{
			Print("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=world_editor_api_unavailable");
			return;
		}

		string reason;

		array<string> expectedPaths = {};
		if (!ME_CollectCandidatePaths(api, US_SPAWN_POINT_NAME, expectedPaths, reason)
			|| !ME_CollectCandidatePaths(api, USSR_SPAWN_POINT_NAME, expectedPaths, reason)
			|| !ME_CollectCandidatePaths(api, US_ARMED_SPAWN_POINT_NAME, expectedPaths, reason)
			|| !ME_CollectCandidatePaths(api, USSR_ARMED_SPAWN_POINT_NAME, expectedPaths, reason)
			|| !ME_CollectCandidatePaths(api, CIV_SPAWN_POINT_NAME, expectedPaths, reason)
			|| !ME_CollectCandidatePaths(api, FIA_SPAWN_POINT_NAME, expectedPaths, reason)
			|| !ME_CollectCandidatePaths(api, FIA_ARMED_SPAWN_POINT_NAME, expectedPaths, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=%1", reason);
			return;
		}

		array<IEntity> markerRoots = {};
		array<string> markerPaths = {};
		if (!ME_CollectMarkerRoots(api, markerRoots, markerPaths, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=%1", reason);
			return;
		}

		expectedPaths.Sort();
		markerPaths.Sort();
		if (!ME_HasExactCoverage(expectedPaths, markerPaths, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=%1", reason);
			return;
		}

		ME_VehicleBoundsSnapshot snapshot = new ME_VehicleBoundsSnapshot();
		snapshot.m_iSchemaVersion = 2;
		snapshot.m_sGeneratorVersion = "fixture-generator-v2";
		snapshot.m_sFixtureIdentity = FIXTURE_IDENTITY;
		snapshot.m_aEntries = {};

		foreach (string prefabPath : expectedPaths)
		{
			IEntity root = ME_FindMarkerRoot(markerRoots, prefabPath);
			if (!root)
			{
				PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=marker_root_unavailable path=%1", prefabPath);
				return;
			}

			ME_VehicleBoundsSnapshotEntry entry;
			if (!ME_CreateEntry(root, prefabPath, entry, reason))
			{
				PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=%1 path=%2", reason, prefabPath);
				return;
			}

			snapshot.m_aEntries.Insert(entry);
		}

		if (!ME_SaveAndValidateStagedSnapshot(snapshot, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=%1", reason);
			return;
		}

		foreach (ME_VehicleBoundsSnapshotEntry entry : snapshot.m_aEntries)
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot_entry prefab=%1 mins=%2 maxs=%3 fixtureRoot=%4", entry.m_sPrefab, entry.m_vLocalMins, entry.m_vLocalMaxs, entry.m_sFixtureEntityName);

		PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=PASS stage=%1 count=%2 coverage=%3", STAGED_PATH, snapshot.m_aEntries.Count(), expectedPaths.Count());
	}

	//------------------------------------------------------------------------------------------------
	//! Adds one fixture spawn point's unique canonical candidate paths to the aggregate set.
	//! Добавляет уникальные канонические пути кандидатов одной spawn-point fixture к общему набору.
	protected bool ME_CollectCandidatePaths(WorldEditorAPI api, string entityName, inout array<string> expectedPaths, out string reason)
	{
		reason = "";
		IEntitySource source = api.FindEntityByName(entityName);
		if (!source)
		{
			reason = string.Format("spawn_point_unavailable entity=%1", entityName);
			return false;
		}

		IEntity entity = api.SourceToEntity(source);
		if (!entity)
		{
			reason = string.Format("spawn_point_unavailable entity=%1", entityName);
			return false;
		}

		SCR_AmbientVehicleSpawnPointComponent spawnPoint = SCR_AmbientVehicleSpawnPointComponent.Cast(entity.FindComponent(SCR_AmbientVehicleSpawnPointComponent));
		if (!spawnPoint)
		{
			reason = string.Format("component_unavailable entity=%1", entityName);
			return false;
		}

		array<string> paths;
		if (!spawnPoint.ME_GetEditorVehicleEnvelopeCandidatePaths(paths, reason))
			return false;

		foreach (string path : paths)
		{
			if (expectedPaths.Contains(path))
				continue;

			expectedPaths.Insert(path);
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Collects each marked vehicle root and its unique declared canonical catalog path.
	//! Собирает каждый помеченный корень техники и его уникальный объявленный канонический путь каталога.
	protected bool ME_CollectMarkerRoots(WorldEditorAPI api, out array<IEntity> markerRoots, out array<string> markerPaths, out string reason)
	{
		markerRoots = {};
		markerPaths = {};
		reason = "";
		int entityCount = api.GetEditorEntityCount();
		for (int index = 0; index < entityCount; index++)
		{
			IEntity entity = api.SourceToEntity(api.GetEditorEntity(index));
			if (!entity)
				continue;

			ME_VehicleBoundsFixtureMarkerComponent marker = ME_VehicleBoundsFixtureMarkerComponent.Cast(entity.FindComponent(ME_VehicleBoundsFixtureMarkerComponent));
			if (!marker)
				continue;

			string path = marker.GetCatalogPrefab();
			if (path.IsEmpty())
			{
				reason = string.Format("empty_marker_prefab entity=%1", entity.GetName());
				return false;
			}

			if (markerPaths.Contains(path))
			{
				reason = string.Format("duplicate_marker_prefab path=%1", path);
				return false;
			}

			if (entity.GetName().IsEmpty())
			{
				reason = string.Format("empty_marker_root_name path=%1", path);
				return false;
			}

			markerRoots.Insert(entity);
			markerPaths.Insert(path);
		}

		if (markerRoots.IsEmpty())
		{
			reason = "no_fixture_markers";
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Verifies that sorted catalog candidates and marker declarations have exact one-to-one coverage.
	//! Проверяет точное взаимно-однозначное покрытие отсортированных кандидатов каталога и marker-деклараций.
	protected bool ME_HasExactCoverage(array<string> expectedPaths, array<string> markerPaths, out string reason)
	{
		reason = "";
		if (expectedPaths.Count() != markerPaths.Count())
		{
			reason = string.Format("count_mismatch expected=%1 markers=%2", expectedPaths.Count(), markerPaths.Count());
			return false;
		}

		for (int index = 0; index < expectedPaths.Count(); index++)
		{
			if (expectedPaths[index] != markerPaths[index])
			{
				reason = string.Format("path_mismatch expected=%1 marker=%2", expectedPaths[index], markerPaths[index]);
				return false;
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds the marked fixture root whose declared catalog path matches the requested entry.
	//! Находит помеченный корень fixture, чей объявленный путь каталога совпадает с запрошенной записью.
	protected IEntity ME_FindMarkerRoot(array<IEntity> markerRoots, string prefabPath)
	{
		foreach (IEntity root : markerRoots)
		{
			ME_VehicleBoundsFixtureMarkerComponent marker = ME_VehicleBoundsFixtureMarkerComponent.Cast(root.FindComponent(ME_VehicleBoundsFixtureMarkerComponent));
			if (marker && marker.GetCatalogPrefab() == prefabPath)
				return root;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Converts a fixture root's complete world AABB into an orientation-aligned local box with independent X/Z extrema.
	//! Преобразует полную мировую AABB корня fixture в ориентированный по осям локальный box с независимыми экстремумами X/Z.
	protected bool ME_CreateEntry(IEntity root, string prefabPath, out ME_VehicleBoundsSnapshotEntry entry, out string reason)
	{
		entry = null;
		reason = "";
		vector worldMins;
		vector worldMaxs;
		SCR_Global.GetWorldBoundsWithChildren(root, worldMins, worldMaxs);
		if (!ME_IsFiniteOrderedBounds(worldMins, worldMaxs))
		{
			reason = "invalid_world_bounds";
			return false;
		}

		vector origin = root.GetOrigin();
		entry = new ME_VehicleBoundsSnapshotEntry();
		entry.m_sPrefab = prefabPath;
		entry.m_vLocalMins = worldMins - origin;
		entry.m_vLocalMaxs = worldMaxs - origin;
		entry.m_sFixtureEntityName = root.GetName();
		if (!ME_IsFiniteOrderedBounds(entry.m_vLocalMins, entry.m_vLocalMaxs))
		{
			reason = "invalid_local_bounds";
			entry = null;
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Rejects unordered, NaN, or unreasonably large vectors before they enter a persisted snapshot.
	//! Отклоняет неупорядоченные, NaN или неоправданно большие vectors до их попадания в сохранённый snapshot.
	protected bool ME_IsFiniteOrderedBounds(vector mins, vector maxs)
	{
		for (int axis = 0; axis < 3; axis++)
		{
			if (mins[axis] != mins[axis] || maxs[axis] != maxs[axis])
				return false;

			if (Math.AbsFloat(mins[axis]) > 1000000 || Math.AbsFloat(maxs[axis]) > 1000000 || mins[axis] > maxs[axis])
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Saves the staged resource, rebuilds it, and compares every reloaded entry with the in-memory model.
	//! Сохраняет staged-ресурс, пересобирает его и сравнивает каждую перезагруженную запись с моделью в памяти.
	protected bool ME_SaveAndValidateStagedSnapshot(ME_VehicleBoundsSnapshot snapshot, out string reason)
	{
		reason = "";
		Resource containerResource = BaseContainerTools.CreateContainerFromInstance(snapshot);
		if (!containerResource)
		{
			reason = "create_container_failed";
			return false;
		}

		string absolutePath;
		if (!Workbench.GetAbsolutePath(STAGED_PATH, absolutePath, false))
		{
			reason = "absolute_path_failed";
			return false;
		}

		BaseContainer container = containerResource.GetResource().ToBaseContainer();
		if (!container || !BaseContainerTools.SaveContainer(container, STAGED_RESOURCE, absolutePath))
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

		Resource loaded = Resource.Load(STAGED_RESOURCE);
		if (!loaded || !loaded.IsValid())
		{
			reason = "resource_load_failed";
			return false;
		}

		BaseContainer loadedContainer = loaded.GetResource().ToBaseContainer();
		ME_VehicleBoundsSnapshot reloaded;
		if (!loadedContainer || !ME_ReadSnapshot(loadedContainer, reloaded) || !ME_SnapshotsMatch(snapshot, reloaded))
		{
			reason = "reload_validation_failed";
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Deserializes a snapshot from a loaded typed config container.
	//! Десериализует snapshot из загруженного typed config container.
	protected bool ME_ReadSnapshot(BaseContainer container, out ME_VehicleBoundsSnapshot snapshot)
	{
		snapshot = ME_VehicleBoundsSnapshot.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
		return snapshot && snapshot.m_aEntries;
	}

	//------------------------------------------------------------------------------------------------
	//! Compares snapshot metadata and ordered entries after config serialization.
	//! Bounds use a one-millimetre tolerance because the config format persists vectors to three decimal places.
	//! Сравнивает metadata snapshot и упорядоченные записи после сериализации config.
	//! Для границ используется допуск в один миллиметр, поскольку config-формат сохраняет vectors с тремя знаками после запятой.
	protected bool ME_SnapshotsMatch(ME_VehicleBoundsSnapshot expected, ME_VehicleBoundsSnapshot actual)
	{
		if (!actual || actual.m_iSchemaVersion != expected.m_iSchemaVersion || actual.m_sGeneratorVersion != expected.m_sGeneratorVersion || actual.m_sFixtureIdentity != expected.m_sFixtureIdentity || actual.m_aEntries.Count() != expected.m_aEntries.Count())
			return false;

		for (int index = 0; index < expected.m_aEntries.Count(); index++)
		{
			ME_VehicleBoundsSnapshotEntry expectedEntry = expected.m_aEntries[index];
			ME_VehicleBoundsSnapshotEntry actualEntry = actual.m_aEntries[index];
			if (!expectedEntry || !actualEntry || actualEntry.m_sPrefab != expectedEntry.m_sPrefab || !ME_AreSerializedVectorsEqual(actualEntry.m_vLocalMins, expectedEntry.m_vLocalMins) || !ME_AreSerializedVectorsEqual(actualEntry.m_vLocalMaxs, expectedEntry.m_vLocalMaxs) || actualEntry.m_sFixtureEntityName != expectedEntry.m_sFixtureEntityName)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Compares vectors after three-decimal config serialization.
	//! Сравнивает vectors после сериализации config с тремя знаками после запятой.
	protected bool ME_AreSerializedVectorsEqual(vector actual, vector expected)
	{
		for (int axis = 0; axis < 3; axis++)
		{
			if (Math.AbsFloat(actual[axis] - expected[axis]) > 0.001)
				return false;
		}

		return true;
	}
}
