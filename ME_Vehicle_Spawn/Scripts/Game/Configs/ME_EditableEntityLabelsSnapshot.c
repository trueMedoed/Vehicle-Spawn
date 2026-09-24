//! Serializable, versioned editable-entity-labels snapshot provided as a read-only reference for faction vehicle catalogs.

//------------------------------------------------------------------------------------------------
//! One prefab record within a label group.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_EditableEntityLabelsSnapshotPrefab
{
	//! Prefab resource path with GUID.
	[Attribute("")]
	string m_sPrefabPath;

}

//------------------------------------------------------------------------------------------------
//! One label group containing all prefabs having that label within a scope.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_EditableEntityLabelsSnapshotLabel
{
	//! EEditableEntityLabel enum name, or __NO_LABELS__ for catalog entries without labels.
	[Attribute("")]
	string m_sLabelName;

	//! Prefab records sorted lexicographically by prefab path.
	[Attribute()]
	ref array<ref ME_EditableEntityLabelsSnapshotPrefab> m_aPrefabs;
}

//------------------------------------------------------------------------------------------------
//! One scope group containing label groups for one faction catalog.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_EditableEntityLabelsSnapshotScope
{
	//! Scope identifier: faction key.
	[Attribute("")]
	string m_sScopeKey;

	//! Label groups sorted lexicographically by label name.
	[Attribute()]
	ref array<ref ME_EditableEntityLabelsSnapshotLabel> m_aLabels;
}

//------------------------------------------------------------------------------------------------
//! Root schema for deterministic editable-entity-labels snapshot across faction vehicle catalogs.
[BaseContainerProps(configRoot: true)]
class ME_EditableEntityLabelsSnapshot
{
	//! Schema compatibility version expected by the reader.
	[Attribute("2")]
	int m_iSchemaVersion;

	//! Generator implementation version that produced this snapshot payload.
	[Attribute("")]
	string m_sGeneratorVersion;

	//! Game version string at snapshot generation time.
	[Attribute("")]
	string m_sGameVersion;

	//! Scope groups sorted lexicographically by scope key.
	[Attribute()]
	ref array<ref ME_EditableEntityLabelsSnapshotScope> m_aScopes;
}
