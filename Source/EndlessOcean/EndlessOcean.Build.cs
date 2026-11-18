// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class EndlessOcean : ModuleRules
{
	public EndlessOcean(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"EndlessOcean",
			"EndlessOcean/Variant_Horror",
			"EndlessOcean/Variant_Horror/UI",
			"EndlessOcean/Variant_Shooter",
			"EndlessOcean/Variant_Shooter/AI",
			"EndlessOcean/Variant_Shooter/UI",
			"EndlessOcean/Variant_Shooter/Weapons",
			"EndlessOcean/Player",
			"EndlessOcean/Interaction",
			"EndlessOcean/Items"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
