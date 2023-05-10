using System.IO;
using UnrealBuildTool;

public class PFNN_AnimEditor : ModuleRules
{
    public PFNN_AnimEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
        // ... other include paths ...
				 Path.Combine(ModuleDirectory, "Animation", "AnimNodes"),
            }
        );

        
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // Disable the warning for discarding the return value of a function with the 'nodiscard' attribute
            PublicDefinitions.Add("_SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING");
            PublicDefinitions.Add("EIGEN_MPL2_ONLY");
            // Enable exception handling
            bEnableExceptions = true;
        }

        PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject", 
				"Engine",
				"AnimGraph", 
				"BlueprintGraph",
                "DeveloperSettings",
				"PFNN",
                "NavigationSystem",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// ... add private dependencies that you statically link with here ...	
			}
		);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
    }
}