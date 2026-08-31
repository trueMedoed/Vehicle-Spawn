//------------------------------------------------------------------------------------------------
//! Logs the outcome of ambient vehicle spawning: which vehicle was created at which spawn point.
//! Subscribes to the system's OnVehicleSpawned invoker, which the spawn point component raises only
//! after the vehicle entity exists. A logged line therefore means the spawn actually succeeded,
//! unlike m_sPrefab, which only records the prefab that was selected before the free-space check.
modded class SCR_AmbientVehicleSystem
{
	//------------------------------------------------------------------------------------------------
    //! Subscribes the spawn reporter once the system is initialised.
    override void OnInit()
    {
            super.OnInit();

            GetOnVehicleSpawned().Insert(ME_ReportVehicleSpawned);
    }

	//------------------------------------------------------------------------------------------------
	//! Reports a completed ambient vehicle spawn.
    //! \param[in] spawnpoint Spawn point that produced the vehicle
    //! \param[in] vehicle Vehicle entity that was created
    protected void ME_ReportVehicleSpawned(SCR_AmbientVehicleSpawnPointComponent spawnpoint, Vehicle vehicle)
    {
            IEntity point = spawnpoint.GetOwner();

            ResourceName prefab;
            EntityPrefabData prefabData = vehicle.GetPrefabData();
            if (prefabData)
                    prefab = prefabData.GetPrefabName();
    }
	
}