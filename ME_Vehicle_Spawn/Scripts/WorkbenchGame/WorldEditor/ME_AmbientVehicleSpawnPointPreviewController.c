class ME_AmbientVehicleSpawnPointPreviewController
{
	protected static ref ME_AmbientVehicleSpawnPointPreviewController s_Instance;
	protected bool m_bLifecycleReported;

	static ME_AmbientVehicleSpawnPointPreviewController GetInstance()
	{
		if (!s_Instance)
			s_Instance = new ME_AmbientVehicleSpawnPointPreviewController();

		return s_Instance;
	}

	static void Activate()
	{
		GetInstance().ActivateInternal();
	}

	protected void ActivateInternal()
	{
		if (m_bLifecycleReported)
			return;

		m_bLifecycleReported = true;
		Print("[ME_DEBUG_AVSP_WB] preview lifecycle status=UNVERIFIABLE reason=world_editor_has_no_confirmed_native_preview_owner");
	}
}
