//! Test-only serializable per-prefab vehicle bounds used for game-update regression comparison.
//! Сериализуемые границы техники по каждому prefab только для Test, используемые для регрессионного сравнения после обновления игры.

//------------------------------------------------------------------------------------------------
//! One canonical catalog prefab with its measured local bounds and classification metadata.
//! Один канонический prefab каталога с измеренными локальными границами и classification metadata.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_VehicleBoundsPerPrefabSnapshotEntry
{
	//! Canonical vehicle catalog prefab resource path.
	//! Канонический путь ресурса prefab техники из каталога.
	[Attribute("")]
	string m_sPrefab;

	//! Orientation-aligned local minimum corner relative to the unrotated fixture root.
	//! Ориентированный по осям локальный минимальный угол относительно неповёрнутого корня fixture.
	[Attribute("0 0 0")]
	vector m_vLocalMins;

	//! Orientation-aligned local maximum corner relative to the unrotated fixture root.
	//! Ориентированный по осям локальный максимальный угол относительно неповёрнутого корня fixture.
	[Attribute("0 0 0")]
	vector m_vLocalMaxs;

	//! Sorted unique faction keys whose filtered catalogs contain this prefab.
	//! Отсортированные уникальные ключи фракций, отфильтрованные каталоги которых содержат этот prefab.
	[Attribute()]
	ref array<string> m_aFactionKeys;

	//! Sorted unique editable entity label names classifying this prefab as a vehicle type.
	//! Отсортированные уникальные имена меток editable entity, классифицирующих этот prefab как тип техники.
	[Attribute()]
	ref array<string> m_aVehicleTypes;
}

//------------------------------------------------------------------------------------------------
//! Root schema for deterministic per-prefab vehicle-bounds regression snapshots.
//! Корневая schema для детерминированных регрессионных snapshots границ техники по каждому prefab.
[BaseContainerProps(configRoot: true)]
class ME_VehicleBoundsPerPrefabSnapshot
{
	//! Schema compatibility version expected by the generator and comparator.
	//! Версия совместимости schema, ожидаемая генератором и comparator.
	[Attribute("1")]
	int m_iSchemaVersion;

	//! Generator implementation version that produced this payload.
	//! Версия реализации генератора, создавшего этот payload.
	[Attribute("")]
	string m_sGeneratorVersion;

	//! Stable identity of the dedicated fixture used for all measurements.
	//! Стабильная идентичность выделенного fixture, использованного для всех измерений.
	[Attribute("")]
	string m_sFixtureIdentity;

	//! Game build version reported while this snapshot was generated.
	//! Версия build игры, сообщённая во время генерации этого snapshot.
	[Attribute("")]
	string m_sGameVersion;

	//! Whether every entry contains complete faction and vehicle-type classification metadata.
	//! Содержит ли каждая запись полные classification metadata фракций и типов техники.
	[Attribute("0")]
	bool m_bClassificationMetadataAvailable;

	//! Entries sorted lexicographically by canonical prefab path.
	//! Записи, отсортированные лексикографически по каноническому пути prefab.
	[Attribute()]
	ref array<ref ME_VehicleBoundsPerPrefabSnapshotEntry> m_aEntries;
}
