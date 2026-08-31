//------------------------------------------------------------------------------------------------
//! Describes the result of validating ambient vehicle spawn-point prerequisites.
//! Описывает результат проверки предварительных условий точки появления техники.
enum EME_AmbientSpawnPointCheckResult
{
	WORLD_EDITOR_UNAVAILABLE,
	NO_GAME_MODE,
	MULTIPLE_GAME_MODES,
	GAME_MODE_LAYER_LOCKED,
	TEST_GAME_FLAGS_UNAVAILABLE,
	SPAWN_VEHICLES_DISABLED,
	ALLOWED
}

//------------------------------------------------------------------------------------------------
//! Stores the result of checking the current world's ambient vehicle prerequisites.
//! Хранит результат проверки предварительных условий появления техники в текущем мире.
class ME_AmbientSpawnPointCheck
{
	EME_AmbientSpawnPointCheckResult m_eResult;
	int m_iGameModeCount;
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
	protected const string STRING_TITLE = "#MEVehicleSpawn_WB_AmbientSpawnWarning_Title";
	protected const string STRING_DROP_NO_GAME_MODE = "#MEVehicleSpawn_WB_AmbientSpawnWarning_DropNoGameMode";
	protected const string STRING_DROP_MULTIPLE_GAME_MODES = "#MEVehicleSpawn_WB_AmbientSpawnWarning_DropMultipleGameModes";
	protected const string STRING_DROP_GAME_MODE_LAYER_LOCKED = "#MEVehicleSpawn_WB_AmbientSpawnWarning_DropGameModeLayerLocked";
	protected const string STRING_DROP_TEST_GAME_FLAGS_UNAVAILABLE = "#MEVehicleSpawn_WB_AmbientSpawnWarning_DropTestGameFlagsUnavailable";
	protected const string STRING_DROP_SPAWN_VEHICLES_DISABLED = "#MEVehicleSpawn_WB_AmbientSpawnWarning_DropSpawnVehiclesDisabled";
	protected const string STRING_CHECK_WORLD_EDITOR_UNAVAILABLE = "#MEVehicleSpawn_WB_AmbientSpawnWarning_CheckWorldEditorUnavailable";
	protected const string STRING_CHECK_NO_GAME_MODE = "#MEVehicleSpawn_WB_AmbientSpawnWarning_CheckNoGameMode";
	protected const string STRING_CHECK_MULTIPLE_GAME_MODES = "#MEVehicleSpawn_WB_AmbientSpawnWarning_CheckMultipleGameModes";
	protected const string STRING_CHECK_GAME_MODE_LAYER_LOCKED = "#MEVehicleSpawn_WB_AmbientSpawnWarning_CheckGameModeLayerLocked";
	protected const string STRING_CHECK_TEST_GAME_FLAGS_UNAVAILABLE = "#MEVehicleSpawn_WB_AmbientSpawnWarning_CheckTestGameFlagsUnavailable";
	protected const string STRING_CHECK_SPAWN_VEHICLES_DISABLED = "#MEVehicleSpawn_WB_AmbientSpawnWarning_CheckSpawnVehiclesDisabled";

	//------------------------------------------------------------------------------------------------
	//! Runs the prerequisite check for the open world.
	//! Выполняет проверку предварительных условий открытого мира.
	override void Run()
	{
		ME_AmbientVehicleSpawnPointPreviewController.Activate();
		CheckOpenWorld();
	}

	//------------------------------------------------------------------------------------------------
	//! Validates a dropped ambient vehicle spawn-point prefab before native placement.
	//! Проверяет сброшенный префаб точки появления техники перед встроенным размещением.
	override bool OnWorldEditWindowDataDropped(int windowType, int posX, int posY, string dataType, array<string> data)
	{
		bool hasAmbientSpawnPoint;
		foreach (string resourcePath : data)
		{
			if (resourcePath.Contains("Prefabs/Systems/AmbientVehicles/AmbientVehicleSpawnpoint_"))
			{
				hasAmbientSpawnPoint = true;
				break;
			}
		}

		if (!hasAmbientSpawnPoint)
			return super.OnWorldEditWindowDataDropped(windowType, posX, posY, dataType, data);

		ME_AmbientSpawnPointCheck check = CanCreateAmbientSpawnPoint();
		if (check.m_eResult != EME_AmbientSpawnPointCheckResult.ALLOWED)
		{
			LogCheck(check);
			PrintFormat("[ME_DEBUG_AVSP_WB] Ambient spawn point drop blocked: result=%1", GetResultCode(check.m_eResult));
			ShowLocalizedDialog(GetDropMessageId(check.m_eResult));
			return true;
		}

		ME_AmbientVehicleSpawnPointPreviewController.Activate();
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
			check.m_eResult = EME_AmbientSpawnPointCheckResult.WORLD_EDITOR_UNAVAILABLE;
			return check;
		}

		WorldEditorAPI api = worldEditor.GetApi();
		if (!api)
		{
			check.m_eResult = EME_AmbientSpawnPointCheckResult.WORLD_EDITOR_UNAVAILABLE;
			return check;
		}

		IEntitySource gameModeSource;
		int entityCount = api.GetEditorEntityCount();
		for (int i = 0; i < entityCount; i++)
		{
			IEntitySource entitySource = api.GetEditorEntity(i);
			if (!entitySource)
				continue;

			IEntity entity = api.SourceToEntity(entitySource);
			if (entity && SCR_BaseGameMode.Cast(entity))
			{
				check.m_iGameModeCount++;
				gameModeSource = entitySource;
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

		check.m_iGameModeSubscene = gameModeSource.GetSubScene();
		check.m_iGameModeLayerId = gameModeSource.GetLayerID();
		check.m_sGameModeLayerPath = api.GetSubsceneLayerPath(check.m_iGameModeSubscene, check.m_iGameModeLayerId);
		check.m_bLockedHierarchy = api.IsEntityLayerLockedHierarchy(check.m_iGameModeSubscene, check.m_iGameModeLayerId);
		if (check.m_bLockedHierarchy)
		{
			check.m_eResult = EME_AmbientSpawnPointCheckResult.GAME_MODE_LAYER_LOCKED;
			return check;
		}

		check.m_bHasTestGameFlags = gameModeSource.Get("m_eTestGameFlags", check.m_eTestGameFlags);
		if (!check.m_bHasTestGameFlags)
		{
			check.m_eResult = EME_AmbientSpawnPointCheckResult.TEST_GAME_FLAGS_UNAVAILABLE;
			return check;
		}

		check.m_bSpawnVehiclesEnabled = (check.m_eTestGameFlags & EGameFlags.SpawnVehicles) != 0;
		if (!check.m_bSpawnVehiclesEnabled)
		{
			check.m_eResult = EME_AmbientSpawnPointCheckResult.SPAWN_VEHICLES_DISABLED;
			return check;
		}

		check.m_eResult = EME_AmbientSpawnPointCheckResult.ALLOWED;
		return check;
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
			ShowLocalizedDialog(GetCheckMessageId(check.m_eResult));
	}

	//------------------------------------------------------------------------------------------------
	private void ShowLocalizedDialog(string messageId)
	{
		string language;
		WidgetManager.GetLanguage(language);
		PrintFormat("[ME_DEBUG_AVSP_WB] Workbench language=%1", language);
		Workbench.Dialog(WidgetManager.Translate(STRING_TITLE), WidgetManager.Translate(messageId));
	}

	//------------------------------------------------------------------------------------------------
	//! Logs the current ambient vehicle prerequisite result and discovered entity state.
	//! Записывает результат проверки предварительных условий и состояние найденных сущностей.
	private void LogCheck(ME_AmbientSpawnPointCheck check, int spawnPointCount = -1)
	{
		PrintFormat("[ME_DEBUG_AVSP_WB] editor scan result=%1 spawnpoints=%2 gameModes=%3 m_eTestGameFlags=%4 available=%5 spawnVehicles=%6 subscene=%7 layerId=%8 layerPath=%9 lockedHierarchy=%10", GetResultCode(check.m_eResult), spawnPointCount, check.m_iGameModeCount, check.m_eTestGameFlags, check.m_bHasTestGameFlags, check.m_bSpawnVehiclesEnabled, check.m_iGameModeSubscene, check.m_iGameModeLayerId, check.m_sGameModeLayerPath, check.m_bLockedHierarchy);
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
		}

		return "allowed";
	}

	//------------------------------------------------------------------------------------------------
	private string GetDropMessageId(EME_AmbientSpawnPointCheckResult result)
	{
		switch (result)
		{
			case EME_AmbientSpawnPointCheckResult.NO_GAME_MODE:
				return STRING_DROP_NO_GAME_MODE;
			case EME_AmbientSpawnPointCheckResult.MULTIPLE_GAME_MODES:
				return STRING_DROP_MULTIPLE_GAME_MODES;
			case EME_AmbientSpawnPointCheckResult.GAME_MODE_LAYER_LOCKED:
				return STRING_DROP_GAME_MODE_LAYER_LOCKED;
			case EME_AmbientSpawnPointCheckResult.TEST_GAME_FLAGS_UNAVAILABLE:
				return STRING_DROP_TEST_GAME_FLAGS_UNAVAILABLE;
			case EME_AmbientSpawnPointCheckResult.SPAWN_VEHICLES_DISABLED:
				return STRING_DROP_SPAWN_VEHICLES_DISABLED;
		}

		return STRING_CHECK_WORLD_EDITOR_UNAVAILABLE;
	}

	//------------------------------------------------------------------------------------------------
	private string GetCheckMessageId(EME_AmbientSpawnPointCheckResult result)
	{
		switch (result)
		{
			case EME_AmbientSpawnPointCheckResult.NO_GAME_MODE:
				return STRING_CHECK_NO_GAME_MODE;
			case EME_AmbientSpawnPointCheckResult.MULTIPLE_GAME_MODES:
				return STRING_CHECK_MULTIPLE_GAME_MODES;
			case EME_AmbientSpawnPointCheckResult.GAME_MODE_LAYER_LOCKED:
				return STRING_CHECK_GAME_MODE_LAYER_LOCKED;
			case EME_AmbientSpawnPointCheckResult.TEST_GAME_FLAGS_UNAVAILABLE:
				return STRING_CHECK_TEST_GAME_FLAGS_UNAVAILABLE;
			case EME_AmbientSpawnPointCheckResult.SPAWN_VEHICLES_DISABLED:
				return STRING_CHECK_SPAWN_VEHICLES_DISABLED;
		}

		return STRING_CHECK_WORLD_EDITOR_UNAVAILABLE;
	}
}
