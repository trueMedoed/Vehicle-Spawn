//! Workbench selection adapter for the validated ambient vehicle spawn-point envelope preview.
//! Адаптер selection Workbench для проверенного предпросмотра envelope точки появления ambient-техники.

//------------------------------------------------------------------------------------------------
class ME_AmbientVehicleEnvelopePreviewHelper
{
	//------------------------------------------------------------------------------------------------
	//! Validates and refreshes the envelope for exactly one selected ambient spawn point.
	//! Проверяет и обновляет envelope для ровно одной выбранной ambient spawn-point.
	static bool ME_ShowSelectedEnvelope(WorldEditorAPI api, out string reason, out string entityName, out int candidateCount, out vector aggregateMins, out vector aggregateMaxs)
	{
		reason = "";
		entityName = "<none>";
		candidateCount = 0;
		aggregateMins = vector.Zero;
		aggregateMaxs = vector.Zero;

		if (!api)
		{
			reason = "world_editor_api_unavailable";
			return false;
		}

		IEntity selectedEntity;
		SCR_AmbientVehicleSpawnPointComponent spawnPoint;
		if (!ME_GetSelectedSpawnPoint(api, selectedEntity, spawnPoint, reason))
			return false;

		entityName = selectedEntity.GetName();
		array<string> candidatePaths;
		if (!spawnPoint.ME_GetEditorVehicleEnvelopeCandidatePaths(candidatePaths, reason))
			return false;

		if (!ME_VehicleBoundsSnapshotHelper.ME_GetValidatedAggregateBounds(candidatePaths, aggregateMins, aggregateMaxs, reason))
			return false;

		candidateCount = candidatePaths.Count();
		spawnPoint.ME_ShowEditorVehicleEnvelopePreview(aggregateMins, aggregateMaxs);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Resolves exactly one selected ambient vehicle spawn point from the World Editor selection.
	//! Разрешает ровно одну выбранную ambient vehicle spawn-point из selection World Editor.
	protected static bool ME_GetSelectedSpawnPoint(WorldEditorAPI api, out IEntity selectedEntity, out SCR_AmbientVehicleSpawnPointComponent spawnPoint, out string reason)
	{
		selectedEntity = null;
		spawnPoint = null;
		reason = "";
		int selectedCount = api.GetSelectedEntitiesCount();
		if (selectedCount != 1)
		{
			reason = string.Format("expected_exactly_one_selected_entity selectedCount=%1", selectedCount);
			return false;
		}

		IEntitySource selectedSource = api.GetSelectedEntity();
		if (!selectedSource)
		{
			reason = "selected_entity_source_unavailable";
			return false;
		}

		selectedEntity = api.SourceToEntity(selectedSource);
		if (!selectedEntity)
		{
			reason = "selected_entity_unavailable";
			return false;
		}

		spawnPoint = SCR_AmbientVehicleSpawnPointComponent.Cast(selectedEntity.FindComponent(SCR_AmbientVehicleSpawnPointComponent));
		if (!spawnPoint)
		{
			reason = "selected_entity_is_not_ambient_spawn_point";
			return false;
		}

		return true;
	}
}
