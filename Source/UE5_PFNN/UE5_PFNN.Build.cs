// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UE5_PFNN : ModuleRules
{
	public UE5_PFNN(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
