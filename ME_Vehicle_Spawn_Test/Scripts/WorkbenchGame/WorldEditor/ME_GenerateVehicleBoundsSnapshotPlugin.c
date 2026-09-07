//! Test-only generator for deterministic aggregate and per-prefab ambient vehicle bounds snapshots.
//! Генератор только для Test детерминированных aggregate и per-prefab snapshots границ ambient-техники.

//------------------------------------------------------------------------------------------------
//! Measures marked fixture roots without creating, moving, or deleting world entities.
//! Измеряет отмеченные корни fixture без создания, перемещения или удаления сущностей мира.
[WorkbenchPluginAttribute(name: "Generate bounds snapshots", description: "Measures fixture aggregates and per-prefab bounds, then reload-validates generated resources.", wbModules: { "WorldEditor" }, category: "ME_Vehicle_Spawn/Vehicle Bounds")]
class ME_GenerateVehicleBoundsSnapshotPlugin : WorldEditorPlugin
{
	protected const string STAGED_PATH = "Configs/Generated/ME_VehicleBoundsSnapshot_Staged.conf";
	protected static const ResourceName STAGED_RESOURCE = "{1C3AE4A8F2630BF8}Configs/Generated/ME_VehicleBoundsSnapshot_Staged.conf";
	protected const int PER_PREFAB_SCHEMA_VERSION = 3;
	protected const string PER_PREFAB_GENERATOR_VERSION = "catalog-per-prefab-generator-v4-grouped-entries";
	protected const string FIXTURE_IDENTITY = "ME_VehicleBoundsSnapshot";
	protected const string CANDIDATE_PATH = "Configs/Generated/ME_VehicleBoundsPerPrefabCandidate.conf";
	protected static const ResourceName CANDIDATE_RESOURCE = "{93526DBFFA4908C1}Configs/Generated/ME_VehicleBoundsPerPrefabCandidate.conf";
	protected static const ResourceName BASELINE_RESOURCE = "{8897392635304AE5}Configs/Generated/ME_VehicleBoundsPerPrefabBaseline.conf";
	protected ref array<string> m_aProcessedCandidates = {};
	protected ref array<string> m_aExpectedCandidateKeys = {};
	protected ref array<vector> m_aExpectedCandidateMins = {};
	protected ref array<vector> m_aExpectedCandidateMaxs = {};
	protected ref array<string> m_aCatalogPrefabPaths = {};
	protected ref array<string> m_aMeasuredPrefabPaths = {};
	protected ref array<vector> m_aMeasuredPrefabMins = {};
	protected ref array<vector> m_aMeasuredPrefabMaxs = {};

	//------------------------------------------------------------------------------------------------
	//! Generates and validates aggregate and per-prefab snapshots from the open fixture.
	//! Генерирует и проверяет aggregate и per-prefab snapshots из открытого fixture.
	override void Run()
	{
		Game game = GetGame();
		string gameVersion;
		if (game)
			gameVersion = game.GetBuildVersion();
		if (gameVersion.IsEmpty())
		{
			Print("[ME_DEBUG_AVSP_WB] bounds_per_prefab_compare status=FAIL reason=game_version_unavailable");
			return;
		}

		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor) { Print("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=world_editor_unavailable"); return; }
		WorldEditorAPI api = worldEditor.GetApi();
		if (!api) { Print("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=world_editor_api_unavailable"); return; }
		array<IEntity> markerRoots;
		string reason;
		if (!ME_CollectMarkerRoots(api, markerRoots, reason)) { PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=%1", reason); return; }

		ME_VehicleBoundsSnapshot snapshot = new ME_VehicleBoundsSnapshot();
		snapshot.m_iSchemaVersion = 4;
		snapshot.m_sGeneratorVersion = "catalog-aggregate-generator-v4-provenance";
		snapshot.m_aEntries = {};
		ME_VehicleBoundsPerPrefabSnapshot candidate = new ME_VehicleBoundsPerPrefabSnapshot();
		candidate.m_iSchemaVersion = PER_PREFAB_SCHEMA_VERSION;
		candidate.m_sGeneratorVersion = PER_PREFAB_GENERATOR_VERSION;
		candidate.m_sFixtureIdentity = FIXTURE_IDENTITY;
		candidate.m_sGameVersion = gameVersion;
		candidate.m_bClassificationMetadataAvailable = true;
		candidate.m_aEntries = {};
		m_aProcessedCandidates = {};
		m_aExpectedCandidateKeys = {};
		m_aExpectedCandidateMins = {};
		m_aExpectedCandidateMaxs = {};
		m_aCatalogPrefabPaths = {};
		m_aMeasuredPrefabPaths = {};
		m_aMeasuredPrefabMins = {};
		m_aMeasuredPrefabMaxs = {};

		array<string> spawnPointNames = { "AmbientVehicleSpawnPoint_US_AllExceptArmed", "AmbientVehicleSpawnPoint_USSR_AllExceptArmed", "AmbientVehicleSpawnPoint_US_All", "AmbientVehicleSpawnPoint_USSR_All", "AmbientVehicleSpawnPoint_CIV_AllExceptArmed", "AmbientVehicleSpawnPoint_FIA_AllExceptArmed", "AmbientVehicleSpawnPoint_FIA_All" };
		foreach (string spawnPointName : spawnPointNames)
			if (!ME_AccumulateSpawnPoint(api, markerRoots, spawnPointName, snapshot, candidate, reason)) { PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=%1 entity=%2", reason, spawnPointName); return; }

		ME_SortEntries(snapshot.m_aEntries);
		if (snapshot.m_aEntries.IsEmpty() || !ME_ValidateExpectedSnapshot(snapshot, reason) || !ME_SaveAndValidateStagedSnapshot(snapshot, reason)) { PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=FAIL reason=%1", reason); return; }
		foreach (ME_VehicleBoundsSnapshotEntry entry : snapshot.m_aEntries)
		{
			string sources = entry.m_sMinXSourcePrefab + "|" + entry.m_sMaxXSourcePrefab + "|" + entry.m_sMinYSourcePrefab + "|" + entry.m_sMaxYSourcePrefab + "|" + entry.m_sMinZSourcePrefab + "|" + entry.m_sMaxZSourcePrefab;
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot_aggregate faction=%1 type=%2 count=%3 mins=%4 maxs=%5 sources=%6", entry.m_sFactionKey, entry.m_sVehicleType, entry.m_iCandidateCount, entry.m_vLocalMins, entry.m_vLocalMaxs, sources);
		}
		PrintFormat("[ME_DEBUG_AVSP_WB] bounds_snapshot status=PASS stage=%1 count=%2", STAGED_PATH, snapshot.m_aEntries.Count());

		PrintFormat("[ME_DEBUG_AVSP_WB] bounds_per_prefab_compare phase=model status=START count=%1", candidate.m_aEntries.Count());
		ME_SortPerPrefabEntries(candidate.m_aEntries);
		PrintFormat("[ME_DEBUG_AVSP_WB] bounds_per_prefab_compare phase=model status=SORTED count=%1", candidate.m_aEntries.Count());
		if (!ME_ValidatePerPrefabCanonicalSnapshot(candidate, true, reason) || !ME_ValidatePerPrefabCoverage(markerRoots, candidate, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_per_prefab_compare status=FAIL reason=%1 candidate=%2", reason, CANDIDATE_PATH);
			return;
		}

		PrintFormat("[ME_DEBUG_AVSP_WB] bounds_per_prefab_compare phase=model status=PASS count=%1", candidate.m_aEntries.Count());
		ME_VehicleBoundsPerPrefabSnapshot reloadedCandidate;
		if (!ME_SaveAndValidateCandidateSnapshot(candidate, reloadedCandidate, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_per_prefab_compare status=FAIL reason=%1 candidate=%2", reason, CANDIDATE_PATH);
			return;
		}

		PrintFormat("[ME_DEBUG_AVSP_WB] bounds_per_prefab_compare phase=candidate status=PASS candidate=%1 game_version=%2 count=%3", CANDIDATE_PATH, reloadedCandidate.m_sGameVersion, ME_GetNormalizedPerPrefabCount(reloadedCandidate));
		ME_VehicleBoundsPerPrefabSnapshot baseline;
		if (!ME_LoadBaselineSnapshot(baseline, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_per_prefab_compare status=FAIL reason=%1 candidate_written=1", reason);
			return;
		}

		ME_ComparePerPrefabSnapshots(baseline, reloadedCandidate);
	}

	//------------------------------------------------------------------------------------------------
	//! Adds every faction-owned catalog candidate to aggregate and per-prefab models.
	//! Добавляет каждого кандидата каталога, принадлежащего фракции, в aggregate и per-prefab модели.
	protected bool ME_AccumulateSpawnPoint(WorldEditorAPI api, array<IEntity> markerRoots, string entityName, ME_VehicleBoundsSnapshot snapshot, ME_VehicleBoundsPerPrefabSnapshot candidate, out string reason)
	{
		reason = "";
		IEntitySource source = api.FindEntityByName(entityName);
		IEntity entity;
		if (source)
			entity = api.SourceToEntity(source);
		SCR_AmbientVehicleSpawnPointComponent spawnPoint;
		if (entity)
			spawnPoint = SCR_AmbientVehicleSpawnPointComponent.Cast(entity.FindComponent(SCR_AmbientVehicleSpawnPointComponent));
		if (!spawnPoint) { reason = "spawn_point_unavailable"; return false; }
		string factionKey; array<string> selectedTypes; array<SCR_EntityCatalogEntry> entries;
		if (!spawnPoint.ME_GetEditorVehicleAggregateSelection(factionKey, selectedTypes, entries, reason)) return false;
		if (factionKey.IsEmpty() || selectedTypes.IsEmpty()) { reason = "aggregate_selection_empty"; return false; }

		foreach (SCR_EntityCatalogEntry catalogEntry : entries)
		{
			string prefabPath = catalogEntry.GetPrefab();
			if (prefabPath.IsEmpty()) { reason = "candidate_prefab_path_empty"; return false; }
			if (!m_aCatalogPrefabPaths.Contains(prefabPath))
				m_aCatalogPrefabPaths.Insert(prefabPath);

			vector localMins;
			vector localMaxs;
			if (!ME_GetMeasuredBounds(markerRoots, prefabPath, localMins, localMaxs, reason))
				return false;

			ME_VehicleBoundsPerPrefabSnapshotEntry perPrefabEntry = ME_FindPerPrefabEntry(candidate, prefabPath);
			if (!perPrefabEntry)
			{
				perPrefabEntry = new ME_VehicleBoundsPerPrefabSnapshotEntry();
				perPrefabEntry.m_sPrefab = prefabPath;
				perPrefabEntry.m_vLocalMins = localMins;
				perPrefabEntry.m_vLocalMaxs = localMaxs;
				perPrefabEntry.m_sFactionKey = factionKey;
				candidate.m_aEntries.Insert(perPrefabEntry);
			}
			else
			{
				if (!ME_AreVectorsClose(perPrefabEntry.m_vLocalMins, localMins) || !ME_AreVectorsClose(perPrefabEntry.m_vLocalMaxs, localMaxs))
				{
					reason = string.Format("cached_bounds_mismatch path=%1", prefabPath);
					return false;
				}

				if (perPrefabEntry.m_sFactionKey != factionKey)
				{
					reason = string.Format("multiple_prefab_factions path=%1 first=%2 second=%3", prefabPath, perPrefabEntry.m_sFactionKey, factionKey);
					return false;
				}
			}

			array<EEditableEntityLabel> labels = {};
			catalogEntry.GetEditableEntityLabels(labels);
			bool hasVehicleType;
			foreach (EEditableEntityLabel label : labels)
			{
				string aggregateType = typename.EnumToString(EEditableEntityLabel, label);
				if (ME_IsVehicleTypeName(aggregateType))
				{
					hasVehicleType = true;
					if (perPrefabEntry.m_sVehicleType.IsEmpty())
						perPrefabEntry.m_sVehicleType = aggregateType;
					else if (perPrefabEntry.m_sVehicleType != aggregateType)
					{
						reason = string.Format("multiple_prefab_vehicle_types path=%1 first=%2 second=%3", prefabPath, perPrefabEntry.m_sVehicleType, aggregateType);
						return false;
					}
				}
				if (!aggregateType.Contains("VEHICLE_"))
					continue;
				string candidateKey = factionKey + "\x1F" + aggregateType + "\x1F" + prefabPath;
				if (m_aProcessedCandidates.Contains(candidateKey))
					continue;
				if (!ME_AccumulateCandidate(snapshot, factionKey, aggregateType, prefabPath, localMins, localMaxs, reason))
					return false;
				m_aProcessedCandidates.Insert(candidateKey);
			}
			if (!hasVehicleType)
			{
				reason = string.Format("candidate_vehicle_type_missing path=%1", prefabPath);
				return false;
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns cached local bounds or measures one fixture root exactly once.
	//! Возвращает кэшированные локальные границы либо измеряет один корень fixture ровно один раз.
	protected bool ME_GetMeasuredBounds(array<IEntity> markerRoots, string prefabPath, out vector localMins, out vector localMaxs, out string reason)
	{
		reason = "";
		int measuredIndex = m_aMeasuredPrefabPaths.Find(prefabPath);
		if (measuredIndex >= 0)
		{
			localMins = m_aMeasuredPrefabMins[measuredIndex];
			localMaxs = m_aMeasuredPrefabMaxs[measuredIndex];
			return true;
		}

		IEntity markerRoot = ME_FindMarkerRoot(markerRoots, prefabPath);
		if (!markerRoot) { reason = string.Format("marker_root_unavailable path=%1", prefabPath); return false; }
		vector worldMins;
		vector worldMaxs;
		SCR_Global.GetWorldBoundsWithChildren(markerRoot, worldMins, worldMaxs);
		localMins = worldMins - markerRoot.GetOrigin();
		localMaxs = worldMaxs - markerRoot.GetOrigin();
		if (!ME_AreFiniteOrderedBounds(localMins, localMaxs)) { reason = string.Format("invalid_local_bounds path=%1", prefabPath); return false; }
		m_aMeasuredPrefabPaths.Insert(prefabPath);
		m_aMeasuredPrefabMins.Insert(localMins);
		m_aMeasuredPrefabMaxs.Insert(localMaxs);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Updates one faction/type aggregate with a unique measured prefab and deterministic provenance.
	//! Обновляет один aggregate фракции/типа уникальным измеренным prefab и детерминированным provenance.
	protected bool ME_AccumulateCandidate(ME_VehicleBoundsSnapshot snapshot, string factionKey, string vehicleType, string prefabPath, vector localMins, vector localMaxs, out string reason)
	{
		reason = "";
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
		}
		else
		{
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
		m_aExpectedCandidateKeys.Insert(factionKey + "\x1F" + vehicleType + "\x1F" + prefabPath);
		m_aExpectedCandidateMins.Insert(localMins);
		m_aExpectedCandidateMaxs.Insert(localMaxs);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns one axis provenance field.
	//! Возвращает одно поле provenance для оси.
	protected string ME_GetSourcePrefab(ME_VehicleBoundsSnapshotEntry entry, int axis, bool isMax)
	{
		if (axis == 0) { if (isMax) return entry.m_sMaxXSourcePrefab; return entry.m_sMinXSourcePrefab; }
		if (axis == 1) { if (isMax) return entry.m_sMaxYSourcePrefab; return entry.m_sMinYSourcePrefab; }
		if (isMax) return entry.m_sMaxZSourcePrefab;
		return entry.m_sMinZSourcePrefab;
	}

	//------------------------------------------------------------------------------------------------
	//! Sets one axis provenance field.
	//! Устанавливает одно поле provenance для оси.
	protected void ME_SetSourcePrefab(ME_VehicleBoundsSnapshotEntry entry, int axis, bool isMax, string prefabPath) { if (axis == 0) { if (isMax) entry.m_sMaxXSourcePrefab = prefabPath; else entry.m_sMinXSourcePrefab = prefabPath; return; } if (axis == 1) { if (isMax) entry.m_sMaxYSourcePrefab = prefabPath; else entry.m_sMinYSourcePrefab = prefabPath; return; } if (isMax) entry.m_sMaxZSourcePrefab = prefabPath; else entry.m_sMinZSourcePrefab = prefabPath; }

	//------------------------------------------------------------------------------------------------
	//! Sorts aggregate entries by faction key and vehicle type.
	//! Сортирует записи aggregate по ключу фракции и типу техники.
	protected void ME_SortEntries(array<ref ME_VehicleBoundsSnapshotEntry> entries) { for (int i = 1; i < entries.Count(); i++) { ME_VehicleBoundsSnapshotEntry value = entries[i]; int j = i - 1; while (j >= 0 && entries[j].m_sFactionKey + "\x1F" + entries[j].m_sVehicleType > value.m_sFactionKey + "\x1F" + value.m_sVehicleType) { entries[j + 1] = entries[j]; j--; } entries[j + 1] = value; } }

	//------------------------------------------------------------------------------------------------
	//! Sorts per-prefab entries by canonical prefab path.
	//! Сортирует per-prefab записи по каноническому пути prefab.
	protected void ME_SortPerPrefabEntries(array<ref ME_VehicleBoundsPerPrefabSnapshotEntry> entries)
	{
		for (int i = 1; i < entries.Count(); i++)
		{
			ME_VehicleBoundsPerPrefabSnapshotEntry value = entries[i];
			int j = i - 1;
			while (j >= 0 && entries[j].m_sPrefab > value.m_sPrefab)
			{
				entries[j + 1] = entries[j];
				j--;
			}
			entries[j + 1] = value;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Collects every uniquely declared fixture marker root.
	//! Собирает каждый уникально объявленный корень marker fixture.
	protected bool ME_CollectMarkerRoots(WorldEditorAPI api, out array<IEntity> markerRoots, out string reason)
	{
		markerRoots = {};
		reason = "";
		for (int index = 0; index < api.GetEditorEntityCount(); index++)
		{
			IEntity entity = api.SourceToEntity(api.GetEditorEntity(index));
			if (!entity)
				continue;
			ME_VehicleBoundsFixtureMarkerComponent marker = ME_VehicleBoundsFixtureMarkerComponent.Cast(entity.FindComponent(ME_VehicleBoundsFixtureMarkerComponent));
			if (!marker)
				continue;
			if (marker.GetCatalogPrefab().IsEmpty()) { reason = "marker_prefab_path_empty"; return false; }
			if (ME_FindMarkerRoot(markerRoots, marker.GetCatalogPrefab())) { reason = string.Format("duplicate_marker_prefab path=%1", marker.GetCatalogPrefab()); return false; }
			markerRoots.Insert(entity);
		}
		if (markerRoots.IsEmpty()) { reason = "no_fixture_markers"; return false; }
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds the fixture root declared for one canonical catalog prefab path.
	//! Находит корень fixture, объявленный для одного канонического пути prefab каталога.
	protected IEntity ME_FindMarkerRoot(array<IEntity> markerRoots, string prefabPath) { foreach (IEntity root : markerRoots) { ME_VehicleBoundsFixtureMarkerComponent marker = ME_VehicleBoundsFixtureMarkerComponent.Cast(root.FindComponent(ME_VehicleBoundsFixtureMarkerComponent)); if (marker && marker.GetCatalogPrefab() == prefabPath) return root; } return null; }

	//------------------------------------------------------------------------------------------------
	//! Finds one aggregate without cross-faction fallback.
	//! Находит один aggregate без fallback между фракциями.
	protected ME_VehicleBoundsSnapshotEntry ME_FindAggregate(ME_VehicleBoundsSnapshot snapshot, string factionKey, string vehicleType)
	{
		foreach (ME_VehicleBoundsSnapshotEntry entry : snapshot.m_aEntries)
			if (entry.m_sFactionKey == factionKey && entry.m_sVehicleType == vehicleType) return entry;
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds one per-prefab entry by canonical path.
	//! Находит одну per-prefab запись по каноническому пути.
	protected ME_VehicleBoundsPerPrefabSnapshotEntry ME_FindPerPrefabEntry(ME_VehicleBoundsPerPrefabSnapshot snapshot, string prefabPath)
	{
		foreach (ME_VehicleBoundsPerPrefabSnapshotEntry entry : snapshot.m_aEntries)
			if (entry.m_sPrefab == prefabPath) return entry;
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns whether a label name is an actual vehicle classification rather than a VEHICLE_* trait.
	//! Возвращает, является ли имя метки фактической классификацией техники, а не trait с VEHICLE_* в имени.
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
			if (mins[axis] != mins[axis] || maxs[axis] != maxs[axis] || Math.AbsFloat(mins[axis]) > 1000000 || Math.AbsFloat(maxs[axis]) > 1000000 || mins[axis] > maxs[axis]) return false;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Validates one provenance field against its measured candidate extrema.
	//! Проверяет одно поле provenance относительно измеренного экстремума кандидата.
	protected bool ME_ValidateSource(ME_VehicleBoundsSnapshotEntry entry, string prefix, string sourcePrefab, int axis, bool isMax, out string reason)
	{
		reason = "";
		if (sourcePrefab.IsEmpty()) { reason = "expected_source_empty"; return false; }
		int candidateIndex = m_aExpectedCandidateKeys.Find(prefix + sourcePrefab);
		if (candidateIndex < 0) { reason = "expected_source_not_in_group"; return false; }
		vector candidateMins = m_aExpectedCandidateMins[candidateIndex];
		vector candidateMaxs = m_aExpectedCandidateMaxs[candidateIndex];
		float expectedValue;
		float aggregateValue;
		if (isMax) { expectedValue = candidateMaxs[axis]; aggregateValue = entry.m_vLocalMaxs[axis]; }
		else { expectedValue = candidateMins[axis]; aggregateValue = entry.m_vLocalMins[axis]; }
		if (Math.AbsFloat(expectedValue - aggregateValue) > 0.0011) { reason = "expected_source_extreme_mismatch"; return false; }
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Validates expected aggregate candidate counts, provenance membership, and extrema.
	//! Проверяет ожидаемые counts кандидатов aggregate, принадлежность provenance и экстремумы.
	protected bool ME_ValidateExpectedSnapshot(ME_VehicleBoundsSnapshot snapshot, out string reason)
	{
		reason = "";
		foreach (ME_VehicleBoundsSnapshotEntry entry : snapshot.m_aEntries)
		{
			string prefix = entry.m_sFactionKey + "\x1F" + entry.m_sVehicleType + "\x1F";
			int count;
			foreach (string key : m_aExpectedCandidateKeys)
				if (key.Contains(prefix)) count++;
			if (count != entry.m_iCandidateCount) { reason = "expected_candidate_count_mismatch"; return false; }
			string sourceReason;
			if (!ME_ValidateSource(entry, prefix, entry.m_sMinXSourcePrefab, 0, false, sourceReason) || !ME_ValidateSource(entry, prefix, entry.m_sMaxXSourcePrefab, 0, true, sourceReason) || !ME_ValidateSource(entry, prefix, entry.m_sMinYSourcePrefab, 1, false, sourceReason) || !ME_ValidateSource(entry, prefix, entry.m_sMaxYSourcePrefab, 1, true, sourceReason) || !ME_ValidateSource(entry, prefix, entry.m_sMinZSourcePrefab, 2, false, sourceReason) || !ME_ValidateSource(entry, prefix, entry.m_sMaxZSourcePrefab, 2, true, sourceReason)) { reason = sourceReason; return false; }
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Requires exact one-to-one path coverage among catalog, marker, measurement, and Candidate sets.
	//! Требует точного one-to-one покрытия путей между наборами catalog, marker, measurement и Candidate.
	protected bool ME_ValidatePerPrefabCoverage(array<IEntity> markerRoots, ME_VehicleBoundsPerPrefabSnapshot candidate, out string reason)
	{
		reason = "";
		array<ref ME_VehicleBoundsPerPrefabSnapshotEntry> candidateEntries;
		bool candidateClassificationAvailable;
		if (!ME_NormalizePerPrefabSnapshot(candidate, candidateEntries, candidateClassificationAvailable, reason)) return false;
		array<string> catalogPaths = {};
		catalogPaths.Copy(m_aCatalogPrefabPaths);
		catalogPaths.Sort();
		array<string> markerPaths = {};
		foreach (IEntity root : markerRoots)
		{
			ME_VehicleBoundsFixtureMarkerComponent marker = ME_VehicleBoundsFixtureMarkerComponent.Cast(root.FindComponent(ME_VehicleBoundsFixtureMarkerComponent));
			markerPaths.Insert(marker.GetCatalogPrefab());
		}
		markerPaths.Sort();
		array<string> measuredPaths = {};
		measuredPaths.Copy(m_aMeasuredPrefabPaths);
		measuredPaths.Sort();
		if (catalogPaths.Count() != markerPaths.Count() || catalogPaths.Count() != measuredPaths.Count() || catalogPaths.Count() != candidateEntries.Count())
		{
			reason = string.Format("per_prefab_coverage_count_mismatch catalog=%1 markers=%2 measured=%3 candidate=%4", catalogPaths.Count(), markerPaths.Count(), measuredPaths.Count(), candidateEntries.Count());
			return false;
		}
		for (int index = 0; index < catalogPaths.Count(); index++)
		{
			if (index > 0 && catalogPaths[index] == catalogPaths[index - 1]) { reason = string.Format("duplicate_catalog_prefab path=%1", catalogPaths[index]); return false; }
			if (catalogPaths[index] != markerPaths[index] || catalogPaths[index] != measuredPaths[index] || catalogPaths[index] != candidateEntries[index].m_sPrefab)
			{
				reason = string.Format("per_prefab_coverage_path_mismatch catalog=%1 marker=%2 measured=%3 candidate=%4", catalogPaths[index], markerPaths[index], measuredPaths[index], candidateEntries[index].m_sPrefab);
				return false;
			}
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Validates either the grouped Candidate representation or the legacy flat Baseline representation.
	//! Проверяет либо сгруппированное представление Candidate, либо legacy плоское представление Baseline.
	protected bool ME_ValidatePerPrefabSnapshot(ME_VehicleBoundsPerPrefabSnapshot snapshot, bool requireCandidateMetadata, out string reason)
	{
		reason = "";
		if (!snapshot || snapshot.m_sGeneratorVersion.IsEmpty() || snapshot.m_sFixtureIdentity != FIXTURE_IDENTITY || snapshot.m_sGameVersion.IsEmpty()) { reason = "per_prefab_metadata_invalid"; return false; }
		bool hasFlat = snapshot.m_aEntries && !snapshot.m_aEntries.IsEmpty();
		bool hasGrouped = snapshot.m_aFactionGroups && !snapshot.m_aFactionGroups.IsEmpty();
		if (hasFlat == hasGrouped) { reason = "per_prefab_representation_invalid"; return false; }

		if (requireCandidateMetadata)
		{
			if (snapshot.m_iSchemaVersion != PER_PREFAB_SCHEMA_VERSION || snapshot.m_sGeneratorVersion != PER_PREFAB_GENERATOR_VERSION || !snapshot.m_bClassificationMetadataAvailable || !hasGrouped) { reason = "candidate_metadata_invalid"; return false; }
			return ME_ValidateGroupedPerPrefabSnapshot(snapshot, reason);
		}

		if (snapshot.m_iSchemaVersion != 2 || !hasFlat) { reason = "baseline_legacy_schema_invalid"; return false; }
		return ME_ValidateFlatPerPrefabEntries(snapshot, snapshot.m_aEntries, reason);
	}

	//------------------------------------------------------------------------------------------------
	//! Validates the flat canonical model before it is converted into grouped serialized data.
	//! Проверяет плоскую каноническую модель перед преобразованием в сгруппированные сериализованные данные.
	protected bool ME_ValidatePerPrefabCanonicalSnapshot(ME_VehicleBoundsPerPrefabSnapshot snapshot, bool requireCandidateMetadata, out string reason)
	{
		reason = "";
		if (!snapshot || snapshot.m_iSchemaVersion != PER_PREFAB_SCHEMA_VERSION || snapshot.m_sGeneratorVersion != PER_PREFAB_GENERATOR_VERSION || snapshot.m_sFixtureIdentity != FIXTURE_IDENTITY || snapshot.m_sGameVersion.IsEmpty() || !snapshot.m_bClassificationMetadataAvailable || !snapshot.m_aEntries || snapshot.m_aEntries.IsEmpty()) { reason = "candidate_canonical_metadata_invalid"; return false; }
		return ME_ValidateFlatPerPrefabEntries(snapshot, snapshot.m_aEntries, reason);
	}

	//------------------------------------------------------------------------------------------------
	//! Validates flat entries, including ordering, bounds, classification, and unique prefab paths.
	//! Проверяет плоские записи, включая порядок, границы, классификацию и уникальность путей prefab.
	protected bool ME_ValidateFlatPerPrefabEntries(ME_VehicleBoundsPerPrefabSnapshot snapshot, array<ref ME_VehicleBoundsPerPrefabSnapshotEntry> entries, out string reason)
	{
		reason = "";
		if (!entries || entries.IsEmpty()) { reason = "per_prefab_entries_empty"; return false; }
		string previousPath;
		foreach (ME_VehicleBoundsPerPrefabSnapshotEntry entry : entries)
		{
			if (!entry || entry.m_sPrefab.IsEmpty() || !previousPath.IsEmpty() && entry.m_sPrefab <= previousPath) { reason = "per_prefab_entry_order_or_identity_invalid"; return false; }
			if (!ME_AreFiniteOrderedBounds(entry.m_vLocalMins, entry.m_vLocalMaxs)) { reason = string.Format("per_prefab_bounds_invalid path=%1", entry.m_sPrefab); return false; }
			if (snapshot.m_bClassificationMetadataAvailable)
			{
				if (entry.m_sFactionKey.IsEmpty() || entry.m_sVehicleType.IsEmpty()) { reason = string.Format("per_prefab_classification_invalid path=%1", entry.m_sPrefab); return false; }
				if (!ME_IsVehicleTypeName(entry.m_sVehicleType)) { reason = string.Format("per_prefab_vehicle_type_invalid path=%1 type=%2", entry.m_sPrefab, entry.m_sVehicleType); return false; }
			}
			previousPath = entry.m_sPrefab;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Validates grouped faction, vehicle-type, and prefab-entry ordering and identity.
	//! Проверяет порядок и идентичность сгруппированных фракций, типов техники и prefab-записей.
	protected bool ME_ValidateGroupedPerPrefabSnapshot(ME_VehicleBoundsPerPrefabSnapshot snapshot, out string reason)
	{
		reason = "";
		if (!snapshot.m_aFactionGroups || snapshot.m_aFactionGroups.IsEmpty()) { reason = "per_prefab_faction_groups_empty"; return false; }
		string previousFaction;
		array<string> prefabPaths = {};
		array<string> prefabNames = {};
		foreach (ME_VehicleBoundsPerPrefabSnapshotFactionGroup factionGroup : snapshot.m_aFactionGroups)
		{
			if (!factionGroup || factionGroup.m_sFactionKey.IsEmpty() || !previousFaction.IsEmpty() && factionGroup.m_sFactionKey <= previousFaction || !factionGroup.m_aVehicleTypeGroups || factionGroup.m_aVehicleTypeGroups.IsEmpty()) { reason = "per_prefab_faction_group_invalid"; return false; }
			string previousType;
			foreach (ME_VehicleBoundsPerPrefabSnapshotVehicleTypeGroup typeGroup : factionGroup.m_aVehicleTypeGroups)
			{
				if (!typeGroup || typeGroup.m_sVehicleType.IsEmpty() || !ME_IsVehicleTypeName(typeGroup.m_sVehicleType) || !previousType.IsEmpty() && typeGroup.m_sVehicleType <= previousType || !typeGroup.m_aEntries || typeGroup.m_aEntries.IsEmpty()) { reason = "per_prefab_vehicle_type_group_invalid"; return false; }
				string previousPath;
				foreach (ME_VehicleBoundsPerPrefabSnapshotGroupedEntry entry : typeGroup.m_aEntries)
				{
					if (!entry || entry.m_sPrefab.IsEmpty() || !previousPath.IsEmpty() && entry.m_sPrefab <= previousPath || prefabPaths.Contains(entry.m_sPrefab)) { reason = "per_prefab_grouped_entry_order_or_identity_invalid"; return false; }
					if (!ME_AreFiniteOrderedBounds(entry.m_vLocalMins, entry.m_vLocalMaxs)) { reason = string.Format("per_prefab_bounds_invalid path=%1", entry.m_sPrefab); return false; }
					string prefabName;
					if (!ME_GetPrefabEntryContainerName(entry.m_sPrefab, prefabName) || !ME_IsValidContainerName(prefabName) || prefabNames.Contains(prefabName)) { reason = string.Format("per_prefab_container_name_invalid path=%1", entry.m_sPrefab); return false; }
					prefabPaths.Insert(entry.m_sPrefab);
					prefabNames.Insert(prefabName);
					previousPath = entry.m_sPrefab;
				}
				previousType = typeGroup.m_sVehicleType;
			}
			previousFaction = factionGroup.m_sFactionKey;
		}
		return !prefabPaths.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	//! Normalizes either flat or grouped snapshot data into sorted canonical entries.
	//! Нормализует плоские или сгруппированные данные snapshot в отсортированные канонические записи.
	protected bool ME_NormalizePerPrefabSnapshot(ME_VehicleBoundsPerPrefabSnapshot snapshot, out array<ref ME_VehicleBoundsPerPrefabSnapshotEntry> entries, out bool classificationAvailable, out string reason)
	{
		entries = {};
		classificationAvailable = false;
		reason = "";
		if (!snapshot || snapshot.m_sFixtureIdentity != FIXTURE_IDENTITY || snapshot.m_sGameVersion.IsEmpty()) { reason = "per_prefab_normalize_metadata_invalid"; return false; }
		bool hasFlat = snapshot.m_aEntries && !snapshot.m_aEntries.IsEmpty();
		bool hasGrouped = snapshot.m_aFactionGroups && !snapshot.m_aFactionGroups.IsEmpty();
		if (hasFlat == hasGrouped) { reason = "per_prefab_representation_invalid"; return false; }
		classificationAvailable = snapshot.m_bClassificationMetadataAvailable;
		if (hasGrouped)
		{
			if (snapshot.m_iSchemaVersion != PER_PREFAB_SCHEMA_VERSION || !classificationAvailable || !ME_ValidateGroupedPerPrefabSnapshot(snapshot, reason)) return false;
			foreach (ME_VehicleBoundsPerPrefabSnapshotFactionGroup factionGroup : snapshot.m_aFactionGroups)
				foreach (ME_VehicleBoundsPerPrefabSnapshotVehicleTypeGroup typeGroup : factionGroup.m_aVehicleTypeGroups)
					foreach (ME_VehicleBoundsPerPrefabSnapshotGroupedEntry groupedEntry : typeGroup.m_aEntries)
					{
						ME_VehicleBoundsPerPrefabSnapshotEntry entry = new ME_VehicleBoundsPerPrefabSnapshotEntry();
						entry.m_sPrefab = groupedEntry.m_sPrefab;
						entry.m_vLocalMins = groupedEntry.m_vLocalMins;
						entry.m_vLocalMaxs = groupedEntry.m_vLocalMaxs;
						entry.m_sFactionKey = factionGroup.m_sFactionKey;
						entry.m_sVehicleType = typeGroup.m_sVehicleType;
						entries.Insert(entry);
					}
		}
		else
		{
			if (snapshot.m_iSchemaVersion != 2 && snapshot.m_iSchemaVersion != PER_PREFAB_SCHEMA_VERSION || !ME_ValidateFlatPerPrefabEntries(snapshot, snapshot.m_aEntries, reason)) return false;
			foreach (ME_VehicleBoundsPerPrefabSnapshotEntry flatEntry : snapshot.m_aEntries)
			{
				ME_VehicleBoundsPerPrefabSnapshotEntry entry = new ME_VehicleBoundsPerPrefabSnapshotEntry();
				entry.m_sPrefab = flatEntry.m_sPrefab;
				entry.m_vLocalMins = flatEntry.m_vLocalMins;
				entry.m_vLocalMaxs = flatEntry.m_vLocalMaxs;
				entry.m_sFactionKey = flatEntry.m_sFactionKey;
				entry.m_sVehicleType = flatEntry.m_sVehicleType;
				entries.Insert(entry);
			}
		}
		ME_SortPerPrefabEntries(entries);
		return !entries.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the number of entries in a normalized flat snapshot model.
	//! Возвращает количество записей в нормализованной плоской модели snapshot.
	protected int ME_GetNormalizedPerPrefabCount(ME_VehicleBoundsPerPrefabSnapshot snapshot)
	{
		array<ref ME_VehicleBoundsPerPrefabSnapshotEntry> entries;
		bool classificationAvailable;
		string reason;
		if (!ME_NormalizePerPrefabSnapshot(snapshot, entries, classificationAvailable, reason)) return 0;
		return entries.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! Checks whether a generated container name is non-empty and safe for resource serialization.
	//! Проверяет, что сгенерированное имя контейнера непустое и безопасно для сериализации ресурса.
	protected bool ME_IsValidContainerName(string name)
	{
		return !name.IsEmpty() && !name.Contains(" ") && !name.Contains("/") && !name.Contains("\\") && !name.Contains("\"") && !name.Contains("{") && !name.Contains("}");
	}

	//------------------------------------------------------------------------------------------------
	//! Assigns stable names to serialized entry containers so repeated generation produces identical files.
	//! Назначает стабильные имена сериализованным контейнерам записей, чтобы повторная генерация создавала идентичные файлы.
	protected bool ME_SetDeterministicEntryContainerNames(BaseContainer container, int expectedCount, string namePrefix, out string reason)
	{
		BaseContainerList entries = container.GetObjectArray("m_aEntries");
		if (!entries || entries.Count() != expectedCount) { reason = "serialized_entry_container_count_mismatch"; return false; }
		for (int index = 0; index < entries.Count(); index++)
		{
			BaseContainer entry = entries.Get(index);
			if (!entry) { reason = "serialized_entry_container_missing"; return false; }
			entry.SetName(string.Format("%1_%2", namePrefix, index));
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Derives a readable serialized container name from the terminal .et prefab filename.
	//! Извлекает читаемое имя сериализованного контейнера из конечного имени prefab-файла .et.
	protected bool ME_GetPrefabEntryContainerName(string prefabPath, out string containerName)
	{
		containerName = "";
		if (prefabPath.IsEmpty())
			return false;

		int slashIndex = prefabPath.LastIndexOf("/");
		int startIndex = slashIndex + 1;
		int filenameLength = prefabPath.Length() - startIndex;
		if (filenameLength <= 3)
			return false;

		string filename = prefabPath.Substring(startIndex, filenameLength);
		if (!filename.EndsWith(".et"))
			return false;

		containerName = filename.Substring(0, filename.Length() - 3);
		return !containerName.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	//! Builds the grouped serialized Candidate model from the sorted flat canonical model.
	//! Создаёт сгруппированную сериализуемую модель Candidate из отсортированной плоской канонической модели.
	protected bool ME_BuildGroupedPerPrefabSnapshot(ME_VehicleBoundsPerPrefabSnapshot canonical, out ME_VehicleBoundsPerPrefabSnapshot grouped, out string reason)
	{
		grouped = null;
		reason = "";
		if (!ME_ValidatePerPrefabCanonicalSnapshot(canonical, true, reason)) return false;
		grouped = new ME_VehicleBoundsPerPrefabSnapshot();
		grouped.m_iSchemaVersion = canonical.m_iSchemaVersion;
		grouped.m_sGeneratorVersion = canonical.m_sGeneratorVersion;
		grouped.m_sFixtureIdentity = canonical.m_sFixtureIdentity;
		grouped.m_sGameVersion = canonical.m_sGameVersion;
		grouped.m_bClassificationMetadataAvailable = canonical.m_bClassificationMetadataAvailable;
		grouped.m_aFactionGroups = {};
		array<string> prefabNames = {};
		foreach (ME_VehicleBoundsPerPrefabSnapshotEntry flatEntry : canonical.m_aEntries)
		{
			string prefabName;
			if (!ME_GetPrefabEntryContainerName(flatEntry.m_sPrefab, prefabName) || !ME_IsValidContainerName(prefabName) || prefabNames.Contains(prefabName))
			{
				reason = string.Format("per_prefab_container_name_invalid path=%1", flatEntry.m_sPrefab);
				grouped = null;
				return false;
			}
			prefabNames.Insert(prefabName);
			ME_VehicleBoundsPerPrefabSnapshotFactionGroup factionGroup;
			foreach (ME_VehicleBoundsPerPrefabSnapshotFactionGroup existingFaction : grouped.m_aFactionGroups)
				if (existingFaction.m_sFactionKey == flatEntry.m_sFactionKey) factionGroup = existingFaction;
			if (!factionGroup)
			{
				factionGroup = new ME_VehicleBoundsPerPrefabSnapshotFactionGroup();
				factionGroup.m_sFactionKey = flatEntry.m_sFactionKey;
				factionGroup.m_aVehicleTypeGroups = {};
				grouped.m_aFactionGroups.Insert(factionGroup);
			}
			ME_VehicleBoundsPerPrefabSnapshotVehicleTypeGroup typeGroup;
			foreach (ME_VehicleBoundsPerPrefabSnapshotVehicleTypeGroup existingType : factionGroup.m_aVehicleTypeGroups)
				if (existingType.m_sVehicleType == flatEntry.m_sVehicleType) typeGroup = existingType;
			if (!typeGroup)
			{
				typeGroup = new ME_VehicleBoundsPerPrefabSnapshotVehicleTypeGroup();
				typeGroup.m_sVehicleType = flatEntry.m_sVehicleType;
				typeGroup.m_aEntries = {};
				factionGroup.m_aVehicleTypeGroups.Insert(typeGroup);
			}
			ME_VehicleBoundsPerPrefabSnapshotGroupedEntry groupedEntry = new ME_VehicleBoundsPerPrefabSnapshotGroupedEntry();
			groupedEntry.m_sPrefab = flatEntry.m_sPrefab;
			groupedEntry.m_vLocalMins = flatEntry.m_vLocalMins;
			groupedEntry.m_vLocalMaxs = flatEntry.m_vLocalMaxs;
			typeGroup.m_aEntries.Insert(groupedEntry);
		}
		ME_SortFactionGroups(grouped.m_aFactionGroups);
		if (!ME_ValidatePerPrefabSnapshot(grouped, true, reason)) { grouped = null; return false; }
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Sorts faction groups, their vehicle types, and their grouped prefab entries deterministically.
	//! Сортирует группы фракций, их типы техники и сгруппированные prefab-записи детерминированно.
	protected void ME_SortFactionGroups(array<ref ME_VehicleBoundsPerPrefabSnapshotFactionGroup> groups)
	{
		for (int i = 1; i < groups.Count(); i++)
		{
			ME_VehicleBoundsPerPrefabSnapshotFactionGroup factionValue = groups[i];
			int factionIndex = i - 1;
			while (factionIndex >= 0 && groups[factionIndex].m_sFactionKey > factionValue.m_sFactionKey) { groups[factionIndex + 1] = groups[factionIndex]; factionIndex--; }
			groups[factionIndex + 1] = factionValue;
		}
		foreach (ME_VehicleBoundsPerPrefabSnapshotFactionGroup factionGroup : groups)
		{
			for (int typeIndex = 1; typeIndex < factionGroup.m_aVehicleTypeGroups.Count(); typeIndex++)
			{
				ME_VehicleBoundsPerPrefabSnapshotVehicleTypeGroup typeValue = factionGroup.m_aVehicleTypeGroups[typeIndex];
				int previousTypeIndex = typeIndex - 1;
				while (previousTypeIndex >= 0 && factionGroup.m_aVehicleTypeGroups[previousTypeIndex].m_sVehicleType > typeValue.m_sVehicleType) { factionGroup.m_aVehicleTypeGroups[previousTypeIndex + 1] = factionGroup.m_aVehicleTypeGroups[previousTypeIndex]; previousTypeIndex--; }
				factionGroup.m_aVehicleTypeGroups[previousTypeIndex + 1] = typeValue;
			}
			foreach (ME_VehicleBoundsPerPrefabSnapshotVehicleTypeGroup typeGroup : factionGroup.m_aVehicleTypeGroups)
				ME_SortGroupedEntries(typeGroup.m_aEntries);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Sorts grouped prefab entries by canonical prefab path.
	//! Сортирует сгруппированные prefab-записи по каноническому пути prefab.
	protected void ME_SortGroupedEntries(array<ref ME_VehicleBoundsPerPrefabSnapshotGroupedEntry> entries)
	{
		for (int i = 1; i < entries.Count(); i++)
		{
			ME_VehicleBoundsPerPrefabSnapshotGroupedEntry value = entries[i];
			int index = i - 1;
			while (index >= 0 && entries[index].m_sPrefab > value.m_sPrefab) { entries[index + 1] = entries[index]; index--; }
			entries[index + 1] = value;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Names nested faction, vehicle-type, and prefab containers and rejects name collisions.
	//! Именует вложенные контейнеры фракций, типов техники и prefab и отклоняет коллизии имён.
	protected bool ME_SetGroupedPerPrefabContainerNames(BaseContainer container, ME_VehicleBoundsPerPrefabSnapshot snapshot, out string reason)
	{
		reason = "";
		BaseContainerList factionContainers = container.GetObjectArray("m_aFactionGroups");
		if (!factionContainers || factionContainers.Count() != snapshot.m_aFactionGroups.Count()) { reason = "serialized_faction_container_count_mismatch"; return false; }
		array<string> assignedFactionNames = {};
		array<string> assignedPrefabNames = {};
		for (int factionIndex = 0; factionIndex < factionContainers.Count(); factionIndex++)
		{
			BaseContainer factionContainer = factionContainers.Get(factionIndex);
			ME_VehicleBoundsPerPrefabSnapshotFactionGroup factionGroup = snapshot.m_aFactionGroups[factionIndex];
			string factionName = factionGroup.m_sFactionKey;
			if (!factionContainer || !ME_IsValidContainerName(factionName) || assignedFactionNames.Contains(factionName)) { reason = "serialized_faction_container_name_invalid"; return false; }
			factionContainer.SetName(factionName);
			assignedFactionNames.Insert(factionName);
			BaseContainerList typeContainers = factionContainer.GetObjectArray("m_aVehicleTypeGroups");
			if (!typeContainers || typeContainers.Count() != factionGroup.m_aVehicleTypeGroups.Count()) { reason = "serialized_type_container_count_mismatch"; return false; }
			array<string> assignedTypeNames = {};
			for (int typeIndex = 0; typeIndex < typeContainers.Count(); typeIndex++)
			{
				BaseContainer typeContainer = typeContainers.Get(typeIndex);
				ME_VehicleBoundsPerPrefabSnapshotVehicleTypeGroup typeGroup = factionGroup.m_aVehicleTypeGroups[typeIndex];
				string typeName = typeGroup.m_sVehicleType;
				if (!typeContainer || !ME_IsValidContainerName(typeName) || assignedTypeNames.Contains(typeName)) { reason = "serialized_type_container_name_invalid"; return false; }
				typeContainer.SetName(typeName);
				assignedTypeNames.Insert(typeName);
				BaseContainerList entryContainers = typeContainer.GetObjectArray("m_aEntries");
				if (!entryContainers || entryContainers.Count() != typeGroup.m_aEntries.Count()) { reason = "serialized_grouped_entry_container_count_mismatch"; return false; }
				for (int entryIndex = 0; entryIndex < entryContainers.Count(); entryIndex++)
				{
					BaseContainer entryContainer = entryContainers.Get(entryIndex);
					string prefabName;
					if (!entryContainer || !ME_GetPrefabEntryContainerName(typeGroup.m_aEntries[entryIndex].m_sPrefab, prefabName) || !ME_IsValidContainerName(prefabName) || assignedPrefabNames.Contains(prefabName)) { reason = "serialized_prefab_container_name_invalid"; return false; }
					entryContainer.SetName(prefabName);
					assignedPrefabNames.Insert(prefabName);
				}
			}
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
		if (!containerResource || !Workbench.GetAbsolutePath(STAGED_PATH, absolutePath, false)) { reason = "create_or_path_failed"; return false; }
		BaseContainer container = containerResource.GetResource().ToBaseContainer();
		if (!container || !ME_SetDeterministicEntryContainerNames(container, snapshot.m_aEntries.Count(), "aggregate_entry", reason)) return false;
		if (!BaseContainerTools.SaveContainer(container, STAGED_RESOURCE, absolutePath)) { reason = "save_container_failed"; return false; }
		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		if (!resourceManager) { reason = "resource_manager_unavailable"; return false; }
		resourceManager.RebuildResourceFile(absolutePath, "", false);
		if (!resourceManager.WaitForFile(absolutePath, 5000)) { reason = "resource_wait_failed"; return false; }
		Resource loaded = Resource.Load(STAGED_RESOURCE);
		BaseContainer loadedContainer;
		if (loaded) loadedContainer = loaded.GetResource().ToBaseContainer();
		ME_VehicleBoundsSnapshot reloaded;
		if (loadedContainer) reloaded = ME_VehicleBoundsSnapshot.Cast(BaseContainerTools.CreateInstanceFromContainer(loadedContainer));
		if (!reloaded || reloaded.m_iSchemaVersion != snapshot.m_iSchemaVersion || reloaded.m_sGeneratorVersion != snapshot.m_sGeneratorVersion || !reloaded.m_aEntries || reloaded.m_aEntries.Count() != snapshot.m_aEntries.Count()) { reason = "reload_validation_failed"; return false; }
		for (int i = 0; i < snapshot.m_aEntries.Count(); i++)
		{
			ME_VehicleBoundsSnapshotEntry expected = snapshot.m_aEntries[i];
			ME_VehicleBoundsSnapshotEntry actual = reloaded.m_aEntries[i];
			if (expected.m_sFactionKey != actual.m_sFactionKey || expected.m_sVehicleType != actual.m_sVehicleType || expected.m_iCandidateCount != actual.m_iCandidateCount || expected.m_sMinXSourcePrefab != actual.m_sMinXSourcePrefab || expected.m_sMaxXSourcePrefab != actual.m_sMaxXSourcePrefab || expected.m_sMinYSourcePrefab != actual.m_sMinYSourcePrefab || expected.m_sMaxYSourcePrefab != actual.m_sMaxYSourcePrefab || expected.m_sMinZSourcePrefab != actual.m_sMinZSourcePrefab || expected.m_sMaxZSourcePrefab != actual.m_sMaxZSourcePrefab || !ME_AreVectorsClose(expected.m_vLocalMins, actual.m_vLocalMins) || !ME_AreVectorsClose(expected.m_vLocalMaxs, actual.m_vLocalMaxs)) { reason = "reload_entry_mismatch"; return false; }
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Saves only the fixed Candidate resource and validates its fully deserialized model.
	//! Сохраняет только фиксированный Candidate-ресурс и проверяет его полностью десериализованную модель.
	protected bool ME_SaveAndValidateCandidateSnapshot(ME_VehicleBoundsPerPrefabSnapshot snapshot, out ME_VehicleBoundsPerPrefabSnapshot reloaded, out string reason)
	{
		reloaded = null;
		reason = "";
		ME_VehicleBoundsPerPrefabSnapshot grouped;
		if (!ME_BuildGroupedPerPrefabSnapshot(snapshot, grouped, reason)) return false;
		Resource containerResource = BaseContainerTools.CreateContainerFromInstance(grouped);
		string absolutePath;
		if (!containerResource || !Workbench.GetAbsolutePath(CANDIDATE_PATH, absolutePath, false)) { reason = "candidate_create_or_path_failed"; return false; }
		BaseContainer container = containerResource.GetResource().ToBaseContainer();
		if (!container || !ME_SetGroupedPerPrefabContainerNames(container, grouped, reason)) return false;
		if (!BaseContainerTools.SaveContainer(container, CANDIDATE_RESOURCE, absolutePath)) { reason = "candidate_save_container_failed"; return false; }
		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		if (!resourceManager) { reason = "candidate_resource_manager_unavailable"; return false; }
		resourceManager.RebuildResourceFile(absolutePath, "", false);
		if (!resourceManager.WaitForFile(absolutePath, 5000)) { reason = "candidate_resource_wait_failed"; return false; }
		Resource loaded = Resource.Load(CANDIDATE_RESOURCE);
		BaseContainer loadedContainer;
		if (loaded) loadedContainer = loaded.GetResource().ToBaseContainer();
		if (loadedContainer) reloaded = ME_VehicleBoundsPerPrefabSnapshot.Cast(BaseContainerTools.CreateInstanceFromContainer(loadedContainer));
		if (!ME_ValidatePerPrefabSnapshot(reloaded, true, reason)) { reloaded = null; return false; }
		if (!ME_ArePerPrefabSnapshotsEqual(snapshot, reloaded)) { reason = "candidate_reload_model_mismatch"; reloaded = null; return false; }
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Loads and validates the manually accepted Baseline without exposing any write path.
	//! Загружает и проверяет вручную принятую Baseline без предоставления какого-либо пути записи.
	protected bool ME_LoadBaselineSnapshot(out ME_VehicleBoundsPerPrefabSnapshot baseline, out string reason)
	{
		baseline = null;
		reason = "";
		Resource loaded = Resource.Load(BASELINE_RESOURCE);
		BaseContainer container;
		if (loaded) container = loaded.GetResource().ToBaseContainer();
		if (container) baseline = ME_VehicleBoundsPerPrefabSnapshot.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
		if (!ME_ValidatePerPrefabSnapshot(baseline, false, reason)) { baseline = null; reason = "baseline_" + reason; return false; }
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Compares two complete per-prefab models including serialization-sensitive bounds.
	//! Сравнивает две полные per-prefab модели, включая чувствительные к сериализации границы.
	protected bool ME_ArePerPrefabSnapshotsEqual(ME_VehicleBoundsPerPrefabSnapshot left, ME_VehicleBoundsPerPrefabSnapshot right)
	{
		if (!left || !right || left.m_iSchemaVersion != right.m_iSchemaVersion || left.m_sGeneratorVersion != right.m_sGeneratorVersion || left.m_sFixtureIdentity != right.m_sFixtureIdentity || left.m_sGameVersion != right.m_sGameVersion || left.m_bClassificationMetadataAvailable != right.m_bClassificationMetadataAvailable) return false;
		array<ref ME_VehicleBoundsPerPrefabSnapshotEntry> leftEntries;
		array<ref ME_VehicleBoundsPerPrefabSnapshotEntry> rightEntries;
		bool leftClassificationAvailable;
		bool rightClassificationAvailable;
		string reason;
		if (!ME_NormalizePerPrefabSnapshot(left, leftEntries, leftClassificationAvailable, reason) || !ME_NormalizePerPrefabSnapshot(right, rightEntries, rightClassificationAvailable, reason) || leftEntries.Count() != rightEntries.Count()) return false;
		for (int index = 0; index < leftEntries.Count(); index++)
		{
			ME_VehicleBoundsPerPrefabSnapshotEntry leftEntry = leftEntries[index];
			ME_VehicleBoundsPerPrefabSnapshotEntry rightEntry = rightEntries[index];
			if (leftEntry.m_sPrefab != rightEntry.m_sPrefab || !ME_AreVectorsClose(leftEntry.m_vLocalMins, rightEntry.m_vLocalMins) || !ME_AreVectorsClose(leftEntry.m_vLocalMaxs, rightEntry.m_vLocalMaxs) || leftClassificationAvailable && (leftEntry.m_sFactionKey != rightEntry.m_sFactionKey || leftEntry.m_sVehicleType != rightEntry.m_sVehicleType)) return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Performs a deterministic linear merge and reports semantic per-prefab differences.
	//! Выполняет детерминированное linear-merge сравнение и выводит semantic различия по prefab.
	protected void ME_ComparePerPrefabSnapshots(ME_VehicleBoundsPerPrefabSnapshot baseline, ME_VehicleBoundsPerPrefabSnapshot candidate)
	{
		array<ref ME_VehicleBoundsPerPrefabSnapshotEntry> baselineEntries;
		array<ref ME_VehicleBoundsPerPrefabSnapshotEntry> candidateEntries;
		bool baselineClassificationAvailable;
		bool candidateClassificationAvailable;
		string normalizeReason;
		if (!ME_NormalizePerPrefabSnapshot(baseline, baselineEntries, baselineClassificationAvailable, normalizeReason) || !ME_NormalizePerPrefabSnapshot(candidate, candidateEntries, candidateClassificationAvailable, normalizeReason))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_per_prefab_compare status=FAIL reason=%1", normalizeReason);
			return;
		}
		bool compareClassification = baselineClassificationAvailable && candidateClassificationAvailable;
		string classificationMode = "compared";
		if (!compareClassification)
			classificationMode = "skipped_legacy_metadata_unavailable";
		PrintFormat("[ME_DEBUG_AVSP_WB] bounds_per_prefab_compare phase=context baseline_game_version=%1 candidate_game_version=%2 classification=%3", baseline.m_sGameVersion, candidate.m_sGameVersion, classificationMode);
		int baselineIndex;
		int candidateIndex;
		int added;
		int removed;
		int changed;
		while (baselineIndex < baselineEntries.Count() || candidateIndex < candidateEntries.Count())
		{
			ME_VehicleBoundsPerPrefabSnapshotEntry baselineEntry;
			ME_VehicleBoundsPerPrefabSnapshotEntry candidateEntry;
			if (baselineIndex < baselineEntries.Count()) baselineEntry = baselineEntries[baselineIndex];
			if (candidateIndex < candidateEntries.Count()) candidateEntry = candidateEntries[candidateIndex];
			if (!baselineEntry || candidateEntry && candidateEntry.m_sPrefab < baselineEntry.m_sPrefab)
			{
				PrintFormat("[ME_DEBUG_AVSP_WB] bounds_per_prefab_diff kind=ADDED prefab=%1", candidateEntry.m_sPrefab);
				added++;
				candidateIndex++;
				continue;
			}
			if (!candidateEntry || baselineEntry.m_sPrefab < candidateEntry.m_sPrefab)
			{
				PrintFormat("[ME_DEBUG_AVSP_WB] bounds_per_prefab_diff kind=REMOVED prefab=%1", baselineEntry.m_sPrefab);
				removed++;
				baselineIndex++;
				continue;
			}

			bool boundsChanged = !ME_AreVectorsClose(baselineEntry.m_vLocalMins, candidateEntry.m_vLocalMins) || !ME_AreVectorsClose(baselineEntry.m_vLocalMaxs, candidateEntry.m_vLocalMaxs);
			bool factionsChanged = compareClassification && baselineEntry.m_sFactionKey != candidateEntry.m_sFactionKey;
			bool vehicleTypesChanged = compareClassification && baselineEntry.m_sVehicleType != candidateEntry.m_sVehicleType;
			if (boundsChanged || factionsChanged || vehicleTypesChanged)
			{
				PrintFormat("[ME_DEBUG_AVSP_WB] bounds_per_prefab_diff kind=CHANGED prefab=%1 bounds=%2 factions=%3 vehicle_types=%4 baseline_mins=%5 baseline_maxs=%6 candidate_mins=%7 candidate_maxs=%8", baselineEntry.m_sPrefab, boundsChanged, factionsChanged, vehicleTypesChanged, baselineEntry.m_vLocalMins, baselineEntry.m_vLocalMaxs, candidateEntry.m_vLocalMins, candidateEntry.m_vLocalMaxs);
				changed++;
			}
			baselineIndex++;
			candidateIndex++;
		}

		string status = "PASS";
		if (added > 0 || removed > 0 || changed > 0)
			status = "DIFF";
		PrintFormat("[ME_DEBUG_AVSP_WB] bounds_per_prefab_compare status=%1 baseline_game_version=%2 candidate_game_version=%3 classification=%4 baseline_count=%5 candidate_count=%6 added=%7 removed=%8 changed=%9 candidate_written=1", status, baseline.m_sGameVersion, candidate.m_sGameVersion, classificationMode, baselineEntries.Count(), candidateEntries.Count(), added, removed, changed);
	}

	//------------------------------------------------------------------------------------------------
	//! Compares vectors within config serialization precision.
	//! Сравнивает векторы с точностью config serialization.
	protected bool ME_AreVectorsClose(vector left, vector right)
	{
		for (int axis = 0; axis < 3; axis++)
			if (Math.AbsFloat(left[axis] - right[axis]) > 0.0011) return false;
		return true;
	}
}
