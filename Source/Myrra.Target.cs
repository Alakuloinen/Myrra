// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class MyrraTarget : TargetRules
{
	public MyrraTarget(TargetInfo Target) : base(Target)
	{
		DefaultBuildSettings = BuildSettingsVersion.V2;
		Type = TargetType.Game;
        ExtraModuleNames.AddRange( new string[] { "Myrra" } );
	}
}
