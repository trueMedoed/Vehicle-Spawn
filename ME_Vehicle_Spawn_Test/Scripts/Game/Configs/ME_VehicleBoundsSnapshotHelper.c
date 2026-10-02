//! Read-only validation of canonical faction and vehicle-type bounds aggregates for test envelope previews.
//! Проверка только для чтения канонических агрегатов границ по фракции и типу техники для test-preview envelope.

//------------------------------------------------------------------------------------------------
class ME_VehicleBoundsSnapshotHelper
{
	// Reserved non-empty scope key for the vanilla global VEHICLE catalog.
	// Зарезервированный непустой scope-key для ванильного глобального VEHICLE-каталога.
	static const string ME_GLOBAL_VEHICLE_CATALOG_SCOPE = "__ME_GLOBAL_VEHICLE_CATALOG__";
	protected const int SNAPSHOT_SCHEMA_VERSION = 6;
	protected const string SNAPSHOT_GENERATOR_VERSION = "catalog-aggregate-generator-v6-largest-footprint";
	protected static const ResourceName SNAPSHOT_RESOURCE = "{1C3AE4A8F2630BF7}Configs/Generated/ME_VehicleBoundsSnapshot.conf";

	//------------------------------------------------------------------------------------------------
	//! Validates the canonical snapshot and unions selected aggregates for one resolved faction.
	//! Проверяет канонический snapshot и объединяет выбранные агрегаты одной разрешённой фракции.
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
	//! Selects the largest vehicle type first, then checks its snapshot prefab against the point filter.
	//! On an excluded prefab, returns false but preserves selectedType for a same-type fallback.
	//! Сначала выбирает крупнейший тип, затем проверяет его prefab по фильтру точки.
	//! При исключённом prefab возвращает false, сохраняя selectedType для замены внутри типа.
	static bool ME_GetLargestFootprintPrefab(string factionKey, array<string> vehicleTypeNames, array<string> filteredPaths, out string prefabPath, out string selectedType, out string reason)
	{
		prefabPath = "";
		selectedType = "";
		reason = "";
		if (factionKey.IsEmpty() || !vehicleTypeNames || vehicleTypeNames.IsEmpty() || !filteredPaths || filteredPaths.IsEmpty())
		{
			reason = "snapshot_selection_input_invalid";
			return false;
		}
		ME_VehicleBoundsSnapshot snapshot;
		if (!ME_LoadSnapshot(snapshot, reason)) return false;
		ME_VehicleBoundsSnapshotFaction faction = ME_FindSnapshotFaction(snapshot, factionKey);
		if (!faction) { reason = "snapshot_faction_missing"; return false; }
		float bestArea = -1;
		float bestHeight = -1;
		foreach (string vehicleType : vehicleTypeNames)
		{
			ME_VehicleBoundsSnapshotEntry entry = ME_FindSnapshotEntry(faction, vehicleType);
			if (!entry) continue;
			if (entry.m_fLargestFootprintArea > bestArea || entry.m_fLargestFootprintArea == bestArea && (entry.m_fLargestFootprintHeight > bestHeight || entry.m_fLargestFootprintHeight == bestHeight && entry.m_sLargestFootprintSourcePrefab < prefabPath))
			{
				bestArea = entry.m_fLargestFootprintArea;
				bestHeight = entry.m_fLargestFootprintHeight;
				prefabPath = entry.m_sLargestFootprintSourcePrefab;
				selectedType = vehicleType;
			}
		}
		if (prefabPath.IsEmpty()) { reason = "snapshot_vehicle_types_missing"; return false; }
		if (!filteredPaths.Contains(prefabPath))
		{
			prefabPath = "";
			reason = "snapshot_largest_prefab_filtered_out";
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Picks a same-faction example when filtered candidates are absent or unavailable.
	//! Prefers the largest explicitly requested vehicle type, then the faction car; it is not selected by the point filter.
	//! Выбирает пример той же фракции, если кандидатов по фильтру нет или они недоступны.
	//! Сначала берёт крупнейший явно заданный тип техники, затем автомобиль фракции; пример не выбран фильтром точки.
	static bool ME_GetFactionFallbackPreviewPrefab(string factionKey, array<string> preferredTypes, out string prefabPath, out string selectedType, out string reason)
	{
		prefabPath = "";
		selectedType = "";
		reason = "";
		if (factionKey.IsEmpty()) { reason = "fallback_faction_key_empty"; return false; }
		ME_VehicleBoundsSnapshot snapshot;
		if (!ME_LoadSnapshot(snapshot, reason)) return false;
		ME_VehicleBoundsSnapshotFaction faction = ME_FindSnapshotFaction(snapshot, factionKey);
		if (!faction) { reason = "fallback_faction_missing"; return false; }

		float bestArea = -1;
		float bestHeight = -1;
		if (preferredTypes)
		{
			foreach (string preferredType : preferredTypes)
			{
				ME_VehicleBoundsSnapshotEntry preferredEntry = ME_FindSnapshotEntry(faction, preferredType);
				if (!preferredEntry) continue;
				if (preferredEntry.m_fLargestFootprintArea > bestArea || preferredEntry.m_fLargestFootprintArea == bestArea && (preferredEntry.m_fLargestFootprintHeight > bestHeight || preferredEntry.m_fLargestFootprintHeight == bestHeight && preferredEntry.m_sLargestFootprintSourcePrefab < prefabPath))
				{
					bestArea = preferredEntry.m_fLargestFootprintArea;
					bestHeight = preferredEntry.m_fLargestFootprintHeight;
					prefabPath = preferredEntry.m_sLargestFootprintSourcePrefab;
					selectedType = preferredType;
				}
			}
		}
		if (!prefabPath.IsEmpty()) return true;

		ME_VehicleBoundsSnapshotEntry carEntry = ME_FindSnapshotEntry(faction, "VEHICLE_CAR");
		if (!carEntry) { reason = "fallback_faction_car_missing"; return false; }
		prefabPath = carEntry.m_sLargestFootprintSourcePrefab;
		selectedType = carEntry.m_sVehicleType;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Loads and validates schema, metadata, faction groups, entries, ordering, and uniqueness.
	//! Загружает и проверяет schema, metadata, группы фракций, записи, порядок и уникальность.
	protected static bool ME_LoadSnapshot(out ME_VehicleBoundsSnapshot snapshot, out string reason)
	{
		snapshot = null;
		reason = "";
		Resource resource = Resource.Load(SNAPSHOT_RESOURCE);
		if (!resource || !resource.IsValid()) { reason = "snapshot_resource_load_failed"; return false; }
		BaseContainer container = resource.GetResource().ToBaseContainer();
		if (!container) { reason = "snapshot_container_unavailable"; return false; }
		snapshot = ME_VehicleBoundsSnapshot.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
		if (!snapshot || !snapshot.m_aFactions) { reason = "snapshot_deserialization_failed"; return false; }
		if (snapshot.m_iSchemaVersion != SNAPSHOT_SCHEMA_VERSION || snapshot.m_sGeneratorVersion != SNAPSHOT_GENERATOR_VERSION) { reason = "snapshot_metadata_mismatch"; snapshot = null; return false; }
		if (snapshot.m_aFactions.IsEmpty()) { reason = "snapshot_factions_empty"; snapshot = null; return false; }

		string previousFactionKey;
		foreach (ME_VehicleBoundsSnapshotFaction faction : snapshot.m_aFactions)
		{
			if (!ME_IsValidSnapshotFaction(faction, previousFactionKey, reason)) { snapshot = null; return false; }
			previousFactionKey = faction.m_sFactionKey;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Validates one sorted faction group and all of its sorted aggregate entries.
	//! Проверяет одну отсортированную группу фракции и все её отсортированные aggregate-записи.
	protected static bool ME_IsValidSnapshotFaction(ME_VehicleBoundsSnapshotFaction faction, string previousFactionKey, out string reason)
	{
		reason = "";
		if (!faction || faction.m_sFactionKey.IsEmpty() || !faction.m_aEntries) { reason = "snapshot_faction_identity_invalid"; return false; }
		if (!previousFactionKey.IsEmpty() && faction.m_sFactionKey <= previousFactionKey) { reason = string.Format("snapshot_faction_order_or_duplicate faction=%1", faction.m_sFactionKey); return false; }
		if (faction.m_aEntries.IsEmpty()) { reason = string.Format("snapshot_faction_entries_empty faction=%1", faction.m_sFactionKey); return false; }

		string previousVehicleType;
		foreach (ME_VehicleBoundsSnapshotEntry entry : faction.m_aEntries)
		{
			if (!ME_IsValidSnapshotEntry(entry, faction.m_sFactionKey, previousVehicleType, reason))
				return false;
			previousVehicleType = entry.m_sVehicleType;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Validates one aggregate entry before it participates in a preview.
	//! Проверяет одну aggregate-запись до её участия в preview.
	protected static bool ME_IsValidSnapshotEntry(ME_VehicleBoundsSnapshotEntry entry, string factionKey, string previousVehicleType, out string reason)
	{
		reason = "";
		if (!entry || entry.m_sVehicleType.IsEmpty()) { reason = string.Format("snapshot_entry_identity_invalid faction=%1", factionKey); return false; }
		if (!previousVehicleType.IsEmpty() && entry.m_sVehicleType <= previousVehicleType) { reason = string.Format("snapshot_entry_order_or_duplicate faction=%1 type=%2", factionKey, entry.m_sVehicleType); return false; }
		if (entry.m_iCandidateCount <= 0) { reason = string.Format("snapshot_entry_candidate_count_invalid faction=%1 type=%2", factionKey, entry.m_sVehicleType); return false; }
		if (entry.m_sMinXSourcePrefab.IsEmpty() || entry.m_sMaxXSourcePrefab.IsEmpty() || entry.m_sMinYSourcePrefab.IsEmpty() || entry.m_sMaxYSourcePrefab.IsEmpty() || entry.m_sMinZSourcePrefab.IsEmpty() || entry.m_sMaxZSourcePrefab.IsEmpty() || entry.m_sLargestFootprintSourcePrefab.IsEmpty()) { reason = string.Format("snapshot_entry_source_prefab_invalid faction=%1 type=%2", factionKey, entry.m_sVehicleType); return false; }
		if (!ME_AreFiniteOrderedBounds(entry.m_vLocalMins, entry.m_vLocalMaxs)) { reason = string.Format("snapshot_entry_bounds_invalid faction=%1 type=%2", factionKey, entry.m_sVehicleType); return false; }
		float aggregateArea = (entry.m_vLocalMaxs[0] - entry.m_vLocalMins[0]) * (entry.m_vLocalMaxs[2] - entry.m_vLocalMins[2]);
		float aggregateHeight = entry.m_vLocalMaxs[1] - entry.m_vLocalMins[1];
		if (entry.m_fLargestFootprintArea != entry.m_fLargestFootprintArea || entry.m_fLargestFootprintHeight != entry.m_fLargestFootprintHeight || entry.m_fLargestFootprintArea <= 0 || entry.m_fLargestFootprintArea > aggregateArea + 0.01 || entry.m_fLargestFootprintHeight <= 0 || entry.m_fLargestFootprintHeight > aggregateHeight + 0.01) { reason = string.Format("snapshot_entry_largest_footprint_invalid faction=%1 type=%2", factionKey, entry.m_sVehicleType); return false; }
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Rejects unordered, NaN, and unreasonable bounds.
	//! Отклоняет неупорядоченные, NaN и неоправданные границы.
	protected static bool ME_AreFiniteOrderedBounds(vector mins, vector maxs)
	{
		for (int axis = 0; axis < 3; axis++)
			if (mins[axis] != mins[axis] || maxs[axis] != maxs[axis] || Math.AbsFloat(mins[axis]) > 1000000 || Math.AbsFloat(maxs[axis]) > 1000000 || mins[axis] > maxs[axis]) return false;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Requires same-faction coverage for every selected vehicle type and returns their union.
	//! Требует покрытия той же фракции для каждого выбранного типа техники и возвращает их union.
	protected static bool ME_GetAggregateBounds(ME_VehicleBoundsSnapshot snapshot, string factionKey, array<string> vehicleTypeNames, out vector aggregateMins, out vector aggregateMaxs, out string reason)
	{
		aggregateMins = vector.Zero;
		aggregateMaxs = vector.Zero;
		reason = "";
		ME_VehicleBoundsSnapshotFaction faction = ME_FindSnapshotFaction(snapshot, factionKey);
		if (!faction) { reason = string.Format("snapshot_faction_missing faction=%1", factionKey); return false; }

		bool hasBounds;
		array<string> requestedTypes = {};
		foreach (string vehicleType : vehicleTypeNames)
		{
			if (vehicleType.IsEmpty() || requestedTypes.Contains(vehicleType)) { reason = "filtered_vehicle_types_invalid"; return false; }
			requestedTypes.Insert(vehicleType);
			ME_VehicleBoundsSnapshotEntry entry = ME_FindSnapshotEntry(faction, vehicleType);
			if (!entry) { reason = string.Format("snapshot_aggregate_missing faction=%1 type=%2", factionKey, vehicleType); return false; }
			if (!hasBounds) { aggregateMins = entry.m_vLocalMins; aggregateMaxs = entry.m_vLocalMaxs; hasBounds = true; continue; }
			for (int axis = 0; axis < 3; axis++) { aggregateMins[axis] = Math.Min(aggregateMins[axis], entry.m_vLocalMins[axis]); aggregateMaxs[axis] = Math.Max(aggregateMaxs[axis], entry.m_vLocalMaxs[axis]); }
		}
		if (!hasBounds || !ME_AreFiniteOrderedBounds(aggregateMins, aggregateMaxs)) { reason = "aggregate_bounds_invalid"; return false; }
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds one exact faction group without fallback.
	//! Находит одну точную группу фракции без fallback.
	protected static ME_VehicleBoundsSnapshotFaction ME_FindSnapshotFaction(ME_VehicleBoundsSnapshot snapshot, string factionKey)
	{
		foreach (ME_VehicleBoundsSnapshotFaction faction : snapshot.m_aFactions)
			if (faction.m_sFactionKey == factionKey) return faction;
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds one aggregate only inside the resolved faction group.
	//! Находит один aggregate только внутри разрешённой группы фракции.
	protected static ME_VehicleBoundsSnapshotEntry ME_FindSnapshotEntry(ME_VehicleBoundsSnapshotFaction faction, string vehicleType)
	{
		foreach (ME_VehicleBoundsSnapshotEntry entry : faction.m_aEntries)
			if (entry.m_sVehicleType == vehicleType) return entry;
		return null;
	}
}
