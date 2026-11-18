#include "PrioritizedCategoriesCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

TSharedRef<IDetailCustomization> FPrioritizedCategoriesCustomization::MakeInstance()
{
	return MakeShared<FPrioritizedCategoriesCustomization>();
}

void FPrioritizedCategoriesCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// For a Blueprint this is the generated class. The editor copies PrioritizeCategories onto it from the C++ parent.
	const UClass* Class = DetailBuilder.GetBaseClass();
	if (!Class)
	{
		return;
	}

	TArray<FString> Prioritized;
	Class->GetPrioritizeCategories(Prioritized);
	if (Prioritized.IsEmpty())
	{
		return;
	}

	/*
	 * Sort orders are Priority * 1000 + insertion index (see ECategoryPriority). Transform uses 1xxx and engine
	 * "Important" categories use 2xxx, so 1500+ lands between them. The lambda captures the list by value (moved)
	 * because it runs later, after CustomizeDetails has returned.
	 */
	DetailBuilder.SortCategories([Prioritized = MoveTemp(Prioritized)](const TMap<FName, IDetailCategoryBuilder*>& AllCategoryMap)
	{
		constexpr int32 BaseSortOrder = ECategoryPriority::Transform * 1000 + 500;

		for (const TPair<FName, IDetailCategoryBuilder*>& Pair : AllCategoryMap)
		{
			// "Flare|Burn" -> "Flare", so a parent entry also covers its subcategories.
			FString TopLevel = Pair.Key.ToString();
			int32 DelimiterIndex = INDEX_NONE;
			if (TopLevel.FindChar(TEXT('|'), DelimiterIndex))
			{
				TopLevel.LeftInline(DelimiterIndex);
			}

			const int32 Index = Prioritized.IndexOfByPredicate([&TopLevel](const FString& Category)
			{
				return Category.Equals(TopLevel, ESearchCase::IgnoreCase);
			});

			if (Index != INDEX_NONE && Pair.Value)
			{
				Pair.Value->SetSortOrder(BaseSortOrder + Index);
			}
		}
	});
}
