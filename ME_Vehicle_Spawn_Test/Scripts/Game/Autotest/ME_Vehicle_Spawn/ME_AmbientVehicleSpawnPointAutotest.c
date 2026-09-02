//! Runtime regression tests for the ambient vehicle spawn-point prerequisite worlds.
//! These tests observe loaded game entities and game flags; they do not test World Editor plugin APIs or placement.
//! Runtime-регрессионные тесты для миров с предпосылками точек появления ambient-техники.
//! Эти тесты наблюдают загруженные игровые сущности и игровые флаги; они не тестируют API плагина World Editor или размещение.

//------------------------------------------------------------------------------------------------
//! Collects observable runtime state used by the ambient spawn-point World Editor prerequisite checks.
//! Собирает наблюдаемое runtime-состояние, используемое проверками предпосылок World Editor для точек появления ambient-техники.
class ME_TEST_AmbientVehicleSpawnPointWorldCase : SCR_AutotestCaseBase
{
	protected int m_iGameModeCount;
	protected int m_iFactionManagerCount;
	protected int m_iSpawnPointCount;
	protected int m_iMainStepCount;
	protected bool m_bSpawnVehiclesEnabled;

	//------------------------------------------------------------------------------------------------
	//! Resets the recorded world state before the scenario has finished loading.
	//! Сбрасывает сохранённое состояние мира до завершения загрузки сценария.
	[TestStep(TestStage.Setup)]
	void Setup_ResetWorldState()
	{
		m_iGameModeCount = 0;
		m_iFactionManagerCount = 0;
		m_iSpawnPointCount = 0;
		m_iMainStepCount = 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Waits one main step, then records game-manager and ambient-system state.
	//! \return True when the runtime state has been collected
	//! Ожидает один main-шаг, затем сохраняет состояние игровых managers и ambient-системы.
	//! \return True, когда runtime-состояние собрано
	protected bool CollectLoadedWorldState()
	{
		if (m_iMainStepCount++ == 0)
			return false;

		m_iGameModeCount = 0;
		if (SCR_BaseGameMode.Cast(GetGame().GetGameMode()))
			m_iGameModeCount = 1;

		m_iFactionManagerCount = 0;
		if (FactionManager.Cast(GetGame().GetFactionManager()))
			m_iFactionManagerCount = 1;
		m_iSpawnPointCount = 0;

		SCR_AmbientVehicleSystem ambientVehicleSystem = SCR_AmbientVehicleSystem.GetInstance();
		if (ambientVehicleSystem)
		{
			ref array<SCR_AmbientVehicleSpawnPointComponent> spawnPoints = {};
			m_iSpawnPointCount = ambientVehicleSystem.GetSpawnpoints(spawnPoints);
		}

		m_bSpawnVehiclesEnabled = GetGame().AreGameFlagsSet(EGameFlags.SpawnVehicles);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Verifies the runtime state expected for the suite's test world.
	//! Проверяет runtime-состояние, ожидаемое для тестового мира suite.
	protected void AssertWorldState(int expectedGameModes, int expectedFactionManagers, int expectedSpawnPoints, bool expectedSpawnVehicles)
	{
		AssertTrue(m_iGameModeCount == expectedGameModes, "Unexpected SCR_BaseGameMode count: " + m_iGameModeCount.ToString());
		AssertTrue(m_iFactionManagerCount == expectedFactionManagers, "Unexpected FactionManager count: " + m_iFactionManagerCount.ToString());
		AssertTrue(m_iSpawnPointCount == expectedSpawnPoints, "Unexpected SCR_AmbientVehicleSpawnPointComponent count: " + m_iSpawnPointCount.ToString());
		AssertTrue(m_bSpawnVehiclesEnabled == expectedSpawnVehicles, "Unexpected SpawnVehicles runtime flag: " + m_bSpawnVehiclesEnabled.ToString());
	}
}

//------------------------------------------------------------------------------------------------
//! Runs ambient spawn-point regression tests in the fully configured test world.
//! Запускает регрессионные тесты точек появления ambient-техники в полностью настроенном тестовом мире.
[BaseContainerProps(category: "Autotest")]
class ME_TEST_AmbientVehicleSpawnPointConfiguredSuite : SCR_AutotestSuiteBase
{
	//------------------------------------------------------------------------------------------------
	//! Returns the configured test world resource.
	//! \\return Configured test world resource path
	//! Возвращает ресурс настроенного тестового мира.
	//! \\return Путь ресурса настроенного тестового мира
	override ResourceName GetWorldFile()
	{
		return "{2C429031B207EA90}worlds/CliAutotest/ME_CLI_AmbientVehicle_Configured.ent";
	}
}

//------------------------------------------------------------------------------------------------
//! Confirms that the configured test world exposes its expected runtime prerequisites.
//! Подтверждает, что настроенный тестовый мир предоставляет ожидаемые runtime-предпосылки.
[Test(suite: ME_TEST_AmbientVehicleSpawnPointConfiguredSuite, timeoutS: 10)]
class ME_TEST_AmbientVehicleSpawnPointConfiguredWorld : ME_TEST_AmbientVehicleSpawnPointWorldCase
{
	//------------------------------------------------------------------------------------------------
	//! Asserts the configured world's runtime entity and game-flag state.
	//! Проверяет runtime-состояние сущностей и игрового флага настроенного мира.
	[TestStep(TestStage.Main)]
	bool Main_AssertConfiguredWorld()
	{
		if (!CollectLoadedWorldState())
			return false;

		AssertWorldState(1, 1, 1, true);
		return true;
	}
}

//------------------------------------------------------------------------------------------------
//! Runs ambient spawn-point regression tests in the SpawnVehicles-disabled world.
//! Запускает регрессионные тесты точек появления ambient-техники в мире с отключённым SpawnVehicles.
[BaseContainerProps(category: "Autotest")]
class ME_TEST_AmbientVehicleSpawnPointSpawnVehiclesDisabledSuite : SCR_AutotestSuiteBase
{
	//------------------------------------------------------------------------------------------------
	//! Returns the SpawnVehicles-disabled test world resource.
	//! \\return SpawnVehicles-disabled test world resource path
	//! Возвращает ресурс тестового мира с отключённым SpawnVehicles.
	//! \\return Путь ресурса тестового мира с отключённым SpawnVehicles
	override ResourceName GetWorldFile()
	{
		return "{7E8CAAE9EF7FCE59}worlds/CliAutotest/ME_CLI_AmbientVehicle_NoSpawnVehicles.ent";
	}
}

//------------------------------------------------------------------------------------------------
//! Confirms that the prepared world has managers but disables SpawnVehicles at runtime.
//! Подтверждает, что подготовленный мир содержит managers, но отключает SpawnVehicles в runtime.
[Test(suite: ME_TEST_AmbientVehicleSpawnPointSpawnVehiclesDisabledSuite, timeoutS: 10)]
class ME_TEST_AmbientVehicleSpawnPointSpawnVehiclesDisabledWorld : ME_TEST_AmbientVehicleSpawnPointWorldCase
{
	//------------------------------------------------------------------------------------------------
	//! Asserts the SpawnVehicles-disabled world's runtime entity and game-flag state.
	//! Проверяет runtime-состояние сущностей и игрового флага мира с отключённым SpawnVehicles.
	[TestStep(TestStage.Main)]
	bool Main_AssertSpawnVehiclesDisabledWorld()
	{
		if (!CollectLoadedWorldState())
			return false;

		AssertWorldState(1, 1, 0, false);
		return true;
	}
}

//------------------------------------------------------------------------------------------------
//! Runs ambient spawn-point regression tests in the world without a GameMode.
//! Запускает регрессионные тесты точек появления ambient-техники в мире без GameMode.
[BaseContainerProps(category: "Autotest")]
class ME_TEST_AmbientVehicleSpawnPointMissingGameModeSuite : SCR_AutotestSuiteBase
{
	//------------------------------------------------------------------------------------------------
	//! Returns the missing-GameMode test world resource.
	//! \\return Missing-GameMode test world resource path
	//! Возвращает ресурс тестового мира без GameMode.
	//! \\return Путь ресурса тестового мира без GameMode
	override ResourceName GetWorldFile()
	{
		return "{3FA6B77C898290B4}worlds/CliAutotest/ME_CLI_AmbientVehicle_NoGameMode.ent";
	}
}

//------------------------------------------------------------------------------------------------
//! Confirms that the prepared world has no GameMode runtime entity.
//! Подтверждает, что подготовленный мир не содержит runtime-сущности GameMode.
[Test(suite: ME_TEST_AmbientVehicleSpawnPointMissingGameModeSuite, timeoutS: 10)]
class ME_TEST_AmbientVehicleSpawnPointMissingGameModeWorld : ME_TEST_AmbientVehicleSpawnPointWorldCase
{
	//------------------------------------------------------------------------------------------------
	//! Asserts the missing-GameMode world's runtime entity and game-flag state.
	//! Проверяет runtime-состояние сущностей и игрового флага мира без GameMode.
	[TestStep(TestStage.Main)]
	bool Main_AssertMissingGameModeWorld()
	{
		if (!CollectLoadedWorldState())
			return false;

		AssertWorldState(0, 0, 0, false);
		return true;
	}
}

//------------------------------------------------------------------------------------------------
//! Runs ambient spawn-point regression tests in the world without a FactionManager.
//! Запускает регрессионные тесты точек появления ambient-техники в мире без FactionManager.
[BaseContainerProps(category: "Autotest")]
class ME_TEST_AmbientVehicleSpawnPointMissingFactionManagerSuite : SCR_AutotestSuiteBase
{
	//------------------------------------------------------------------------------------------------
	//! Returns the missing-FactionManager test world resource.
	//! \\return Missing-FactionManager test world resource path
	//! Возвращает ресурс тестового мира без FactionManager.
	//! \\return Путь ресурса тестового мира без FactionManager
	override ResourceName GetWorldFile()
	{
		return "{EE260C9E10E891CA}worlds/CliAutotest/ME_CLI_AmbientVehicle_NoFactionManager.ent";
	}
}

//------------------------------------------------------------------------------------------------
//! Confirms that the prepared world contains a point but no FactionManager runtime entity.
//! Подтверждает, что подготовленный мир содержит точку, но не содержит runtime-сущности FactionManager.
[Test(suite: ME_TEST_AmbientVehicleSpawnPointMissingFactionManagerSuite, timeoutS: 10)]
class ME_TEST_AmbientVehicleSpawnPointMissingFactionManagerWorld : ME_TEST_AmbientVehicleSpawnPointWorldCase
{
	//------------------------------------------------------------------------------------------------
	//! Asserts the missing-FactionManager world's runtime entity and game-flag state.
	//! Проверяет runtime-состояние сущностей и игрового флага мира без FactionManager.
	[TestStep(TestStage.Main)]
	bool Main_AssertMissingFactionManagerWorld()
	{
		if (!CollectLoadedWorldState())
			return false;

		AssertWorldState(1, 0, 1, true);
		return true;
	}
}

//------------------------------------------------------------------------------------------------
//! Runs ambient spawn-point regression tests in the incompatible-FactionManager world.
//! Запускает регрессионные тесты точек появления ambient-техники в мире с несовместимым FactionManager.
[BaseContainerProps(category: "Autotest")]
class ME_TEST_AmbientVehicleSpawnPointIncompatibleFactionManagerSuite : SCR_AutotestSuiteBase
{
	//------------------------------------------------------------------------------------------------
	//! Returns the incompatible-FactionManager test world resource.
	//! \\return Incompatible-FactionManager test world resource path
	//! Возвращает ресурс тестового мира с несовместимым FactionManager.
	//! \\return Путь ресурса тестового мира с несовместимым FactionManager
	override ResourceName GetWorldFile()
	{
		return "{AB04B4D6968D78FD}worlds/CliAutotest/ME_CLI_AmbientVehicle_IncompatibleFactionManager.ent";
	}
}

//------------------------------------------------------------------------------------------------
//! Confirms that an incompatible faction does not prevent spawn-point registration.
//! Подтверждает, что несовместимая faction не предотвращает регистрацию точки появления.
[Test(suite: ME_TEST_AmbientVehicleSpawnPointIncompatibleFactionManagerSuite, timeoutS: 10)]
class ME_TEST_AmbientVehicleSpawnPointIncompatibleFactionManagerWorld : ME_TEST_AmbientVehicleSpawnPointWorldCase
{
	//------------------------------------------------------------------------------------------------
	//! Asserts the incompatible-FactionManager world's runtime entity and game-flag state.
	//! Проверяет runtime-состояние сущностей и игрового флага мира с несовместимым FactionManager.
	[TestStep(TestStage.Main)]
	bool Main_AssertIncompatibleFactionManagerWorld()
	{
		if (!CollectLoadedWorldState())
			return false;

		AssertWorldState(1, 1, 1, true);
		return true;
	}
}
