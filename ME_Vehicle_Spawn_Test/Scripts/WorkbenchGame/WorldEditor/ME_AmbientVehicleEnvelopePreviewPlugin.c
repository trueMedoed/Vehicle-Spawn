//! Test-only manual Workbench preview for the conservative vehicle envelope of one selected ambient spawn point.
//! Ручной предпросмотр только для Test в Workbench консервативного vehicle-envelope одной выбранной ambient spawn-point.

//------------------------------------------------------------------------------------------------
//! Renders one orientation-aligned worst-case snapshot box covering every catalog candidate of the selected spawn point.
//! Draws nothing when snapshot coverage is incomplete, so a partial envelope is never presented as verified.
//! Отрисовывает один ориентированный по осям worst-case snapshot-box, покрывающий каждого кандидата каталога выбранной spawn-point.
//! Ничего не рисует при неполном покрытии snapshot, поэтому частичный envelope никогда не выдаётся за проверенный.
[WorkbenchPluginAttribute(name: "Preview ambient vehicle envelope", description: "Shows the conservative snapshot envelope for one selected ambient vehicle spawn point.", wbModules: { "WorldEditor" })]
class ME_AmbientVehicleEnvelopePreviewPlugin : WorldEditorPlugin
{
	//------------------------------------------------------------------------------------------------
	//! Validates the selected point and snapshot coverage, then refreshes only that point's aggregate envelope.
	//! Проверяет выбранную точку и покрытие snapshot, затем обновляет aggregate-envelope только этой точки.
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
		{
			Print("[ME_DEBUG_AVSP_WB] status=UNVERIFIABLE operation=ambient_vehicle_envelope_preview reason=world_editor_unavailable entity=<none>");
			return;
		}

		string reason;
		string entityName;
		int candidateCount;
		vector aggregateMins;
		vector aggregateMaxs;
		if (!ME_AmbientVehicleEnvelopePreviewHelper.ME_ShowSelectedEnvelope(worldEditor.GetApi(), reason, entityName, candidateCount, aggregateMins, aggregateMaxs))
		{
			PrintFormat("[ME_DEBUG_AVSP_WB] status=UNVERIFIABLE operation=ambient_vehicle_envelope_preview reason=%1 entity=%2", reason, entityName);
			return;
		}

		PrintFormat("[ME_DEBUG_AVSP_WB] status=PASS operation=ambient_vehicle_envelope_preview entity=%1 candidateCount=%2 localMins=%3 localMaxs=%4 edgeCount=12", entityName, candidateCount, aggregateMins, aggregateMaxs);
	}
}
