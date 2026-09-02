//! Enables ambient vehicle spawning in CLI autotest fixture worlds after their GameMode initializes.
//! Включает появление ambient-техники в fixture-мирах CLI-автотестов после инициализации их GameMode.
[ComponentEditorProps(category: "Autotest")]
class ME_TEST_AmbientVehicleSpawnFlagsComponentClass : ScriptComponentClass
{
}

//! Enables ambient vehicle spawning in CLI autotest fixture worlds after their GameMode initializes.
//! Включает появление ambient-техники в fixture-мирах CLI-автотестов после инициализации их GameMode.
class ME_TEST_AmbientVehicleSpawnFlagsComponent : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	//! Sets the SpawnVehicles runtime game flag for the owning configured fixture GameMode.
	//! \param[in] owner Entity that owns this fixture component
	//! Устанавливает runtime-флаг игры SpawnVehicles для GameMode настроенного fixture-мира, которому принадлежит компонент.
	//! \param[in] owner Сущность, которой принадлежит этот fixture-компонент
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		GetGame().SetGameFlags(EGameFlags.SpawnVehicles, false);
	}
}
