modded class SCR_AmbientVehicleSpawnPointComponent
{
	override void OnPostInit(IEntity owner)
	{
		/*SCR_FactionAffiliationComponent factionComponent = SCR_FactionAffiliationComponent.Cast(owner.FindComponent(SCR_FactionAffiliationComponent));
		
		Print("[ME_DEBUG_AVSP] factionComponent: " + factionComponent);*/
		
		
		
		
		PrintFormat("[ME_DEBUG_AVSP] OnPostInit BEFORE super entity=%1 class=%2 coords=%3", owner.GetName(), owner.Type().ToString(), owner.GetOrigin());

		super.OnPostInit(owner);

		PrintFormat("[ME_DEBUG_AVSP] OnPostInit AFTER super entity=%1 class=%2 coords=%3", owner.GetName(), owner.Type().ToString(), owner.GetOrigin());

		vector foundPosition;
		bool found = SCR_WorldTools.FindEmptyTerrainPosition(foundPosition, owner.GetOrigin(), 0, 0.5, 2, TraceFlags.ENTS|TraceFlags.OCEAN, owner.GetWorld());
		//bool found = SCR_WorldTools.FindEmptyTerrainPosition(foundPosition, owner.GetOrigin(), 5, 5);
		PrintFormat("[ME_DEBUG_AVSP_POS] entity=%1 found=%2 source=%3 result=%4", owner.GetName(), found, owner.GetOrigin(), foundPosition);

		/*
		World world = GetGame().GetWorld();
		
		if (world)
			Print("[ME_DEBUG_AVSP] world: " + world);
		

		SCR_AmbientVehicleSystem ambientVehicleSystem1 = SCR_AmbientVehicleSystem.Cast(world.FindSystem(SCR_AmbientVehicleSystem));
		
		Print("[ME_DEBUG_AVSP] ambientVehicleSystem1: " + ambientVehicleSystem1);
		*/
		
		
		SCR_AmbientVehicleSystem ambientVehicleSystem = SCR_AmbientVehicleSystem.GetInstance();
		if (!ambientVehicleSystem)
		{
			Print("[ME_DEBUG_AVSP] Ambient vehicle system unavailable");
			return;
		}

		array<SCR_AmbientVehicleSpawnPointComponent> spawnpoints = {};
		int spawnpointCount = ambientVehicleSystem.GetSpawnpoints(spawnpoints);
		PrintFormat("[ME_DEBUG_AVSP] Ambient vehicle system available spawnpoints=%1 listSize=%2 editModeEnabled=%3", spawnpointCount, spawnpoints.Count(), ambientVehicleSystem.ShouldBeEnabledInEditMode());
	}
}
