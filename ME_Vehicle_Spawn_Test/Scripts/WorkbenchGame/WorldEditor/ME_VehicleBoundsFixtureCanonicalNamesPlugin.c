//! Test-only Workbench plugins that preflight and apply canonical editor names for vehicle-bounds fixture roots.
//! Workbench-плагины только для Test, выполняющие preflight и применяющие канонические editor-имена корней fixture границ техники.

//------------------------------------------------------------------------------------------------
//! Holds one validated marked fixture root and its canonical editor name.
//! Хранит один проверенный помеченный корень fixture и его каноническое editor-имя.
class ME_VehicleBoundsFixtureRenameEntry
{
	IEntitySource m_Source;
	IEntity m_Entity;
	string m_sCatalogPrefab;
	string m_sTargetName;
}

//------------------------------------------------------------------------------------------------
//! Provides the shared read-only inventory and guarded batch rename operation for marked fixture roots.
//! Предоставляет общий read-only inventory и защищённую пакетную операцию переименования помеченных корней fixture.
class ME_VehicleBoundsFixtureCanonicalNames
{
	protected const int EXPECTED_MARKER_COUNT = 146;

	//------------------------------------------------------------------------------------------------
	//! Collects, validates, and reports every marked fixture root without changing the world.
	//! Собирает, проверяет и выводит каждый помеченный корень fixture без изменения мира.
	static bool Preflight(WorldEditorAPI api, out array<ref ME_VehicleBoundsFixtureRenameEntry> entries)
	{
		entries = {};
		if (!api)
		{
			Print("[ME_DEBUG_AVSP_WB] bounds_fixture_names status=FAIL phase=preflight reason=world_editor_api_unavailable");
			return false;
		}

		array<IEntitySource> allSources = {};
		int entityCount = api.GetEditorEntityCount();
		for (int index = 0; index < entityCount; index++)
		{
			IEntitySource source = api.GetEditorEntity(index);
			IEntity entity = api.SourceToEntity(source);
			if (!entity)
				continue;

			allSources.Insert(source);
			ME_VehicleBoundsFixtureMarkerComponent marker = ME_VehicleBoundsFixtureMarkerComponent.Cast(entity.FindComponent(ME_VehicleBoundsFixtureMarkerComponent));
			if (!marker)
				continue;

			string catalogPrefab = marker.GetCatalogPrefab();
			string targetName;
			if (!ME_GetTargetName(catalogPrefab, targetName))
			{
				PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_names status=FAIL phase=preflight reason=invalid_catalog_prefab entity=%1 prefab=%2", entity.GetName(), catalogPrefab);
				return false;
			}

			foreach (ME_VehicleBoundsFixtureRenameEntry existing : entries)
			{
				if (existing.m_sCatalogPrefab == catalogPrefab)
				{
					PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_names status=FAIL phase=preflight reason=duplicate_catalog_prefab prefab=%1", catalogPrefab);
					return false;
				}

				if (existing.m_sTargetName == targetName)
				{
					PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_names status=FAIL phase=preflight reason=duplicate_target_name target=%1", targetName);
					return false;
				}
			}

			ME_VehicleBoundsFixtureRenameEntry entry = new ME_VehicleBoundsFixtureRenameEntry();
			entry.m_Source = source;
			entry.m_Entity = entity;
			entry.m_sCatalogPrefab = catalogPrefab;
			entry.m_sTargetName = targetName;
			entries.Insert(entry);
		}

		if (entries.Count() != EXPECTED_MARKER_COUNT)
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_names status=FAIL phase=preflight reason=marker_count_mismatch markers=%1 expected=%2", entries.Count(), EXPECTED_MARKER_COUNT);
			return false;
		}

		foreach (ME_VehicleBoundsFixtureRenameEntry entry : entries)
		{
			foreach (IEntitySource source : allSources)
			{
				if (source == entry.m_Source)
					continue;

				IEntity entity = api.SourceToEntity(source);
				if (entity && entity.GetName() == entry.m_sTargetName)
				{
					PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_names status=FAIL phase=preflight reason=editor_name_conflict target=%1 entity=%2", entry.m_sTargetName, entity.GetName());
					return false;
				}
			}
		}

		int unchanged;
		foreach (ME_VehicleBoundsFixtureRenameEntry entry : entries)
		{
			if (entry.m_Entity.GetName() == entry.m_sTargetName)
				unchanged++;

			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_name_mapping prefab=%1 current=%2 target=%3 position=%4 rotation=%5", entry.m_sCatalogPrefab, entry.m_Entity.GetName(), entry.m_sTargetName, entry.m_Entity.GetOrigin(), entry.m_Entity.GetAngles());
		}

		PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_names status=PASS phase=preflight markers=%1 unchanged=%2", entries.Count(), unchanged);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Applies a single undoable rename batch after a complete successful preflight.
	//! Применяет одну отменяемую пакетную операцию переименования после полного успешного preflight.
	static void Apply(WorldEditorAPI api)
	{
		array<ref ME_VehicleBoundsFixtureRenameEntry> entries;
		if (!Preflight(api, entries))
			return;

		int renames;
		int unchanged;
		foreach (ME_VehicleBoundsFixtureRenameEntry entry : entries)
		{
			if (entry.m_Entity.GetName() == entry.m_sTargetName)
				unchanged++;
			else
				renames++;
		}

		if (renames == 0)
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_names status=PASS phase=apply markers=%1 renames=0 unchanged=%2", entries.Count(), unchanged);
			return;
		}

		api.BeginEntityAction("Canonical vehicle-bounds fixture names");
		foreach (ME_VehicleBoundsFixtureRenameEntry entry : entries)
		{
			if (entry.m_Entity.GetName() != entry.m_sTargetName)
				api.RenameEntity(entry.m_Source, entry.m_sTargetName);
		}
		api.EndEntityAction();

		PrintFormat("[ME_DEBUG_AVSP_WB] bounds_fixture_names status=PASS phase=apply markers=%1 renames=%2 unchanged=%3", entries.Count(), renames, unchanged);
	}

	//------------------------------------------------------------------------------------------------
	//! Derives the filename stem from a non-empty terminal .et catalog prefab path.
	//! Извлекает stem имени файла из непустого terminal .et пути catalog prefab.
	protected static bool ME_GetTargetName(string catalogPrefab, out string targetName)
	{
		targetName = "";
		if (catalogPrefab.IsEmpty())
			return false;

		int slashIndex = catalogPrefab.LastIndexOf("/");
		int startIndex = slashIndex + 1;
		int filenameLength = catalogPrefab.Length() - startIndex;
		if (filenameLength <= 3)
			return false;

		string filename = catalogPrefab.Substring(startIndex, filenameLength);
		if (!filename.EndsWith(".et"))
			return false;

		targetName = filename.Substring(0, filename.Length() - 3);
		return !targetName.IsEmpty();
	}
}

//------------------------------------------------------------------------------------------------
//! Reports the canonical rename mapping for the dedicated vehicle-bounds fixture without mutating it.
//! Выводит каноническое сопоставление переименования выделенного fixture границ техники без его изменения.
[WorkbenchPluginAttribute(name: "Preflight vehicle bounds fixture names", description: "Reports and validates canonical fixture-root names without changing the world.", wbModules: { "WorldEditor" })]
class ME_VehicleBoundsFixtureCanonicalNamesPlugin : WorldEditorPlugin
{
	//------------------------------------------------------------------------------------------------
	//! Runs the read-only canonical-name preflight.
	//! Выполняет read-only preflight канонических имён.
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_DEBUG_AVSP_WB] bounds_fixture_names status=FAIL phase=preflight reason=world_editor_unavailable");
			return;
		}

		array<ref ME_VehicleBoundsFixtureRenameEntry> entries;
		ME_VehicleBoundsFixtureCanonicalNames.Preflight(worldEditor.GetApi(), entries);
	}
}
