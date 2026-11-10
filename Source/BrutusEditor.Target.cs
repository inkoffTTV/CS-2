using UnrealBuildTool;

public class BrutusEditorTarget : TargetRules
{
	public BrutusEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "Brutus", "BrutusSim" });
	}
}
