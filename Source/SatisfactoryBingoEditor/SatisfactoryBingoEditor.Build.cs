using UnrealBuildTool;

public class SatisfactoryBingoEditor : ModuleRules
{
    public SatisfactoryBingoEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "Kismet",
            "BlueprintGraph",
            "UMG",
            "UMGEditor"
        });
    }
}

