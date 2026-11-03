using UnrealBuildTool;

public class SatisfactoryBingo : ModuleRules
{
    public SatisfactoryBingo(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject", "InputCore",
            "Engine"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "UMG",
            "Slate",
            "SlateCore",
            
            "Projects",
            "OnlineSubsystem",
            "FactoryGame",
            "SML"
        });
    }
}
