#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/**
 * Puts a class's own categories directly under Transform in the Details panel.
 *
 * Unreal's built-in PrioritizeCategories only reorders the "default" categories. Engine customizations
 * (Static Mesh, Materials, Light, Rendering...) still sit above them. This customization reads the same
 * PrioritizeCategories list and, via IDetailLayoutBuilder::SortCategories, assigns those categories a sort order
 * just after Transform. Matching is on the top-level name, so "Flare" also moves "Flare|Burn", "Flare|Light"...
 *
 * Usage: add PrioritizeCategories=(MyCategory, Other) to any EndlessOcean UCLASS. The editor module registers
 * this customization for it automatically. Unity comparison: a CustomEditor that reorders the Inspector,
 * applied to every class that opts in with an attribute.
 */
class FPrioritizedCategoriesCustomization : public IDetailCustomization
{
public:

	/** Factory used when registering with the PropertyEditor module. */
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
