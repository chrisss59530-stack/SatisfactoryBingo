#include "BingoRadarScanner.h"

#include "FGDropPod.h"
#include "FGItemPickup.h"

#include "Engine/World.h"
#include "EngineUtils.h"

TArray<FBingoRadarResult> UBingoRadarScanner::ScanWorld(
    UObject* WorldContextObject)
{
    TArray<FBingoRadarResult> Results;

    if (!WorldContextObject)
    {
        return Results;
    }

    UWorld* World = WorldContextObject->GetWorld();

    if (!World)
    {
        return Results;
    }

    // Disques durs : capsules non pillees.
    for (TActorIterator<AFGDropPod> It(World); It; ++It)
    {
        AFGDropPod* Pod = *It;

        if (!IsValid(Pod) || Pod->HasBeenLooted())
        {
            continue;
        }

        FBingoRadarResult Result;
        Result.Type = EBingoRadarTarget::HardDrive;
        Result.Location = Pod->GetActorLocation();
        Results.Add(Result);
    }

    // Artefacts encore presents dans le monde.
    for (TActorIterator<AFGItemPickup> It(World); It; ++It)
    {
        AFGItemPickup* Pickup = *It;

        if (!IsValid(Pickup) || Pickup->IsPickedUp())
        {
            continue;
        }

        const UClass* ItemClass = Pickup->GetPickupItemClass().Get();

        if (!ItemClass)
        {
            continue;
        }

        const FString ItemName = ItemClass->GetName();

        FBingoRadarResult Result;

        if (ItemName.StartsWith(TEXT("Desc_WAT1")))
        {
            Result.Type = EBingoRadarTarget::MercerSphere;
        }
        else if (ItemName.StartsWith(TEXT("Desc_WAT2")))
        {
            Result.Type = EBingoRadarTarget::Somersloop;
        }
        else
        {
            continue;
        }

        Result.Location = Pickup->GetActorLocation();
        Results.Add(Result);
    }

    UE_LOG(LogTemp, Log,
        TEXT("[BingoRadar] Scanner mondial : %d cibles"),
        Results.Num());

    return Results;
}

