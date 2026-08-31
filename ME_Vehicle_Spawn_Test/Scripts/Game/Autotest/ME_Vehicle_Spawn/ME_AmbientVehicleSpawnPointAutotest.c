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
	protected bool m_bSpawnVehiclesEnabled;

	//------------------------------------------------------------------------------------------------
	//! Counts relevant entities found by the world query.
	//! \\param[in] entity Entity returned by the world query
	//! \\return True to continue querying entities
	//! Подсчитывает релевантные сущности, найденные запросом мира.
	//! \\param[in] entity Сущность, возвращённая запросом мира
	//! \\return True, чтобы продолжить запрос сущностей
	protected bool CollectWorldEntity(IEntity entity)
	{
		if (SCR_BaseGameMode.Cast(entity))
			m_iGameModeCount++;

		if (FactionManager.Cast(entity))
			m_iFactionManagerCount++;

		if (SCR_AmbientVehicleSpawnPointComponent.Cast(entity.FindComponent(SCR_AmbientVehicleSpawnPointComponent)))
			m_iSpawnPointCount++;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Queries the loaded world and records entities and the public SpawnVehicles game flag.
	//! Запрашивает загруженный мир и сохраняет сущности и публичный игровой флаг SpawnVehicles.
	[TestStep(TestStage.Setup)]
	void Setup_CollectWorldState()
	{
		m_iGameModeCount = 0;
		m_iFactionManagerCount = 0;
		m_iSpawnPointCount = 0;

		BaseWorld world = GetGame().GetWorld();
		AssertTrue(world != null, "Autotest world is unavailable");
		if (!world)
			return;

		world.QueryEntitiesBySphere(vector.Zero, 1000000, CollectWorldEntity);
		m_bSpawnVehiclesEnabled = GetGame().AreGameFlagsSet(EGameFlags.SpawnVehicles);
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
		return "{B7E4D91C6A2F5083}worlds/TestCases/ME_AmbientVehicleSpawnPoint_Configured.ent";
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
	void Main_AssertConfiguredWorld()
	{
		AssertWorldState(1, 1, 1, true);
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
		return "{B7E4D91C6A2F5083}worlds/TestCases/ME_GameMode_Without_Spawn_Vehicles.ent";
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
	void Main_AssertSpawnVehiclesDisabledWorld()
	{
		AssertWorldState(1, 1, 0, false);
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
		return "{B7E4D91C6A2F5083}worlds/TestCases/ME_Missed_GameMode.ent";
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
	void Main_AssertMissingGameModeWorld()
	{
		AssertWorldState(0, 0, 0, false);
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
		return "{B7E4D91C6A2F5083}worlds/TestCases/ME_Missed_Faction_Manager.ent";
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
	void Main_AssertMissingFactionManagerWorld()
	{
		AssertWorldState(1, 0, 1, true);
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
		return "{B7E4D91C6A2F5083}worlds/TestCases/ME_Incompatible_Faction_Manager.ent";
	}
}

//------------------------------------------------------------------------------------------------
//! Confirms that the incompatible-manager world still exposes the observable runtime prerequisites.
//! Подтверждает, что мир с несовместимым manager всё ещё предоставляет наблюдаемые runtime-предпосылки.
[Test(suite: ME_TEST_AmbientVehicleSpawnPointIncompatibleFactionManagerSuite, timeoutS: 10)]
class ME_TEST_AmbientVehicleSpawnPointIncompatibleFactionManagerWorld : ME_TEST_AmbientVehicleSpawnPointWorldCase
{
	//------------------------------------------------------------------------------------------------
	//! Asserts the incompatible-FactionManager world's runtime entity and game-flag state.
	//! Проверяет runtime-состояние сущностей и игрового флага мира с несовместимым FactionManager.
	[TestStep(TestStage.Main)]
	void Main_AssertIncompatibleFactionManagerWorld()
	{
		AssertWorldState(1, 1, 0, true);
	}
}
