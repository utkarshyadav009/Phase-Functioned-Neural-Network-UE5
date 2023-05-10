using System.IO;
using UnrealBuildTool;

public class PFNN : ModuleRules
{
    public PFNN(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // Get the base path of the plugin
        string PluginDirectory = Path.GetDirectoryName(RulesCompiler.GetFileNameFromType(GetType()));

        PublicIncludePaths.AddRange(
            new string[] {
                // Add Eigen and GLM include paths
                Path.Combine(PluginDirectory, "ThirdParty", "Eigen"),
                Path.Combine(PluginDirectory, "ThirdParty", "glm"),
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
                "DeveloperSettings",
                "Projects",
                "NavigationSystem"
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

        // Only include editor-specific modules when building for the editor
        // if (Target.bBuildEditor)
        // {
        //     PrivateDependencyModuleNames.AddRange(
        //         new string[]
        //         {
        //             "AnimGraph",
        //             "BlueprintGraph",
        //             "UnrealEd" // Add this line
        //         }
        //     );
        // }
    }
}
