/*
modded class SCR_EntityCatalogEntry
{
	override bool HasAnyEditableEntityLabels(notnull array<EEditableEntityLabel> editableEntityLables)
	{
		bool result = super.HasAnyEditableEntityLabels(editableEntityLables);
		
		Print("[ME_DEBUG] SCR_EntityCatalogEntry::HasAnyEditableEntityLabels");
		
		return result;
	}
	
	override bool HasAllEditableEntityLabels(notnull array<EEditableEntityLabel> editableEntityLables)
	{
		bool result = super.HasAllEditableEntityLabels(editableEntityLables);
		
		Print("[ME_DEBUG] SCR_EntityCatalogEntry::HasAllEditableEntityLabels");
		
		return result;
	}
};
*/