modded class SCR_EntityCatalog
{
	//override int GetFullFilteredEntityListWithLabels(notnull out array<SCR_EntityCatalogEntry> filteredEntityList, array<EEditableEntityLabel> includedLabels = null, array<EEditableEntityLabel> excludedLabels = null, bool needsAllIncluded = true)
	override int GetFullFilteredEntityListWithLabels(notnull out array<SCR_EntityCatalogEntry> filteredEntityList, array<EEditableEntityLabel> includedLabels = null, array<EEditableEntityLabel> excludedLabels = null, bool needsAllIncluded = true)
	{
		//super.GetFullFilteredEntityListWithLabels(filteredEntityList, includedLabels, excludedLabels, needsAllIncluded);
		
		int result = super.GetFullFilteredEntityListWithLabels(filteredEntityList, includedLabels, excludedLabels, needsAllIncluded);
		
		Print("[ME_DEBUG] SCR_EntityCatalog::GetFullFilteredEntityListWithLabels");
		
		/*
		//~ Clear Given list
		filteredEntityList.Clear();
		
		//~ Copy list
		foreach (SCR_EntityCatalogEntry entityEntry: m_aEntityEntryList)
		{
			//PrintFormat("[ME_DEBUG_SCR_EntityCatalog] GetFullFilteredEntityListWithLabels: entityEntry=%1", entityEntry); // SCR_EntityCatalogEntry<0x0000022EEC96CDC0>
			//PrintFormat("[ME_DEBUG_SCR_EntityCatalog] GetFullFilteredEntityListWithLabels: entityEntry.GetEntityUiInfo()=%1", entityEntry.GetEntityUiInfo()); // SCR_EditableVehicleUIInfo<0x0000022EB089EED0>
			//PrintFormat("[ME_DEBUG_SCR_EntityCatalog] GetFullFilteredEntityListWithLabels: entityEntry.GetEntityName()=%1", entityEntry.GetEntityName()); // #AR-Vehicle_M151A2_Open_Name
			//PrintFormat("[ME_DEBUG_SCR_EntityCatalog] GetFullFilteredEntityListWithLabels: entityEntry.GetPrefab()=%1", entityEntry.GetPrefab()); // {F649585ABB3706C4}Prefabs/Vehicles/Wheeled/M151A2/M151A2.et
			
			//~ If entity has any of the given exclude continue to next
			if (excludedLabels != null && !excludedLabels.IsEmpty() && entityEntry.HasAnyEditableEntityLabels(excludedLabels))
				continue;
			
			//~ Check included labels
			if (includedLabels != null && !includedLabels.IsEmpty())
			{
				//~ Needs all included
				if (needsAllIncluded)
				{
					if (!entityEntry.HasAllEditableEntityLabels(includedLabels))
						continue;
				}
				//~ Needs any included
				else 
				{
					if (!entityEntry.HasAnyEditableEntityLabels(includedLabels))
						continue;
				}
			}
				
			//~ Add to list
			filteredEntityList.Insert(entityEntry);
		}
			
		return filteredEntityList.Count();		
		*/
		
		return result;
	}
};
