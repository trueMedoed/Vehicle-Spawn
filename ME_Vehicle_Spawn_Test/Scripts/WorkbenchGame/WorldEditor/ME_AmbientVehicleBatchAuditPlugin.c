//! Runs the existing editor audit sequentially for a manifest of scenario worlds.
//! Последовательно запускает существующий аудит редактора для списка миров сценариев.
[WorkbenchPluginAttribute(name: "Batch audit ambient vehicle scenarios", description: "Loads worlds from $profile:ME_AmbientAuditWorlds.txt and writes a numbered summary report. Save your work before running.", wbModules: { "WorldEditor" }, category: "ME_Vehicle_Spawn/Diagnostics")]
class ME_AmbientVehicleBatchAuditPlugin : WorldEditorPlugin
{
	protected static bool s_Running;
	protected const string MANIFEST = "$profile:ME_AmbientAuditWorlds.txt";

	//! Reads a user-editable manifest; never saves worlds or bypasses native save/cancel dialogs.
	//! Читает редактируемый список; не сохраняет миры и не обходит штатные диалоги сохранения/отмены.
	override void Run()
	{
		if (s_Running || !SCR_Global.IsEditMode())
		{
			PrintFormat("[ME_DEBUG_AVSP_BATCH] status=ABORTED reason=busy_or_not_edit_mode", level: LogLevel.WARNING);
			return;
		}
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor || !editor.GetApi())
			return;
		ME_AmbientVehicleWorldAuditPlugin audit = ME_AmbientVehicleWorldAuditPlugin.Cast(editor.GetPlugin(ME_AmbientVehicleWorldAuditPlugin));
		if (!audit)
		{
			PrintFormat("[ME_DEBUG_AVSP_BATCH] status=ABORTED reason=audit_plugin_unavailable", level: LogLevel.ERROR);
			return;
		}
		if (!FileIO.FileExists(MANIFEST))
		{
			FileHandle template = FileIO.OpenFile(MANIFEST, FileMode.WRITE);
			if (!template)
				return;
			template.WriteLine("# One scenario world per line. Save your current world before running.");
			template.WriteLine("worlds/MP/CTI_Campaign_Cain.ent");
			template.WriteLine("worlds/MP/CTI_Campaign_Eden.ent");
			template.WriteLine("worlds/MP/CTI_Campaign_Arland.ent");
			template.WriteLine("worlds/MP/CTI_Campaign_HQC_Eden.ent");
			template.WriteLine("worlds/MP/CTI_Campaign_Montignac.ent");
			template.WriteLine("worlds/MP/CTI_Campaign_WesternEveron.ent");
			template.WriteLine("worlds/MP/CTI_Campaign_HQC_Arland.ent");
			template.Close();
			Workbench.Dialog("Ambient scenario batch audit", "Created $profile:ME_AmbientAuditWorlds.txt with seven campaign scenarios. Edit the list if needed, save your current work, then run this command again. Worlds will be loaded sequentially; the last world stays open.");
			return;
		}
		FileHandle input = FileIO.OpenFile(MANIFEST, FileMode.READ);
		if (!input)
			return;
		array<string> worlds = {};
		string line;
		bool invalid;
		while (input.ReadLine(line) >= 0)
		{
			line.TrimInPlace();
			if (line.IsEmpty() || line.StartsWith("#"))
				continue;
			line.Replace("\\", "/");
			if (!line.EndsWith(".ent"))
			{
				PrintFormat("[ME_DEBUG_AVSP_BATCH] status=ABORTED reason=invalid_manifest_entry world=%1", line, level: LogLevel.ERROR);
				invalid = true;
				break;
			}
			if (!worlds.Contains(line))
				worlds.Insert(line);
		}
		input.Close();
		if (invalid || worlds.IsEmpty())
			return;

		string reportPath;
		int reportIndex = 1;
		while (true)
		{
			reportPath = string.Format("$profile:ME_AmbientAuditBatch_%1.txt", reportIndex);
			if (!FileIO.FileExists(reportPath))
				break;
			reportIndex++;
		}
		FileHandle report = FileIO.OpenFile(reportPath, FileMode.WRITE);
		if (!report)
		{
			PrintFormat("[ME_DEBUG_AVSP_BATCH] status=ABORTED reason=report_open_failed", level: LogLevel.ERROR);
			return;
		}
		s_Running = true;
		int completed;
		bool stopped;
		WriteReport(report, string.Format("status=STARTED requested=%1 report=%2", worlds.Count(), reportPath));
		foreach (string world : worlds)
		{
			WriteReport(report, string.Format("status=WORLD_LOADING world=%1", world));
			if (!editor.SetOpenedResource(world))
			{
				WriteReport(report, string.Format("status=ABORTED reason=load_failed_or_cancelled world=%1", world));
				stopped = true;
				break;
			}
			WorldEditorAPI api = editor.GetApi();
			string loaded;
			if (api)
				api.GetWorldPath(loaded);
			if (!api || !SCR_Global.IsEditMode() || NormalizePath(loaded) != NormalizePath(world))
			{
				WriteReport(report, string.Format("status=ABORTED reason=loaded_world_not_confirmed requested=%1 actual=%2", world, loaded));
				stopped = true;
				break;
			}
			WriteReport(report, string.Format("status=WORLD_STARTED world=%1", world));
			audit.Run();
			report.WriteLine(string.Format("DETAILS world=%1", world));
			foreach (string detail : audit.m_aLastDetails)
				report.WriteLine(detail);
			if (!audit.m_bLastRunCompleted)
			{
				WriteReport(report, string.Format("status=ABORTED reason=audit_not_completed world=%1", world));
				stopped = true;
				break;
			}
			WriteReport(report, string.Format("status=WORLD_FINISHED world=%1 %2", world, audit.m_sLastSummary));
			WriteReport(report, string.Format("world=%1 %2", world, audit.m_sLastCoverage));
			completed++;
		}
		string status = "FINISHED";
		if (stopped)
			status = "ABORTED";
		WriteReport(report, string.Format("status=%1 completed=%2 requested=%3", status, completed, worlds.Count()));
		report.Close();
		s_Running = false;
		Workbench.Dialog("Ambient scenario batch audit", string.Format("%1: %2/%3 worlds. Report: %4. Per-point coordinates and diagnostics are included in the report and script.log.", status, completed, worlds.Count(), reportPath));
	}

	//! Removes resource GUID/addon prefixes for a strict relative world-path comparison.
	//! Убирает GUID и префикс аддона для строгого сравнения относительного пути мира.
	protected string NormalizePath(string path)
	{
		path.Replace("\\", "/");
		int endGuid = path.IndexOf("}");
		if (endGuid >= 0)
			path = path.Substring(endGuid + 1, path.Length() - endGuid - 1);
		int addonEnd = path.IndexOf(":");
		if (path.StartsWith("$") && addonEnd >= 0)
			path = path.Substring(addonEnd + 1, path.Length() - addonEnd - 1);
		path.ToLower();
		return path;
	}

	//! Mirrors each report row to the log so detailed point diagnostics can be attributed to a world.
	//! Дублирует строки отчёта в лог для привязки подробной диагностики точек к миру.
	protected void WriteReport(FileHandle report, string row)
	{
		report.WriteLine(row);
		PrintFormat("[ME_DEBUG_AVSP_BATCH] %1", row);
	}
}
