//! Serializable, versioned vehicle-bounds snapshot used by the editor-only ambient vehicle envelope preview.
//! Сериализуемый версионированный snapshot границ техники, используемый предпросмотром envelope ambient-техники только в редакторе.

//------------------------------------------------------------------------------------------------
//! One conservative orientation-aligned local box for a catalog vehicle prefab.
//! Один консервативный ориентированный по осям локальный box для prefab техники из каталога.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_VehicleBoundsSnapshotEntry
{
	//! Canonical vehicle catalog prefab resource path.
	//! Канонический путь ресурса prefab техники из каталога.
	[Attribute("")]
	string m_sPrefab;

	//! Conservative orientation-aligned local minimum corner relative to the unrotated fixture root.
	//! Консервативный ориентированный по осям минимальный локальный угол относительно неповёрнутого корня fixture.
	[Attribute("0 0 0")]
	vector m_vLocalMins;

	//! Conservative orientation-aligned local maximum corner relative to the unrotated fixture root.
	//! Консервативный ориентированный по осям максимальный локальный угол относительно неповёрнутого корня fixture.
	[Attribute("0 0 0")]
	vector m_vLocalMaxs;

	//! Stable fixture entity name used to produce this entry.
	//! Стабильное имя fixture-сущности, использованной для создания этой записи.
	[Attribute("")]
	string m_sFixtureEntityName;
}

//------------------------------------------------------------------------------------------------
//! Root schema for a deterministic vehicle-bounds snapshot resource.
//! Корневая schema для детерминированного ресурса snapshot границ техники.
[BaseContainerProps(configRoot: true)]
class ME_VehicleBoundsSnapshot
{
	//! Schema compatibility version expected by the reader.
	//! Версия совместимости schema, ожидаемая reader.
	[Attribute("2")]
	int m_iSchemaVersion;

	//! Generator implementation version that produced this payload.
	//! Версия реализации генератора, создавшего этот payload.
	[Attribute("")]
	string m_sGeneratorVersion;

	//! Stable identity of the dedicated fixture world.
	//! Стабильный идентификатор выделенного fixture-мира.
	[Attribute("")]
	string m_sFixtureIdentity;

	//! Entries sorted lexicographically by canonical prefab path.
	//! Записи, отсортированные лексикографически по каноническому пути prefab.
	[Attribute()]
	ref array<ref ME_VehicleBoundsSnapshotEntry> m_aEntries;
}
