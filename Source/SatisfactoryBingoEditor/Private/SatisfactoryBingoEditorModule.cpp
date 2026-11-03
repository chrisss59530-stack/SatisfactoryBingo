#include "Engine/BlueprintGeneratedClass.h"
#include "Modules/ModuleManager.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "UObject/UObjectGlobals.h"

class FSatisfactoryBingoEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        AuditCommand = IConsoleManager::Get().RegisterConsoleCommand(
            TEXT("Bingo.AuditTerminal"),
            TEXT("Audit SCS du terminal Bingo"),
            FConsoleCommandDelegate::CreateRaw(
                this,
                &FSatisfactoryBingoEditorModule::AuditTerminal
            ),
            ECVF_Default
        );
    }

    virtual void ShutdownModule() override
    {
        if (AuditCommand)
        {
            IConsoleManager::Get().UnregisterConsoleObject(AuditCommand);
            AuditCommand = nullptr;
        }
    }

private:
    IConsoleObject* AuditCommand = nullptr;

    void AuditTerminal()
    {
        const TCHAR* Path =
            TEXT("/SatisfactoryBingo/Bingo/Buildables/"
                 "BP_Bingo_Terminal_3D.BP_Bingo_Terminal_3D");

        UBlueprint* BP = LoadObject<UBlueprint>(nullptr, Path);

        if (!BP || !BP->SimpleConstructionScript)
        {
            UE_LOG(LogTemp, Error,
                TEXT("[BINGO] Blueprint ou SCS introuvable"));
            return;
        }

        const TArray<USCS_Node*>& Nodes =
            BP->SimpleConstructionScript->GetAllNodes();

        int32 MeshCount = 0;
        int32 SuspectCount = 0;

        UE_LOG(LogTemp, Warning,
            TEXT("[BINGO] === AUDIT SCS ==="));

        for (USCS_Node* Node : Nodes)
        {
            if (!Node)
            {
                continue;
            }

            const FString Name =
                Node->GetVariableName().ToString();

            const FString ClassName =
                Node->ComponentClass
                ? Node->ComponentClass->GetName()
                : TEXT("CLASSE_MANQUANTE");

            if (Node->ComponentClass &&
                Node->ComponentClass->IsChildOf(
                    UStaticMeshComponent::StaticClass()))
            {
                ++MeshCount;
            }

            const bool bSuspect =
                Name.Contains(TEXT("Animator")) ||
                Name.Contains(TEXT("ContractoryTerminal")) ||
                Node->ComponentClass == nullptr;

            if (bSuspect)
            {
                ++SuspectCount;

                UE_LOG(LogTemp, Warning,
                    TEXT("[BINGO] SUSPECT : %s | Classe : %s"),
                    *Name,
                    *ClassName);
            }
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[BINGO] Noeuds : %d | Meshes : %d | Suspects : %d"),
            Nodes.Num(),
            MeshCount,
            SuspectCount);

        UE_LOG(LogTemp, Warning,
            TEXT("[BINGO] === VARIABLES BLUEPRINT ==="));

        for (const FBPVariableDescription& Variable : BP->NewVariables)
        {
            const FString Name = Variable.VarName.ToString();

            if (Name.Contains(TEXT("Contractory")) ||
                Name.Contains(TEXT("Animator")))
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("[BINGO] VARIABLE SUSPECTE : %s"),
                    *Name);
            }
        }

        if (UBlueprintGeneratedClass* Generated =
            Cast<UBlueprintGeneratedClass>(BP->GeneratedClass))
        {
            for (TFieldIterator<FProperty> It(Generated); It; ++It)
            {
                const FString Name = It->GetName();

                if (Name.Contains(TEXT("Contractory")) ||
                    Name.Contains(TEXT("Animator")))
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("[BINGO] PROPRIETE GENEREE : %s"),
                        *Name);
                }
            }
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[BINGO] === FIN AUDIT ==="));
    }
};

IMPLEMENT_MODULE(
    FSatisfactoryBingoEditorModule,
    SatisfactoryBingoEditor
);

