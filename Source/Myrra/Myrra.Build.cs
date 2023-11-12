// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Myrra : ModuleRules
{

    public Myrra(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        //дабавлено в ручную, чтобы после модификации TRarget.cs не потерялись подпапки
        PublicIncludePaths.AddRange(
            new string[]
            {
                "Myrra",
                "Myrra/Creature",
                "Myrra/Dendro",
                "Myrra/Control",
                "Myrra/Artefact",
                "Myrra/AssetStructures",
                "Myrra/BaseDefinitions",
            }
        );
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "UMG",
                "Slate",
                "SlateCore",
                "Landscape",
                "Engine",
                "InputCore",
                "HeadMountedDisplay",
                "AIModule",
                "GameplayTasks",
                "GameplayAbilities",
                "MoviePlayer",
                "Niagara",
                "Paper2D",
                "PhysicsCore"       //для UPhysicalMaterial
            }
            );

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
