//! Editor-only entity catalog initialization used by read-only spawn-point diagnostics.

//------------------------------------------------------------------------------------------------
//! Exposes the configured factionless catalog manager only while the World Editor is active.
modded class SCR_EntityCatalogManagerComponent
{
	protected static SCR_EntityCatalogManagerComponent s_ME_EditorInstance;

	//------------------------------------------------------------------------------------------------
	//! Registers the configured manager for editor-only catalog diagnostics.
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (SCR_Global.IsEditMode())
			s_ME_EditorInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the vanilla runtime singleton or the configured World Editor manager.
	//!
	//! \return The active catalog manager, or null when none is available
	static SCR_EntityCatalogManagerComponent ME_GetEditorInstance()
	{
		SCR_EntityCatalogManagerComponent instance = GetInstance();
		if (instance || !SCR_Global.IsEditMode())
			return instance;

		return s_ME_EditorInstance;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the configured factionless VEHICLE catalog without initializing or combining faction catalogs.
	//!
	//! \param[out] reason Stable reason when the manager or catalog is unavailable
	//! \return The configured global VEHICLE catalog, or null when it cannot be read
	static SCR_EntityCatalog ME_GetEditorGlobalVehicleCatalog(out string reason)
	{
		reason = "";
		SCR_EntityCatalogManagerComponent manager = ME_GetEditorInstance();
		if (!manager)
		{
			reason = "global_catalog_manager_unavailable";
			return null;
		}

		SCR_EntityCatalog catalog = manager.GetEntityCatalogOfType(EEntityCatalogType.VEHICLE);
		if (!catalog)
			reason = "global_vehicle_catalog_unavailable";

		return catalog;
	}

	//------------------------------------------------------------------------------------------------
	//! Clears the editor-only manager reference when its entity is deleted.
	override void OnDelete(IEntity owner)
	{
		if (s_ME_EditorInstance == this)
			s_ME_EditorInstance = null;

		super.OnDelete(owner);
	}
}

//------------------------------------------------------------------------------------------------
//! Extends faction data with an explicit editor-only catalog initialization step.
modded class SCR_Faction
{
	//------------------------------------------------------------------------------------------------
	//! Initializes this faction's catalog map only while the World Editor is active.
	//! This changes no editable entities and does not create or probe a vehicle prefab.
	//!
	//! \return True when the catalog map is ready for read-only candidate filtering
	//!
	bool ME_EnsureEditorCatalogsInitialized()
	{
		if (m_bCatalogInitDone)
			return true;

		if (!SCR_Global.IsEditMode() || !m_aEntityCatalogs)
			return false;

		SCR_EntityCatalogManagerComponent.InitCatalogs(m_aEntityCatalogs, m_mEntityCatalogs);
		m_bCatalogInitDone = true;
		m_aEntityCatalogs = null;
		return true;
	}
}
