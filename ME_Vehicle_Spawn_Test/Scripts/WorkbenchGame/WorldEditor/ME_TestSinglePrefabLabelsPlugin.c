//! Test plugin to extract editable entity labels from a single vehicle prefab.
//! Тестовый плагин для извлечения editable entity labels из одного vehicle prefab.

[WorkbenchPluginAttribute(name: "Approach B prefab labels", description: "Tests label extraction from one vehicle prefab", category: "ME_Vehicle_Spawn/Diagnostics")]
class ME_TestSinglePrefabLabelsPlugin : WorkbenchPlugin
{
	//------------------------------------------------------------------------------------------------
	//! Tests label extraction on a single known vehicle prefab.
	//! Тестирует извлечение labels на одном известном vehicle prefab.
	override void Run()
	{
		// Test with US M923A1 command truck - a simple wheeled vehicle
		// Тестируем на US M923A1 command truck - простая колёсная техника
		string testPrefab = "{36BDCC88B17B3BFA}Prefabs/Vehicles/Wheeled/M923A1/M923A1_command.et";

		PrintFormat("[ME_TEST_LABELS] Loading prefab: %1", testPrefab);

		Resource resource = Resource.Load(testPrefab);
		if (!resource || !resource.IsValid())
		{
			Print("[ME_TEST_LABELS] FAIL: Resource.Load failed");
			return;
		}

		Print("[ME_TEST_LABELS] Resource loaded successfully");

		BaseResourceObject baseResObj = resource.GetResource();
		if (!baseResObj)
		{
			Print("[ME_TEST_LABELS] FAIL: GetResource returned null");
			return;
		}

		Print("[ME_TEST_LABELS] BaseResourceObject obtained");

		// Try to get the prefab's BaseContainer
		// Попробуем получить BaseContainer prefab
		BaseContainer container = baseResObj.ToBaseContainer();
		if (!container)
		{
			Print("[ME_TEST_LABELS] FAIL: ToBaseContainer returned null");
			return;
		}

		Print("[ME_TEST_LABELS] BaseContainer obtained");

		// Get components array from the prefab
		// Получим массив компонентов из prefab
		BaseContainerList components = container.GetObjectArray("components");
		if (!components)
		{
			Print("[ME_TEST_LABELS] FAIL: GetObjectArray('components') returned null");
			return;
		}

		int componentCount = components.Count();
		PrintFormat("[ME_TEST_LABELS] Prefab has %1 components", componentCount);

		// Find SCR_EditableVehicleComponent
		// Найдём SCR_EditableVehicleComponent
		for (int i = 0; i < componentCount; i++)
		{
			BaseContainer component = components.Get(i);
			if (!component)
				continue;

			string componentClassName = component.GetClassName();
			PrintFormat("[ME_TEST_LABELS] Component %1: %2", i, componentClassName);

			if (componentClassName == "SCR_EditableVehicleComponent")
			{
				Print("[ME_TEST_LABELS] Found SCR_EditableVehicleComponent!");

				// Try to get m_UIInfo
				// Попробуем получить m_UIInfo
				BaseContainer uiInfo = component.GetObject("m_UIInfo");
				if (uiInfo)
				{
					string uiInfoClass = uiInfo.GetClassName();
					PrintFormat("[ME_TEST_LABELS] m_UIInfo class: %1", uiInfoClass);

					// Try to read m_aAuthoredLabels array
					// Попробуем прочитать массив m_aAuthoredLabels
					array<int> authoredLabels = {};
					if (uiInfo.Get("m_aAuthoredLabels", authoredLabels))
					{
						PrintFormat("[ME_TEST_LABELS] m_aAuthoredLabels count: %1", authoredLabels.Count());
						foreach (int labelValue : authoredLabels)
						{
							string labelName = typename.EnumToString(EEditableEntityLabel, labelValue);
							PrintFormat("[ME_TEST_LABELS]   Authored label: %1 (%2)", labelName, labelValue);
						}
					}
					else
					{
						Print("[ME_TEST_LABELS] Could not read m_aAuthoredLabels");
					}

					// Try to read m_aAutoLabels array
					// Попробуем прочитать массив m_aAutoLabels
					array<int> autoLabels = {};
					if (uiInfo.Get("m_aAutoLabels", autoLabels))
					{
						PrintFormat("[ME_TEST_LABELS] m_aAutoLabels count: %1", autoLabels.Count());
						foreach (int labelValue : autoLabels)
						{
							string labelName = typename.EnumToString(EEditableEntityLabel, labelValue);
							PrintFormat("[ME_TEST_LABELS]   Auto label: %1 (%2)", labelName, labelValue);
						}
					}
					else
					{
						Print("[ME_TEST_LABELS] Could not read m_aAutoLabels");
					}
				}
				else
				{
					Print("[ME_TEST_LABELS] Could not get m_UIInfo");
				}

				break;
			}
		}

		Print("[ME_TEST_LABELS] Test complete");
	}
}
