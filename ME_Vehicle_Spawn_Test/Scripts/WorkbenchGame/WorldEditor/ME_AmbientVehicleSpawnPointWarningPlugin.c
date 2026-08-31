//! Diagnostic World Editor plugin that validates ambient vehicle spawn-point prerequisites.
//! Диагностический плагин редактора мира, проверяющий предварительные условия точек появления техники.

//------------------------------------------------------------------------------------------------
enum EME_AmbientSpawnPointCheckResult
{
	WORLD_EDITOR_UNAVAILABLE,
	NO_GAME_MODE,
	MULTIPLE_GAME_MODES,
	GAME_MODE_LAYER_LOCKED,
	TEST_GAME_FLAGS_UNAVAILABLE,
	SPAWN_VEHICLES_DISABLED,
	NO_FACTION_MANAGER,
	ALLOWED
}

//------------------------------------------------------------------------------------------------
//! Stores the result of checking the current world's ambient vehicle prerequisites.
//! Хранит результат проверки предварительных условий появления техники в текущем мире.
class ME_AmbientSpawnPointCheck
{
	EME_AmbientSpawnPointCheckResult m_eResult;
	int m_iGameModeCount;
	int m_iFactionManagerCount;
	EGameFlags m_eTestGameFlags;
	bool m_bHasTestGameFlags;
	bool m_bSpawnVehiclesEnabled;
	int m_iGameModeSubscene = -1;
	int m_iGameModeLayerId = -1;
	string m_sGameModeLayerPath;
	bool m_bLockedHierarchy;
}

//------------------------------------------------------------------------------------------------
//! Checks GameMode, layer, and Spawn Vehicles prerequisites in the World Editor.
//! Проверяет в редакторе мира GameMode, слой и предварительное условие Spawn Vehicles.
[WorkbenchPluginAttribute(name: "Check ambient vehicle spawning", description: "Checks the open world's ambient vehicle spawn points and GameMode test flags.", wbModules: { "WorldEditor" })]
class ME_AmbientVehicleSpawnPointWarningPlugin : WorldEditorPlugin
{
	private const string MESSAGE_TITLE = "Ambient vehicle spawn points";
	private const string MESSAGE_DROP_NO_GAME_MODE = "The point was not created because this world has no SCR_BaseGameMode. Create or configure exactly one editable GameMode, then enable Spawn Vehicles.";
	private const string MESSAGE_DROP_MULTIPLE_GAME_MODES = "The point was not created because multiple SCR_BaseGameMode entities were found. Configure exactly one editable GameMode, then enable Spawn Vehicles.";
	private const string MESSAGE_DROP_GAME_MODE_LAYER_LOCKED = "The point was not created because the only GameMode is on a locked layer or inside a locked parent layer and cannot be configured. Create another world with an editable GameMode.";
	private const string MESSAGE_DROP_TEST_GAME_FLAGS_UNAVAILABLE = "The point was not created because m_eTestGameFlags is unavailable on the only SCR_BaseGameMode. Use an editable GameMode that exposes Test Game Flags.";
	private const string MESSAGE_DROP_SPAWN_VEHICLES_DISABLED = "The point was not created because Spawn Vehicles is disabled in the only GameMode's Test Game Flags / m_eTestGameFlags.";
	private const string MESSAGE_DROP_NO_FACTION_MANAGER = "The point was not created because this world has no FactionManager. Add the vanilla Prefabs/MP/Managers/Factions/FactionManager_Editor.et prefab.";
	private const string MESSAGE_CHECK_WORLD_EDITOR_UNAVAILABLE = "The World Editor API is unavailable, so ambient vehicle spawning cannot be checked.";
	private const string MESSAGE_CHECK_NO_GAME_MODE = "Ambient vehicle spawning needs a GameMode to apply Test Game Flags. Add or configure a GameMode derived from SCR_BaseGameMode.";
	private const string MESSAGE_CHECK_MULTIPLE_GAME_MODES = "Multiple SCR_BaseGameMode entities were found. Ambient vehicle spawning configuration is ambiguous; configure the intended GameMode before using ambient vehicle spawn points.";
	private const string MESSAGE_CHECK_GAME_MODE_LAYER_LOCKED = "The only GameMode is on a locked layer or inside a locked parent layer and cannot be configured. Create another world with an editable GameMode.";
	private const string MESSAGE_CHECK_TEST_GAME_FLAGS_UNAVAILABLE = "Test Game Flags / m_eTestGameFlags is unavailable on the only SCR_BaseGameMode. Use an editable GameMode that exposes Test Game Flags.";
	private const string MESSAGE_CHECK_SPAWN_VEHICLES_DISABLED = "Ambient vehicle spawning is disabled for the current GameMode. Select the GameMode and enable Spawn Vehicles in Test Game Flags / m_eTestGameFlags. EGameFlags.SpawnVehicles = 2; 6 also enables SpawnAI.";
	private const string MESSAGE_CHECK_NO_FACTION_MANAGER = "Running a world with ambient vehicle spawn points requires a FactionManager. Add a FactionManager to this world.";

	//------------------------------------------------------------------------------------------------
	//! Runs the explicit ambient vehicle spawning check for the open world.
	//! Выполняет явную проверку создания ambient-техники для открытого мира.
	override void Run()
	{
		CheckOpenWorld();
	}

	//------------------------------------------------------------------------------------------------
	//! Validates a dropped ambient vehicle spawn-point prefab before native placement.
	//! Проверяет сброшенный префаб точки появления техники перед встроенным размещением.
	override bool OnWorldEditWindowDataDropped(int windowType, int posX, int posY, string dataType, array<string> data)
	{
		PrintFormat("[ME_DEBUG_AVSP_WB] Ambient spawn point drop callback entered: dataType=%1 dataCount=%2", dataType, data.Count());

		bool hasAmbientSpawnPoint;
		foreach (string resourcePath : data)
		{
			if (resourcePath.Contains("Prefabs/Systems/AmbientVehicles/AmbientVehicleSpawnpoint_"))
			{
				hasAmbientSpawnPoint = true;
				break;
			}
		}

		PrintFormat("[ME_DEBUG_AVSP_WB] Ambient spawn point drop recognised=%1", hasAmbientSpawnPoint);
		if (!hasAmbientSpawnPoint)
			return super.OnWorldEditWindowDataDropped(windowType, posX, posY, dataType, data);

		ME_AmbientSpawnPointCheck check = CanCreateAmbientSpawnPoint();
		LogCheck(check);
		if (check.m_eResult != EME_AmbientSpawnPointCheckResult.ALLOWED)
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] Ambient spawn point drop blocked: result=%1", GetResultCode(check.m_eResult));
			Workbench.Dialog(MESSAGE_TITLE, GetDropMessage(check.m_eResult));
			return true;
		}

		//ME_AmbientVehicleSpawnPointPreviewController.Activate();
		Print("[ME_DEBUG_AVSP_WB] Ambient spawn point drop allowed: forwarding native placement");
		
		return super.OnWorldEditWindowDataDropped(windowType, posX, posY, dataType, data);
	}

	//------------------------------------------------------------------------------------------------
	//! Determines whether an ambient vehicle spawn point can be created in the current world.
	//! Определяет, можно ли создать точку появления техники в текущем мире.
	private ME_AmbientSpawnPointCheck CanCreateAmbientSpawnPoint()
	{
		ME_AmbientSpawnPointCheck check = new ME_AmbientSpawnPointCheck();
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_DEBUG_AVSP_WB] Ambient spawn point check: WorldEditor unavailable");
			check.m_eResult = EME_AmbientSpawnPointCheckResult.WORLD_EDITOR_UNAVAILABLE;
			return check;
		}

		WorldEditorAPI api = worldEditor.GetApi();
		if (!api)
		{
			Print("[ME_DEBUG_AVSP_WB] Ambient spawn point check: WorldEditorAPI unavailable");
			check.m_eResult = EME_AmbientSpawnPointCheckResult.WORLD_EDITOR_UNAVAILABLE;
			return check;
		}

		LogLoadedSubsceneDefaultLayerLocks(api);

		IEntitySource gameModeSource;
		int entityCount = api.GetEditorEntityCount();
		PrintFormat("[ME_DEBUG_AVSP_WB] Ambient spawn point check: editorEntityCount=%1", entityCount);
		for (int i = 0; i < entityCount; i++)
		{
			IEntitySource entitySource = api.GetEditorEntity(i);
			if (!entitySource)
				continue;

			IEntity entity = api.SourceToEntity(entitySource);
			if (!entity)
				continue;

			if (SCR_BaseGameMode.Cast(entity))
			{
				check.m_iGameModeCount++;
				gameModeSource = entitySource;
				PrintFormat("[ME_DEBUG_AVSP_WB] Ambient spawn point check: GameMode found source=%1 name=%2", entitySource, entity.GetName());
			}

			if (FactionManager.Cast(entity))
			{
				check.m_iFactionManagerCount++;
				PrintFormat("[ME_DEBUG_AVSP_WB] Ambient spawn point check: FactionManager found source=%1 name=%2", entitySource, entity.GetName());
			}
		}

		if (check.m_iGameModeCount == 0)
		{
			check.m_eResult = EME_AmbientSpawnPointCheckResult.NO_GAME_MODE;
			return check;
		}

		if (check.m_iGameModeCount > 1)
		{
			check.m_eResult = EME_AmbientSpawnPointCheckResult.MULTIPLE_GAME_MODES;
			return check;
		}

		if (check.m_iFactionManagerCount == 0)
		{
			check.m_eResult = EME_AmbientSpawnPointCheckResult.NO_FACTION_MANAGER;
			return check;
		}

		check.m_iGameModeSubscene = gameModeSource.GetSubScene();
		check.m_iGameModeLayerId = gameModeSource.GetLayerID();
		check.m_sGameModeLayerPath = api.GetSubsceneLayerPath(check.m_iGameModeSubscene, check.m_iGameModeLayerId);
		// This checks only the discovered GameMode's own layer hierarchy. A visually locked subscene container may not be represented here; see ARMD-51.
		check.m_bLockedHierarchy = api.IsEntityLayerLockedHierarchy(check.m_iGameModeSubscene, check.m_iGameModeLayerId);
		PrintFormat("[ME_DEBUG_AVSP_WB] Ambient spawn point check: GameMode layer subscene=%1 layerId=%2 layerPath=%3 lockedHierarchy=%4", check.m_iGameModeSubscene, check.m_iGameModeLayerId, check.m_sGameModeLayerPath, check.m_bLockedHierarchy);
		if (check.m_bLockedHierarchy)
		{
			check.m_eResult = EME_AmbientSpawnPointCheckResult.GAME_MODE_LAYER_LOCKED;
			return check;
		}

		check.m_bHasTestGameFlags = gameModeSource.Get("m_eTestGameFlags", check.m_eTestGameFlags);
		PrintFormat("[ME_DEBUG_AVSP_WB] Ambient spawn point check: m_eTestGameFlags available=%1 value=%2", check.m_bHasTestGameFlags, check.m_eTestGameFlags);
		if (!check.m_bHasTestGameFlags)
		{
			check.m_eResult = EME_AmbientSpawnPointCheckResult.TEST_GAME_FLAGS_UNAVAILABLE;
			return check;
		}

		check.m_bSpawnVehiclesEnabled = (check.m_eTestGameFlags & EGameFlags.SpawnVehicles) != 0;
		PrintFormat("[ME_DEBUG_AVSP_WB] Ambient spawn point check: SpawnVehicles enabled=%1", check.m_bSpawnVehiclesEnabled);
		if (!check.m_bSpawnVehiclesEnabled)
		{
			check.m_eResult = EME_AmbientSpawnPointCheckResult.SPAWN_VEHICLES_DISABLED;
			return check;
		}

		check.m_eResult = EME_AmbientSpawnPointCheckResult.ALLOWED;
		return check;
	}

	//------------------------------------------------------------------------------------------------
	//! Logs default-layer lock state for every subscene currently loaded in the editor.
	private void LogLoadedSubsceneDefaultLayerLocks(WorldEditorAPI api)
	{
		int subsceneCount = api.GetNumSubScenes();
		PrintFormat("[ME_DEBUG_AVSP_WB] loadedSubscenes=%1", subsceneCount);
		for (int subscene = 0; subscene < subsceneCount; subscene++)
		{
			int entityCount = api.GetEntityCount(subscene);
			PrintFormat("[ME_DEBUG_AVSP_WB] subscene=%1 entityCount=%2", subscene, entityCount);

			int layerId = api.GetSubsceneLayerId(subscene, "default");
			if (layerId < 0)
			{
				PrintFormat("[ME_DEBUG_AVSP_WB] subscene=%1 defaultLayer unresolved", subscene);
				continue;
			}

			string layerPath = api.GetSubsceneLayerPath(subscene, layerId);
			// See https://report.bistudio.com/issues/ARMD-51 for UI/API lock-state clarification.
			bool layerLocked = api.IsEntityLayerLocked(subscene, layerId);
			bool hierarchyLocked = api.IsEntityLayerLockedHierarchy(subscene, layerId);
			PrintFormat("[ME_DEBUG_AVSP_WB] subscene=%1 defaultLayerId=%2 defaultLayerPath=%3 layerLocked=%4 hierarchyLocked=%5", subscene, layerId, layerPath, layerLocked, hierarchyLocked);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Checks the open world and shows a warning when ambient vehicle prerequisites are not met.
	//! Проверяет открытый мир и показывает предупреждение, если предварительные условия не выполнены.
	private void CheckOpenWorld(bool hasIncomingSpawnPoint = false)
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			LogCheck(CanCreateAmbientSpawnPoint());
			return;
		}

		WorldEditorAPI api = worldEditor.GetApi();
		if (!api)
		{
			LogCheck(CanCreateAmbientSpawnPoint());
			return;
		}

		int spawnPointCount;
		int entityCount = api.GetEditorEntityCount();
		for (int i = 0; i < entityCount; i++)
		{
			IEntitySource entitySource = api.GetEditorEntity(i);
			if (!entitySource)
				continue;

			IEntity entity = api.SourceToEntity(entitySource);
			if (entity && entity.FindComponent(SCR_AmbientVehicleSpawnPointComponent))
				spawnPointCount++;
		}

		ME_AmbientSpawnPointCheck check = CanCreateAmbientSpawnPoint();
		LogCheck(check, spawnPointCount);

		if (spawnPointCount == 0 && !hasIncomingSpawnPoint)
			return;

		if (check.m_eResult != EME_AmbientSpawnPointCheckResult.ALLOWED)
			Workbench.Dialog(MESSAGE_TITLE, GetCheckMessage(check.m_eResult));
	}

	//------------------------------------------------------------------------------------------------
	//! Logs the current ambient vehicle prerequisite result and discovered entity state.
	//! Записывает результат проверки предварительных условий и состояние найденных сущностей.
	private void LogCheck(ME_AmbientSpawnPointCheck check, int spawnPointCount = -1)
	{
		PrintFormat("[ME_DEBUG_AVSP_WB] editor scan result=%1 spawnpoints=%2 gameModes=%3 factionManagers=%4 m_eTestGameFlags=%5 available=%6 spawnVehicles=%7 subscene=%8 layerId=%9", GetResultCode(check.m_eResult), spawnPointCount, check.m_iGameModeCount, check.m_iFactionManagerCount, check.m_eTestGameFlags, check.m_bHasTestGameFlags, check.m_bSpawnVehiclesEnabled, check.m_iGameModeSubscene, check.m_iGameModeLayerId);
		PrintFormat("[ME_DEBUG_AVSP_WB] editor scan layerPath=%1 lockedHierarchy=%2", check.m_sGameModeLayerPath, check.m_bLockedHierarchy);
	}

	//------------------------------------------------------------------------------------------------
	//! Converts a prerequisite check result to a diagnostic log code.
	//! Преобразует результат проверки предварительных условий в диагностический код журнала.
	private string GetResultCode(EME_AmbientSpawnPointCheckResult result)
	{
		switch (result)
		{
			case EME_AmbientSpawnPointCheckResult.WORLD_EDITOR_UNAVAILABLE:
				return "world_editor_unavailable";
			case EME_AmbientSpawnPointCheckResult.NO_GAME_MODE:
				return "no_game_mode";
			case EME_AmbientSpawnPointCheckResult.MULTIPLE_GAME_MODES:
				return "multiple_game_modes";
			case EME_AmbientSpawnPointCheckResult.GAME_MODE_LAYER_LOCKED:
				return "game_mode_layer_locked";
			case EME_AmbientSpawnPointCheckResult.TEST_GAME_FLAGS_UNAVAILABLE:
				return "test_game_flags_unavailable";
			case EME_AmbientSpawnPointCheckResult.SPAWN_VEHICLES_DISABLED:
				return "spawn_vehicles_disabled";
			case EME_AmbientSpawnPointCheckResult.NO_FACTION_MANAGER:
				return "no_faction_manager";
		}

		return "allowed";
	}

	private string GetDropMessage(EME_AmbientSpawnPointCheckResult result)
	{
		switch (result)
		{
			case EME_AmbientSpawnPointCheckResult.NO_GAME_MODE:
				return MESSAGE_DROP_NO_GAME_MODE;
			case EME_AmbientSpawnPointCheckResult.MULTIPLE_GAME_MODES:
				return MESSAGE_DROP_MULTIPLE_GAME_MODES;
			case EME_AmbientSpawnPointCheckResult.GAME_MODE_LAYER_LOCKED:
				return MESSAGE_DROP_GAME_MODE_LAYER_LOCKED;
			case EME_AmbientSpawnPointCheckResult.TEST_GAME_FLAGS_UNAVAILABLE:
				return MESSAGE_DROP_TEST_GAME_FLAGS_UNAVAILABLE;
			case EME_AmbientSpawnPointCheckResult.SPAWN_VEHICLES_DISABLED:
				return MESSAGE_DROP_SPAWN_VEHICLES_DISABLED;
			case EME_AmbientSpawnPointCheckResult.NO_FACTION_MANAGER:
				return MESSAGE_DROP_NO_FACTION_MANAGER;
		}

		return MESSAGE_CHECK_WORLD_EDITOR_UNAVAILABLE;
	}

	//------------------------------------------------------------------------------------------------
	//! Selects the warning shown by the explicit world check.
	private string GetCheckMessage(EME_AmbientSpawnPointCheckResult result)
	{
		switch (result)
		{
			case EME_AmbientSpawnPointCheckResult.NO_GAME_MODE:
				return MESSAGE_CHECK_NO_GAME_MODE;
			case EME_AmbientSpawnPointCheckResult.MULTIPLE_GAME_MODES:
				return MESSAGE_CHECK_MULTIPLE_GAME_MODES;
			case EME_AmbientSpawnPointCheckResult.GAME_MODE_LAYER_LOCKED:
				return MESSAGE_CHECK_GAME_MODE_LAYER_LOCKED;
			case EME_AmbientSpawnPointCheckResult.TEST_GAME_FLAGS_UNAVAILABLE:
				return MESSAGE_CHECK_TEST_GAME_FLAGS_UNAVAILABLE;
			case EME_AmbientSpawnPointCheckResult.SPAWN_VEHICLES_DISABLED:
				return MESSAGE_CHECK_SPAWN_VEHICLES_DISABLED;
			case EME_AmbientSpawnPointCheckResult.NO_FACTION_MANAGER:
				return MESSAGE_CHECK_NO_FACTION_MANAGER;
		}

		return MESSAGE_CHECK_WORLD_EDITOR_UNAVAILABLE;
	}
}
