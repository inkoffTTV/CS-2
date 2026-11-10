// Dedicated server target: builds only with an engine compiled from source.
using UnrealBuildTool;

public class BrutusServerTarget : TargetRules
{
	public BrutusServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "Brutus", "BrutusSim" });
	}
}
