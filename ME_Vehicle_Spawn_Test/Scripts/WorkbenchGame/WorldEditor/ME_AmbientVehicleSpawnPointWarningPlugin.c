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

enum EME_AmbientSpawnPointLabelResult
{
	LABEL_CATALOG_UNAVAILABLE,
	LABEL_SOURCE_ARRAYS_UNAVAILABLE,
	LABEL_FILTER_EMPTY_EXCLUDED_ALL,
	LABEL_FILTER_EMPTY,
	LABEL_NEUTRAL
}

class ME_AmbientSpawnPointLabelCheck
{
	EME_AmbientSpawnPointLabelResult m_eResult;
	string m_sEntityName;
	string m_sCatalog;
	array<EEditableEntityLabel> m_aIncludedLabels;
	array<EEditableEntityLabel> m_aExcludedLabels;
	int m_iCandidatesBefore;
	int m_iCandidatesAfter;
	bool m_bIncludedArraysAvailable;
	bool m_bExcludedArraysAvailable;
}

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
	protected const string STRING_CHECK_LABEL_CATALOG_UNAVAILABLE = "#MEVehicleSpawn_WB_AmbientSpawnWarning_CheckLabelCatalogUnavailable";
	protected const string STRING_CHECK_LABEL_SOURCE_ARRAYS_UNAVAILABLE = "#MEVehicleSpawn_WB_AmbientSpawnWarning_CheckLabelSourceArraysUnavailable";
	protected const string STRING_CHECK_LABEL_FILTER_EMPTY_EXCLUDED_ALL = "#MEVehicleSpawn_WB_AmbientSpawnWarning_CheckLabelFilterEmptyExcludedAll";
	protected const string STRING_CHECK_LABEL_FILTER_EMPTY = "#MEVehicleSpawn_WB_AmbientSpawnWarning_CheckLabelFilterEmpty";

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		ME_AmbientVehicleSpawnPointPreviewController.Activate();
		Print("[ME_DEBUG_AVSP_LABEL] Full scan requested. After changing labels in Inspector, run 'Check ambient vehicle spawning' again because WorldEditorPlugin has no property-change callback.");
		CheckOpenWorld();
	}

	//------------------------------------------------------------------------------------------------
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
		bool handled = super.OnWorldEditWindowDataDropped(windowType, posX, posY, dataType, data);
		CheckOpenWorld(true);
		return handled;
	}

	//------------------------------------------------------------------------------------------------
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
		array<ref ME_AmbientSpawnPointLabelCheck> labelChecks;
		int entityCount = api.GetEditorEntityCount();
		for (int i = 0; i < entityCount; i++)
		{
			IEntitySource entitySource = api.GetEditorEntity(i);
			if (!entitySource)
				continue;

			IEntity entity = api.SourceToEntity(entitySource);
			if (entity && entity.FindComponent(SCR_AmbientVehicleSpawnPointComponent))
			{
				spawnPointCount++;
				labelChecks.Insert(CheckSpawnPointLabels(entitySource, entity));
			}
		}

		ME_AmbientSpawnPointCheck check = CanCreateAmbientSpawnPoint();
		LogCheck(check, spawnPointCount);
		bool hasLabelWarning;
		string labelWarningMessageId;
		foreach (ME_AmbientSpawnPointLabelCheck labelCheck : labelChecks)
		{
			LogLabelCheck(labelCheck);
			if (!hasLabelWarning && labelCheck.m_eResult != EME_AmbientSpawnPointLabelResult.LABEL_NEUTRAL)
			{
				hasLabelWarning = true;
				labelWarningMessageId = GetLabelCheckMessageId(labelCheck.m_eResult);
			}
		}

		if (spawnPointCount == 0 && !hasIncomingSpawnPoint)
			return;

		if (check.m_eResult != EME_AmbientSpawnPointCheckResult.ALLOWED)
			ShowLocalizedDialog(GetCheckMessageId(check.m_eResult));
		else if (hasLabelWarning)
			ShowLocalizedDialog(labelWarningMessageId);
	}

	//------------------------------------------------------------------------------------------------
	private ME_AmbientSpawnPointLabelCheck CheckSpawnPointLabels(IEntitySource entitySource, IEntity entity)
	{
		ME_AmbientSpawnPointLabelCheck check = new ME_AmbientSpawnPointLabelCheck();
		check.m_sEntityName = entitySource.GetName();

		IEntityComponentSource componentSource;
		int componentCount = entitySource.GetComponentCount();
		for (int i = 0; i < componentCount; i++)
		{
			IEntityComponentSource candidateSource = entitySource.GetComponent(i);
			if (candidateSource && candidateSource.GetClassName().Contains("SCR_AmbientVehicleSpawnPointComponent"))
			{
				componentSource = candidateSource;
				break;
			}
		}

		if (!componentSource)
		{
			check.m_eResult = EME_AmbientSpawnPointLabelResult.LABEL_SOURCE_ARRAYS_UNAVAILABLE;
			return check;
		}

		check.m_bIncludedArraysAvailable = componentSource.Get("m_aIncludedEditableEntityLabels", check.m_aIncludedLabels);
		check.m_bExcludedArraysAvailable = componentSource.Get("m_aExcludedEditableEntityLabels", check.m_aExcludedLabels);
		if (!check.m_bIncludedArraysAvailable)
			check.m_aIncludedLabels.Clear();
		if (!check.m_bExcludedArraysAvailable)
			check.m_aExcludedLabels.Clear();

		if (SCR_Global.IsEditMode())
		{
			PrintFormat("[ME_DEBUG_AVSP_LABEL] entity=%1 catalog unavailable: SCR_EntityCatalogManagerComponent is intentionally not initialized in Edit mode", check.m_sEntityName);
			check.m_eResult = EME_AmbientSpawnPointLabelResult.LABEL_CATALOG_UNAVAILABLE;
			return check;
		}

		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalogManager)
		{
			check.m_eResult = EME_AmbientSpawnPointLabelResult.LABEL_CATALOG_UNAVAILABLE;
			return check;
		}

		SCR_FactionAffiliationComponent factionAffiliation = SCR_FactionAffiliationComponent.Cast(entity.FindComponent(SCR_FactionAffiliationComponent));
		if (!factionAffiliation)
		{
			PrintFormat("[ME_DEBUG_AVSP_LABEL] entity=%1 catalog unavailable: faction affiliation component is unavailable", check.m_sEntityName);
			check.m_eResult = EME_AmbientSpawnPointLabelResult.LABEL_CATALOG_UNAVAILABLE;
			return check;
		}

		Faction faction = factionAffiliation.GetAffiliatedFaction();
		if (!faction)
			faction = factionAffiliation.GetDefaultAffiliatedFaction();

		if (!faction)
		{
			PrintFormat("[ME_DEBUG_AVSP_LABEL] entity=%1 catalog unavailable: faction context is unavailable", check.m_sEntityName);
			check.m_eResult = EME_AmbientSpawnPointLabelResult.LABEL_CATALOG_UNAVAILABLE;
			return check;
		}

		FactionKey factionKey = faction.GetFactionKey();
		SCR_EntityCatalog vehicleCatalog = catalogManager.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE, factionKey, false);
		if (!vehicleCatalog)
		{
			check.m_eResult = EME_AmbientSpawnPointLabelResult.LABEL_CATALOG_UNAVAILABLE;
			return check;
		}

		check.m_sCatalog = faction.GetFactionKey();
		array<SCR_EntityCatalogEntry> candidatesBefore = {};
		array<SCR_EntityCatalogEntry> candidatesAfter = {};
		check.m_iCandidatesBefore = vehicleCatalog.GetFullFilteredEntityListWithLabels(candidatesBefore, check.m_aIncludedLabels, null, true);
		check.m_iCandidatesAfter = vehicleCatalog.GetFullFilteredEntityListWithLabels(candidatesAfter, check.m_aIncludedLabels, check.m_aExcludedLabels, true);

		if (check.m_iCandidatesAfter == 0)
		{
			if (check.m_iCandidatesBefore > 0)
				check.m_eResult = EME_AmbientSpawnPointLabelResult.LABEL_FILTER_EMPTY_EXCLUDED_ALL;
			else
				check.m_eResult = EME_AmbientSpawnPointLabelResult.LABEL_FILTER_EMPTY;
			return check;
		}

		check.m_eResult = EME_AmbientSpawnPointLabelResult.LABEL_NEUTRAL;
		return check;
	}

	//------------------------------------------------------------------------------------------------
	private void LogLabelCheck(ME_AmbientSpawnPointLabelCheck check)
	{
		PrintFormat("[ME_DEBUG_AVSP_LABEL] entity=%1 result=%2 catalog=%3 includedAvailable=%4 excludedAvailable=%5 included=%6 excluded=%7 candidatesBefore=%8 candidatesAfter=%9", check.m_sEntityName, GetLabelResultCode(check.m_eResult), check.m_sCatalog, check.m_bIncludedArraysAvailable, check.m_bExcludedArraysAvailable, check.m_aIncludedLabels, check.m_aExcludedLabels, check.m_iCandidatesBefore, check.m_iCandidatesAfter);
	}

	//------------------------------------------------------------------------------------------------
	private string GetLabelResultCode(EME_AmbientSpawnPointLabelResult result)
	{
		switch (result)
		{
			case EME_AmbientSpawnPointLabelResult.LABEL_CATALOG_UNAVAILABLE:
				return "catalog_unavailable_or_ambiguous";
			case EME_AmbientSpawnPointLabelResult.LABEL_SOURCE_ARRAYS_UNAVAILABLE:
				return "serialized_label_arrays_unavailable";
			case EME_AmbientSpawnPointLabelResult.LABEL_FILTER_EMPTY_EXCLUDED_ALL:
				return "excluded_labels_removed_all_candidates";
			case EME_AmbientSpawnPointLabelResult.LABEL_FILTER_EMPTY:
				return "filtered_result_empty";
		}

		return "neutral";
	}

	//------------------------------------------------------------------------------------------------
	private string GetLabelCheckMessageId(EME_AmbientSpawnPointLabelResult result)
	{
		switch (result)
		{
			case EME_AmbientSpawnPointLabelResult.LABEL_CATALOG_UNAVAILABLE:
				return STRING_CHECK_LABEL_CATALOG_UNAVAILABLE;
			case EME_AmbientSpawnPointLabelResult.LABEL_SOURCE_ARRAYS_UNAVAILABLE:
				return STRING_CHECK_LABEL_SOURCE_ARRAYS_UNAVAILABLE;
			case EME_AmbientSpawnPointLabelResult.LABEL_FILTER_EMPTY_EXCLUDED_ALL:
				return STRING_CHECK_LABEL_FILTER_EMPTY_EXCLUDED_ALL;
			case EME_AmbientSpawnPointLabelResult.LABEL_FILTER_EMPTY:
				return STRING_CHECK_LABEL_FILTER_EMPTY;
		}

		return STRING_CHECK_WORLD_EDITOR_UNAVAILABLE;
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
	private void LogCheck(ME_AmbientSpawnPointCheck check, int spawnPointCount = -1)
	{
		PrintFormat("[ME_DEBUG_AVSP_WB] editor scan result=%1 spawnpoints=%2 gameModes=%3 m_eTestGameFlags=%4 available=%5 spawnVehicles=%6 subscene=%7 layerId=%8 layerPath=%9 lockedHierarchy=%10", GetResultCode(check.m_eResult), spawnPointCount, check.m_iGameModeCount, check.m_eTestGameFlags, check.m_bHasTestGameFlags, check.m_bSpawnVehiclesEnabled, check.m_iGameModeSubscene, check.m_iGameModeLayerId, check.m_sGameModeLayerPath, check.m_bLockedHierarchy);
	}

	//------------------------------------------------------------------------------------------------
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
