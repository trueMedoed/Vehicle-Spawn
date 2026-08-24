modded class SCR_AmbientVehicleSystem
{
	override bool ShouldBeEnabledInEditMode()
	{
		bool enabled = super.ShouldBeEnabledInEditMode();
		PrintFormat("[ME_DEBUG_AVSP_SYS] ShouldBeEnabledInEditMode=%1", enabled);
		return enabled;
	}

	override void OnInit()
	{
		Print("[ME_DEBUG_AVSP_SYS] OnInit BEFORE super");
		super.OnInit();

		array<SCR_AmbientVehicleSpawnPointComponent> spawnpoints = {};
		int spawnpointCount = GetSpawnpoints(spawnpoints);
		PrintFormat("[ME_DEBUG_AVSP_SYS] OnInit AFTER super spawnpoints=%1 listSize=%2", spawnpointCount, spawnpoints.Count());
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
