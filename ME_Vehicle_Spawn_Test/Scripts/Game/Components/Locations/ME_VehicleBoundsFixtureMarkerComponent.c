//! Test-only marker that binds one placed fixture vehicle root to its canonical catalog prefab path.
//! Marker только для Test, связывающий один размещённый корень vehicle fixture с каноническим путём prefab из каталога.

//------------------------------------------------------------------------------------------------
//! Declares the catalog prefab represented by the owning fixture vehicle root.
//! Объявляет prefab из каталога, представляемый корнем vehicle fixture-владельцем.
class ME_VehicleBoundsFixtureMarkerComponentClass : ScriptComponentClass
{
}

class ME_VehicleBoundsFixtureMarkerComponent : ScriptComponent
{
	//! Canonical ResourceName returned by SCR_EntityCatalogEntry.GetPrefab().
	//! Fixture validation requires it to identify the owning fixture vehicle root.
	//! Канонический ResourceName, возвращаемый SCR_EntityCatalogEntry.GetPrefab().
	//! Проверка fixture требует, чтобы он идентифицировал корень vehicle fixture-владельца.
	[Attribute("")]
	string m_sCatalogPrefab;

	//------------------------------------------------------------------------------------------------
	//! Returns the declared canonical catalog prefab path.
	//! Возвращает объявленный канонический путь prefab из каталога.
	string GetCatalogPrefab()
	{
		return m_sCatalogPrefab;
	}
}
