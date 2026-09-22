//! Test-only editor entity catalog initialization used by read-only spawn-point diagnostics.
//! Инициализация editor-каталогов сущностей только для Test, используемая диагностикой только на чтение.

//------------------------------------------------------------------------------------------------
//! Exposes the configured factionless catalog manager only while the World Editor is active.
//! Предоставляет настроенный factionless catalog manager только в активном World Editor.
modded class SCR_EntityCatalogManagerComponent
{
	protected static SCR_EntityCatalogManagerComponent s_ME_EditorInstance;

	//------------------------------------------------------------------------------------------------
	//! Registers the configured manager for editor-only catalog diagnostics.
	//! Регистрирует настроенный manager для editor-only диагностики каталогов.
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
	//! Возвращает ванильный runtime singleton либо настроенный manager World Editor.
	//!
	//! \return Активный catalog manager либо null, когда он недоступен
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
	//! Возвращает настроенный factionless-каталог VEHICLE без инициализации или объединения каталогов фракций.
	//!
	//! \param[out] reason Стабильная причина, когда manager или catalog недоступен
	//! \return Настроенный глобальный VEHICLE-каталог либо null, когда его нельзя прочитать
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
	//! Очищает editor-only ссылку на manager при удалении его сущности.
	override void OnDelete(IEntity owner)
	{
		if (s_ME_EditorInstance == this)
			s_ME_EditorInstance = null;

		super.OnDelete(owner);
	}
}

//------------------------------------------------------------------------------------------------
//! Extends faction data with an explicit editor-only catalog initialization step used by diagnostics.
//! Расширяет данные фракции явным шагом инициализации editor-каталогов для диагностики.
modded class SCR_Faction
{
	//------------------------------------------------------------------------------------------------
	//! Initializes this faction's catalog map only while the World Editor is active.
	//! This changes no editable entities and never creates or probes a vehicle prefab.
	//! In vanilla runtime Init() is called only when !IsEditMode(), so in the editor
	//! m_bCatalogInitDone remains false and m_aEntityCatalogs stays populated.
	//!
	//! \return True when the catalog map is ready for read-only candidate filtering
	//! Инициализирует map каталога этой фракции только в активном World Editor.
	//! Это не изменяет редактируемые сущности и никогда не создаёт либо не проверяет prefab техники.
	//! В vanilla runtime Init() вызывается только при !IsEditMode(), поэтому в редакторе
	//! m_bCatalogInitDone остаётся false и m_aEntityCatalogs остаётся заполненным.
	//!
	//! \return True, когда map каталога готов для фильтрации кандидатов только на чтение
	bool ME_EnsureEditorCatalogsInitialized()
	{
		// Already initialized at runtime or in a previous editor call
		// Уже инициализирован в runtime или в предыдущем editor-вызове
		if (m_bCatalogInitDone)
			return true;

		// Not in editor mode or config has no catalogs array
		// Не в режиме редактора или в конфиге нет массива каталогов
		if (!SCR_Global.IsEditMode() || !m_aEntityCatalogs)
			return false;

		// Initialize the catalog map using the same vanilla helper
		// Инициализировать map каталога с помощью того же vanilla-хелпера
		SCR_EntityCatalogManagerComponent.InitCatalogs(m_aEntityCatalogs, m_mEntityCatalogs);
		m_bCatalogInitDone = true;

		// Clear the array as vanilla runtime does
		// Очистить массив, как это делает vanilla runtime
		m_aEntityCatalogs = null;
		return true;
	}
}
