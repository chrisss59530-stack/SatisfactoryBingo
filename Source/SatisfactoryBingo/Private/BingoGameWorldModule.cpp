#include "BingoGameWorldModule.h"
#include "BingoSubsystem.h"

#include "FGSchematicManager.h"
#include "FGSchematic.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogBingoUnlock, Log, All);


UBingoGameWorldModule::UBingoGameWorldModule()
{
    bRootModule = false;
}
void UBingoGameWorldModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
    if (Phase == ELifecyclePhase::CONSTRUCTION)
    {
        if (!ModSubsystems.Contains(ABingoSubsystem::StaticClass()))
        {
            ModSubsystems.Add(ABingoSubsystem::StaticClass());
        }
    }

    Super::DispatchLifecycleEvent(Phase);

    if (Phase != ELifecyclePhase::POST_INITIALIZATION)
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    UnlockAttempts = 0;

    World->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(
            this,
            &UBingoGameWorldModule::TryUnlockBingoTerminal
        )
    );
}

void UBingoGameWorldModule::TryUnlockBingoTerminal()
{
    UWorld* World = GetWorld();

    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    AFGSchematicManager* Manager = AFGSchematicManager::Get(World);

    if (!IsValid(Manager))
    {
        if (++UnlockAttempts < 30)
        {
            World->GetTimerManager().SetTimer(
                UnlockTimerHandle,
                FTimerDelegate::CreateUObject(
                    this,
                    &UBingoGameWorldModule::TryUnlockBingoTerminal
                ),
                1.0f,
                false
            );
        }

        return;
    }

    const TSubclassOf<UFGSchematic> BingoSchematic =
        LoadClass<UFGSchematic>(
            nullptr,
            TEXT("/SatisfactoryBingo/Bingo/Schematics/Schematic_Bingo_Terminal.Schematic_Bingo_Terminal_C")
        );

    if (!BingoSchematic)
    {
        UE_LOG(LogBingoUnlock, Error,
            TEXT("Schematic Bingo introuvable"));
        return;
    }

    if (Manager->IsSchematicPurchased(BingoSchematic))
    {
        UE_LOG(LogBingoUnlock, Log,
            TEXT("Terminal Bingo deja debloque"));
        return;
    }

    Manager->GiveAccessToSchematic(
        BingoSchematic,
        nullptr,
        ESchematicUnlockFlags::None
    );

    UE_LOG(LogBingoUnlock, Log,
        TEXT("Terminal Bingo : deblocage demande"));
}

