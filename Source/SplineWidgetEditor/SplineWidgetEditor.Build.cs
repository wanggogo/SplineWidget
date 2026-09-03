using UnrealBuildTool;

public class SplineWidgetEditor : ModuleRules
{
	public SplineWidgetEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"SplineWidget",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"UMG",
				"UMGEditor",
				"UnrealEd",
				"InputCore",
			}
		);
	}
}
