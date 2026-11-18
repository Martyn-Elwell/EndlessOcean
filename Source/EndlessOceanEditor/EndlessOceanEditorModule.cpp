#include "EndlessOceanEditorModule.h"

#include "Modules/ModuleManager.h"
#include "PrioritizedCategoriesCustomization.h"
#include "PropertyEditorModule.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"

IMPLEMENT_MODULE(FEndlessOceanEditorModule, EndlessOceanEditor);

void FEndlessOceanEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	/*
	 * Find every class in our runtime module that declares PrioritizeCategories and give it the customization.
	 * No list to maintain: adding PrioritizeCategories=(...) to any EndlessOcean UCLASS is enough.
	 * Customizations also apply to subclasses (including Blueprints), so BP_Flare picks this up from AFlareItem.
	 */
	static const FName RuntimePackageName(TEXT("/Script/EndlessOcean"));
	static const FName PrioritizeCategoriesKey(TEXT("PrioritizeCategories"));

	for (TObjectIterator<UClass> It; It; ++It)
	{
		const UClass* Class = *It;
		if (Class->GetOutermost()->GetFName() != RuntimePackageName || !Class->HasMetaData(PrioritizeCategoriesKey))
		{
			continue;
		}

		// Class layouts are keyed by name without the A/U prefix ("FlareItem").
		PropertyEditor.RegisterCustomClassLayout(Class->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FPrioritizedCategoriesCustomization::MakeInstance));
		RegisteredClassLayouts.Add(Class->GetFName());
	}

	PropertyEditor.NotifyCustomizationModuleChanged();
}

void FEndlessOceanEditorModule::ShutdownModule()
{
	// PropertyEditor may already be gone when the editor is closing.
	if (FPropertyEditorModule* PropertyEditor = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
	{
		for (const FName& ClassName : RegisteredClassLayouts)
		{
			PropertyEditor->UnregisterCustomClassLayout(ClassName);
		}
		PropertyEditor->NotifyCustomizationModuleChanged();
	}
	RegisteredClassLayouts.Reset();
}
