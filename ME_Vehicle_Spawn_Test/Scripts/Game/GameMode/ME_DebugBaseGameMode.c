modded class SCR_BaseGameMode
{
	protected bool m_bDebugSpawnVehiclesHintShown;

	override void OnGameStateChanged()
	{
		PrintFormat("[ME_DEBUG_AVSP_GM] OnGameStateChanged BEFORE state=%1 running=%2 master=%3", GetState(), IsRunning(), IsMaster());
		super.OnGameStateChanged();
		PrintFormat("[ME_DEBUG_AVSP_GM] OnGameStateChanged AFTER state=%1 running=%2 master=%3", GetState(), IsRunning(), IsMaster());
	}

	override void OnGameModeStart()
	{
		PrintFormat("[ME_DEBUG_AVSP_GM] OnGameModeStart BEFORE state=%1 running=%2 master=%3", GetState(), IsRunning(), IsMaster());
		super.OnGameModeStart();
		PrintFormat("[ME_DEBUG_AVSP_GM] OnGameModeStart AFTER state=%1 running=%2 master=%3", GetState(), IsRunning(), IsMaster());
	}

	override void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{
		PrintFormat("[ME_DEBUG_AVSP_GM] OnPlayerSpawned BEFORE playerId=%1 entity=%2", playerId, controlledEntity);
		super.OnPlayerSpawned(playerId, controlledEntity);
		PrintFormat("[ME_DEBUG_AVSP_GM] OnPlayerSpawned AFTER playerId=%1 entity=%2", playerId, controlledEntity);
		ShowSpawnVehiclesHintForLocalPlayer(playerId);
	}

	protected void ShowSpawnVehiclesHintForLocalPlayer(int playerId)
	{
		if (m_bDebugSpawnVehiclesHintShown || GetGame().AreGameFlagsSet(EGameFlags.SpawnVehicles))
			return;

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (playerId == 0 || playerId != localPlayerId)
			return;

		if (!SCR_HintManagerComponent.GetInstance())
			return;

		m_bDebugSpawnVehiclesHintShown = SCR_HintManagerComponent.ShowCustomHint("Ambient vehicle spawning is disabled: an ambient spawn point alone is not enough. In Game Mode, enable SpawnVehicles in Test Game Flags / m_eTestGameFlags (EGameFlags.SpawnVehicles = 2; 6 also enables SpawnAI).", "Ambient vehicle spawning disabled");
	}
}
