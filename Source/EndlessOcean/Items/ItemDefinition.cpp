#include "ItemDefinition.h"

FPrimaryAssetId UItemDefinition::GetPrimaryAssetId() const
{
	static const FPrimaryAssetType ItemType(TEXT("ItemDefinition"));
	return FPrimaryAssetId(ItemType, GetFName());
}
