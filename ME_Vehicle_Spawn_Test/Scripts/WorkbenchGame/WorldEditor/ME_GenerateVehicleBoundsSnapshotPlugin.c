//! Test-only generator for VBT-backed ambient vehicle aggregate bounds.
//! Генератор только для Test для aggregate bounds ambient-техники на основе VBT.

//------------------------------------------------------------------------------------------------
//! Builds aggregate bounds from the VBT Candidate while preserving the Test spawn-point filter contract.
//! Создаёт aggregate bounds из VBT Candidate, сохраняя контракт фильтров spawn point в Test.
[WorkbenchPluginAttribute(name: "Generate bounds snapshots", description: "Generates VBT-backed aggregate bounds and reload-validates the staged snapshot.", wbModules: { "WorldEditor" }, category: "ME_Vehicle_Spawn/Vehicle Bounds")]
class ME_GenerateVehicleBoundsSnapshotPlugin : WorldEditorPlugin
{
	protected const string STAGED_PATH = "Configs/Generated/ME_VehicleBoundsSnapshot_Staged.conf";
	protected static const ResourceName STAGED_RESOURCE = "{1C3AE4A8F2630BF8}Configs/Generated/ME_VehicleBoundsSnapshot_Staged.conf";
	protected const int VBT_SCHEMA_VERSION = 1;
	protected const string VBT_GENERATOR_VERSION = "ME_VBT_per_prefab_generator_v1";
	protected const string VBT_FIXTURE_IDENTITY = "ME_VBT_VehicleBoundsFixture_v1";
	protected static const ResourceName VBT_CANDIDATE_RESOURCE = "{0F8D7A7D004E2D06}Configs/Generated/ME_VBT_VehicleBoundsPerPrefabCandidate.conf";
	protected ref ME_VBT_VehicleBoundsPerPrefabSnapshot m_pVbtCandidate;
	protected ref array<string> m_aVbtPrefabPaths = {};
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

		ME_SortEntries(snapshot.m_aEntries);
		if (snapshot.m_aEntries.IsEmpty() || !ME_ValidateAggregateSnapshot(snapshot, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=%1", reason);
			return;
		}

		if (!ME_SaveAndValidateStagedSnapshot(snapshot, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=%1", reason);
			return;
		}

		foreach (ME_VehicleBoundsSnapshotEntry entry : snapshot.m_aEntries)
		{
			string sources = entry.m_sMinXSourcePrefab + "|" + entry.m_sMaxXSourcePrefab + "|" + entry.m_sMinYSourcePrefab + "|" + entry.m_sMaxYSourcePrefab + "|" + entry.m_sMinZSourcePrefab + "|" + entry.m_sMaxZSourcePrefab;
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot_aggregate faction=%1 type=%2 count=%3 mins=%4 maxs=%5 sources=%6", entry.m_sFactionKey, entry.m_sVehicleType, entry.m_iCandidateCount, entry.m_vLocalMins, entry.m_vLocalMaxs, sources);
		}
		PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=PASS source=VBT stage=%1 count=%2 memberships=%3", STAGED_PATH, snapshot.m_aEntries.Count(), m_aProcessedCandidates.Count());
	}

	//------------------------------------------------------------------------------------------------
	//! Creates an empty aggregate snapshot with the accepted staged schema contract.
	//! Создаёт пустой aggregate snapshot с принятым контрактом staged schema.
	protected ME_VehicleBoundsSnapshot ME_CreateAggregateSnapshot()
	{
		ME_VehicleBoundsSnapshot snapshot = new ME_VehicleBoundsSnapshot();
		snapshot.m_iSchemaVersion = 4;
		snapshot.m_sGeneratorVersion = "catalog-aggregate-generator-v4-provenance";
		snapshot.m_aEntries = {};
		return snapshot;
	}

	//------------------------------------------------------------------------------------------------
	//! Loads and strictly validates the VBT Candidate before any staged output can be written.
	//! Загружает и строго проверяет VBT Candidate до возможности записи staged output.
	protected bool ME_LoadAndIndexVbtCandidate(string gameVersion, out string reason)
	{
		reason = "";
		m_pVbtCandidate = null;
		m_aVbtPrefabPaths = {};
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
		if (!m_pVbtCandidate || !m_pVbtCandidate.m_aEntries)
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
		if (m_pVbtCandidate.m_aEntries.IsEmpty())
		{
			reason = "vbt_candidate_entries_empty";
			return false;
		}

		string previousPath;
		foreach (ME_VBT_VehicleBoundsPerPrefabSnapshotEntry entry : m_pVbtCandidate.m_aEntries)
		{
			if (!entry || entry.m_sPrefab.IsEmpty())
			{
				reason = "vbt_candidate_entry_identity_invalid";
				return false;
			}
			if (!previousPath.IsEmpty() && entry.m_sPrefab <= previousPath)
			{
				reason = string.Format("vbt_candidate_entry_order_or_duplicate path=%1", entry.m_sPrefab);
				return false;
			}
			if (!ME_AreFiniteOrderedBounds(entry.m_vLocalMins, entry.m_vLocalMaxs))
			{
				reason = string.Format("vbt_candidate_bounds_invalid path=%1", entry.m_sPrefab);
				return false;
			}
			if (!ME_ValidateSortedUniqueNames(entry.m_aFactionKeys, "faction", entry.m_sPrefab, reason) || !ME_ValidateSortedUniqueNames(entry.m_aVehicleTypes, "vehicle_type", entry.m_sPrefab, reason))
				return false;
			foreach (string vehicleType : entry.m_aVehicleTypes)
			{
				if (!ME_IsVehicleTypeName(vehicleType))
				{
					reason = string.Format("vbt_candidate_vehicle_type_invalid path=%1 type=%2", entry.m_sPrefab, vehicleType);
					return false;
				}
			}
			m_aVbtPrefabPaths.Insert(entry.m_sPrefab);
			previousPath = entry.m_sPrefab;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Requires a non-empty lexically sorted array without duplicate values.
	//! Требует непустой лексикографически отсортированный массив без повторяющихся значений.
	protected bool ME_ValidateSortedUniqueNames(array<string> names, string fieldName, string prefabPath, out string reason)
	{
		reason = "";
		if (!names || names.IsEmpty())
		{
			reason = string.Format("vbt_candidate_%1_array_empty path=%2", fieldName, prefabPath);
			return false;
		}

		string previousName;
		foreach (string name : names)
		{
			if (name.IsEmpty() || !previousName.IsEmpty() && name <= previousName)
			{
				reason = string.Format("vbt_candidate_%1_order_or_duplicate path=%2 value=%3", fieldName, prefabPath, name);
				return false;
			}
			previousName = name;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds exactly one VBT entry by its canonical ResourceName path.
	//! Находит ровно одну VBT-запись по её каноническому пути ResourceName.
	protected ME_VBT_VehicleBoundsPerPrefabSnapshotEntry ME_FindVbtEntry(string prefabPath)
	{
		int index = m_aVbtPrefabPaths.Find(prefabPath);
		if (index < 0)
			return null;
		return m_pVbtCandidate.m_aEntries[index];
	}

	//------------------------------------------------------------------------------------------------
	//! Verifies the selected faction and complete basic VEHICLE classification against VBT metadata.
	//! Проверяет выбранную фракцию и полную basic VEHICLE-классификацию по metadata VBT.
	protected bool ME_ValidateVbtMembership(ME_VBT_VehicleBoundsPerPrefabSnapshotEntry entry, string factionKey, array<string> basicTypes, out string reason)
	{
		reason = "";
		if (!entry.m_aFactionKeys.Contains(factionKey))
		{
			reason = string.Format("vbt_faction_membership_mismatch path=%1 faction=%2", entry.m_sPrefab, factionKey);
			return false;
		}
		if (!basicTypes || basicTypes.Count() != entry.m_aVehicleTypes.Count())
		{
			reason = string.Format("vbt_vehicle_type_count_mismatch path=%1 catalog=%2 vbt=%3", entry.m_sPrefab, basicTypes.Count(), entry.m_aVehicleTypes.Count());
			return false;
		}
		for (int index = 0; index < basicTypes.Count(); index++)
		{
			if (basicTypes[index] != entry.m_aVehicleTypes[index])
			{
				reason = string.Format("vbt_vehicle_type_membership_mismatch path=%1 catalog=%2 vbt=%3", entry.m_sPrefab, basicTypes[index], entry.m_aVehicleTypes[index]);
				return false;
			}
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

			ME_VBT_VehicleBoundsPerPrefabSnapshotEntry vbtEntry = ME_FindVbtEntry(prefabPath);
			if (!vbtEntry)
			{
				reason = string.Format("vbt_prefab_missing path=%1", prefabPath);
				return false;
			}

			array<EEditableEntityLabel> labels = {};
			catalogEntry.GetEditableEntityLabels(labels);
			array<string> basicTypes = {};
			foreach (EEditableEntityLabel basicLabel : labels)
			{
				string basicType = typename.EnumToString(EEditableEntityLabel, basicLabel);
				if (ME_IsVehicleTypeName(basicType) && !basicTypes.Contains(basicType))
					basicTypes.Insert(basicType);
			}
			basicTypes.Sort();
			if (!ME_ValidateVbtMembership(vbtEntry, factionKey, basicTypes, reason))
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
		ME_VehicleBoundsSnapshotEntry aggregate = ME_FindAggregate(snapshot, factionKey, vehicleType);
		if (!aggregate)
		{
			aggregate = new ME_VehicleBoundsSnapshotEntry();
			aggregate.m_sFactionKey = factionKey;
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
			snapshot.m_aEntries.Insert(aggregate);
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
	//! Validates unique membership keys, aggregate counts, reverse coverage, bounds, and provenance.
	//! Проверяет уникальные membership keys, counts aggregate, reverse coverage, bounds и provenance.
	protected bool ME_ValidateAggregateSnapshot(ME_VehicleBoundsSnapshot snapshot, out string reason)
	{
		reason = "";
		if (!snapshot || !snapshot.m_aEntries || !m_aProcessedCandidates || m_aProcessedCandidates.IsEmpty())
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

		foreach (ME_VehicleBoundsSnapshotEntry entry : snapshot.m_aEntries)
		{
			if (!entry || entry.m_sFactionKey.IsEmpty() || entry.m_sVehicleType.IsEmpty() || entry.m_iCandidateCount <= 0 || !ME_AreFiniteOrderedBounds(entry.m_vLocalMins, entry.m_vLocalMaxs))
			{
				reason = "aggregate_entry_invalid";
				return false;
			}

			string prefix = entry.m_sFactionKey + "\x1F" + entry.m_sVehicleType + "\x1F";
			int count = 0;
			foreach (string key : sortedKeys)
			{
				if (key.Contains(prefix))
					count++;
			}
			if (count != entry.m_iCandidateCount)
			{
				reason = string.Format("membership_count_mismatch faction=%1 type=%2 aggregate=%3 memberships=%4", entry.m_sFactionKey, entry.m_sVehicleType, entry.m_iCandidateCount, count);
				return false;
			}

			if (!ME_ValidateAggregateSource(entry, prefix, entry.m_sMinXSourcePrefab, 0, false, reason) || !ME_ValidateAggregateSource(entry, prefix, entry.m_sMaxXSourcePrefab, 0, true, reason) || !ME_ValidateAggregateSource(entry, prefix, entry.m_sMinYSourcePrefab, 1, false, reason) || !ME_ValidateAggregateSource(entry, prefix, entry.m_sMaxYSourcePrefab, 1, true, reason) || !ME_ValidateAggregateSource(entry, prefix, entry.m_sMinZSourcePrefab, 2, false, reason) || !ME_ValidateAggregateSource(entry, prefix, entry.m_sMaxZSourcePrefab, 2, true, reason))
				return false;
		}

		foreach (string membershipKey : sortedKeys)
		{
			bool covered = false;
			foreach (ME_VehicleBoundsSnapshotEntry aggregate : snapshot.m_aEntries)
			{
				string aggregatePrefix = aggregate.m_sFactionKey + "\x1F" + aggregate.m_sVehicleType + "\x1F";
				if (membershipKey.Contains(aggregatePrefix))
				{
					covered = true;
					break;
				}
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
	//! Finds one aggregate without cross-faction fallback.
	//! Находит один aggregate без fallback между фракциями.
	protected ME_VehicleBoundsSnapshotEntry ME_FindAggregate(ME_VehicleBoundsSnapshot snapshot, string factionKey, string vehicleType)
	{
		foreach (ME_VehicleBoundsSnapshotEntry entry : snapshot.m_aEntries)
		{
			if (entry.m_sFactionKey == factionKey && entry.m_sVehicleType == vehicleType)
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
	//! Sorts aggregate entries by faction key and vehicle type.
	//! Сортирует записи aggregate по ключу фракции и типу техники.
	protected void ME_SortEntries(array<ref ME_VehicleBoundsSnapshotEntry> entries)
	{
		for (int index = 1; index < entries.Count(); index++)
		{
			ME_VehicleBoundsSnapshotEntry value = entries[index];
			int previousIndex = index - 1;
			while (previousIndex >= 0 && entries[previousIndex].m_sFactionKey + "\x1F" + entries[previousIndex].m_sVehicleType > value.m_sFactionKey + "\x1F" + value.m_sVehicleType)
			{
				entries[previousIndex + 1] = entries[previousIndex];
				previousIndex--;
			}
			entries[previousIndex + 1] = value;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Assigns stable names to serialized entry containers so repeated generation produces identical files.
	//! Назначает стабильные имена сериализованным контейнерам записей, чтобы повторная генерация создавала идентичные файлы.
	protected bool ME_SetDeterministicEntryContainerNames(BaseContainer container, int expectedCount, out string reason)
	{
		reason = "";
		BaseContainerList entries = container.GetObjectArray("m_aEntries");
		if (!entries || entries.Count() != expectedCount)
		{
			reason = "serialized_entry_container_count_mismatch";
			return false;
		}
		for (int index = 0; index < entries.Count(); index++)
		{
			BaseContainer entry = entries.Get(index);
			if (!entry)
			{
				reason = "serialized_entry_container_missing";
				return false;
			}
			entry.SetName(string.Format("aggregate_entry_%1", index));
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Saves the aggregate staged resource and compares every reloaded field.
	//! Сохраняет staged-ресурс aggregate и сравнивает каждое перезагруженное поле.
	protected bool ME_SaveAndValidateStagedSnapshot(ME_VehicleBoundsSnapshot snapshot, out string reason)
	{
		reason = "";
		Resource containerResource = BaseContainerTools.CreateContainerFromInstance(snapshot);
		string absolutePath;
		if (!containerResource || !Workbench.GetAbsolutePath(STAGED_PATH, absolutePath, false))
		{
			reason = "create_or_path_failed";
			return false;
		}

		BaseContainer container = containerResource.GetResource().ToBaseContainer();
		if (!container || !ME_SetDeterministicEntryContainerNames(container, snapshot.m_aEntries.Count(), reason))
			return false;
		if (!BaseContainerTools.SaveContainer(container, STAGED_RESOURCE, absolutePath))
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
		BaseContainer loadedContainer;
		if (loaded)
			loadedContainer = loaded.GetResource().ToBaseContainer();
		ME_VehicleBoundsSnapshot reloaded;
		if (loadedContainer)
			reloaded = ME_VehicleBoundsSnapshot.Cast(BaseContainerTools.CreateInstanceFromContainer(loadedContainer));
		if (!reloaded || reloaded.m_iSchemaVersion != snapshot.m_iSchemaVersion || reloaded.m_sGeneratorVersion != snapshot.m_sGeneratorVersion || !reloaded.m_aEntries || reloaded.m_aEntries.Count() != snapshot.m_aEntries.Count())
		{
			reason = "reload_validation_failed";
			return false;
		}

		for (int index = 0; index < snapshot.m_aEntries.Count(); index++)
		{
			ME_VehicleBoundsSnapshotEntry expected = snapshot.m_aEntries[index];
			ME_VehicleBoundsSnapshotEntry actual = reloaded.m_aEntries[index];
			if (expected.m_sFactionKey != actual.m_sFactionKey || expected.m_sVehicleType != actual.m_sVehicleType || expected.m_iCandidateCount != actual.m_iCandidateCount || expected.m_sMinXSourcePrefab != actual.m_sMinXSourcePrefab || expected.m_sMaxXSourcePrefab != actual.m_sMaxXSourcePrefab || expected.m_sMinYSourcePrefab != actual.m_sMinYSourcePrefab || expected.m_sMaxYSourcePrefab != actual.m_sMaxYSourcePrefab || expected.m_sMinZSourcePrefab != actual.m_sMinZSourcePrefab || expected.m_sMaxZSourcePrefab != actual.m_sMaxZSourcePrefab || !ME_AreVectorsClose(expected.m_vLocalMins, actual.m_vLocalMins) || !ME_AreVectorsClose(expected.m_vLocalMaxs, actual.m_vLocalMaxs))
			{
				reason = "reload_entry_mismatch";
				return false;
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Compares vectors with the staged resource precision tolerance.
	//! Сравнивает векторы с допуском точности staged-ресурса.
	protected bool ME_AreVectorsClose(vector left, vector right)
	{
		return Math.AbsFloat(left[0] - right[0]) <= 0.0011 && Math.AbsFloat(left[1] - right[1]) <= 0.0011 && Math.AbsFloat(left[2] - right[2]) <= 0.0011;
	}
}
