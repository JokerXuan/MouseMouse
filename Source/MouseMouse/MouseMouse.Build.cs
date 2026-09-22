// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MouseMouse : ModuleRules
{
	public MouseMouse(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"NavigationSystem"
		});

		PublicIncludePaths.AddRange(new string[] {
			"MouseMouse",
			"MouseMouse/Variant_Horror",
			"MouseMouse/Variant_Horror/UI",
			"MouseMouse/Variant_Shooter",
			"MouseMouse/Variant_Shooter/AI",
			"MouseMouse/Variant_Shooter/UI",
			"MouseMouse/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
