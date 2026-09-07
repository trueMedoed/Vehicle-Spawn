//! Test-only serializable per-prefab vehicle bounds used for game-update regression comparison.
//! Сериализуемые границы техники по каждому prefab только для Test, используемые для регрессионного сравнения после обновления игры.

//------------------------------------------------------------------------------------------------
//! One canonical catalog prefab with its measured local bounds and legacy flat classification metadata.
//! Один канонический prefab каталога с измеренными локальными границами и legacy classification metadata плоской модели.
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

	//! Faction key whose filtered catalog contains this prefab in the legacy flat representation.
	//! Ключ фракции, отфильтрованный каталог которой содержит этот prefab в legacy плоском представлении.
	[Attribute("")]
	string m_sFactionKey;

	//! Editable entity label name classifying this prefab in the legacy flat representation.
	//! Имя метки editable entity, классифицирующей этот prefab в legacy плоском представлении.
	[Attribute("")]
	string m_sVehicleType;
}

//------------------------------------------------------------------------------------------------
//! One grouped prefab with measured local bounds; faction and vehicle type come from its parent groups.
//! Один сгруппированный prefab с измеренными локальными границами; фракция и тип техники задаются родительскими группами.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_VehicleBoundsPerPrefabSnapshotGroupedEntry
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
}

//------------------------------------------------------------------------------------------------
//! Group of grouped prefab entries belonging to one vehicle type.
//! Группа сгруппированных prefab-записей, относящихся к одному типу техники.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_VehicleBoundsPerPrefabSnapshotVehicleTypeGroup
{
	//! Editable entity label name classifying all entries in this group.
	//! Имя метки editable entity, классифицирующей все записи этой группы.
	[Attribute("")]
	string m_sVehicleType;

	//! Grouped prefab entries sorted by canonical prefab path.
	//! Сгруппированные prefab-записи, отсортированные по каноническому пути prefab.
	[Attribute()]
	ref array<ref ME_VehicleBoundsPerPrefabSnapshotGroupedEntry> m_aEntries;
}

//------------------------------------------------------------------------------------------------
//! Group of vehicle-type groups belonging to one faction.
//! Группа типов техники, относящихся к одной фракции.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_VehicleBoundsPerPrefabSnapshotFactionGroup
{
	//! Faction key whose filtered catalog contains all entries in this group.
	//! Ключ фракции, отфильтрованный каталог которой содержит все записи этой группы.
	[Attribute("")]
	string m_sFactionKey;

	//! Vehicle-type groups sorted by vehicle type.
	//! Группы типов техники, отсортированные по типу техники.
	[Attribute()]
	ref array<ref ME_VehicleBoundsPerPrefabSnapshotVehicleTypeGroup> m_aVehicleTypeGroups;
}

//------------------------------------------------------------------------------------------------
//! Root schema for deterministic grouped Candidate and legacy flat Baseline vehicle-bounds snapshots.
//! Корневая schema для детерминированного сгруппированного Candidate и legacy плоского Baseline snapshot границ техники.
[BaseContainerProps(configRoot: true)]
class ME_VehicleBoundsPerPrefabSnapshot
{
	//! Schema compatibility version; version 2 is reserved for the legacy flat Baseline and version 3 for grouped Candidate resources.
	//! Версия совместимости schema; версия 2 предназначена для legacy плоского Baseline, а версия 3 — для сгруппированных Candidate-ресурсов.
	[Attribute("3")]
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

	//! Faction groups used by the grouped Candidate representation.
	//! Группы фракций, используемые сгруппированным представлением Candidate.
	[Attribute()]
	ref array<ref ME_VehicleBoundsPerPrefabSnapshotFactionGroup> m_aFactionGroups;

	//! Entries sorted by canonical prefab path in the legacy flat Baseline representation.
	//! Записи, отсортированные по каноническому пути prefab в legacy плоском представлении Baseline.
	[Attribute()]
	ref array<ref ME_VehicleBoundsPerPrefabSnapshotEntry> m_aEntries;
}
