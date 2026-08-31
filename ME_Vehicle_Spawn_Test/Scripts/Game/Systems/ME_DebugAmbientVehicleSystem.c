/*
modded class SCR_AmbientVehicleSystem
{
	protected bool m_bDebugPauseStateKnown;
	protected bool m_bDebugLastPaused;
	protected int m_iDebugUpdatePointCalls;
	protected bool m_bDebugUpdatePointSeen;
	protected bool m_bDebugSpawnVehiclesHintLogged;
	protected int m_iDebugVehicleSpawnedEvents;

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
		if (!spawnVehiclesBefore && !m_bDebugSpawnVehiclesHintLogged)
		{
			m_bDebugSpawnVehiclesHintLogged = true;
			Print("[ME_DEBUG_AVSP_HINT] Ambient vehicle spawning is disabled: an ambient spawn point alone is not enough. In Game Mode, enable SpawnVehicles in Test Game Flags / m_eTestGameFlags (EGameFlags.SpawnVehicles = 2; 6 also enables SpawnAI).");
		}

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
		m_iDebugVehicleSpawnedEvents++;
		IEntity owner = spawnpoint.GetOwner();
		PrintFormat("[ME_DEBUG_AVSP_SYS] VehicleSpawned vehicle=%1 vehicleCoords=%2 spawnpoint=%3 spawnpointCoords=%4", vehicle.GetName(), vehicle.GetOrigin(), owner.GetName(), owner.GetOrigin());
	}

	override void ProcessSpawnpoint(int spawnpointIndex)
	{
		array<SCR_AmbientVehicleSpawnPointComponent> spawnpoints = {};
		int spawnpointCount = GetSpawnpoints(spawnpoints);
		SCR_AmbientVehicleSpawnPointComponent spawnpoint;
		bool depletedBefore;
		bool firstSpawnDoneBefore;
		int finalCandidateCount = -1;
		int vehicleSpawnedEventsBefore = m_iDebugVehicleSpawnedEvents;
		if (spawnpointIndex >= 0 && spawnpointIndex < spawnpointCount)
		{
			spawnpoint = spawnpoints[spawnpointIndex];
			IEntity owner = spawnpoint.GetOwner();
			depletedBefore = spawnpoint.GetIsDepleted();
			firstSpawnDoneBefore = spawnpoint.GetIsFirstSpawnDone();
			PrintFormat("[ME_DEBUG_AVSP_SYS] BEFORE index=%1 listCount=%2 entity=%3 coords=%4 depleted=%5 processed=%6 firstSpawnDone=%7", spawnpointIndex, spawnpointCount, owner.GetName(), owner.GetOrigin(), depletedBefore, spawnpoint.GetIsSpawnProcessed(), firstSpawnDoneBefore);
			finalCandidateCount = spawnpoint.ME_LogLabelFilter();
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
			SCR_AmbientVehicleSpawnPointComponent spawnpointAfter = spawnpoints[spawnpointIndex];
			IEntity owner = spawnpointAfter.GetOwner();
			bool depletedAfter = spawnpointAfter.GetIsDepleted();
			bool firstSpawnDoneAfter = spawnpointAfter.GetIsFirstSpawnDone();
			PrintFormat("[ME_DEBUG_AVSP_SYS] AFTER index=%1 listCount=%2 entity=%3 coords=%4 depleted=%5 processed=%6 firstSpawnDone=%7", spawnpointIndex, spawnpointCount, owner.GetName(), owner.GetOrigin(), depletedAfter, spawnpointAfter.GetIsSpawnProcessed(), firstSpawnDoneAfter);

			if (finalCandidateCount == 0)
				PrintFormat("[ME_DEBUG_AVSP_ERROR] entity=%1 coords=%2 reason=label_filter_empty", owner.GetName(), owner.GetOrigin());
			else if (finalCandidateCount > 0 && !depletedBefore && depletedAfter && !firstSpawnDoneBefore)
				PrintFormat("[ME_DEBUG_AVSP_ERROR] entity=%1 coords=%2 reason=empty_terrain_position", owner.GetName(), owner.GetOrigin());
			else if (m_iDebugVehicleSpawnedEvents > vehicleSpawnedEventsBefore)
				PrintFormat("[ME_DEBUG_AVSP_SYS] entity=%1 coords=%2 status=spawn_completed", owner.GetName(), owner.GetOrigin());
			else
				PrintFormat("[ME_DEBUG_AVSP_SYS] entity=%1 coords=%2 status=spawn_not_completed", owner.GetName(), owner.GetOrigin());
		}
		else
		{
			PrintFormat("[ME_DEBUG_AVSP_SYS] AFTER index=%1 listCount=%2 invalid", spawnpointIndex, spawnpointCount);
		}
	}
}
*/


//------------------------------------------------------------------------------------------------
//! Reports completed ambient-vehicle creation without replacing the vanilla spawning flow.
//! The callback is invoked only after the spawn point has created a vehicle entity. Consequently,
//! a log entry confirms a completed spawn, unlike m_sPrefab, which is only the prefab selected
//! before the vanilla free-space check.
//! Сообщает о завершённом создании ambient-техники, не заменяя ванильный процесс её появления.
//! Callback вызывается только после создания сущности техники точкой появления. Поэтому запись
//! в журнале подтверждает завершённое появление, в отличие от m_sPrefab, который содержит лишь
//! префаб, выбранный до ванильной проверки свободного места.
modded class SCR_AmbientVehicleSystem
{
	//------------------------------------------------------------------------------------------------
	//! Subscribes the reporter to the ambient system's completed-spawn invoker after base initialization.
	//! The subscription observes vanilla behavior only and does not initiate or alter spawning.
	//! Подписывает обработчик отчёта на invoker завершённого появления ambient-системы после базовой инициализации.
	//! Подписка только наблюдает ванильное поведение и не запускает и не изменяет появление техники.
    override void OnInit()
    {
            super.OnInit();

            GetOnVehicleSpawned().Insert(ME_ReportVehicleSpawned);
    }

	//------------------------------------------------------------------------------------------------
	//! Logs a vehicle that the vanilla spawn point has successfully created.
	//!
	//! \param[in] spawnpoint Spawn point that raised the completed-spawn callback
	//! \param[in] vehicle Created vehicle entity; its prefab is read for the diagnostic entry
	//! Сообщает о технике, успешно созданной ванильной точкой появления.
	//!
	//! \param[in] spawnpoint Точка появления, вызвавшая callback завершённого появления
	//! \param[in] vehicle Созданная сущность техники; её префаб считывается для диагностической записи
    protected void ME_ReportVehicleSpawned(SCR_AmbientVehicleSpawnPointComponent spawnpoint, Vehicle vehicle)
    {
            IEntity point = spawnpoint.GetOwner();

            ResourceName prefab;
            EntityPrefabData prefabData = vehicle.GetPrefabData();
            if (prefabData)
                    prefab = prefabData.GetPrefabName();

            PrintFormat("[ME_DEBUG_AVSP_SYS] spawn completed vehiclePrefab=%1 vehicleCoordinates=%2 Entity=%3, coordinates=%4",
                    prefab, vehicle.GetOrigin(), point.GetName(), point.GetOrigin());
    }
	
}