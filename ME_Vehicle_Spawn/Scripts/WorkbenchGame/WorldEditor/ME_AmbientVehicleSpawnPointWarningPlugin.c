//! World Editor plugin that prevents invalid ambient vehicle spawn-point placement and checks prerequisites for existing points.
//! It validates the world GameMode, its Spawn Vehicles test flag, editable GameMode layer state, and FactionManager presence.

//------------------------------------------------------------------------------------------------
//! Editor diagnostic states returned while checking ambient vehicle spawn-point prerequisites.
//! These values describe editor observations and do not replace runtime spawning validation.
enum EME_AmbientSpawnPointCheckResult
{
	WORLD_EDITOR_UNAVAILABLE,
	NO_GAME_MODE,
	MULTIPLE_GAME_MODES,
	GAME_MODE_LAYER_LOCKED,
	TEST_GAME_FLAGS_UNAVAILABLE,
	SPAWN_VEHICLES_DISABLED,
	NO_FACTION_MANAGER,
	DROPPED_SPAWNPOINT_FACTION_UNAVAILABLE,
	ALLOWED
}

//------------------------------------------------------------------------------------------------
//! Stores the editor diagnostic result and observations collected for one ambient spawn-point prerequisite check.
//! It records GameMode and FactionManager counts, GameMode layer state, Test Game Flags, and SpawnVehicles state.
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
	private const string MESSAGE_DROP_SPAWNPOINT_FACTION_UNAVAILABLE = "The point was not created because an incoming ambient vehicle spawn point requires a faction unavailable from this world's FactionManager or its faction affiliation could not be read. Configure the FactionManager or use a compatible point.";
	private const string MESSAGE_CHECK_WORLD_EDITOR_UNAVAILABLE = "The World Editor API is unavailable, so ambient vehicle spawning cannot be checked.";
	private const string MESSAGE_CHECK_NO_GAME_MODE = "Ambient vehicle spawning needs a GameMode to apply Test Game Flags. Add or configure a GameMode derived from SCR_BaseGameMode.";
	private const string MESSAGE_CHECK_MULTIPLE_GAME_MODES = "Multiple SCR_BaseGameMode entities were found. Ambient vehicle spawning configuration is ambiguous; configure the intended GameMode before using ambient vehicle spawn points.";
	private const string MESSAGE_CHECK_GAME_MODE_LAYER_LOCKED = "The only GameMode is on a locked layer or inside a locked parent layer and cannot be configured. Create another world with an editable GameMode.";
	private const string MESSAGE_CHECK_TEST_GAME_FLAGS_UNAVAILABLE = "Test Game Flags / m_eTestGameFlags is unavailable on the only SCR_BaseGameMode. Use an editable GameMode that exposes Test Game Flags.";
	private const string MESSAGE_CHECK_SPAWN_VEHICLES_DISABLED = "Ambient vehicle spawning is disabled for the current GameMode. Select the GameMode and enable Spawn Vehicles in Test Game Flags / m_eTestGameFlags. EGameFlags.SpawnVehicles = 2; 6 also enables SpawnAI.";
	private const string MESSAGE_CHECK_NO_FACTION_MANAGER = "Running a world with ambient vehicle spawn points requires a FactionManager. Add a FactionManager to this world.";
	
	//------------------------------------------------------------------------------------------------
//! Runs the explicit check for ambient spawn-point prerequisites in the open world.
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
//!
//! \param[in] windowType Workbench window receiving the drop
//! \param[in] posX Horizontal drop position in the window
//! \param[in] posY Vertical drop position in the window
//! \param[in] dataType Identifier for dropped data
//! \param[in] data Dropped resource paths
//! \return True when the plugin consumes an invalid ambient-point drop; otherwise super's result
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

		FactionManager factionManager;
		ME_AmbientSpawnPointCheck check = CanCreateAmbientSpawnPoint(factionManager);
		if (check.m_eResult != EME_AmbientSpawnPointCheckResult.ALLOWED)
		{
			Workbench.Dialog(MESSAGE_TITLE, GetDropMessage(check.m_eResult));
			return true;
		}

		if (!ValidateIncomingAmbientSpawnPointFactions(data, factionManager, check))
		{
			Workbench.Dialog(MESSAGE_TITLE, GetDropMessage(check.m_eResult));
			return true;
		}

		return super.OnWorldEditWindowDataDropped(windowType, posX, posY, dataType, data);
	}

	//------------------------------------------------------------------------------------------------
	//! Collects editor state to decide whether an ambient vehicle spawn point may be placed.
	//! It checks GameMode count and editable layer, m_eTestGameFlags including SpawnVehicles, and FactionManager.
	//!
	//! \param[out] activeFactionManager FactionManager found in the current world when the check succeeds
	//! \return Populated prerequisite-check result for the current World Editor state
	//!
	//! \param[out] activeFactionManager FactionManager, found in the current world when the check succeeds
	//! \return Populated prerequisite-check result for the current World Editor state
	private ME_AmbientSpawnPointCheck CanCreateAmbientSpawnPoint(out FactionManager activeFactionManager = null)
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

		LogLoadedSubsceneDefaultLayerLocks(api);

		IEntitySource gameModeSource;
		FactionManager factionManager;
		int entityCount = api.GetEditorEntityCount();
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
			}

			FactionManager manager = FactionManager.Cast(entity);
			if (manager)
			{
				check.m_iFactionManagerCount++;
				factionManager = manager;
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

		activeFactionManager = factionManager;
		check.m_eResult = EME_AmbientSpawnPointCheckResult.ALLOWED;
		return check;
	}

//------------------------------------------------------------------------------------------------
//! Validates only incoming ambient spawn-point prefabs against the selected FactionManager before native placement.
//! It does not create entities or change the world. The first unreadable or unavailable prefab cancels the complete drop.
//!
//! \param[in] resourcePaths Dropped resource paths to inspect
//! \param[in] factionManager FactionManager selected by the successful prerequisite scan
//! \param[in,out] check Result data populated when an incoming prefab is rejected
//! \return True when every incoming ambient prefab is factionless or compatible with the FactionManager
//!
//! \param[in] resourcePaths Dropped resource paths for inspection
//! \param[in] factionManager FactionManager selected by the successful prerequisite scan
//! \param[in,out] check Result data populated when an incoming prefab is rejected
//! \return True when every incoming ambient prefab is factionless or compatible with the FactionManager
private bool ValidateIncomingAmbientSpawnPointFactions(array<string> resourcePaths, FactionManager factionManager, ME_AmbientSpawnPointCheck check)
	{
		array<Faction> factions = {};
		factionManager.GetFactionsList(factions);

		foreach (string resourcePath : resourcePaths)
		{
			if (!resourcePath.Contains("Prefabs/Systems/AmbientVehicles/AmbientVehicleSpawnpoint_"))
				continue;

			FactionKey requiredKey;
			if (!TryGetIncomingAmbientSpawnPointFactionKey(resourcePath, requiredKey))
			{
				check.m_eResult = EME_AmbientSpawnPointCheckResult.DROPPED_SPAWNPOINT_FACTION_UNAVAILABLE;
				return false;
			}

			if (requiredKey.IsEmpty())
				continue;

			bool factionAvailable;
			foreach (Faction faction : factions)
			{
				FactionKey factionKey = faction.GetFactionKey();
				if (!factionKey.IsEmpty() && factionKey == requiredKey && factionManager.GetFactionByKey(requiredKey))
				{
					factionAvailable = true;
					break;
				}
			}

			if (factionAvailable)
				continue;

			check.m_eResult = EME_AmbientSpawnPointCheckResult.DROPPED_SPAWNPOINT_FACTION_UNAVAILABLE;
			return false;
		}

		return true;
	}

//------------------------------------------------------------------------------------------------
//! Reads the resolved faction affiliation key from an incoming ambient spawn-point prefab without creating an entity.
//! An empty key denotes a factionless point.
//!
//! \param[in] resourcePath Ambient spawn-point prefab resource path
//! \param[out] requiredKey Resolved faction affiliation key, possibly empty
//! \return True when the resolved affiliation key was read; false when placement must be blocked conservatively
//!
//! \param[in] resourcePath Ambient spawn-point prefab resource path
//! \param[out] requiredKey Resolved faction affiliation key, possibly empty
//! \return True when the resolved affiliation key was read; false when placement must be blocked conservatively
private bool TryGetIncomingAmbientSpawnPointFactionKey(ResourceName resourcePath, out FactionKey requiredKey)
	{
		Resource resource = Resource.Load(resourcePath);
		if (!resource || !resource.IsValid())
			return false;

		IEntitySource prefabSource = resource.GetResource().ToEntitySource();
		if (!prefabSource)
			return false;

		IEntityComponentSource affiliationSource = FindFactionAffiliationComponentSource(prefabSource);
		if (!affiliationSource)
			return false;

		return affiliationSource.Get("faction affiliation", requiredKey);
	}

//------------------------------------------------------------------------------------------------
//! Finds the resolved faction-affiliation component source in an incoming prefab source.
//!
//! \param[in] prefabSource Resolved incoming prefab entity source
//! \return Resolved SCR_FactionAffiliationComponent source, or null when it is absent
//!
//! \param[in] prefabSource Resolved incoming prefab entity source
//! \return Resolved SCR_FactionAffiliationComponent source, or null when it is absent
private IEntityComponentSource FindFactionAffiliationComponentSource(IEntitySource prefabSource)
	{
		for (int i = 0; i < prefabSource.GetComponentCount(); i++)
		{
			IEntityComponentSource componentSource = prefabSource.GetComponent(i);
			if (componentSource && componentSource.GetClassName() == "SCR_FactionAffiliationComponent")
				return componentSource;
		}

		return null;
	}

//------------------------------------------------------------------------------------------------
//! Logs the lock state of every loaded subscene's default layer for Workbench diagnostics.
//! This supplements the discovered GameMode layer check and does not modify layers.
//!
//! \param[in] api World Editor API used to enumerate subscenes and layers
//!
//! \param[in] api World Editor API used to enumerate subscenes and layers
private void LogLoadedSubsceneDefaultLayerLocks(WorldEditorAPI api)
	{
		int subsceneCount = api.GetNumSubScenes();
		for (int subscene = 0; subscene < subsceneCount; subscene++)
		{
			int entityCount = api.GetEntityCount(subscene);
			
			int layerId = api.GetSubsceneLayerId(subscene, "default");
			if (layerId < 0)
			{
				continue;
			}

			string layerPath = api.GetSubsceneLayerPath(subscene, layerId);
			// See https://report.bistudio.com/issues/ARMD-51 for UI/API lock-state clarification.
			bool layerLocked = api.IsEntityLayerLocked(subscene, layerId);
			bool hierarchyLocked = api.IsEntityLayerLockedHierarchy(subscene, layerId);
		}
	}

//------------------------------------------------------------------------------------------------
//! Checks the open world and displays a warning only when existing or incoming points need it.
//!
//! \param[in] hasIncomingSpawnPoint True when the check accompanies an incoming ambient spawn point
//!
//! \param[in] hasIncomingSpawnPoint True when the check accompanies an incoming ambient spawn point
private void CheckOpenWorld(bool hasIncomingSpawnPoint = false)
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			return;
		}

		WorldEditorAPI api = worldEditor.GetApi();
		if (!api)
		{
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

		if (spawnPointCount == 0 && !hasIncomingSpawnPoint)
			return;

		if (check.m_eResult != EME_AmbientSpawnPointCheckResult.ALLOWED)
			Workbench.Dialog(MESSAGE_TITLE, GetCheckMessage(check.m_eResult));
	}

//------------------------------------------------------------------------------------------------
//! Converts a prerequisite-check state into a stable diagnostic log code.
//!
//! \param[in] result Editor diagnostic state to convert
//! \return Stable lowercase code used in diagnostic log output
//!
//! \param[in] result Editor diagnostic state to convert
//! \return Stable lowercase code used in diagnostic log output
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
		case EME_AmbientSpawnPointCheckResult.DROPPED_SPAWNPOINT_FACTION_UNAVAILABLE:
			return "dropped_spawnpoint_faction_unavailable";
	}

	return "allowed";
}

//------------------------------------------------------------------------------------------------
//! Selects the placement-blocking message for a failed prerequisite-check state.
//!
//! \param[in] result Editor diagnostic state that blocked placement
//! \return Corresponding placement-blocking message
//!
//! \param[in] result Editor diagnostic state that blocked placement
//! \return Corresponding placement-blocking message
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
			case EME_AmbientSpawnPointCheckResult.DROPPED_SPAWNPOINT_FACTION_UNAVAILABLE:
				return MESSAGE_DROP_SPAWNPOINT_FACTION_UNAVAILABLE;
		}

		return MESSAGE_CHECK_WORLD_EDITOR_UNAVAILABLE;
	}

	//------------------------------------------------------------------------------------------------
	//! Selects the warning shown by the explicit world check.
	//!
	//! \param[in] result Editor diagnostic state to describe
	//! \return Corresponding explicit-check warning message
	//!
	//! \param[in] result Editor diagnostic state to describe
	//! \return Corresponding explicit-check warning message
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
