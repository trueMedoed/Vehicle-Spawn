//! Test-only audit of ambient points exposed by the open World Editor world.
//! Проверка только для Test ambient-точек, доступных в открытом мире World Editor.
[WorkbenchPluginAttribute(name: "Audit all ambient vehicle spawn points", description: "Checks loaded point filters, catalogs and static object bounds without Game mode; logs problems and a summary.", wbModules: { "WorldEditor" }, category: "ME_Vehicle_Spawn/Diagnostics")]
class ME_AmbientVehicleWorldAuditPlugin : WorldEditorPlugin
{
	//! Enumerates editor entities and reports errors and incomplete catalog checks.
	//! Обходит сущности редактора и сообщает об ошибках и незавершённых проверках каталогов.
	override void Run()
	{
		if (!SCR_Global.IsEditMode())
		{
			PrintFormat("[ME_DEBUG_AVSP_AUDIT] status=ABORTED reason=edit_mode_required", level: LogLevel.WARNING);
			return;
		}
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			PrintFormat("[ME_DEBUG_AVSP_AUDIT] status=ABORTED reason=world_editor_unavailable", level: LogLevel.WARNING);
			return;
		}
		WorldEditorAPI api = worldEditor.GetApi();
		if (!api)
		{
			PrintFormat("[ME_DEBUG_AVSP_AUDIT] status=ABORTED reason=world_editor_api_unavailable", level: LogLevel.WARNING);
			return;
		}
		PrintFormat("[ME_DEBUG_AVSP_AUDIT] status=STARTED scope=loaded_editor_entities checks=labels,catalogs,clearance,static_bounds");
		int scanned;
		int passed;
		int errorPoints;
		int warningPoints;
		int clearanceErrorPoints;
		int staticObjectIntersections;
		int unavailablePoints;
		int unresolvedEntities;
		int sourceMatchedPoints;
		int sourceMissedPoints;
		int missingSources;
		int unresolvedPointSources;
		int unresolvedOtherSources;
		ref map<string, int> unresolvedClasses = new map<string, int>();
		array<IEntity> visited = {};
		int entityCount = api.GetEditorEntityCount();
		for (int i = 0; i < entityCount; i++)
		{
			IEntitySource source = api.GetEditorEntity(i);
			if (!source)
			{
				missingSources++;
				unresolvedEntities++;
				continue;
			}
			IEntity entity = api.SourceToEntity(source);
			if (!entity)
			{
				unresolvedEntities++;
				string sourceClass = source.GetClassName();
				int classCount;
				unresolvedClasses.Find(sourceClass, classCount);
				unresolvedClasses.Set(sourceClass, classCount + 1);
				bool pointSource = HasAmbientPointSource(source);
				if (pointSource)
					unresolvedPointSources++;
				else
					unresolvedOtherSources++;
				// Log every known missed point, but cap generic samples to keep large worlds readable.
				// Пишем все известные пропущенные точки, но ограничиваем примеры прочих сущностей.
				if (pointSource || unresolvedOtherSources <= 10)
				{
					PrintFormat("[ME_DEBUG_AVSP_AUDIT] status=UNRESOLVED_SOURCE pointComponent=%1 class=%2 name=%3 id=%4 subscene=%5 layer=%6 prefab=%7",
						pointSource, sourceClass, source.GetName(), source.GetID(), source.GetSubScene(), source.GetLayerID(), source.GetResourceName());
				}
				continue;
			}
			SCR_AmbientVehicleSpawnPointComponent point = SCR_AmbientVehicleSpawnPointComponent.Cast(entity.FindComponent(SCR_AmbientVehicleSpawnPointComponent));
			if (!point || visited.Contains(entity))
				continue;
			visited.Insert(entity);
			scanned++;
			// Validate source detection against each independently identified live point.
			// Проверяем поиск по source на каждой независимо найденной игровой точке.
			if (HasAmbientPointSource(source))
				sourceMatchedPoints++;
			else
			{
				sourceMissedPoints++;
				PrintFormat("[ME_DEBUG_AVSP_AUDIT] status=SOURCE_MISMATCH name=%1 coordinates=%2 id=%3 subscene=%4 layer=%5",
					entity.GetName(), entity.GetOrigin(), source.GetID(), source.GetSubScene(), source.GetLayerID(), level: LogLevel.WARNING);
			}
			bool hasError;
			bool unavailable;
			point.ME_AuditEditorVehicleFilter(hasError, unavailable);
			bool geometryUnavailable;
			bool clearanceFailed;
			int objectConflicts = point.ME_AuditEditorStaticObjects(geometryUnavailable, clearanceFailed);
			if (clearanceFailed)
			{
				clearanceErrorPoints++;
				hasError = true;
			}
			staticObjectIntersections += objectConflicts;
			if (objectConflicts > 0)
				warningPoints++;
			unavailable = unavailable || geometryUnavailable;
			if (hasError)
				errorPoints++;
			if (unavailable)
				unavailablePoints++;
			if (!hasError && !unavailable && objectConflicts == 0)
				passed++;
		}
		string result = "PASS";
		if (warningPoints > 0)
			result = "WARNINGS_FOUND";
		if (errorPoints > 0)
			result = "ERRORS_FOUND";
		if (scanned == 0)
			result = "NO_POINTS";
		if (unavailablePoints > 0)
			result = "INCOMPLETE";
		// Coverage is separate from the result for points that were actually inspected.
		// Полнота обхода отделена от результата реально проверенных точек.
		string coverage = "COMPLETE_LOADED_ENTITIES";
		if (unresolvedEntities > 0 || sourceMissedPoints > 0)
			coverage = "UNVERIFIED";
		if (unresolvedPointSources > 0)
			coverage = "MISSED_POINTS";
		foreach (string className, int count : unresolvedClasses)
			PrintFormat("[ME_DEBUG_AVSP_AUDIT] status=UNRESOLVED_CLASS class=%1 count=%2", className, count);
		PrintFormat("[ME_DEBUG_AVSP_AUDIT] status=COVERAGE coverage=%1 missingSources=%2 unresolvedPointSources=%3 unresolvedOtherSources=%4 unresolvedEntities=%5",
			coverage, missingSources, unresolvedPointSources, unresolvedOtherSources, unresolvedEntities);
		string sourceCheck = "NOT_TESTED";
		if (scanned > 0)
			sourceCheck = "MATCH";
		if (sourceMissedPoints > 0)
			sourceCheck = "MISMATCH";
		PrintFormat("[ME_DEBUG_AVSP_AUDIT] status=SOURCE_CHECK result=%1 knownPoints=%2 matched=%3 missed=%4",
			sourceCheck, scanned, sourceMatchedPoints, sourceMissedPoints);
		PrintFormat("[ME_DEBUG_AVSP_AUDIT] status=FINISHED result=%1 scope=inspected_points scanned=%2 passed=%3 errorPoints=%4 warningPoints=%5 unavailablePoints=%6 staticObjectIntersections=%7 clearanceErrorPoints=%8",
			result, scanned, passed, errorPoints, warningPoints, unavailablePoints, staticObjectIntersections, clearanceErrorPoints);
	}

	//! Detects an ambient component in the source or its prefab ancestry without needing a live entity.
	//! A negative result is only an observation, not proof that no derived or unloaded point exists.
	//! Ищет ambient-компонент в source и предках prefab без игровой сущности.
	//! Отрицательный результат не доказывает отсутствие производной или незагруженной точки.
	protected bool HasAmbientPointSource(IEntitySource source)
	{
		array<BaseContainer> visited = {};
		IEntitySource current = source;
		while (current && !visited.Contains(current))
		{
			visited.Insert(current);
			for (int i = 0; i < current.GetComponentCount(); i++)
			{
				IEntityComponentSource component = current.GetComponent(i);
				if (component && component.GetClassName() == "SCR_AmbientVehicleSpawnPointComponent")
					return true;
			}
			current = IEntitySource.Cast(current.GetAncestor());
		}
		return false;
	}
}
