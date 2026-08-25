modded class SCR_AmbientVehicleSystem
{
	protected bool m_bDebugPauseStateKnown;
	protected bool m_bDebugLastPaused;
	protected int m_iDebugUpdatePointCalls;
	protected bool m_bDebugUpdatePointSeen;

	static override void InitInfo(WorldSystemInfo outInfo)
	{
		super.InitInfo(outInfo);
		PrintFormat("[ME_DEBUG_AVSP_SYS] InitInfo=%1", outInfo.ToString());
	}

	override bool ShouldBeEnabledInEditMode()
	{
		bool enabled = super.ShouldBeEnabledInEditMode();
		PrintFormat("[ME_DEBUG_AVSP_SYS] ShouldBeEnabledInEditMode=%1", enabled);
		return enabled;
	}

	override bool ShouldBePaused()
	{
		bool paused = super.ShouldBePaused();
		if (!m_bDebugPauseStateKnown || paused != m_bDebugLastPaused)
		{
			PrintFormat("[ME_DEBUG_AVSP_SYS] ShouldBePaused=%1", paused);
			m_bDebugPauseStateKnown = true;
			m_bDebugLastPaused = paused;
		}

		return paused;
	}

	override void OnUpdatePoint(WorldUpdatePointArgs args)
	{
		m_iDebugUpdatePointCalls++;
		if (!m_bDebugUpdatePointSeen)
		{
			Print("[ME_DEBUG_AVSP_SYS] OnUpdatePoint FIRST");
			m_bDebugUpdatePointSeen = true;
		}

		float timerBefore = m_fTimer;
		float checkIntervalBefore = m_fCheckInterval;
		int indexToCheckBefore = m_iIndexToCheck;
		int playerCountBefore = m_aPlayers.Count();
		int spawnpointCountBefore = m_aSpawnpoints.Count();
		bool enabledBefore = IsEnabled();
		bool spawnVehiclesBefore = GetGame().AreGameFlagsSet(EGameFlags.SpawnVehicles);
		PrintFormat("[ME_DEBUG_AVSP_SYS] OnUpdatePoint BEFORE call=%1 timeSlice=%2 timer=%3 interval=%4 index=%5 players=%6 spawnpoints=%7 enabled=%8 spawnVehicles=%9", m_iDebugUpdatePointCalls, args.GetTimeSliceSeconds(), timerBefore, checkIntervalBefore, indexToCheckBefore, playerCountBefore, spawnpointCountBefore, enabledBefore, spawnVehiclesBefore);

		super.OnUpdatePoint(args);

		bool spawnVehiclesAfter = GetGame().AreGameFlagsSet(EGameFlags.SpawnVehicles);
		PrintFormat("[ME_DEBUG_AVSP_SYS] OnUpdatePoint AFTER call=%1 timeSlice=%2 timer=%3 interval=%4 index=%5 players=%6 spawnpoints=%7 enabled=%8 spawnVehicles=%9", m_iDebugUpdatePointCalls, args.GetTimeSliceSeconds(), m_fTimer, m_fCheckInterval, m_iIndexToCheck, m_aPlayers.Count(), m_aSpawnpoints.Count(), IsEnabled(), spawnVehiclesAfter);
	}

	override void OnInit()
	{
		Print("[ME_DEBUG_AVSP_SYS] OnInit BEFORE super");
		super.OnInit();

		BaseContainer source = GetSystems().FindSystemSource(SCR_AmbientVehicleSystem);
		if (!source)
			Print("[ME_DEBUG_AVSP_SYS] OnInit source=NULL");
		else
			PrintFormat("[ME_DEBUG_AVSP_SYS] OnInit source class=%1 name=%2", source.GetClassName(), source.GetName());

		GetOnVehicleSpawned().Insert(OnVehicleSpawnedDebug);

		BaseGameMode gameMode = GetGame().GetGameMode();
		PrintFormat("[ME_DEBUG_AVSP_SYS] OnInit CONTEXT gameMode=%1 worldSystems=%2 enabled=%3", gameMode, GetWorld().GetSystems(), IsEnabled());

		array<SCR_AmbientVehicleSpawnPointComponent> spawnpoints = {};
		int spawnpointCount = GetSpawnpoints(spawnpoints);
		PrintFormat("[ME_DEBUG_AVSP_SYS] OnInit AFTER super spawnpoints=%1 listSize=%2", spawnpointCount, spawnpoints.Count());
	}

	override void OnCleanup()
	{
		Print("[ME_DEBUG_AVSP_SYS] OnCleanup BEFORE super");
		super.OnCleanup();
		Print("[ME_DEBUG_AVSP_SYS] OnCleanup AFTER super");
	}

	override void RegisterSpawnpoint(notnull SCR_AmbientVehicleSpawnPointComponent spawnpoint)
	{
		IEntity owner = spawnpoint.GetOwner();
		PrintFormat("[ME_DEBUG_AVSP_SYS] RegisterSpawnpoint BEFORE entity=%1 coords=%2 listSize=%3", owner.GetName(), owner.GetOrigin(), m_aSpawnpoints.Count());
		super.RegisterSpawnpoint(spawnpoint);
		PrintFormat("[ME_DEBUG_AVSP_SYS] RegisterSpawnpoint AFTER entity=%1 coords=%2 listSize=%3", owner.GetName(), owner.GetOrigin(), m_aSpawnpoints.Count());
	}

	override void UnregisterSpawnpoint(notnull SCR_AmbientVehicleSpawnPointComponent spawnpoint)
	{
		IEntity owner = spawnpoint.GetOwner();
		PrintFormat("[ME_DEBUG_AVSP_SYS] UnregisterSpawnpoint BEFORE entity=%1 coords=%2 listSize=%3", owner.GetName(), owner.GetOrigin(), m_aSpawnpoints.Count());
		super.UnregisterSpawnpoint(spawnpoint);
		PrintFormat("[ME_DEBUG_AVSP_SYS] UnregisterSpawnpoint AFTER entity=%1 coords=%2 listSize=%3", owner.GetName(), owner.GetOrigin(), m_aSpawnpoints.Count());
	}

	protected void OnVehicleSpawnedDebug(SCR_AmbientVehicleSpawnPointComponent spawnpoint, Vehicle vehicle)
	{
		IEntity owner = spawnpoint.GetOwner();
		PrintFormat("[ME_DEBUG_AVSP_SYS] VehicleSpawned vehicle=%1 vehicleCoords=%2 spawnpoint=%3 spawnpointCoords=%4", vehicle.GetName(), vehicle.GetOrigin(), owner.GetName(), owner.GetOrigin());
	}

	override void ProcessSpawnpoint(int spawnpointIndex)
	{
		array<SCR_AmbientVehicleSpawnPointComponent> spawnpoints = {};
		int spawnpointCount = GetSpawnpoints(spawnpoints);
		if (spawnpointIndex >= 0 && spawnpointIndex < spawnpointCount)
		{
			SCR_AmbientVehicleSpawnPointComponent spawnpoint = spawnpoints[spawnpointIndex];
			IEntity owner = spawnpoint.GetOwner();
			PrintFormat("[ME_DEBUG_AVSP_SYS] BEFORE index=%1 listCount=%2 entity=%3 coords=%4 depleted=%5 processed=%6 firstSpawnDone=%7", spawnpointIndex, spawnpointCount, owner.GetName(), owner.GetOrigin(), spawnpoint.GetIsDepleted(), spawnpoint.GetIsSpawnProcessed(), spawnpoint.GetIsFirstSpawnDone());
		}
		else
		{
			PrintFormat("[ME_DEBUG_AVSP_SYS] BEFORE index=%1 listCount=%2 invalid", spawnpointIndex, spawnpointCount);
		}

		PrintFormat("[ME_DEBUG_AVSP_SYS] ENTER ProcessSpawnpoint index=%1", spawnpointIndex);
		super.ProcessSpawnpoint(spawnpointIndex);
		PrintFormat("[ME_DEBUG_AVSP_SYS] EXIT ProcessSpawnpoint index=%1", spawnpointIndex);

		spawnpoints.Clear();
		spawnpointCount = GetSpawnpoints(spawnpoints);
		if (spawnpointIndex >= 0 && spawnpointIndex < spawnpointCount)
		{
			SCR_AmbientVehicleSpawnPointComponent spawnpoint = spawnpoints[spawnpointIndex];
			IEntity owner = spawnpoint.GetOwner();
			PrintFormat("[ME_DEBUG_AVSP_SYS] AFTER index=%1 listCount=%2 entity=%3 coords=%4 depleted=%5 processed=%6 firstSpawnDone=%7", spawnpointIndex, spawnpointCount, owner.GetName(), owner.GetOrigin(), spawnpoint.GetIsDepleted(), spawnpoint.GetIsSpawnProcessed(), spawnpoint.GetIsFirstSpawnDone());
		}
		else
		{
			PrintFormat("[ME_DEBUG_AVSP_SYS] AFTER index=%1 listCount=%2 invalid", spawnpointIndex, spawnpointCount);
		}
	}
}
