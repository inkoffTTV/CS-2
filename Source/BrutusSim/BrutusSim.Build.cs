using UnrealBuildTool;

// Engine-free gameplay math. Also built natively by SimTests/ for unit tests.
public class BrutusSim : ModuleRules
{
	public BrutusSim(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.Add("Core");
	}
}
