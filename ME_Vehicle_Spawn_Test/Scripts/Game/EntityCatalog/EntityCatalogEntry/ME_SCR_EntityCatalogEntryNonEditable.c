modded class SCR_EntityCatalogEntryNonEditable : SCR_EntityCatalogEntry
{	
	override bool HasEditableEntityLabel(EEditableEntityLabel editableEntityLabel)
	{
		bool result = super.HasEditableEntityLabel(editableEntityLabel);
		
		Print("[ME_DEBUG] SCR_EntityCatalogEntryNonEditable::HasEditableEntityLabel");
		
		return result;
	}
}
