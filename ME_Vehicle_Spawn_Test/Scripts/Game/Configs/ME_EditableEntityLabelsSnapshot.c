//! Serializable, versioned editable-entity-labels snapshot used by the editor-only ambient vehicle spawn-point diagnostics.
//! Сериализуемый версионированный snapshot editable-entity-labels, используемый диагностикой точек спавна ambient-техники только в редакторе.

//------------------------------------------------------------------------------------------------
//! One prefab record within a label group.
//! Одна запись prefab внутри группы label.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_EditableEntityLabelsSnapshotPrefab
{
	//! Prefab resource path with GUID.
	//! Путь prefab-ресурса с GUID.
	[Attribute("")]
	string m_sPrefabPath;

	//! Catalog entry index for this prefab within its scope.
	//! Индекс catalog entry для этого prefab внутри его scope.
	[Attribute("0")]
	int m_iCatalogIndex;

	//! Catalog entry name for this prefab.
	//! Имя catalog entry для этого prefab.
	[Attribute("")]
	string m_sEntityName;
}

//------------------------------------------------------------------------------------------------
//! One label group containing all prefabs having that label within a scope.
//! Одна группа label, содержащая все prefabs с этой меткой внутри scope.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_EditableEntityLabelsSnapshotLabel
{
	//! EEditableEntityLabel enum value name.
	//! Имя enum-значения EEditableEntityLabel.
	[Attribute("")]
	string m_sLabelName;

	//! Prefab records sorted lexicographically by prefab path.
	//! Записи prefab, отсортированные лексикографически по prefab path.
	[Attribute()]
	ref array<ref ME_EditableEntityLabelsSnapshotPrefab> m_aPrefabs;
}

//------------------------------------------------------------------------------------------------
//! One scope group containing label groups for the global or one faction catalog.
//! Одна группа scope, содержащая группы label для глобального каталога или каталога одной фракции.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_EditableEntityLabelsSnapshotScope
{
	//! Scope identifier: "__ME_GLOBAL_VEHICLE_CATALOG__" or faction key.
	//! Идентификатор scope: "__ME_GLOBAL_VEHICLE_CATALOG__" или ключ фракции.
	[Attribute("")]
	string m_sScopeKey;

	//! Label groups sorted lexicographically by label name.
	//! Группы label, отсортированные лексикографически по label name.
	[Attribute()]
	ref array<ref ME_EditableEntityLabelsSnapshotLabel> m_aLabels;
}

//------------------------------------------------------------------------------------------------
//! Root schema for deterministic editable-entity-labels snapshot across global and faction vehicle catalogs.
//! Корневая schema для детерминированного snapshot editable-entity-labels по глобальному каталогу и каталогам фракций.
[BaseContainerProps(configRoot: true)]
class ME_EditableEntityLabelsSnapshot
{
	//! Schema compatibility version expected by the reader.
	//! Версия совместимости schema, ожидаемая reader.
	[Attribute("1")]
	int m_iSchemaVersion;

	//! Generator implementation version that produced this snapshot payload.
	//! Версия реализации генератора, создавшего этот snapshot payload.
	[Attribute("")]
	string m_sGeneratorVersion;

	//! Game version string at snapshot generation time.
	//! Строка версии игры на момент генерации snapshot.
	[Attribute("")]
	string m_sGameVersion;

	//! Scope groups sorted lexicographically by scope key (global first, then faction keys).
	//! Группы scope, отсортированные лексикографически по scope key (сначала global, затем ключи фракций).
	[Attribute()]
	ref array<ref ME_EditableEntityLabelsSnapshotScope> m_aScopes;
}
