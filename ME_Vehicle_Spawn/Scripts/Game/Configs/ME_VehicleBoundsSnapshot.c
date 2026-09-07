//! Serializable, versioned vehicle-bounds aggregates used by the editor-only ambient vehicle envelope preview.

//------------------------------------------------------------------------------------------------
//! One conservative local box for all catalog vehicles of one faction and vehicle type.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_VehicleBoundsSnapshotEntry
{
	//! Faction key owning this catalog aggregate.
	[Attribute("")]
	string m_sFactionKey;

	//! Editable entity label name identifying this vehicle type.
	[Attribute("")]
	string m_sVehicleType;

	//! Conservative orientation-aligned local minimum corner.
	[Attribute("0 0 0")]
	vector m_vLocalMins;

	//! Conservative orientation-aligned local maximum corner.
	[Attribute("0 0 0")]
	vector m_vLocalMaxs;

	//! Number of unique catalog prefab candidates represented by this aggregate.
	[Attribute("0")]
	int m_iCandidateCount;

	//! Canonical prefab producing the aggregate minimum X coordinate.
	[Attribute("")]
	string m_sMinXSourcePrefab;

	//! Canonical prefab producing the aggregate maximum X coordinate.
	[Attribute("")]
	string m_sMaxXSourcePrefab;

	//! Canonical prefab producing the aggregate minimum Y coordinate.
	[Attribute("")]
	string m_sMinYSourcePrefab;

	//! Canonical prefab producing the aggregate maximum Y coordinate.
	[Attribute("")]
	string m_sMaxYSourcePrefab;

	//! Canonical prefab producing the aggregate minimum Z coordinate.
	[Attribute("")]
	string m_sMinZSourcePrefab;

	//! Canonical prefab producing the aggregate maximum Z coordinate.
	[Attribute("")]
	string m_sMaxZSourcePrefab;
}

//------------------------------------------------------------------------------------------------
//! Root schema for deterministic faction and vehicle-type bounds aggregates.
[BaseContainerProps(configRoot: true)]
class ME_VehicleBoundsSnapshot
{
	//! Schema compatibility version expected by the reader.
	[Attribute("4")]
	int m_iSchemaVersion;

	//! Generator implementation version that produced this aggregate payload.
	[Attribute("")]
	string m_sGeneratorVersion;

	//! Entries sorted lexicographically by faction key and vehicle type.
	[Attribute()]
	ref array<ref ME_VehicleBoundsSnapshotEntry> m_aEntries;
}
