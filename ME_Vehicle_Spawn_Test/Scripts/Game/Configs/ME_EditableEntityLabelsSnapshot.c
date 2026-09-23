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

}

//------------------------------------------------------------------------------------------------
//! One label group containing all prefabs having that label within a scope.
//! Одна группа label, содержащая все prefabs с этой меткой внутри scope.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_EditableEntityLabelsSnapshotLabel
{
	//! EEditableEntityLabel enum name, or __NO_LABELS__ for catalog entries without labels.
	//! Имя enum-значения EEditableEntityLabel либо __NO_LABELS__ для записей без меток.
	[Attribute("")]
	string m_sLabelName;

	//! Prefab records sorted lexicographically by prefab path.
	//! Записи prefab, отсортированные лексикографически по prefab path.
	[Attribute()]
	ref array<ref ME_EditableEntityLabelsSnapshotPrefab> m_aPrefabs;
}

//------------------------------------------------------------------------------------------------
//! One scope group containing label groups for one faction catalog.
//! Одна группа scope, содержащая группы label для каталога одной фракции.
[BaseContainerProps(namingConvention: NamingConvention.NC_MUST_HAVE_NAME)]
class ME_EditableEntityLabelsSnapshotScope
{
	//! Scope identifier: faction key.
	//! Идентификатор scope: ключ фракции.
	[Attribute("")]
	string m_sScopeKey;

	//! Label groups sorted lexicographically by label name.
	//! Группы label, отсортированные лексикографически по label name.
	[Attribute()]
	ref array<ref ME_EditableEntityLabelsSnapshotLabel> m_aLabels;
}

//------------------------------------------------------------------------------------------------
//! Root schema for deterministic editable-entity-labels snapshot across faction vehicle catalogs.
//! Корневая schema для детерминированного snapshot editable-entity-labels по каталогам фракций.
[BaseContainerProps(configRoot: true)]
class ME_EditableEntityLabelsSnapshot
{
	//! Schema compatibility version expected by the reader.
	//! Версия совместимости schema, ожидаемая reader.
	[Attribute("2")]
	int m_iSchemaVersion;

	//! Generator implementation version that produced this snapshot payload.
	//! Версия реализации генератора, создавшего этот snapshot payload.
	[Attribute("")]
	string m_sGeneratorVersion;

	//! Game version string at snapshot generation time.
	//! Строка версии игры на момент генерации snapshot.
	[Attribute("")]
	string m_sGameVersion;

	//! Scope groups sorted lexicographically by scope key.
	//! Группы scope, отсортированные лексикографически по scope key.
	[Attribute()]
	ref array<ref ME_EditableEntityLabelsSnapshotScope> m_aScopes;
}
