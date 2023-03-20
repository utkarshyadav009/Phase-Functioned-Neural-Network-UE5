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
                "Projects"
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
