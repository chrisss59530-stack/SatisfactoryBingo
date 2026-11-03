#include "CoreMinimal.h"
#include "Editor.h"
#include "HAL/PlatformMisc.h"
#include "HAL/IConsoleManager.h"
#include "FigmaImportSubsystem.h"
#include "REST/RequestParams.h"

namespace BingoFigmaBridge
{
    static URequestParams* ActiveParams = nullptr;
    static bool bImportRunning = false;

    static void OnImportStatus(eRequestStatus Status, FString Message)
    {
        if (Status == eRequestStatus::Succeeded)
        {
            UE_LOG(LogTemp, Display,
                TEXT("[BINGO FIGMA] Importation terminee : %s"),
                *Message);

            bImportRunning = false;
            if (ActiveParams)
            {
                ActiveParams->RemoveFromRoot();
                ActiveParams = nullptr;
            }
        }
        else if (Status == eRequestStatus::Failed)
        {
            UE_LOG(LogTemp, Error,
                TEXT("[BINGO FIGMA] Echec : %s"),
                *Message);

            bImportRunning = false;
            if (ActiveParams)
            {
                ActiveParams->RemoveFromRoot();
                ActiveParams = nullptr;
            }
        }
        else
        {
            UE_LOG(LogTemp, Display,
                TEXT("[BINGO FIGMA] %s"),
                *Message);
        }
    }

    static void Import()
    {
        if (bImportRunning)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[BINGO FIGMA] Importation deja en cours"));
            return;
        }

        const FString Token =
            FPlatformMisc::GetEnvironmentVariable(TEXT("FIGMA_ACCESS_TOKEN"));

        if (Token.IsEmpty())
        {
            UE_LOG(LogTemp, Error,
                TEXT("[BINGO FIGMA] Variable FIGMA_ACCESS_TOKEN absente"));
            return;
        }

        if (!GEditor)
        {
            return;
        }

        UFigmaImportSubsystem* Subsystem =
            GEditor->GetEditorSubsystem<UFigmaImportSubsystem>();

        if (!Subsystem)
        {
            UE_LOG(LogTemp, Error,
                TEXT("[BINGO FIGMA] Subsystem indisponible"));
            return;
        }

        URequestParams* Params = NewObject<URequestParams>(GetTransientPackage());

        Params->AccessToken = Token;
        Params->FileKey = TEXT("oTzDZl0e0lu6HvMq5IcuT2");
        Params->ContentRootFolder =
            TEXT("/SatisfactoryBingo/Bingo/UI/FigmaV3");
        Params->Ids.Empty();
        Params->UsePrototypeFlow = false;
        Params->SaveAllAtEnd = true;

        ActiveParams = Params;
        ActiveParams->AddToRoot();
        bImportRunning = true;

        Subsystem->Request(
            Params,
            FOnFigmaImportUpdateStatusCB::CreateStatic(
                &OnImportStatus
            )
        );

        UE_LOG(LogTemp, Display,
            TEXT("[BINGO FIGMA] Importation lancee"));
    }

    static FAutoConsoleCommand ImportCommand(
        TEXT("Bingo.ImportFigma"),
        TEXT("Importer les interfaces Bingo V3 depuis Figma"),
        FConsoleCommandDelegate::CreateStatic(&Import)
    );
}

