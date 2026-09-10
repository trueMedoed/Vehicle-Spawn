//! Test-only generator for VBT-backed ambient vehicle aggregate bounds.
//! Генератор только для Test для aggregate bounds ambient-техники на основе VBT.

//------------------------------------------------------------------------------------------------
//! One indexed VBT prefab entry with its single parent faction/type membership.
//! Одна индексированная VBT-запись prefab с единственной родительской принадлежностью faction/type.
class ME_VehicleBoundsVbtCandidateIndexRecord
{
	string m_sFactionKey;
	string m_sVehicleType;
	ME_VBT_VehicleBoundsPerPrefabSnapshotEntry m_Entry;
}

//------------------------------------------------------------------------------------------------
//! Builds aggregate bounds from the VBT Candidate while preserving the Test spawn-point filter contract.
//! Создаёт aggregate bounds из VBT Candidate, сохраняя контракт фильтров spawn point в Test.
[WorkbenchPluginAttribute(name: "Generate bounds snapshots", description: "Generates and reload-validates the canonical VBT-backed aggregate bounds snapshot.", wbModules: { "WorldEditor" }, category: "ME_Vehicle_Spawn/Vehicle Bounds")]
class ME_GenerateVehicleBoundsSnapshotPlugin : WorldEditorPlugin
{
	protected const string SNAPSHOT_PATH = "Configs/Generated/ME_VehicleBoundsSnapshot.conf";
	protected static const ResourceName SNAPSHOT_RESOURCE = "{1C3AE4A8F2630BF7}Configs/Generated/ME_VehicleBoundsSnapshot.conf";
	protected const int VBT_SCHEMA_VERSION = 2;
	protected const int VBT_EXPECTED_PREFAB_COUNT = 146;
	protected const string VBT_GENERATOR_VERSION = "ME_VBT_per_prefab_generator_v2-faction-type-groups";
	protected const string VBT_FIXTURE_IDENTITY = "ME_VBT_VehicleBoundsFixture_v1";
	protected static const ResourceName VBT_CANDIDATE_RESOURCE = "{0F8D7A7D004E2D06}Configs/Generated/ME_VBT_VehicleBoundsPerPrefabCandidate.conf";
	protected ref ME_VBT_VehicleBoundsPerPrefabSnapshot m_pVbtCandidate;
	protected ref array<string> m_aVbtPrefabPaths = {};
	protected ref array<ref ME_VehicleBoundsVbtCandidateIndexRecord> m_aVbtRecords = {};
	protected ref array<string> m_aProcessedCandidates = {};

	//------------------------------------------------------------------------------------------------
	//! Generates and reload-validates the VBT-backed aggregate snapshot from the open Test fixture.
	//! Генерирует и проверяет после перезагрузки VBT-backed aggregate snapshot из открытого Test fixture.
	override void Run()
	{
		Game game = GetGame();
		string gameVersion;
		if (game)
			gameVersion = game.GetBuildVersion();
		if (gameVersion.IsEmpty())
		{
			Print("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=game_version_unavailable");
			return;
		}

		string reason;
		if (!ME_LoadAndIndexVbtCandidate(gameVersion, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=%1", reason);
			return;
		}

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

		ME_VehicleBoundsSnapshot snapshot = ME_CreateAggregateSnapshot();
		m_aProcessedCandidates = {};
		array<string> spawnPointNames = {
			"AmbientVehicleSpawnPoint_US_AllExceptArmed",
			"AmbientVehicleSpawnPoint_USSR_AllExceptArmed",
			"AmbientVehicleSpawnPoint_US_All",
			"AmbientVehicleSpawnPoint_USSR_All",
			"AmbientVehicleSpawnPoint_CIV_AllExceptArmed",
			"AmbientVehicleSpawnPoint_FIA_AllExceptArmed",
			"AmbientVehicleSpawnPoint_FIA_All"
		};

		foreach (string spawnPointName : spawnPointNames)
		{
			if (!ME_AccumulateSpawnPoint(api, spawnPointName, snapshot, reason))
			{
				PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=%1 entity=%2", reason, spawnPointName);
				return;
			}
		}

		ME_SortFactionsAndEntries(snapshot);
		if (snapshot.m_aFactions.IsEmpty() || !ME_ValidateAggregateSnapshot(snapshot, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=%1", reason);
			return;
		}

		if (!ME_SaveAndValidateSnapshot(snapshot, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=%1", reason);
			return;
		}

		int aggregateCount;
		foreach (ME_VehicleBoundsSnapshotFaction faction : snapshot.m_aFactions)
		{
			aggregateCount += faction.m_aEntries.Count();
			foreach (ME_VehicleBoundsSnapshotEntry entry : faction.m_aEntries)
			{
				string sources = entry.m_sMinXSourcePrefab + "|" + entry.m_sMaxXSourcePrefab + "|" + entry.m_sMinYSourcePrefab + "|" + entry.m_sMaxYSourcePrefab + "|" + entry.m_sMinZSourcePrefab + "|" + entry.m_sMaxZSourcePrefab;
				PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot_aggregate faction=%1 type=%2 count=%3 mins=%4 maxs=%5 sources=%6", faction.m_sFactionKey, entry.m_sVehicleType, entry.m_iCandidateCount, entry.m_vLocalMins, entry.m_vLocalMaxs, sources);
			}
		}
		PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=PASS source=VBT resource=%1 count=%2 memberships=%3", SNAPSHOT_PATH, aggregateCount, m_aProcessedCandidates.Count());
	}

	//------------------------------------------------------------------------------------------------
	//! Creates an empty aggregate snapshot with the accepted canonical schema contract.
	//! Создаёт пустой aggregate snapshot с принятым каноническим контрактом schema.
	protected ME_VehicleBoundsSnapshot ME_CreateAggregateSnapshot()
	{
		ME_VehicleBoundsSnapshot snapshot = new ME_VehicleBoundsSnapshot();
		snapshot.m_iSchemaVersion = 5;
		snapshot.m_sGeneratorVersion = "catalog-aggregate-generator-v5-faction-groups-provenance";
		snapshot.m_aFactions = {};
		return snapshot;
	}

	//------------------------------------------------------------------------------------------------
	//! Loads and strictly validates the VBT Candidate before the canonical snapshot can be written.
	//! Загружает и строго проверяет VBT Candidate до возможности записи канонического snapshot.
	protected bool ME_LoadAndIndexVbtCandidate(string gameVersion, out string reason)
	{
		reason = "";
		m_pVbtCandidate = null;
		m_aVbtPrefabPaths = {};
		m_aVbtRecords = {};
		Resource resource = Resource.Load(VBT_CANDIDATE_RESOURCE);
		if (!resource || !resource.IsValid())
		{
			reason = "vbt_candidate_resource_load_failed";
			return false;
		}

		BaseContainer container = resource.GetResource().ToBaseContainer();
		if (!container)
		{
			reason = "vbt_candidate_container_unavailable";
			return false;
		}

		m_pVbtCandidate = ME_VBT_VehicleBoundsPerPrefabSnapshot.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
		if (!m_pVbtCandidate || !m_pVbtCandidate.m_aFactions)
		{
			reason = "vbt_candidate_deserialization_failed";
			return false;
		}
		if (m_pVbtCandidate.m_iSchemaVersion != VBT_SCHEMA_VERSION || m_pVbtCandidate.m_sGeneratorVersion != VBT_GENERATOR_VERSION || m_pVbtCandidate.m_sFixtureIdentity != VBT_FIXTURE_IDENTITY)
		{
			reason = "vbt_candidate_metadata_mismatch";
			return false;
		}
		if (m_pVbtCandidate.m_sGameVersion != gameVersion)
		{
			reason = string.Format("vbt_candidate_game_version_mismatch candidate=%1 current=%2", m_pVbtCandidate.m_sGameVersion, gameVersion);
			return false;
		}
		if (m_pVbtCandidate.m_aFactions.IsEmpty())
		{
			reason = "vbt_candidate_factions_empty";
			return false;
		}

		string previousFactionKey;
		foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotFaction faction : m_pVbtCandidate.m_aFactions)
		{
			if (!faction || faction.m_sFactionKey.IsEmpty() || !faction.m_aVehicleTypes || faction.m_aVehicleTypes.IsEmpty() || !previousFactionKey.IsEmpty() && faction.m_sFactionKey <= previousFactionKey)
			{
				reason = "vbt_candidate_faction_invalid_or_unsorted";
				return false;
			}

			string previousVehicleType;
			foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotVehicleType vehicleType : faction.m_aVehicleTypes)
			{
				if (!vehicleType || !ME_IsVehicleTypeName(vehicleType.m_sVehicleType) || !vehicleType.m_aEntries || vehicleType.m_aEntries.IsEmpty() || !previousVehicleType.IsEmpty() && vehicleType.m_sVehicleType <= previousVehicleType)
				{
					reason = string.Format("vbt_candidate_vehicle_type_invalid_or_unsorted faction=%1", faction.m_sFactionKey);
					return false;
				}

				string previousPath;
				foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotEntry entry : vehicleType.m_aEntries)
				{
					if (!entry || entry.m_sPrefab.IsEmpty() || !previousPath.IsEmpty() && entry.m_sPrefab <= previousPath)
					{
						reason = string.Format("vbt_candidate_entry_order_or_identity_invalid faction=%1 type=%2", faction.m_sFactionKey, vehicleType.m_sVehicleType);
						return false;
					}
					if (m_aVbtPrefabPaths.Contains(entry.m_sPrefab))
					{
						reason = string.Format("vbt_candidate_prefab_duplicate path=%1", entry.m_sPrefab);
						return false;
					}
					if (!ME_AreFiniteOrderedBounds(entry.m_vLocalMins, entry.m_vLocalMaxs))
					{
						reason = string.Format("vbt_candidate_bounds_invalid path=%1", entry.m_sPrefab);
						return false;
					}

					ME_VehicleBoundsVbtCandidateIndexRecord record = new ME_VehicleBoundsVbtCandidateIndexRecord();
					record.m_sFactionKey = faction.m_sFactionKey;
					record.m_sVehicleType = vehicleType.m_sVehicleType;
					record.m_Entry = entry;
					m_aVbtPrefabPaths.Insert(entry.m_sPrefab);
					m_aVbtRecords.Insert(record);
					previousPath = entry.m_sPrefab;
				}
				previousVehicleType = vehicleType.m_sVehicleType;
			}
			previousFactionKey = faction.m_sFactionKey;
		}

		if (m_aVbtRecords.Count() != VBT_EXPECTED_PREFAB_COUNT)
		{
			reason = string.Format("vbt_candidate_entry_count_invalid actual=%1 expected=%2", m_aVbtRecords.Count(), VBT_EXPECTED_PREFAB_COUNT);
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds exactly one indexed VBT record by its canonical ResourceName path.
	//! Находит ровно одну индексированную VBT-запись по её каноническому пути ResourceName.
	protected ME_VehicleBoundsVbtCandidateIndexRecord ME_FindVbtRecord(string prefabPath)
	{
		int index = m_aVbtPrefabPaths.Find(prefabPath);
		if (index < 0)
			return null;
		return m_aVbtRecords[index];
	}

	//------------------------------------------------------------------------------------------------
	//! Finds exactly one VBT bounds entry by its canonical ResourceName path.
	//! Находит ровно одну VBT-запись bounds по её каноническому пути ResourceName.
	protected ME_VBT_VehicleBoundsPerPrefabSnapshotEntry ME_FindVbtEntry(string prefabPath)
	{
		ME_VehicleBoundsVbtCandidateIndexRecord record = ME_FindVbtRecord(prefabPath);
		if (!record)
			return null;
		return record.m_Entry;
	}

	//------------------------------------------------------------------------------------------------
	//! Verifies one selected faction and basic VEHICLE classification against VBT parent groups.
	//! Проверяет одну выбранную faction и базовую VEHICLE-классификацию по родительским группам VBT.
	protected bool ME_ValidateVbtMembership(ME_VehicleBoundsVbtCandidateIndexRecord record, string factionKey, string basicType, out string reason)
	{
		reason = "";
		if (record.m_sFactionKey != factionKey)
		{
			reason = string.Format("vbt_faction_membership_mismatch path=%1 catalog=%2 vbt=%3", record.m_Entry.m_sPrefab, factionKey, record.m_sFactionKey);
			return false;
		}
		if (record.m_sVehicleType != basicType)
		{
			reason = string.Format("vbt_vehicle_type_membership_mismatch path=%1 catalog=%2 vbt=%3", record.m_Entry.m_sPrefab, basicType, record.m_sVehicleType);
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Adds every selected catalog candidate to the VBT-backed aggregate model.
	//! Добавляет каждого выбранного кандидата каталога в VBT-backed aggregate-модель.
	protected bool ME_AccumulateSpawnPoint(WorldEditorAPI api, string entityName, ME_VehicleBoundsSnapshot snapshot, out string reason)
	{
		reason = "";
		IEntitySource source = api.FindEntityByName(entityName);
		IEntity entity;
		if (source)
			entity = api.SourceToEntity(source);
		SCR_AmbientVehicleSpawnPointComponent spawnPoint;
		if (entity)
			spawnPoint = SCR_AmbientVehicleSpawnPointComponent.Cast(entity.FindComponent(SCR_AmbientVehicleSpawnPointComponent));
		if (!spawnPoint)
		{
			reason = "spawn_point_unavailable";
			return false;
		}

		string factionKey;
		array<string> selectedTypes;
		array<SCR_EntityCatalogEntry> entries;
		if (!spawnPoint.ME_GetEditorVehicleAggregateSelection(factionKey, selectedTypes, entries, reason))
			return false;
		if (factionKey.IsEmpty() || selectedTypes.IsEmpty())
		{
			reason = "aggregate_selection_empty";
			return false;
		}

		foreach (SCR_EntityCatalogEntry catalogEntry : entries)
		{
			string prefabPath = catalogEntry.GetPrefab();
			if (prefabPath.IsEmpty())
			{
				reason = "candidate_prefab_path_empty";
				return false;
			}

			ME_VehicleBoundsVbtCandidateIndexRecord vbtRecord = ME_FindVbtRecord(prefabPath);
			if (!vbtRecord)
			{
				reason = string.Format("vbt_prefab_missing path=%1", prefabPath);
				return false;
			}
			ME_VBT_VehicleBoundsPerPrefabSnapshotEntry vbtEntry = vbtRecord.m_Entry;

			array<EEditableEntityLabel> labels = {};
			catalogEntry.GetEditableEntityLabels(labels);
			array<string> basicTypes = {};
			foreach (EEditableEntityLabel basicLabel : labels)
			{
				string labelName = typename.EnumToString(EEditableEntityLabel, basicLabel);
				if (ME_IsVehicleTypeName(labelName) && !basicTypes.Contains(labelName))
					basicTypes.Insert(labelName);
			}
			if (basicTypes.Count() != 1)
			{
				reason = string.Format("catalog_basic_vehicle_type_count_invalid path=%1 count=%2", prefabPath, basicTypes.Count());
				return false;
			}
			if (!ME_ValidateVbtMembership(vbtRecord, factionKey, basicTypes[0], reason))
				return false;

			foreach (EEditableEntityLabel label : labels)
			{
				string aggregateType = typename.EnumToString(EEditableEntityLabel, label);
				if (!aggregateType.Contains("VEHICLE_"))
					continue;

				string candidateKey = factionKey + "\x1F" + aggregateType + "\x1F" + prefabPath;
				if (m_aProcessedCandidates.Contains(candidateKey))
					continue;

				ME_AccumulateCandidate(snapshot, factionKey, aggregateType, prefabPath, vbtEntry.m_vLocalMins, vbtEntry.m_vLocalMaxs);
				m_aProcessedCandidates.Insert(candidateKey);
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Updates one faction/type aggregate with a unique VBT prefab and deterministic provenance.
	//! Обновляет один aggregate фракции/типа уникальным VBT prefab и детерминированным provenance.
	protected void ME_AccumulateCandidate(ME_VehicleBoundsSnapshot snapshot, string factionKey, string vehicleType, string prefabPath, vector localMins, vector localMaxs)
	{
		ME_VehicleBoundsSnapshotFaction faction = ME_FindFaction(snapshot, factionKey);
		if (!faction)
		{
			faction = new ME_VehicleBoundsSnapshotFaction();
			faction.m_sFactionKey = factionKey;
			faction.m_aEntries = {};
			snapshot.m_aFactions.Insert(faction);
		}

		ME_VehicleBoundsSnapshotEntry aggregate = ME_FindAggregate(faction, vehicleType);
		if (!aggregate)
		{
			aggregate = new ME_VehicleBoundsSnapshotEntry();
			aggregate.m_sVehicleType = vehicleType;
			aggregate.m_vLocalMins = localMins;
			aggregate.m_vLocalMaxs = localMaxs;
			aggregate.m_iCandidateCount = 1;
			aggregate.m_sMinXSourcePrefab = prefabPath;
			aggregate.m_sMaxXSourcePrefab = prefabPath;
			aggregate.m_sMinYSourcePrefab = prefabPath;
			aggregate.m_sMaxYSourcePrefab = prefabPath;
			aggregate.m_sMinZSourcePrefab = prefabPath;
			aggregate.m_sMaxZSourcePrefab = prefabPath;
			faction.m_aEntries.Insert(aggregate);
			return;
		}

		aggregate.m_iCandidateCount++;
		for (int axis = 0; axis < 3; axis++)
		{
			string minSource = ME_GetSourcePrefab(aggregate, axis, false);
			string maxSource = ME_GetSourcePrefab(aggregate, axis, true);
			if (localMins[axis] < aggregate.m_vLocalMins[axis] || localMins[axis] == aggregate.m_vLocalMins[axis] && prefabPath < minSource)
			{
				aggregate.m_vLocalMins[axis] = localMins[axis];
				ME_SetSourcePrefab(aggregate, axis, false, prefabPath);
			}
			if (localMaxs[axis] > aggregate.m_vLocalMaxs[axis] || localMaxs[axis] == aggregate.m_vLocalMaxs[axis] && prefabPath < maxSource)
			{
				aggregate.m_vLocalMaxs[axis] = localMaxs[axis];
				ME_SetSourcePrefab(aggregate, axis, true, prefabPath);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Validates sorted faction groups and entries, unique memberships, counts, reverse coverage, bounds, and provenance.
	//! Проверяет сортировку групп и записей, уникальные memberships, counts, reverse coverage, bounds и provenance.
	protected bool ME_ValidateAggregateSnapshot(ME_VehicleBoundsSnapshot snapshot, out string reason)
	{
		reason = "";
		if (!snapshot || !snapshot.m_aFactions || snapshot.m_aFactions.IsEmpty() || !m_aProcessedCandidates || m_aProcessedCandidates.IsEmpty())
		{
			reason = "aggregate_validation_input_invalid";
			return false;
		}

		array<string> sortedKeys = {};
		sortedKeys.Copy(m_aProcessedCandidates);
		sortedKeys.Sort();
		for (int index = 0; index < sortedKeys.Count(); index++)
		{
			if (index > 0 && sortedKeys[index] == sortedKeys[index - 1])
			{
				reason = string.Format("membership_duplicate key=%1", sortedKeys[index]);
				return false;
			}
		}

		string previousFactionKey;
		foreach (ME_VehicleBoundsSnapshotFaction faction : snapshot.m_aFactions)
		{
			if (!faction || faction.m_sFactionKey.IsEmpty() || !faction.m_aEntries || faction.m_aEntries.IsEmpty() || !previousFactionKey.IsEmpty() && faction.m_sFactionKey <= previousFactionKey)
			{
				reason = "aggregate_faction_invalid_or_unsorted";
				return false;
			}

			string previousVehicleType;
			foreach (ME_VehicleBoundsSnapshotEntry entry : faction.m_aEntries)
			{
				if (!entry || entry.m_sVehicleType.IsEmpty() || !previousVehicleType.IsEmpty() && entry.m_sVehicleType <= previousVehicleType || entry.m_iCandidateCount <= 0 || !ME_AreFiniteOrderedBounds(entry.m_vLocalMins, entry.m_vLocalMaxs))
				{
					reason = string.Format("aggregate_entry_invalid_or_unsorted faction=%1", faction.m_sFactionKey);
					return false;
				}

				string prefix = faction.m_sFactionKey + "\x1F" + entry.m_sVehicleType + "\x1F";
				int count = 0;
				foreach (string key : sortedKeys)
				{
					if (key.Contains(prefix))
						count++;
				}
				if (count != entry.m_iCandidateCount)
				{
					reason = string.Format("membership_count_mismatch faction=%1 type=%2 aggregate=%3 memberships=%4", faction.m_sFactionKey, entry.m_sVehicleType, entry.m_iCandidateCount, count);
					return false;
				}

				if (!ME_ValidateAggregateSource(entry, prefix, entry.m_sMinXSourcePrefab, 0, false, reason) || !ME_ValidateAggregateSource(entry, prefix, entry.m_sMaxXSourcePrefab, 0, true, reason) || !ME_ValidateAggregateSource(entry, prefix, entry.m_sMinYSourcePrefab, 1, false, reason) || !ME_ValidateAggregateSource(entry, prefix, entry.m_sMaxYSourcePrefab, 1, true, reason) || !ME_ValidateAggregateSource(entry, prefix, entry.m_sMinZSourcePrefab, 2, false, reason) || !ME_ValidateAggregateSource(entry, prefix, entry.m_sMaxZSourcePrefab, 2, true, reason))
					return false;
				previousVehicleType = entry.m_sVehicleType;
			}
			previousFactionKey = faction.m_sFactionKey;
		}

		foreach (string membershipKey : sortedKeys)
		{
			bool covered = false;
			foreach (ME_VehicleBoundsSnapshotFaction faction : snapshot.m_aFactions)
			{
				foreach (ME_VehicleBoundsSnapshotEntry aggregate : faction.m_aEntries)
				{
					string aggregatePrefix = faction.m_sFactionKey + "\x1F" + aggregate.m_sVehicleType + "\x1F";
					if (membershipKey.Contains(aggregatePrefix))
					{
						covered = true;
						break;
					}
				}
				if (covered)
					break;
			}
			if (!covered)
			{
				reason = string.Format("membership_reverse_coverage_missing key=%1", membershipKey);
				return false;
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Validates one provenance field against its VBT candidate extrema and aggregate membership.
	//! Проверяет одно поле provenance по экстремуму кандидата VBT и принадлежности aggregate.
	protected bool ME_ValidateAggregateSource(ME_VehicleBoundsSnapshotEntry entry, string prefix, string sourcePrefab, int axis, bool isMax, out string reason)
	{
		reason = "";
		if (sourcePrefab.IsEmpty())
		{
			reason = "aggregate_source_empty";
			return false;
		}
		if (!m_aProcessedCandidates.Contains(prefix + sourcePrefab))
		{
			reason = string.Format("aggregate_source_not_in_group path=%1", sourcePrefab);
			return false;
		}

		ME_VBT_VehicleBoundsPerPrefabSnapshotEntry sourceEntry = ME_FindVbtEntry(sourcePrefab);
		if (!sourceEntry)
		{
			reason = string.Format("aggregate_source_missing_in_vbt path=%1", sourcePrefab);
			return false;
		}

		float sourceValue;
		float aggregateValue;
		if (isMax)
		{
			sourceValue = sourceEntry.m_vLocalMaxs[axis];
			aggregateValue = entry.m_vLocalMaxs[axis];
		}
		else
		{
			sourceValue = sourceEntry.m_vLocalMins[axis];
			aggregateValue = entry.m_vLocalMins[axis];
		}
		if (Math.AbsFloat(sourceValue - aggregateValue) > 0.0011)
		{
			reason = string.Format("aggregate_source_extreme_mismatch path=%1 axis=%2", sourcePrefab, axis);
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds one exact faction group without fallback.
	//! Находит одну точную группу фракции без fallback.
	protected ME_VehicleBoundsSnapshotFaction ME_FindFaction(ME_VehicleBoundsSnapshot snapshot, string factionKey)
	{
		foreach (ME_VehicleBoundsSnapshotFaction faction : snapshot.m_aFactions)
		{
			if (faction.m_sFactionKey == factionKey)
				return faction;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds one aggregate only inside its faction group.
	//! Находит один aggregate только внутри его группы фракции.
	protected ME_VehicleBoundsSnapshotEntry ME_FindAggregate(ME_VehicleBoundsSnapshotFaction faction, string vehicleType)
	{
		foreach (ME_VehicleBoundsSnapshotEntry entry : faction.m_aEntries)
		{
			if (entry.m_sVehicleType == vehicleType)
				return entry;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns whether a label name is one of the six basic vehicle classifications.
	//! Возвращает, является ли имя метки одной из шести базовых классификаций техники.
	protected bool ME_IsVehicleTypeName(string labelName)
	{
		return labelName == "VEHICLE_CAR" || labelName == "VEHICLE_HELICOPTER" || labelName == "VEHICLE_AIRPLANE" || labelName == "VEHICLE_APC" || labelName == "VEHICLE_TRUCK" || labelName == "VEHICLE_TURRET";
	}

	//------------------------------------------------------------------------------------------------
	//! Rejects unordered, NaN, and unreasonable bounds.
	//! Отклоняет неупорядоченные, NaN и неоправданные границы.
	protected bool ME_AreFiniteOrderedBounds(vector mins, vector maxs)
	{
		for (int axis = 0; axis < 3; axis++)
		{
			if (mins[axis] != mins[axis] || maxs[axis] != maxs[axis] || Math.AbsFloat(mins[axis]) > 1000000 || Math.AbsFloat(maxs[axis]) > 1000000 || mins[axis] > maxs[axis])
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns one axis provenance field.
	//! Возвращает одно поле provenance для оси.
	protected string ME_GetSourcePrefab(ME_VehicleBoundsSnapshotEntry entry, int axis, bool isMax)
	{
		if (axis == 0)
		{
			if (isMax)
				return entry.m_sMaxXSourcePrefab;
			return entry.m_sMinXSourcePrefab;
		}
		if (axis == 1)
		{
			if (isMax)
				return entry.m_sMaxYSourcePrefab;
			return entry.m_sMinYSourcePrefab;
		}
		if (isMax)
			return entry.m_sMaxZSourcePrefab;
		return entry.m_sMinZSourcePrefab;
	}

	//------------------------------------------------------------------------------------------------
	//! Sets one axis provenance field.
	//! Устанавливает одно поле provenance для оси.
	protected void ME_SetSourcePrefab(ME_VehicleBoundsSnapshotEntry entry, int axis, bool isMax, string prefabPath)
	{
		if (axis == 0)
		{
			if (isMax)
				entry.m_sMaxXSourcePrefab = prefabPath;
			else
				entry.m_sMinXSourcePrefab = prefabPath;
			return;
		}
		if (axis == 1)
		{
			if (isMax)
				entry.m_sMaxYSourcePrefab = prefabPath;
			else
				entry.m_sMinYSourcePrefab = prefabPath;
			return;
		}
		if (isMax)
			entry.m_sMaxZSourcePrefab = prefabPath;
		else
			entry.m_sMinZSourcePrefab = prefabPath;
	}

	//------------------------------------------------------------------------------------------------
	//! Sorts faction groups by key and each group's aggregate entries by vehicle type.
	//! Сортирует группы фракций по ключу, а aggregate-записи каждой группы — по типу техники.
	protected void ME_SortFactionsAndEntries(ME_VehicleBoundsSnapshot snapshot)
	{
		for (int index = 1; index < snapshot.m_aFactions.Count(); index++)
		{
			ME_VehicleBoundsSnapshotFaction value = snapshot.m_aFactions[index];
			int previousIndex = index - 1;
			while (previousIndex >= 0 && snapshot.m_aFactions[previousIndex].m_sFactionKey > value.m_sFactionKey)
			{
				snapshot.m_aFactions[previousIndex + 1] = snapshot.m_aFactions[previousIndex];
				previousIndex--;
			}
			snapshot.m_aFactions[previousIndex + 1] = value;
		}

		foreach (ME_VehicleBoundsSnapshotFaction faction : snapshot.m_aFactions)
			ME_SortEntries(faction.m_aEntries);
	}

	//------------------------------------------------------------------------------------------------
	//! Sorts one faction group's aggregate entries by vehicle type.
	//! Сортирует aggregate-записи одной группы фракции по типу техники.
	protected void ME_SortEntries(array<ref ME_VehicleBoundsSnapshotEntry> entries)
	{
		for (int index = 1; index < entries.Count(); index++)
		{
			ME_VehicleBoundsSnapshotEntry value = entries[index];
			int previousIndex = index - 1;
			while (previousIndex >= 0 && entries[previousIndex].m_sVehicleType > value.m_sVehicleType)
			{
				entries[previousIndex + 1] = entries[previousIndex];
				previousIndex--;
			}
			entries[previousIndex + 1] = value;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Accepts only identifier-safe ASCII letters, digits, and underscores in a container name.
	//! Допускает в имени контейнера только безопасные для идентификатора ASCII-буквы, цифры и подчёркивания.
	protected bool ME_IsSafeContainerName(string value)
	{
		if (value.IsEmpty())
			return false;

		string firstCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";
		if (!firstCharacters.Contains(value.Substring(0, 1)))
			return false;

		string allowedCharacters = firstCharacters + "0123456789";
		for (int index = 0; index < value.Length(); index++)
		{
			if (!allowedCharacters.Contains(value.Substring(index, 1)))
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Assigns unique faction-key names and per-faction vehicle-type names to serialized containers.
	//! Назначает сериализованным контейнерам уникальные имена faction key и vehicle type внутри каждой фракции.
	protected bool ME_SetDeterministicContainerNames(BaseContainer container, ME_VehicleBoundsSnapshot snapshot, out string reason)
	{
		reason = "";
		BaseContainerList factionContainers = container.GetObjectArray("m_aFactions");
		if (!factionContainers || !snapshot || !snapshot.m_aFactions || factionContainers.Count() != snapshot.m_aFactions.Count())
		{
			reason = "serialized_faction_container_count_mismatch";
			return false;
		}

		array<string> factionNames = {};
		for (int factionIndex = 0; factionIndex < factionContainers.Count(); factionIndex++)
		{
			BaseContainer factionContainer = factionContainers.Get(factionIndex);
			ME_VehicleBoundsSnapshotFaction faction = snapshot.m_aFactions[factionIndex];
			if (!factionContainer || !faction || !ME_IsSafeContainerName(faction.m_sFactionKey))
			{
				reason = "serialized_faction_container_name_invalid";
				return false;
			}
			if (factionNames.Contains(faction.m_sFactionKey))
			{
				reason = string.Format("serialized_faction_container_name_duplicate name=%1", faction.m_sFactionKey);
				return false;
			}
			factionContainer.SetName(faction.m_sFactionKey);
			factionNames.Insert(faction.m_sFactionKey);

			BaseContainerList entryContainers = factionContainer.GetObjectArray("m_aEntries");
			if (!entryContainers || !faction.m_aEntries || entryContainers.Count() != faction.m_aEntries.Count())
			{
				reason = string.Format("serialized_entry_container_count_mismatch faction=%1", faction.m_sFactionKey);
				return false;
			}

			array<string> entryNames = {};
			for (int entryIndex = 0; entryIndex < entryContainers.Count(); entryIndex++)
			{
				BaseContainer entryContainer = entryContainers.Get(entryIndex);
				ME_VehicleBoundsSnapshotEntry entry = faction.m_aEntries[entryIndex];
				if (!entryContainer || !entry || !ME_IsSafeContainerName(entry.m_sVehicleType))
				{
					reason = string.Format("serialized_entry_container_name_invalid faction=%1", faction.m_sFactionKey);
					return false;
				}
				if (entryNames.Contains(entry.m_sVehicleType))
				{
					reason = string.Format("serialized_entry_container_name_duplicate faction=%1 name=%2", faction.m_sFactionKey, entry.m_sVehicleType);
					return false;
				}
				entryContainer.SetName(entry.m_sVehicleType);
				entryNames.Insert(entry.m_sVehicleType);
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Verifies that faction and nested entry names survived serialization and reload unchanged.
	//! Проверяет, что имена фракций и вложенных записей сохранились после сериализации и перезагрузки.
	protected bool ME_ValidateReloadedContainerNames(BaseContainer container, ME_VehicleBoundsSnapshot snapshot, out string reason)
	{
		reason = "";
		BaseContainerList factionContainers = container.GetObjectArray("m_aFactions");
		if (!factionContainers || !snapshot || !snapshot.m_aFactions || factionContainers.Count() != snapshot.m_aFactions.Count())
		{
			reason = "reload_faction_container_count_mismatch";
			return false;
		}

		array<string> factionNames = {};
		for (int factionIndex = 0; factionIndex < factionContainers.Count(); factionIndex++)
		{
			BaseContainer factionContainer = factionContainers.Get(factionIndex);
			ME_VehicleBoundsSnapshotFaction faction = snapshot.m_aFactions[factionIndex];
			if (!factionContainer || !faction)
			{
				reason = "reload_faction_container_missing";
				return false;
			}

			string actualFactionName = factionContainer.GetName();
			if (actualFactionName != faction.m_sFactionKey || factionNames.Contains(faction.m_sFactionKey))
			{
				reason = string.Format("reload_faction_container_name_mismatch expected=%1 actual=%2", faction.m_sFactionKey, actualFactionName);
				return false;
			}
			factionNames.Insert(faction.m_sFactionKey);

			BaseContainerList entryContainers = factionContainer.GetObjectArray("m_aEntries");
			if (!entryContainers || !faction.m_aEntries || entryContainers.Count() != faction.m_aEntries.Count())
			{
				reason = string.Format("reload_entry_container_count_mismatch faction=%1", faction.m_sFactionKey);
				return false;
			}

			array<string> entryNames = {};
			for (int entryIndex = 0; entryIndex < entryContainers.Count(); entryIndex++)
			{
				BaseContainer entryContainer = entryContainers.Get(entryIndex);
				string expectedName = faction.m_aEntries[entryIndex].m_sVehicleType;
				if (!entryContainer)
				{
					reason = string.Format("reload_entry_container_missing faction=%1 index=%2", faction.m_sFactionKey, entryIndex);
					return false;
				}

				string actualEntryName = entryContainer.GetName();
				if (actualEntryName != expectedName || entryNames.Contains(expectedName))
				{
					reason = string.Format("reload_entry_container_name_mismatch faction=%1 expected=%2 actual=%3", faction.m_sFactionKey, expectedName, actualEntryName);
					return false;
				}
				entryNames.Insert(expectedName);
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Saves the canonical aggregate resource and compares every reloaded field and entry name.
	//! Сохраняет канонический aggregate-ресурс и сравнивает каждое перезагруженное поле и имя записи.
	protected bool ME_SaveAndValidateSnapshot(ME_VehicleBoundsSnapshot snapshot, out string reason)
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
		if (!container || !ME_SetDeterministicContainerNames(container, snapshot, reason))
			return false;
		if (!BaseContainerTools.SaveContainer(container, SNAPSHOT_RESOURCE, absolutePath))
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

		Resource loaded = BaseContainerTools.LoadContainer(SNAPSHOT_RESOURCE);
		BaseContainer loadedContainer;
		if (loaded)
			loadedContainer = loaded.GetResource().ToBaseContainer();
		if (!loadedContainer || !ME_ValidateReloadedContainerNames(loadedContainer, snapshot, reason))
			return false;

		ME_VehicleBoundsSnapshot reloaded = ME_VehicleBoundsSnapshot.Cast(BaseContainerTools.CreateInstanceFromContainer(loadedContainer));
		if (!reloaded || reloaded.m_iSchemaVersion != snapshot.m_iSchemaVersion || reloaded.m_sGeneratorVersion != snapshot.m_sGeneratorVersion || !reloaded.m_aFactions || reloaded.m_aFactions.Count() != snapshot.m_aFactions.Count())
		{
			reason = "reload_validation_failed";
			return false;
		}

		for (int factionIndex = 0; factionIndex < snapshot.m_aFactions.Count(); factionIndex++)
		{
			ME_VehicleBoundsSnapshotFaction expectedFaction = snapshot.m_aFactions[factionIndex];
			ME_VehicleBoundsSnapshotFaction actualFaction = reloaded.m_aFactions[factionIndex];
			if (!actualFaction || expectedFaction.m_sFactionKey != actualFaction.m_sFactionKey || !actualFaction.m_aEntries || actualFaction.m_aEntries.Count() != expectedFaction.m_aEntries.Count())
			{
				reason = "reload_faction_mismatch";
				return false;
			}

			for (int entryIndex = 0; entryIndex < expectedFaction.m_aEntries.Count(); entryIndex++)
			{
				ME_VehicleBoundsSnapshotEntry expected = expectedFaction.m_aEntries[entryIndex];
				ME_VehicleBoundsSnapshotEntry actual = actualFaction.m_aEntries[entryIndex];
				if (!actual || expected.m_sVehicleType != actual.m_sVehicleType || expected.m_iCandidateCount != actual.m_iCandidateCount || expected.m_sMinXSourcePrefab != actual.m_sMinXSourcePrefab || expected.m_sMaxXSourcePrefab != actual.m_sMaxXSourcePrefab || expected.m_sMinYSourcePrefab != actual.m_sMinYSourcePrefab || expected.m_sMaxYSourcePrefab != actual.m_sMaxYSourcePrefab || expected.m_sMinZSourcePrefab != actual.m_sMinZSourcePrefab || expected.m_sMaxZSourcePrefab != actual.m_sMaxZSourcePrefab || !ME_AreVectorsClose(expected.m_vLocalMins, actual.m_vLocalMins) || !ME_AreVectorsClose(expected.m_vLocalMaxs, actual.m_vLocalMaxs))
				{
					reason = "reload_entry_mismatch";
					return false;
				}
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Compares vectors with the canonical resource precision tolerance.
	//! Сравнивает векторы с допуском точности канонического ресурса.
	protected bool ME_AreVectorsClose(vector left, vector right)
	{
		return Math.AbsFloat(left[0] - right[0]) <= 0.0011 && Math.AbsFloat(left[1] - right[1]) <= 0.0011 && Math.AbsFloat(left[2] - right[2]) <= 0.0011;
	}
}
