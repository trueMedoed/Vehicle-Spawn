//! Test-only Workbench plugin that applies validated canonical editor names to vehicle-bounds fixture roots.
//! Workbench-плагин только для Test, применяющий проверенные канонические editor-имена к корням fixture границ техники.

//------------------------------------------------------------------------------------------------
//! Applies canonical filename-stem editor names to validated vehicle-bounds fixture roots.
//! Применяет канонические editor-имена stem файлов к проверенным корням fixture границ техники.
[WorkbenchPluginAttribute(name: "Apply vehicle bounds fixture names", description: "Renames validated marked fixture roots in one undoable batch.", wbModules: { "WorldEditor" })]
class ME_ApplyVehicleBoundsFixtureCanonicalNamesPlugin : WorldEditorPlugin
{
	//------------------------------------------------------------------------------------------------
	//! Runs the guarded canonical-name rename batch.
	//! Выполняет защищённую пакетную операцию канонического переименования.
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_DEBUG_AVSP_WB] bounds_fixture_names status=FAIL phase=apply reason=world_editor_unavailable");
			return;
		}

		ME_VehicleBoundsFixtureCanonicalNames.Apply(worldEditor.GetApi());
	}
}
