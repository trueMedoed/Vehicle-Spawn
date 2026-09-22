//! Test-only Workbench diagnostic for the raw authored and auto editable labels of ambient vehicle prefabs.
//! Диагностика только для Test в Workbench для raw authored- и auto-меток prefab ambient-техники.

//------------------------------------------------------------------------------------------------
//! Stores the read-only result of one raw editable-label array inspection.
//! Хранит результат read-only-проверки одного raw-массива editable-меток.
class ME_AmbientVehicleRawEditableLabelsResult
{
	string m_sStatus;
	string m_sReason;
	string m_sFieldName;
	string m_sSourceKind;
	string m_sSourceLevel;
	string m_sSourcePath;
	string m_sDataType;
}

//------------------------------------------------------------------------------------------------
//! Reads filtered catalog labels and investigates their raw prefab metadata without editing the world.
//! Читает метки отфильтрованного каталога и исследует raw metadata prefab без изменения мира.
[WorkbenchPluginAttribute(name: "Diagnose ambient vehicle editable labels", description: "Logs resolved catalog labels and raw authored/auto prefab metadata for one selected ambient spawn point.", wbModules: { "WorldEditor" }, category: "ME_Vehicle_Spawn/Diagnostics")]
class ME_AmbientVehicleEditableLabelsDiagnosticPlugin : WorldEditorPlugin
{
	//------------------------------------------------------------------------------------------------
	//! Validates one selected ambient spawn point and logs every filtered catalog candidate.
	//! Проверяет одну выбранную ambient spawn point и записывает каждого кандидата отфильтрованного каталога.
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_DEBUG_AVSP_LABELS_WB] status=UNAVAILABLE reason=world_editor_unavailable");
			return;
		}

		WorldEditorAPI api = worldEditor.GetApi();
		if (!api)
		{
			Print("[ME_DEBUG_AVSP_LABELS_WB] status=UNAVAILABLE reason=world_editor_api_unavailable");
			return;
		}

		int selectedCount = api.GetSelectedEntitiesCount();
		if (selectedCount != 1)
		{
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] status=UNAVAILABLE reason=expected_exactly_one_selected_entity selectedCount=%1", selectedCount);
			return;
		}

		IEntitySource selectedSource = api.GetSelectedEntity();
		IEntity selectedEntity = api.SourceToEntity(selectedSource);
		if (!selectedEntity)
		{
			Print("[ME_DEBUG_AVSP_LABELS_WB] status=UNAVAILABLE reason=selected_entity_unavailable");
			return;
		}

		SCR_AmbientVehicleSpawnPointComponent spawnPoint = SCR_AmbientVehicleSpawnPointComponent.Cast(selectedEntity.FindComponent(SCR_AmbientVehicleSpawnPointComponent));
		if (!spawnPoint)
		{
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] status=UNAVAILABLE reason=selected_entity_is_not_ambient_spawn_point entity=%1", selectedEntity.GetName());
			return;
		}

		array<SCR_EntityCatalogEntry> entries;
		string reason;
		if (!spawnPoint.ME_GetEditorVehicleEnvelopeCandidates(entries, reason))
		{
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] status=UNAVAILABLE reason=%1 entity=%2", reason, selectedEntity.GetName());
			return;
		}

		PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] status=PASS entity=%1 candidateScope=FILTERED_CANDIDATES candidateCount=%2 notInFilteredCandidates=NOT_INSPECTED", selectedEntity.GetName(), entries.Count());
		if (entries.IsEmpty())
			return;

		foreach (SCR_EntityCatalogEntry entry : entries)
		{
			if (!entry)
				continue;

			array<EEditableEntityLabel> resolvedLabels = {};
			entry.GetEditableEntityLabels(resolvedLabels);
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] candidateStatus=FILTERED_CANDIDATE prefab=%1 catalogIndex=%2 catalogName=%3 resolvedLabels=%4", entry.GetPrefab(), entry.GetCatalogIndex(), entry.GetEntityName(), ME_LabelsToString(resolvedLabels));
			ME_LogPrefabMetadata(entry.GetPrefab(), resolvedLabels);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Loads one candidate prefab and locates its resolved editable vehicle UI-info container.
	//! Загружает один prefab кандидата и находит его resolved UI-info контейнер editable vehicle.
	protected void ME_LogPrefabMetadata(ResourceName prefabPath, array<EEditableEntityLabel> resolvedLabels)
	{
		Resource resource = Resource.Load(prefabPath);
		if (!resource || !resource.IsValid())
		{
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] metadata prefab=%1 status=UNAVAILABLE reason=resource_load_failed resolvedLabels=%2", prefabPath, ME_LabelsToString(resolvedLabels));
			return;
		}

		BaseResourceObject baseResObj = resource.GetResource();
		if (!baseResObj)
		{
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] metadata prefab=%1 status=UNAVAILABLE reason=base_resource_object_unavailable resolvedLabels=%2", prefabPath, ME_LabelsToString(resolvedLabels));
			return;
		}

		IEntitySource prefabSource = baseResObj.ToEntitySource();
		BaseContainer prefabContainer = baseResObj.ToBaseContainer();

		if (!prefabSource)
		{
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] metadata prefab=%1 status=UNAVAILABLE reason=entity_source_unavailable resolvedLabels=%2", prefabPath, ME_LabelsToString(resolvedLabels));
			return;
		}

		IEntityComponentSource vehicleComponent = ME_FindEditableVehicleComponent(prefabSource);
		if (!vehicleComponent)
		{
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] metadata prefab=%1 status=UNAVAILABLE reason=editable_vehicle_component_unavailable resolvedLabels=%2", prefabPath, ME_LabelsToString(resolvedLabels));
			return;
		}

		BaseContainer uiInfo = vehicleComponent.GetObject("m_UIInfo");
		if (!uiInfo)
		{
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] metadata prefab=%1 status=UNAVAILABLE reason=editable_vehicle_ui_info_unavailable resolvedLabels=%2", prefabPath, ME_LabelsToString(resolvedLabels));
			return;
		}

		string uiInfoSourceKind;
		string uiInfoSourceLevel;
		string uiInfoSourcePath;
		ME_FindUiInfoDeclaration(prefabSource, uiInfoSourceKind, uiInfoSourceLevel, uiInfoSourcePath);
		PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] metadata prefab=%1 uiInfoSourceKind=%2 uiInfoSourceLevel=%3 uiInfoSourcePath=%4 resolvedLabels=%5", prefabPath, uiInfoSourceKind, uiInfoSourceLevel, uiInfoSourcePath, ME_LabelsToString(resolvedLabels));

		ME_AmbientVehicleRawEditableLabelsResult authored = ME_ReadRawEditableLabels(prefabSource, uiInfo, "m_aAuthoredLabels");
		ME_AmbientVehicleRawEditableLabelsResult autoLabels = ME_ReadRawEditableLabels(prefabSource, uiInfo, "m_aAutoLabels");
		ME_LogRawResult(prefabPath, authored, resolvedLabels);
		ME_LogRawResult(prefabPath, autoLabels, resolvedLabels);

		if (authored.m_sStatus == "AVAILABLE" && autoLabels.m_sStatus == "AVAILABLE")
		{
			ME_CompareRawAndResolvedLabels(prefabPath, uiInfo, resolvedLabels);
		}
		else if (prefabContainer)
		{
			ME_TryApproachBBaseContainerReading(prefabPath, prefabContainer, resolvedLabels);
		}
		else
		{
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] comparison prefab=%1 status=NOT_PERFORMED reason=raw_array_unavailable_and_base_container_unavailable authoredStatus=%2 autoStatus=%3 resolvedLabels=%4", prefabPath, authored.m_sStatus, autoLabels.m_sStatus, ME_LabelsToString(resolvedLabels));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Finds the first editable vehicle component while walking the prefab source and its ancestors.
	//! Находит первый компонент editable vehicle при обходе source prefab и его предков.
	protected IEntityComponentSource ME_FindEditableVehicleComponent(IEntitySource prefabSource)
	{
		IEntitySource levelSource = prefabSource;
		while (levelSource)
		{
			for (int componentIndex = 0; componentIndex < levelSource.GetComponentCount(); componentIndex++)
			{
				IEntityComponentSource componentSource = levelSource.GetComponent(componentIndex);
				if (componentSource && componentSource.GetClassName() == "SCR_EditableVehicleComponent")
					return componentSource;
			}

			BaseContainer ancestor = levelSource.GetAncestor();
			if (!ancestor)
				break;
			levelSource = ancestor.ToEntitySource();
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds the direct declaration level of m_UIInfo in the prefab source chain.
	//! Находит уровень прямого объявления m_UIInfo в цепочке source prefab.
	protected void ME_FindUiInfoDeclaration(IEntitySource prefabSource, out string sourceKind, out string sourceLevel, out string sourcePath)
	{
		sourceKind = "NOT_FOUND";
		sourceLevel = "-1";
		sourcePath = "";

		IEntitySource levelSource = prefabSource;
		int level = 0;
		while (levelSource)
		{
			for (int componentIndex = 0; componentIndex < levelSource.GetComponentCount(); componentIndex++)
			{
				IEntityComponentSource componentSource = levelSource.GetComponent(componentIndex);
				if (!componentSource || componentSource.GetClassName() != "SCR_EditableVehicleComponent")
					continue;

				if (componentSource.IsVariableSetDirectly("m_UIInfo"))
				{
					sourceKind = "ENTITY_SOURCE_DIRECT";
					sourceLevel = level.ToString();
					sourcePath = ME_GetSourcePath(levelSource, level);
					return;
				}
			}

			BaseContainer ancestor = levelSource.GetAncestor();
			if (!ancestor)
				break;
			levelSource = ancestor.ToEntitySource();
			level++;
		}

		sourceKind = "INHERITED_OR_UNRESOLVED";
	}

	//------------------------------------------------------------------------------------------------
	//! Reads one raw enum array from the resolved UI-info container.
	//! Читает один raw enum-массив из resolved UI-info контейнера.
	protected ME_AmbientVehicleRawEditableLabelsResult ME_ReadRawEditableLabels(IEntitySource prefabSource, BaseContainer resolvedUiInfo, string variableName)
	{
		ME_AmbientVehicleRawEditableLabelsResult result = new ME_AmbientVehicleRawEditableLabelsResult();
		result.m_sStatus = "NOT_SERIALIZED_OR_UNEXPOSED";
		result.m_sReason = "variable_not_set";
		result.m_sFieldName = variableName;
		result.m_sSourceKind = "NOT_FOUND";
		result.m_sSourceLevel = "-1";
		result.m_sSourcePath = "";
		result.m_sDataType = "unknown";

		int variableCount = resolvedUiInfo.GetNumVars();
		for (int variableIndex = 0; variableIndex < variableCount; variableIndex++)
		{
			if (resolvedUiInfo.GetVarName(variableIndex) != variableName)
				continue;

			result.m_sDataType = resolvedUiInfo.GetDataVarType(variableIndex).ToString();
			break;
		}

		if (!resolvedUiInfo.IsVariableSet(variableName))
			return result;

		array<int> rawLabels = {};
		if (!resolvedUiInfo.Get(variableName, rawLabels))
		{
			result.m_sStatus = "UNAVAILABLE";
			result.m_sReason = "get_failed";
			ME_FindRawFieldDeclaration(prefabSource, resolvedUiInfo, variableName, result);
			return result;
		}

		result.m_sStatus = "AVAILABLE";
		result.m_sReason = string.Format("count=%1", rawLabels.Count());
		ME_FindRawFieldDeclaration(prefabSource, resolvedUiInfo, variableName, result);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Compares raw label arrays with resolved labels to verify they match.
	//! Сравнивает raw массивы меток с resolved метками для проверки соответствия.
	protected void ME_CompareRawAndResolvedLabels(ResourceName prefabPath, BaseContainer uiInfo, array<EEditableEntityLabel> resolvedLabels)
	{
		array<int> rawAuthored = {};
		array<int> rawAuto = {};

		if (!uiInfo.Get("m_aAuthoredLabels", rawAuthored))
		{
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] comparison prefab=%1 status=FAILED reason=authored_get_failed", prefabPath);
			return;
		}

		if (!uiInfo.Get("m_aAutoLabels", rawAuto))
		{
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] comparison prefab=%1 status=FAILED reason=auto_get_failed", prefabPath);
			return;
		}

		// Combine raw arrays
		array<int> rawCombined = {};
		foreach (int val : rawAuthored)
			rawCombined.Insert(val);
		foreach (int val : rawAuto)
			rawCombined.Insert(val);

		// Convert resolved labels to int array
		array<int> resolvedInts = {};
		foreach (EEditableEntityLabel label : resolvedLabels)
			resolvedInts.Insert(label);

		// Sort both for comparison
		rawCombined.Sort();
		resolvedInts.Sort();

		// Compare counts
		if (rawCombined.Count() != resolvedInts.Count())
		{
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] comparison prefab=%1 status=MISMATCH reason=count_difference rawCount=%2 resolvedCount=%3",
				prefabPath, rawCombined.Count(), resolvedInts.Count());
			return;
		}

		// Compare values
		for (int i = 0; i < rawCombined.Count(); i++)
		{
			if (rawCombined[i] != resolvedInts[i])
			{
				PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] comparison prefab=%1 status=MISMATCH reason=value_difference index=%2 rawValue=%3 resolvedValue=%4",
					prefabPath, i, rawCombined[i], resolvedInts[i]);
				return;
			}
		}

		PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] comparison prefab=%1 status=MATCH count=%2 rawAuthored=%3 rawAuto=%4",
			prefabPath, rawCombined.Count(), rawAuthored.Count(), rawAuto.Count());
	}

	//------------------------------------------------------------------------------------------------
	//! Finds whether one raw field is directly authored in the nested UI-info container or an entity source level.
	//! Находит, задано ли raw-поле напрямую во вложенном UI-info контейнере или на уровне entity source.
	protected void ME_FindRawFieldDeclaration(IEntitySource prefabSource, BaseContainer resolvedUiInfo, string variableName, ME_AmbientVehicleRawEditableLabelsResult result)
	{
		BaseContainer containerLevel = resolvedUiInfo;
		int containerIndex = 0;
		while (containerLevel)
		{
			if (containerLevel.IsVariableSetDirectly(variableName))
			{
				result.m_sSourceKind = "UI_INFO_CONTAINER_DIRECT";
				result.m_sSourceLevel = "container_" + containerIndex.ToString();
				result.m_sSourcePath = containerLevel.GetResourceName();
				return;
			}

			containerLevel = containerLevel.GetAncestor();
			containerIndex++;
		}

		IEntitySource levelSource = prefabSource;
		int sourceIndex = 0;
		while (levelSource)
		{
			for (int componentIndex = 0; componentIndex < levelSource.GetComponentCount(); componentIndex++)
			{
				IEntityComponentSource componentSource = levelSource.GetComponent(componentIndex);
				if (!componentSource || componentSource.GetClassName() != "SCR_EditableVehicleComponent")
					continue;

				BaseContainer uiInfo = componentSource.GetObject("m_UIInfo");
				if (uiInfo && uiInfo.IsVariableSetDirectly(variableName))
				{
					result.m_sSourceKind = "ENTITY_SOURCE_UI_INFO_DIRECT";
					result.m_sSourceLevel = sourceIndex.ToString();
					result.m_sSourcePath = ME_GetSourcePath(levelSource, sourceIndex);
					return;
				}
			}

			BaseContainer ancestor = levelSource.GetAncestor();
			if (!ancestor)
				break;
			levelSource = ancestor.ToEntitySource();
			sourceIndex++;
		}

		if (resolvedUiInfo.IsVariableSet(variableName))
		{
			result.m_sSourceKind = "INHERITED_SOURCE_LEVEL_UNRESOLVED";
			result.m_sSourceLevel = "inherited_unresolved";
			result.m_sReason = "variable_set_but_direct_source_level_not_found";
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Logs one raw-array status with its direct or inherited source location.
	//! Записывает статус одного raw-массива вместе с прямым или унаследованным источником.
	protected void ME_LogRawResult(ResourceName prefabPath, ME_AmbientVehicleRawEditableLabelsResult result, array<EEditableEntityLabel> resolvedLabels)
	{
		PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] raw prefab=%1 field=%2 status=%3 reason=%4 sourceKind=%5 sourceLevel=%6 sourcePath=%7 dataType=%8", prefabPath, result.m_sFieldName, result.m_sStatus, result.m_sReason, result.m_sSourceKind, result.m_sSourceLevel, result.m_sSourcePath, result.m_sDataType);
		PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] rawResolved prefab=%1 field=%2 resolvedLabels=%3", prefabPath, result.m_sFieldName, ME_LabelsToString(resolvedLabels));
	}

	//------------------------------------------------------------------------------------------------
	//! Returns a stable source path, falling back to the numeric ancestor level when unavailable.
	//! Возвращает стабильный путь source, используя номер уровня предка, если путь недоступен.
	protected string ME_GetSourcePath(IEntitySource source, int level)
	{
		string path = source.GetResourceName();
		if (!path.IsEmpty())
			return path;
		return "<source_level_" + level.ToString() + ">";
	}

	//------------------------------------------------------------------------------------------------
	//! Tries Approach B: reading labels from BaseContainer when IEntityComponentSource fails.
	//! Пробует Approach B: чтение меток из BaseContainer, когда IEntityComponentSource не работает.
	protected void ME_TryApproachBBaseContainerReading(ResourceName prefabPath, BaseContainer prefabContainer, array<EEditableEntityLabel> resolvedLabels)
	{
		BaseContainerList components = prefabContainer.GetObjectArray("components");
		if (!components)
		{
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] approachB prefab=%1 status=UNAVAILABLE reason=components_array_unavailable", prefabPath);
			return;
		}

		int componentCount = components.Count();
		for (int i = 0; i < componentCount; i++)
		{
			BaseContainer component = components.Get(i);
			if (!component)
				continue;

			string componentClassName = component.GetClassName();
			if (componentClassName != "SCR_EditableVehicleComponent")
				continue;

			BaseContainer uiInfo = component.GetObject("m_UIInfo");
			if (!uiInfo)
			{
				PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] approachB prefab=%1 status=UNAVAILABLE reason=ui_info_unavailable_in_base_container", prefabPath);
				return;
			}

			array<int> authoredLabels = {};
			bool authoredSuccess = uiInfo.Get("m_aAuthoredLabels", authoredLabels);

			array<int> autoLabels = {};
			bool autoSuccess = uiInfo.Get("m_aAutoLabels", autoLabels);

			if (authoredSuccess || autoSuccess)
			{
				PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] approachB prefab=%1 status=PASS authoredCount=%2 autoCount=%3", prefabPath, authoredLabels.Count(), autoLabels.Count());

				if (authoredSuccess)
				{
					PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] approachB_authored prefab=%1 count=%2", prefabPath, authoredLabels.Count());
					foreach (int labelValue : authoredLabels)
					{
						string labelName = typename.EnumToString(EEditableEntityLabel, labelValue);
						PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] approachB_authored_label prefab=%1 labelName=%2 labelValue=%3", prefabPath, labelName, labelValue);
					}
				}

				if (autoSuccess)
				{
					PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] approachB_auto prefab=%1 count=%2", prefabPath, autoLabels.Count());
					foreach (int labelValue : autoLabels)
					{
						string labelName = typename.EnumToString(EEditableEntityLabel, labelValue);
						PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] approachB_auto_label prefab=%1 labelName=%2 labelValue=%3", prefabPath, labelName, labelValue);
					}
				}

				ME_CompareApproachBWithResolved(prefabPath, authoredLabels, autoLabels, resolvedLabels);
			}
			else
			{
				PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] approachB prefab=%1 status=UNAVAILABLE reason=both_get_calls_failed", prefabPath);
			}
			return;
		}

		PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] approachB prefab=%1 status=UNAVAILABLE reason=editable_vehicle_component_not_found_in_components", prefabPath);
	}

	//------------------------------------------------------------------------------------------------
	//! Compares Approach B raw arrays with resolved labels.
	//! Сравнивает raw-массивы Approach B с resolved метками.
	protected void ME_CompareApproachBWithResolved(ResourceName prefabPath, array<int> rawAuthored, array<int> rawAuto, array<EEditableEntityLabel> resolvedLabels)
	{
		array<int> rawCombined = {};
		foreach (int val : rawAuthored)
			rawCombined.Insert(val);
		foreach (int val : rawAuto)
			rawCombined.Insert(val);

		array<int> resolvedInts = {};
		foreach (EEditableEntityLabel label : resolvedLabels)
			resolvedInts.Insert(label);

		rawCombined.Sort();
		resolvedInts.Sort();

		if (rawCombined.Count() != resolvedInts.Count())
		{
			PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] approachB_comparison prefab=%1 status=MISMATCH reason=count_difference rawCount=%2 resolvedCount=%3", prefabPath, rawCombined.Count(), resolvedInts.Count());
			return;
		}

		for (int i = 0; i < rawCombined.Count(); i++)
		{
			if (rawCombined[i] != resolvedInts[i])
			{
				PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] approachB_comparison prefab=%1 status=MISMATCH reason=value_difference index=%2 rawValue=%3 resolvedValue=%4", prefabPath, i, rawCombined[i], resolvedInts[i]);
				return;
			}
		}

		PrintFormat("[ME_DEBUG_AVSP_LABELS_WB] approachB_comparison prefab=%1 status=MATCH count=%2 rawAuthored=%3 rawAuto=%4", prefabPath, rawCombined.Count(), rawAuthored.Count(), rawAuto.Count());
	}

	//------------------------------------------------------------------------------------------------
	//! Formats editable labels as a stable comma-separated enum-name list.
	//! Форматирует editable-метки как стабильный список имён enum через запятую.
	protected string ME_LabelsToString(array<EEditableEntityLabel> labels)
	{
		if (!labels || labels.IsEmpty())
			return "[]";

		string result = "[";
		for (int i = 0; i < labels.Count(); i++)
		{
			if (i > 0)
				result += ",";
			result += typename.EnumToString(EEditableEntityLabel, labels[i]);
		}
		return result + "]";
	}
}
