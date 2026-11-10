using UnrealBuildTool;

public class BrutusTarget : TargetRules
{
	public BrutusTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "Brutus", "BrutusSim" });
	}
}
