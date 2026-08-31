//! World Editor plugin that prevents invalid ambient vehicle spawn-point placement and checks prerequisites for existing points.
//! It validates the world GameMode, its Spawn Vehicles test flag, editable GameMode layer state, and FactionManager presence.
//! Плагин редактора мира, предотвращающий некорректное размещение точек появления ambient-техники и проверяющий предварительные условия для существующих точек.
//! Он проверяет GameMode мира, его тестовый флаг Spawn Vehicles, состояние редактируемости слоя GameMode и наличие FactionManager.

//------------------------------------------------------------------------------------------------
//! Editor diagnostic states returned while checking ambient vehicle spawn-point prerequisites.
//! These values describe editor observations and do not replace runtime spawning validation.
//! Диагностические состояния редактора, возвращаемые при проверке предпосылок точек появления ambient-техники.
//! Эти значения описывают наблюдения редактора и не заменяют проверку runtime-появления.
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
//! Stores the editor diagnostic result and observations collected for one ambient spawn-point prerequisite check.
//! It records GameMode and FactionManager counts, GameMode layer state, Test Game Flags, and SpawnVehicles state.
//! Хранит диагностический результат редактора и наблюдения одной проверки предпосылок точки появления ambient-техники.
//! Он сохраняет количество GameMode и FactionManager, состояние слоя GameMode, Test Game Flags и состояние SpawnVehicles.
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
//! Validates ambient vehicle spawn-point prerequisites in the World Editor before placement and through an explicit command.
//! It examines the GameMode count, GameMode layer state, Test Game Flags, and FactionManager presence without changing the world.
//! Проверяет предварительные условия точек появления ambient-техники в редакторе мира до размещения и по явной команде.
//! Он анализирует количество GameMode, состояние слоя GameMode, Test Game Flags и наличие FactionManager, не изменяя мир.
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
	//! Runs the explicit diagnostic for ambient spawn-point prerequisites in the open world.
	//! The check observes GameMode, its layer and Test Game Flags, SpawnVehicles, and FactionManager.
	//! Запускает явную диагностику предпосылок точек появления ambient-техники в открытом мире.
	//! Проверка наблюдает GameMode, его слой и Test Game Flags, SpawnVehicles и FactionManager.
	override void Run()
	{
		CheckOpenWorld();
	}

	//------------------------------------------------------------------------------------------------
	//! Validates a dropped ambient vehicle spawn-point prefab before native placement.
	//! On a failed prerequisite check, it consumes the drop; otherwise it forwards the event to super
	//! so Workbench performs normal placement.
	//!
	//! \param[in] windowType Workbench window receiving the drop
	//! \param[in] posX Horizontal drop position in the window
	//! \param[in] posY Vertical drop position in the window
	//! \param[in] dataType Type identifier for the dropped data
	//! \param[in] data Dropped resource paths
	//! \return True when the plugin consumes an invalid ambient-point drop; otherwise super's result
	//! Проверяет сброшенный префаб точки появления ambient-техники до встроенного размещения.
	//! При неуспешной проверке предпосылок метод поглощает drop; иначе передаёт событие super,
	//! чтобы Workbench выполнил обычное размещение.
	//!
	//! \param[in] windowType Окно Workbench, принимающее drop
	//! \param[in] posX Горизонтальная позиция drop в окне
	//! \param[in] posY Вертикальная позиция drop в окне
	//! \param[in] dataType Идентификатор типа сброшенных данных
	//! \param[in] data Пути к сброшенным ресурсам
	//! \return True, когда плагин поглощает недопустимый drop ambient-точки; иначе результат super
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
	//! Collects editor diagnostic state to decide whether an ambient vehicle spawn point may be placed.
	//! It checks GameMode count and editable layer, m_eTestGameFlags including SpawnVehicles, and FactionManager.
	//!
	//! \return Populated prerequisite-check result for the current World Editor state
	//! Собирает диагностическое состояние редактора, чтобы определить, можно ли разместить точку появления ambient-техники.
	//! Метод проверяет количество GameMode и редактируемость слоя, m_eTestGameFlags включая SpawnVehicles и FactionManager.
	//!
	//! \return Заполненный результат проверки предпосылок для текущего состояния World Editor
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
	//! Logs the lock state of every loaded subscene's default layer for Workbench diagnostics.
	//! This supplements the discovered GameMode layer check and does not modify layers.
	//!
	//! \param[in] api World Editor API used to enumerate subscenes and layers
	//! Записывает состояние блокировки default-слоя каждой загруженной subscene для диагностики Workbench.
	//! Это дополняет проверку слоя найденного GameMode и не изменяет слои.
	//!
	//! \param[in] api API World Editor для перечисления subscene и слоёв
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
	//! Checks the open world, logs diagnostic state, and displays a warning only when existing or incoming points need it.
	//!
	//! \param[in] hasIncomingSpawnPoint True when the check accompanies an incoming ambient spawn point
	//! Проверяет открытый мир, записывает диагностическое состояние и показывает предупреждение только при необходимости для существующих или входящих точек.
	//!
	//! \param[in] hasIncomingSpawnPoint True, когда проверка сопровождает входящую ambient-точку
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
	//! Logs a prerequisite-check result together with discovered entity, GameMode layer, flag, and FactionManager state.
	//!
	//! \param[in] check Collected prerequisite-check result to log
	//! \param[in] spawnPointCount Number of existing points, or -1 when not counted
	//! Записывает результат проверки предпосылок вместе с состоянием найденных сущностей, слоя GameMode, флагов и FactionManager.
	//!
	//! \param[in] check Собранный результат проверки предпосылок для журнала
	//! \param[in] spawnPointCount Количество существующих точек либо -1, когда подсчёт не выполнялся
	private void LogCheck(ME_AmbientSpawnPointCheck check, int spawnPointCount = -1)
	{
		PrintFormat("[ME_DEBUG_AVSP_WB] editor scan result=%1 spawnpoints=%2 gameModes=%3 factionManagers=%4 m_eTestGameFlags=%5 available=%6 spawnVehicles=%7 subscene=%8 layerId=%9", GetResultCode(check.m_eResult), spawnPointCount, check.m_iGameModeCount, check.m_iFactionManagerCount, check.m_eTestGameFlags, check.m_bHasTestGameFlags, check.m_bSpawnVehiclesEnabled, check.m_iGameModeSubscene, check.m_iGameModeLayerId);
		PrintFormat("[ME_DEBUG_AVSP_WB] editor scan layerPath=%1 lockedHierarchy=%2", check.m_sGameModeLayerPath, check.m_bLockedHierarchy);
	}

	//------------------------------------------------------------------------------------------------
	//! Converts a prerequisite-check state into a stable diagnostic log code.
	//!
	//! \param[in] result Editor diagnostic state to convert
	//! \return Stable lowercase code used in diagnostic log output
	//! Преобразует состояние проверки предпосылок в стабильный диагностический код журнала.
	//!
	//! \param[in] result Диагностическое состояние редактора для преобразования
	//! \return Стабильный строчный код, используемый в диагностическом выводе
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

	//------------------------------------------------------------------------------------------------
	//! Selects the placement-blocking message for a failed prerequisite-check state.
	//!
	//! \param[in] result Editor diagnostic state that blocked placement
	//! \return Corresponding placement-blocking message
	//! Выбирает сообщение, блокирующее размещение, для состояния неуспешной проверки предпосылок.
	//!
	//! \param[in] result Диагностическое состояние редактора, заблокировавшее размещение
	//! \return Соответствующее сообщение о блокировке размещения
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
	//!
	//! \param[in] result Editor diagnostic state to describe
	//! \return Corresponding explicit-check warning message
	//! Выбирает предупреждение, отображаемое явной проверкой мира.
	//!
	//! \param[in] result Диагностическое состояние редактора для описания
	//! \return Соответствующее сообщение предупреждения явной проверки
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
