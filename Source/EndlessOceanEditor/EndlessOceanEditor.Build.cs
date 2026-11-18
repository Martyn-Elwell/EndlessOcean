using UnrealBuildTool;

/**
 * Editor-only module: Details panel customizations, visualizers, asset tools.
 * Never shipped: it's listed with Type "Editor" in EndlessOcean.uproject and only added to the Editor target.
 */
public class EndlessOceanEditor : ModuleRules
{
	public EndlessOceanEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"EndlessOcean",
			"PropertyEditor",
			"Slate",
			"SlateCore",
			"UnrealEd"
		});

		PublicIncludePaths.AddRange(new string[] {
			"EndlessOceanEditor",
			"EndlessOceanEditor/DetailCustomizations"
		});
	}
}
