//! Workbench-only diagnostic that verifies whether a selected ambient vehicle spawn point exposes safe prefab bounds.
//! It enumerates the same filtered catalog candidates as vanilla selection without spawning temporary vehicle entities.
//! Диагностика только для Workbench, проверяющая, предоставляет ли выбранная точка появления ambient-техники безопасные границы prefab.
//! Она перечисляет те же отфильтрованные кандидаты каталога, что и ванильный выбор, не создавая временные сущности техники.

//------------------------------------------------------------------------------------------------
//! Reports the safe prefab-bounds API status for exactly one selected ambient vehicle spawn point.
//! The current engine API exposes bounds only through instantiated entities; this harness intentionally does not
//! instantiate candidates in the editable world because that would run vehicle initialization and cannot be isolated.
//! Сообщает статус безопасного API границ prefab для ровно одной выбранной точки появления ambient-техники.
//! Текущий API движка предоставляет границы только через созданные сущности; этот harness намеренно не создаёт
//! кандидаты в редактируемом мире, так как это запустило бы инициализацию техники и не может быть изолировано.
[WorkbenchPluginAttribute(name: "Diagnose ambient vehicle prefab bounds", description: "Lists selected ambient spawn-point catalog candidates without creating vehicle entities.", wbModules: { "WorldEditor" })]
class ME_AmbientVehiclePrefabBoundsDiagnosticPlugin : WorldEditorPlugin
{
	//------------------------------------------------------------------------------------------------
	//! Inspects the currently selected spawn point and reports whether its complete prefab bounds can be measured safely.
	//! Проверяет текущую выбранную точку появления и сообщает, можно ли безопасно измерить полные границы её prefab.
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_DEBUG_AVSP_WB] prefab_bounds status=UNVERIFIABLE reason=world_editor_unavailable");
			return;
		}

		WorldEditorAPI api = worldEditor.GetApi();
		if (!api)
		{
			Print("[ME_DEBUG_AVSP_WB] prefab_bounds status=UNVERIFIABLE reason=world_editor_api_unavailable");
			return;
		}

		int selectedCount = api.GetSelectedEntitiesCount();
		if (selectedCount != 1)
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] prefab_bounds status=UNVERIFIABLE reason=expected_exactly_one_selected_entity selectedCount=%1", selectedCount);
			return;
		}

		IEntitySource selectedSource = api.GetSelectedEntity();
		IEntity selectedEntity = api.SourceToEntity(selectedSource);
		if (!selectedEntity)
		{
			Print("[ME_DEBUG_AVSP_WB] prefab_bounds status=UNVERIFIABLE reason=selected_entity_unavailable");
			return;
		}

		SCR_AmbientVehicleSpawnPointComponent spawnPoint = SCR_AmbientVehicleSpawnPointComponent.Cast(selectedEntity.FindComponent(SCR_AmbientVehicleSpawnPointComponent));
		if (!spawnPoint)
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] prefab_bounds status=UNVERIFIABLE reason=selected_entity_is_not_ambient_spawn_point entity=%1", selectedEntity.GetName());
			return;
		}

		array<SCR_EntityCatalogEntry> entries;
		string reason;
		if (!spawnPoint.ME_GetEditorVehicleEnvelopeCandidates(entries, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] prefab_bounds status=UNVERIFIABLE reason=%1 entity=%2", reason, selectedEntity.GetName());
			return;
		}

		if (entries.IsEmpty())
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] prefab_bounds status=UNVERIFIABLE reason=empty_filtered_catalog entity=%1", selectedEntity.GetName());
			return;
		}

		foreach (SCR_EntityCatalogEntry entry: entries)
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] prefab_bounds candidate=%1 measurement=UNAVAILABLE reason=no_safe_prefab_or_isolated_world_bounds_api", entry.GetPrefab());
		}

		PrintFormat("[ME_DEBUG_AVSP_WB] prefab_bounds status=UNVERIFIABLE reason=no_safe_prefab_or_isolated_world_bounds_api entity=%1 candidateCount=%2", selectedEntity.GetName(), entries.Count());
	}
}
