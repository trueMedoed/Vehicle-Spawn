//! Serializable, versioned vehicle-bounds aggregates used by the editor-only ambient vehicle envelope preview.
//! Сериализуемые версионированные агрегаты границ техники, используемые предпросмотром envelope ambient-техники только в редакторе.

//------------------------------------------------------------------------------------------------
//! One conservative local box for all catalog vehicles of one faction and vehicle type.
//! Один консервативный локальный box для всей техники каталога одной фракции и типа техники.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_VehicleBoundsSnapshotEntry
{
	//! Editable entity label name identifying this vehicle type.
	//! Имя метки editable entity, определяющее этот тип техники.
	[Attribute("")]
	string m_sVehicleType;

	//! Conservative orientation-aligned local minimum corner.
	//! Консервативный ориентированный по осям локальный минимальный угол.
	[Attribute("0 0 0")]
	vector m_vLocalMins;

	//! Conservative orientation-aligned local maximum corner.
	//! Консервативный ориентированный по осям локальный максимальный угол.
	[Attribute("0 0 0")]
	vector m_vLocalMaxs;

	//! Number of unique catalog prefab candidates represented by this aggregate.
	//! Количество уникальных prefab-кандидатов каталога, представленных этим агрегатом.
	[Attribute("0")]
	int m_iCandidateCount;

	//! Canonical prefab producing the aggregate minimum X coordinate.
	//! Канонический prefab, давший минимальную координату X агрегата.
	[Attribute("")]
	string m_sMinXSourcePrefab;

	//! Canonical prefab producing the aggregate maximum X coordinate.
	//! Канонический prefab, давший максимальную координату X агрегата.
	[Attribute("")]
	string m_sMaxXSourcePrefab;

	//! Canonical prefab producing the aggregate minimum Y coordinate.
	//! Канонический prefab, давший минимальную координату Y агрегата.
	[Attribute("")]
	string m_sMinYSourcePrefab;

	//! Canonical prefab producing the aggregate maximum Y coordinate.
	//! Канонический prefab, давший максимальную координату Y агрегата.
	[Attribute("")]
	string m_sMaxYSourcePrefab;

	//! Canonical prefab producing the aggregate minimum Z coordinate.
	//! Канонический prefab, давший минимальную координату Z агрегата.
	[Attribute("")]
	string m_sMinZSourcePrefab;

	//! Canonical prefab producing the aggregate maximum Z coordinate.
	//! Канонический prefab, давший максимальную координату Z агрегата.
	[Attribute("")]
	string m_sMaxZSourcePrefab;
}

//------------------------------------------------------------------------------------------------
//! One faction group containing vehicle-type aggregates for its catalog.
//! Одна группа фракции, содержащая агрегаты типов техники для её каталога.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_VehicleBoundsSnapshotFaction
{
	//! Faction key owning every aggregate in this group.
	//! Ключ фракции, владеющей каждым агрегатом в этой группе.
	[Attribute("")]
	string m_sFactionKey;

	//! Aggregate entries sorted lexicographically by vehicle type.
	//! Aggregate-записи, отсортированные лексикографически по типу техники.
	[Attribute()]
	ref array<ref ME_VehicleBoundsSnapshotEntry> m_aEntries;
}

//------------------------------------------------------------------------------------------------
//! Root schema for deterministic faction and vehicle-type bounds aggregates.
//! Корневая schema для детерминированных агрегатов границ по фракции и типу техники.
[BaseContainerProps(configRoot: true)]
class ME_VehicleBoundsSnapshot
{
	//! Schema compatibility version expected by the reader.
	//! Версия совместимости schema, ожидаемая reader.
	[Attribute("5")]
	int m_iSchemaVersion;

	//! Generator implementation version that produced this aggregate payload.
	//! Версия реализации генератора, создавшего этот aggregate payload.
	[Attribute("")]
	string m_sGeneratorVersion;

	//! Faction groups sorted lexicographically by faction key.
	//! Группы фракций, отсортированные лексикографически по ключу фракции.
	[Attribute()]
	ref array<ref ME_VehicleBoundsSnapshotFaction> m_aFactions;
}
