using UnrealBuildTool;

public class TDMShooter : ModuleRules
{
	public TDMShooter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"NetCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UMG",
			"Slate",
			"SlateCore"
		});

		// Allows includes relative to module root, e.g. #include "Player/TDMCharacter.h"
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
