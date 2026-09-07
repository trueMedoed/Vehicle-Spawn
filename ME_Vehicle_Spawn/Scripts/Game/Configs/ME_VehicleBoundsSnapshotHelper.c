//! Read-only validation of published faction and vehicle-type bounds aggregates for ambient vehicle envelope previews.

//------------------------------------------------------------------------------------------------
class ME_VehicleBoundsSnapshotHelper
{
	static const string ME_GLOBAL_VEHICLE_CATALOG_SCOPE = "__ME_GLOBAL_VEHICLE_CATALOG__";
	protected const int SNAPSHOT_SCHEMA_VERSION = 4;
	protected const string SNAPSHOT_GENERATOR_VERSION = "catalog-aggregate-generator-v4-provenance";
	protected static const ResourceName SNAPSHOT_RESOURCE = "{E3738ADB51674DA6}Configs/Generated/ME_VehicleBoundsSnapshot.conf";

	//------------------------------------------------------------------------------------------------
	//! Validates the published snapshot and unions the selected aggregates for one resolved faction.
	//!
	//! \param[in] factionKey Resolved faction key whose aggregates are required
	//! \param[in] vehicleTypeNames Unique VEHICLE_* label names selected by the catalog filter
	//! \param[out] aggregateMins Conservative local minimum corner across selected aggregates
	//! \param[out] aggregateMaxs Conservative local maximum corner across selected aggregates
	//! \param[out] reason Stable failure reason when validation or aggregate coverage is incomplete
	//! \return True only when every selected type has a valid aggregate for the resolved faction
	static bool ME_GetValidatedAggregateBounds(string factionKey, array<string> vehicleTypeNames, out vector aggregateMins, out vector aggregateMaxs, out string reason)
	{
		aggregateMins = vector.Zero;
		aggregateMaxs = vector.Zero;
		reason = "";
		if (factionKey.IsEmpty())
		{
			reason = "resolved_faction_key_empty";
			return false;
		}

		if (!vehicleTypeNames || vehicleTypeNames.IsEmpty())
		{
			reason = "filtered_vehicle_types_empty";
			return false;
		}

		ME_VehicleBoundsSnapshot snapshot;
		if (!ME_LoadSnapshot(snapshot, reason))
			return false;

		return ME_GetAggregateBounds(snapshot, factionKey, vehicleTypeNames, aggregateMins, aggregateMaxs, reason);
	}

	//------------------------------------------------------------------------------------------------
	//! Loads the typed snapshot resource and validates its schema, metadata, geometry, and aggregate uniqueness.
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

		if (snapshot.m_iSchemaVersion != SNAPSHOT_SCHEMA_VERSION || snapshot.m_sGeneratorVersion != SNAPSHOT_GENERATOR_VERSION)
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

		array<string> registeredKeys = {};
		foreach (ME_VehicleBoundsSnapshotEntry entry : snapshot.m_aEntries)
		{
			if (!ME_IsValidSnapshotEntry(entry, registeredKeys, reason))
			{
				snapshot = null;
				return false;
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Validates one faction and vehicle-type aggregate before it can participate in a preview.
	protected static bool ME_IsValidSnapshotEntry(ME_VehicleBoundsSnapshotEntry entry, inout array<string> registeredKeys, out string reason)
	{
		reason = "";
		if (!entry || entry.m_sFactionKey.IsEmpty() || entry.m_sVehicleType.IsEmpty())
		{
			reason = "snapshot_entry_identity_invalid";
			return false;
		}

		string compositeKey = ME_GetCompositeKey(entry.m_sFactionKey, entry.m_sVehicleType);
		if (registeredKeys.Contains(compositeKey))
		{
			reason = string.Format("snapshot_entry_duplicate faction=%1 type=%2", entry.m_sFactionKey, entry.m_sVehicleType);
			return false;
		}

		if (entry.m_iCandidateCount <= 0)
		{
			reason = string.Format("snapshot_entry_candidate_count_invalid faction=%1 type=%2", entry.m_sFactionKey, entry.m_sVehicleType);
			return false;
		}

		if (entry.m_sMinXSourcePrefab.IsEmpty() || entry.m_sMaxXSourcePrefab.IsEmpty() || entry.m_sMinYSourcePrefab.IsEmpty() || entry.m_sMaxYSourcePrefab.IsEmpty() || entry.m_sMinZSourcePrefab.IsEmpty() || entry.m_sMaxZSourcePrefab.IsEmpty())
		{
			reason = string.Format("snapshot_entry_source_prefab_invalid faction=%1 type=%2", entry.m_sFactionKey, entry.m_sVehicleType);
			return false;
		}

		if (!ME_AreFiniteOrderedBounds(entry.m_vLocalMins, entry.m_vLocalMaxs))
		{
			reason = string.Format("snapshot_entry_bounds_invalid faction=%1 type=%2", entry.m_sFactionKey, entry.m_sVehicleType);
			return false;
		}

		registeredKeys.Insert(compositeKey);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Rejects unordered, NaN, and unreasonably large bounds before they are used for a preview.
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
	//! Requires every selected vehicle type to have a same-faction aggregate and returns their union.
	protected static bool ME_GetAggregateBounds(ME_VehicleBoundsSnapshot snapshot, string factionKey, array<string> vehicleTypeNames, out vector aggregateMins, out vector aggregateMaxs, out string reason)
	{
		aggregateMins = vector.Zero;
		aggregateMaxs = vector.Zero;
		reason = "";
		bool hasBounds;
		array<string> requestedTypes = {};
		foreach (string vehicleType : vehicleTypeNames)
		{
			if (vehicleType.IsEmpty() || requestedTypes.Contains(vehicleType))
			{
				reason = "filtered_vehicle_types_invalid";
				return false;
			}
			requestedTypes.Insert(vehicleType);

			ME_VehicleBoundsSnapshotEntry entry = ME_FindSnapshotEntry(snapshot, factionKey, vehicleType);
			if (!entry)
			{
				reason = string.Format("snapshot_aggregate_missing faction=%1 type=%2", factionKey, vehicleType);
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
	//! Finds one validated aggregate without falling back to a different faction.
	protected static ME_VehicleBoundsSnapshotEntry ME_FindSnapshotEntry(ME_VehicleBoundsSnapshot snapshot, string factionKey, string vehicleType)
	{
		foreach (ME_VehicleBoundsSnapshotEntry entry : snapshot.m_aEntries)
		{
			if (entry.m_sFactionKey == factionKey && entry.m_sVehicleType == vehicleType)
				return entry;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Builds an unambiguous key for aggregate uniqueness validation.
	protected static string ME_GetCompositeKey(string factionKey, string vehicleType)
	{
		return factionKey + "\x1F" + vehicleType;
	}
}
