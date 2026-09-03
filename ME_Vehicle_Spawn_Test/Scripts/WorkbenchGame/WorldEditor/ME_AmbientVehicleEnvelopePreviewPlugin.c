//! Test-only Workbench preview for the conservative vehicle envelope of one selected ambient spawn point.
//! Предпросмотр только для Test в Workbench консервативного vehicle-envelope одной выбранной ambient spawn-point.

//------------------------------------------------------------------------------------------------
//! Renders one orientation-aligned worst-case snapshot box covering every catalog candidate of the selected spawn point.
//! Draws nothing when snapshot coverage is incomplete, so a partial envelope is never presented as verified.
//! Отрисовывает один ориентированный по осям worst-case snapshot-box, покрывающий каждого кандидата каталога выбранной spawn-point.
//! Ничего не рисует при неполном покрытии snapshot, поэтому частичный envelope никогда не выдаётся за проверенный.
[WorkbenchPluginAttribute(name: "Preview ambient vehicle envelope", description: "Shows the conservative snapshot envelope for one selected ambient vehicle spawn point.", wbModules: { "WorldEditor" })]
class ME_AmbientVehicleEnvelopePreviewPlugin : WorldEditorPlugin
{
	protected const int SNAPSHOT_SCHEMA_VERSION = 2;
	protected const string SNAPSHOT_GENERATOR_VERSION = "fixture-generator-v2";
	protected const string SNAPSHOT_FIXTURE_IDENTITY = "ME_VehicleBoundsSnapshot";
	protected static const ResourceName SNAPSHOT_RESOURCE = "{E3738ADB51674DA6}Configs/Generated/ME_VehicleBoundsSnapshot.conf";

	//------------------------------------------------------------------------------------------------
	//! Clears any previous preview, validates the selected point and snapshot coverage, then shows one aggregate envelope.
	//! Очищает предыдущий preview, проверяет выбранную точку и покрытие snapshot, затем показывает один aggregate-envelope.
	override void Run()
	{
		SCR_AmbientVehicleSpawnPointComponent.ME_ClearActiveEditorVehicleEnvelopePreview();

		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_DEBUG_AVSP_WB] status=UNVERIFIABLE operation=ambient_vehicle_envelope_preview reason=world_editor_unavailable entity=<none>");
			return;
		}

		WorldEditorAPI api = worldEditor.GetApi();
		if (!api)
		{
			Print("[ME_DEBUG_AVSP_WB] status=UNVERIFIABLE operation=ambient_vehicle_envelope_preview reason=world_editor_api_unavailable entity=<none>");
			return;
		}

		IEntity selectedEntity;
		SCR_AmbientVehicleSpawnPointComponent spawnPoint;
		string reason;
		if (!ME_GetSelectedSpawnPoint(api, selectedEntity, spawnPoint, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] status=UNVERIFIABLE operation=ambient_vehicle_envelope_preview reason=%1 entity=<none>", reason);
			return;
		}

		array<string> candidatePaths;
		if (!spawnPoint.ME_GetEditorVehicleEnvelopeCandidatePaths(candidatePaths, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] status=UNVERIFIABLE operation=ambient_vehicle_envelope_preview reason=%1 entity=%2", reason, selectedEntity.GetName());
			return;
		}

		if (candidatePaths.IsEmpty())
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] status=UNVERIFIABLE operation=ambient_vehicle_envelope_preview reason=empty_filtered_catalog entity=%1", selectedEntity.GetName());
			return;
		}

		ME_VehicleBoundsSnapshot snapshot;
		if (!ME_LoadSnapshot(snapshot, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] status=UNVERIFIABLE operation=ambient_vehicle_envelope_preview reason=%1 entity=%2", reason, selectedEntity.GetName());
			return;
		}

		vector aggregateMins;
		vector aggregateMaxs;
		if (!ME_GetAggregateBounds(snapshot, candidatePaths, aggregateMins, aggregateMaxs, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] status=UNVERIFIABLE operation=ambient_vehicle_envelope_preview reason=%1 entity=%2", reason, selectedEntity.GetName());
			return;
		}

		spawnPoint.ME_ShowEditorVehicleEnvelopePreview(aggregateMins, aggregateMaxs);
		PrintFormat("[ME_DEBUG_AVSP_WB] status=PASS operation=ambient_vehicle_envelope_preview entity=%1 candidateCount=%2 localMins=%3 localMaxs=%4 edgeCount=12", selectedEntity.GetName(), candidatePaths.Count(), aggregateMins, aggregateMaxs);
	}

	//------------------------------------------------------------------------------------------------
	//! Resolves exactly one selected ambient vehicle spawn point from the World Editor selection.
	//! Разрешает ровно одну выбранную ambient vehicle spawn-point из selection World Editor.
	protected bool ME_GetSelectedSpawnPoint(WorldEditorAPI api, out IEntity selectedEntity, out SCR_AmbientVehicleSpawnPointComponent spawnPoint, out string reason)
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

	//------------------------------------------------------------------------------------------------
	//! Loads the published typed snapshot resource and validates its schema, identity, and every entry.
	//! Загружает опубликованный typed snapshot-ресурс и проверяет его schema, identity и каждую запись.
	protected bool ME_LoadSnapshot(out ME_VehicleBoundsSnapshot snapshot, out string reason)
	{
		snapshot = null;
		reason = "";
		Resource resource = Resource.Load(SNAPSHOT_RESOURCE);
		if (!resource || !resource.IsValid())
		{
			reason = "snapshot_resource_load_failed";
			return false;
		}

		BaseContainer container = resource.GetResource().ToBaseContainer();
		if (!container)
		{
			reason = "snapshot_container_unavailable";
			return false;
		}

		snapshot = ME_VehicleBoundsSnapshot.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
		if (!snapshot || !snapshot.m_aEntries)
		{
			reason = "snapshot_deserialization_failed";
			return false;
		}

		if (snapshot.m_iSchemaVersion != SNAPSHOT_SCHEMA_VERSION || snapshot.m_sGeneratorVersion != SNAPSHOT_GENERATOR_VERSION || snapshot.m_sFixtureIdentity != SNAPSHOT_FIXTURE_IDENTITY)
		{
			reason = "snapshot_metadata_mismatch";
			snapshot = null;
			return false;
		}

		if (snapshot.m_aEntries.IsEmpty())
		{
			reason = "snapshot_entries_empty";
			snapshot = null;
			return false;
		}

		array<string> registeredPaths = {};
		foreach (ME_VehicleBoundsSnapshotEntry entry : snapshot.m_aEntries)
		{
			if (!ME_IsValidSnapshotEntry(entry, registeredPaths, reason))
			{
				snapshot = null;
				return false;
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Validates one snapshot entry before it can participate in aggregate preview bounds.
	//! Проверяет одну запись snapshot перед её участием в aggregate preview-границах.
	protected bool ME_IsValidSnapshotEntry(ME_VehicleBoundsSnapshotEntry entry, inout array<string> registeredPaths, out string reason)
	{
		reason = "";
		if (!entry || entry.m_sPrefab.IsEmpty() || entry.m_sFixtureEntityName.IsEmpty())
		{
			reason = "snapshot_entry_identity_invalid";
			return false;
		}

		if (registeredPaths.Contains(entry.m_sPrefab))
		{
			reason = string.Format("snapshot_entry_duplicate prefab=%1", entry.m_sPrefab);
			return false;
		}

		if (!ME_AreFiniteOrderedBounds(entry.m_vLocalMins, entry.m_vLocalMaxs))
		{
			reason = string.Format("snapshot_entry_bounds_invalid prefab=%1", entry.m_sPrefab);
			return false;
		}

		registeredPaths.Insert(entry.m_sPrefab);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Rejects unordered, NaN, and unreasonably large bounds before drawing them in Workbench.
	//! Отклоняет неупорядоченные, NaN и неоправданно большие границы перед их отрисовкой в Workbench.
	protected bool ME_AreFiniteOrderedBounds(vector mins, vector maxs)
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
	//! Requires complete snapshot coverage for all candidates and returns their conservative union bounds.
	//! Требует полного покрытия snapshot для всех кандидатов и возвращает их консервативные union-границы.
	protected bool ME_GetAggregateBounds(ME_VehicleBoundsSnapshot snapshot, array<string> candidatePaths, out vector aggregateMins, out vector aggregateMaxs, out string reason)
	{
		aggregateMins = vector.Zero;
		aggregateMaxs = vector.Zero;
		reason = "";
		bool hasBounds;
		foreach (string candidatePath : candidatePaths)
		{
			ME_VehicleBoundsSnapshotEntry entry = ME_FindSnapshotEntry(snapshot, candidatePath);
			if (!entry)
			{
				reason = string.Format("snapshot_entry_missing prefab=%1", candidatePath);
				return false;
			}

			if (!hasBounds)
			{
				aggregateMins = entry.m_vLocalMins;
				aggregateMaxs = entry.m_vLocalMaxs;
				hasBounds = true;
				continue;
			}

			for (int axis = 0; axis < 3; axis++)
			{
				aggregateMins[axis] = Math.Min(aggregateMins[axis], entry.m_vLocalMins[axis]);
				aggregateMaxs[axis] = Math.Max(aggregateMaxs[axis], entry.m_vLocalMaxs[axis]);
			}
		}

		if (!hasBounds || !ME_AreFiniteOrderedBounds(aggregateMins, aggregateMaxs))
		{
			reason = "aggregate_bounds_invalid";
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds the validated snapshot entry for one canonical catalog prefab path.
	//! Находит проверенную запись snapshot для одного канонического пути prefab каталога.
	protected ME_VehicleBoundsSnapshotEntry ME_FindSnapshotEntry(ME_VehicleBoundsSnapshot snapshot, string prefabPath)
	{
		foreach (ME_VehicleBoundsSnapshotEntry entry : snapshot.m_aEntries)
		{
			if (entry.m_sPrefab == prefabPath)
				return entry;
		}

		return null;
	}
}
