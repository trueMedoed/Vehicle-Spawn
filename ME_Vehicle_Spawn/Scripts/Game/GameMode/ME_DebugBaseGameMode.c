modded class SCR_BaseGameMode
{
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
	}
}
