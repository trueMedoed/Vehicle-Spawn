//! Diagnostic controller for reporting the lifecycle of the native ambient vehicle spawn-point preview.
//! Диагностический контроллер для регистрации жизненного цикла встроенного предпросмотра точки появления техники.

/*
class ME_AmbientVehicleSpawnPointPreviewController
{
	protected static ref ME_AmbientVehicleSpawnPointPreviewController s_Instance;
	protected bool m_bLifecycleReported;

	//------------------------------------------------------------------------------------------------
	//! Returns the singleton preview controller instance.
	//! Возвращает единственный экземпляр контроллера предпросмотра.
	static ME_AmbientVehicleSpawnPointPreviewController GetInstance()
	{
		if (!s_Instance)
			s_Instance = new ME_AmbientVehicleSpawnPointPreviewController();

		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Activates lifecycle reporting for the preview controller.
	//! Включает регистрацию жизненного цикла контроллера предпросмотра.
	static void Activate()
	{
		GetInstance().ActivateInternal();
	}

	//------------------------------------------------------------------------------------------------
	//! Reports the preview lifecycle once when no native preview owner is confirmed.
	//! Однократно сообщает о жизненном цикле предпросмотра, если встроенный владелец не подтверждён.
	protected void ActivateInternal()
	{
		if (m_bLifecycleReported)
			return;

		m_bLifecycleReported = true;
		Print("[ME_DEBUG_AVSP_WB] preview lifecycle status=UNVERIFIABLE reason=world_editor_has_no_confirmed_native_preview_owner");
	}
}
*/