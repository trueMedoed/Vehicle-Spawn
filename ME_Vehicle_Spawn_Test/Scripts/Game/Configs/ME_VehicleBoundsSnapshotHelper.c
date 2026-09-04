//! Read-only validation of the published vehicle-bounds snapshot for ambient vehicle envelope previews.
//! Проверка только для чтения опубликованного snapshot границ техники для предпросмотров envelope ambient-техники.

//------------------------------------------------------------------------------------------------
class ME_VehicleBoundsSnapshotHelper
{
	protected const int SNAPSHOT_SCHEMA_VERSION = 2;
	protected const string SNAPSHOT_GENERATOR_VERSION = "fixture-generator-v2";
	protected const string SNAPSHOT_FIXTURE_IDENTITY = "ME_VehicleBoundsSnapshot";
	protected static const ResourceName SNAPSHOT_RESOURCE = "{1C3AE4A8F2630BF7}Configs/Generated/ME_VehicleBoundsSnapshot.conf";

	//------------------------------------------------------------------------------------------------
	//! Validates the published snapshot and returns conservative union bounds for every candidate prefab.
	//!
	//! \param[in] candidatePaths Canonical catalog prefab paths requiring complete snapshot coverage
	//! \param[out] aggregateMins Conservative local minimum corner across all candidates
	//! \param[out] aggregateMaxs Conservative local maximum corner across all candidates
	//! \param[out] reason Stable failure reason when validation or coverage is incomplete
	//! \return True only when every candidate has a valid snapshot entry
	//! Проверяет опубликованный snapshot и возвращает консервативные union-границы для каждого prefab-кандидата.
	//!
	//! \param[in] candidatePaths Канонические пути prefab из каталога, требующие полного покрытия snapshot
	//! \param[out] aggregateMins Консервативный локальный минимальный угол для всех кандидатов
	//! \param[out] aggregateMaxs Консервативный локальный максимальный угол для всех кандидатов
	//! \param[out] reason Стабильная причина ошибки, когда validation или coverage неполны
	//! \return True, только когда каждый кандидат имеет корректную запись snapshot
	static bool ME_GetValidatedAggregateBounds(array<string> candidatePaths, out vector aggregateMins, out vector aggregateMaxs, out string reason)
	{
		aggregateMins = vector.Zero;
		aggregateMaxs = vector.Zero;
		reason = "";
		if (!candidatePaths || candidatePaths.IsEmpty())
		{
			reason = "empty_filtered_catalog";
			return false;
		}

		ME_VehicleBoundsSnapshot snapshot;
		if (!ME_LoadSnapshot(snapshot, reason))
			return false;

		return ME_GetAggregateBounds(snapshot, candidatePaths, aggregateMins, aggregateMaxs, reason);
	}

	//------------------------------------------------------------------------------------------------
	//! Loads the published typed snapshot resource and validates its schema, coverage, geometry, and prefab-path uniqueness.
	//! Загружает опубликованный typed snapshot-ресурс и проверяет его schema, покрытие, геометрию и уникальность путей prefab.
	protected static bool ME_LoadSnapshot(out ME_VehicleBoundsSnapshot snapshot, out string reason)
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
	protected static bool ME_IsValidSnapshotEntry(ME_VehicleBoundsSnapshotEntry entry, inout array<string> registeredPaths, out string reason)
	{
		reason = "";
		if (!entry || entry.m_sPrefab.IsEmpty())
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
	//! Rejects unordered, NaN, and unreasonably large bounds before they are used for a preview.
	//! Отклоняет неупорядоченные, NaN и неоправданно большие границы до их использования в preview.
	protected static bool ME_AreFiniteOrderedBounds(vector mins, vector maxs)
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
	protected static bool ME_GetAggregateBounds(ME_VehicleBoundsSnapshot snapshot, array<string> candidatePaths, out vector aggregateMins, out vector aggregateMaxs, out string reason)
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
	protected static ME_VehicleBoundsSnapshotEntry ME_FindSnapshotEntry(ME_VehicleBoundsSnapshot snapshot, string prefabPath)
	{
		foreach (ME_VehicleBoundsSnapshotEntry entry : snapshot.m_aEntries)
		{
			if (entry.m_sPrefab == prefabPath)
				return entry;
		}

		return null;
	}
}
