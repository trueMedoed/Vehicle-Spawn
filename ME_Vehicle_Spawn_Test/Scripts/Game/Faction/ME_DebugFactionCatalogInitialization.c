//! Test-only editor catalog initialization for faction-owned entity catalogs.
//! Инициализация editor-каталогов фракции только для Test.

//------------------------------------------------------------------------------------------------
//! Extends faction data with an explicit editor-only catalog initialization step used by diagnostics.
//! Расширяет данные фракции явным шагом инициализации editor-каталогов для диагностики.
modded class SCR_Faction
{
	//------------------------------------------------------------------------------------------------
	//! Initializes this faction's catalog map only while the World Editor is active.
	//! This changes no editable entities and never creates or probes a vehicle prefab.
	//!
	//! \return True when the catalog map is ready for read-only candidate filtering
	//! Инициализирует map каталога этой фракции только в активном World Editor.
	//! Это не изменяет редактируемые сущности и никогда не создаёт либо не проверяет prefab техники.
	//!
	//! \return True, когда map каталога готов для фильтрации кандидатов только на чтение
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
