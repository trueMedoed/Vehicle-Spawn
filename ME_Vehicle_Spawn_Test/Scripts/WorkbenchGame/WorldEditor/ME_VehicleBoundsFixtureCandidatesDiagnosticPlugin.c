//! Test-only Workbench diagnostic that reports canonical catalog candidates for the dedicated vehicle-bounds fixture.
//! Диагностика Workbench только для Test, выводящая канонические кандидаты каталога для выделенного fixture границ техники.

//------------------------------------------------------------------------------------------------
//! Enumerates the faction-filtered catalog candidates of the named fixture spawn points without changing the world.
//! Перечисляет отфильтрованные по faction кандидаты каталога именованных fixture spawn-point без изменения мира.
[WorkbenchPluginAttribute(name: "Diagnose vehicle bounds fixture candidates", description: "Lists canonical vehicle candidates for the dedicated bounds fixture.", wbModules: { "WorldEditor" })]
class ME_VehicleBoundsFixtureCandidatesDiagnosticPlugin : WorldEditorPlugin
{
	protected const string US_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_US";
	protected const string USSR_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_USSR";
	protected const string US_ARMED_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_US_Armed";
	protected const string USSR_ARMED_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_USSR_Armed";
	protected const string CIV_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_CIV";
	protected const string FIA_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_FIA";
	protected const string FIA_ARMED_SPAWN_POINT_NAME = "AmbientVehicleSpawnPoint_FIA_Armed";

	//------------------------------------------------------------------------------------------------
	//! Reports candidates for every required faction fixture spawn point.
	//! Выводит кандидаты для каждой обязательной faction fixture spawn-point.
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_DEBUG_AVSP_WB] bounds_fixture_candidates status=FAIL reason=world_editor_unavailable");
			return;
		}

		WorldEditorAPI api = worldEditor.GetApi();
		if (!api)
		{
			Print("[ME_DEBUG_AVSP_WB] bounds_fixture_candidates status=FAIL reason=world_editor_api_unavailable");
			return;
		}

		if (!ME_ReportCandidates(api, US_SPAWN_POINT_NAME) || !ME_ReportCandidates(api, USSR_SPAWN_POINT_NAME) || !ME_ReportCandidates(api, US_ARMED_SPAWN_POINT_NAME) || !ME_ReportCandidates(api, USSR_ARMED_SPAWN_POINT_NAME) || !ME_ReportCandidates(api, CIV_SPAWN_POINT_NAME) || !ME_ReportCandidates(api, FIA_SPAWN_POINT_NAME) || !ME_ReportCandidates(api, FIA_ARMED_SPAWN_POINT_NAME))
			return;

		Print("[ME_DEBUG_AVSP_WB] bounds_fixture_candidates status=PASS");
	}

	//------------------------------------------------------------------------------------------------
	//! Resolves one fixture spawn point and prints its canonical catalog prefab paths.
	//! Находит одну fixture spawn-point и выводит её канонические пути prefab каталога.
	protected bool ME_ReportCandidates(WorldEditorAPI api, string entityName)
	{
		IEntitySource source = api.FindEntityByName(entityName);
		IEntity entity = api.SourceToEntity(source);
		if (!entity)
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_candidates status=FAIL reason=spawn_point_unavailable entity=%1", entityName);
			return false;
		}

		SCR_AmbientVehicleSpawnPointComponent spawnPoint = SCR_AmbientVehicleSpawnPointComponent.Cast(entity.FindComponent(SCR_AmbientVehicleSpawnPointComponent));
		if (!spawnPoint)
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_candidates status=FAIL reason=component_unavailable entity=%1", entityName);
			return false;
		}

		array<string> paths;
		string reason;
		if (!spawnPoint.ME_GetEditorVehicleEnvelopeCandidatePaths(paths, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_candidates status=FAIL reason=%1 entity=%2", reason, entityName);
			return false;
		}

		if (paths.IsEmpty())
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_candidates status=FAIL reason=empty_filtered_catalog entity=%1", entityName);
			return false;
		}

		if (entityName == FIA_SPAWN_POINT_NAME || entityName == FIA_ARMED_SPAWN_POINT_NAME)
		{
			array<SCR_EntityCatalogEntry> entries;
			if (!spawnPoint.ME_GetEditorVehicleEnvelopeCandidates(entries, reason))
			{
				PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_candidates status=FAIL reason=%1 entity=%2", reason, entityName);
				return false;
			}

			bool hasArmedCandidate;
			foreach (SCR_EntityCatalogEntry entry : entries)
			{
				bool armed = entry.HasEditableEntityLabel(EEditableEntityLabel.TRAIT_ARMED);
				PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_candidate_labels entity=%1 prefab=%2 traitArmed=%3", entityName, entry.GetPrefab(), armed);
				if (armed)
					hasArmedCandidate = true;

				if (entityName == FIA_SPAWN_POINT_NAME && armed)
				{
					PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_candidates status=FAIL reason=armed_candidate entity=%1 prefab=%2", entityName, entry.GetPrefab());
					return false;
				}
			}

			if (entityName == FIA_ARMED_SPAWN_POINT_NAME && !hasArmedCandidate)
			{
				PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_candidates status=FAIL reason=no_armed_candidate entity=%1", entityName);
				return false;
			}
		}

		paths.Sort();
		foreach (string path : paths)
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_candidate entity=%1 prefab=%2", entityName, path);

		return true;
	}
}
