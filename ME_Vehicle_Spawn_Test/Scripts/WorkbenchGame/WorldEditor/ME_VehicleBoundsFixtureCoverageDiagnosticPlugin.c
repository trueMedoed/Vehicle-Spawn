//! Test-only validation that the dedicated bounds fixture covers every current catalog candidate exactly once.
//! Проверка только для Test, что выделенный fixture границ покрывает каждого текущего кандидата каталога ровно один раз.

//------------------------------------------------------------------------------------------------
//! Compares marked fixture vehicle roots with the non-mutating candidate output of all fixture spawn points.
//! Сравнивает помеченные корни техники fixture с немутирующим выводом кандидатов всех spawn-point fixture.
[WorkbenchPluginAttribute(name: "Validate vehicle bounds fixture coverage", description: "Checks exact catalog-to-marker coverage for the dedicated bounds fixture.", wbModules: { "WorldEditor" })]
class ME_VehicleBoundsFixtureCoverageDiagnosticPlugin : WorldEditorPlugin
{
	protected const string US_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_US";
	protected const string USSR_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_USSR";
	protected const string US_ARMED_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_US_Armed";
	protected const string USSR_ARMED_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_USSR_Armed";
	protected const string CIV_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_CIV";

	//------------------------------------------------------------------------------------------------
	//! Validates candidate and marker coverage without changing the world or spawning a prefab.
	//! Проверяет покрытие кандидатов и markers без изменения мира либо создания prefab.
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_DEBUG_AVSP_WB] bounds_fixture_coverage status=FAIL reason=world_editor_unavailable");
			return;
		}

		WorldEditorAPI api = worldEditor.GetApi();
		if (!api)
		{
			Print("[ME_DEBUG_AVSP_WB] bounds_fixture_coverage status=FAIL reason=world_editor_api_unavailable");
			return;
		}

		array<string> expectedPaths = {};
		string reason;
		if (!ME_CollectCandidatePaths(api, US_SPAWN_POINT_NAME, expectedPaths, reason)
			|| !ME_CollectCandidatePaths(api, USSR_SPAWN_POINT_NAME, expectedPaths, reason)
			|| !ME_CollectCandidatePaths(api, US_ARMED_SPAWN_POINT_NAME, expectedPaths, reason)
			|| !ME_CollectCandidatePaths(api, USSR_ARMED_SPAWN_POINT_NAME, expectedPaths, reason)
			|| !ME_CollectCandidatePaths(api, CIV_SPAWN_POINT_NAME, expectedPaths, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_coverage status=FAIL reason=%1", reason);
			return;
		}

		array<string> markerPaths;
		if (!ME_CollectMarkerPaths(api, markerPaths, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_coverage status=FAIL reason=%1", reason);
			return;
		}

		expectedPaths.Sort();
		markerPaths.Sort();
		if (expectedPaths.Count() != markerPaths.Count())
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_coverage status=FAIL reason=count_mismatch expected=%1 markers=%2", expectedPaths.Count(), markerPaths.Count());
			return;
		}

		for (int index = 0; index < expectedPaths.Count(); index++)
		{
			if (expectedPaths[index] != markerPaths[index])
			{
				PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_coverage status=FAIL reason=path_mismatch expected=%1 marker=%2", expectedPaths[index], markerPaths[index]);
				return;
			}
		}

		PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_coverage status=PASS count=%1", expectedPaths.Count());
	}

	//------------------------------------------------------------------------------------------------
	//! Appends one fixture spawn point's unique canonical candidate paths to the aggregate set.
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
	//! Collects the unique declared prefab paths from all fixture marker components in the editor.
	//! Собирает уникальные объявленные пути prefab из всех marker-компонентов fixture в редакторе.
	protected bool ME_CollectMarkerPaths(WorldEditorAPI api, out array<string> markerPaths, out string reason)
	{
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

			markerPaths.Insert(path);
		}

		if (markerPaths.IsEmpty())
		{
			reason = "no_fixture_markers";
			return false;
		}

		return true;
	}
}
