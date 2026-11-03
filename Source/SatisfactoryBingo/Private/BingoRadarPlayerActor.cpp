#include "BingoRadarPlayerActor.h"

#include "BingoRadarScanner.h"
#include "FGActorRepresentationManager.h"
#include "FGActorRepresentation.h"
#include "Engine/World.h"
#include "TimerManager.h"

ABingoRadarPlayerActor::ABingoRadarPlayerActor()
{
    bReplicates = true;

    // BINGO_OWNER_RELEVANCY_V1
    // L'acteur reseau reste pertinent pour son proprietaire.
    bOnlyRelevantToOwner = true;
    bAlwaysRelevant = false;

    SetReplicateMovement(false);
}

void ABingoRadarPlayerActor::Client_ReceiveBingoProgress_Implementation(
    int32 CurrentPhase,
    int32 Points,
    const TArray<FString>& CompletedChallenges)
{
    // BINGO_PERSONAL_NETWORK_RECEIVE_V1
    if (!PersonalBingoData)
    {
        PersonalBingoData = NewObject<UBingoGameData>(this);
    }

    if (!IsValid(PersonalBingoData))
    {
        return;
    }

    PersonalBingoData->InitializeGame();

    const int32 TargetPhase = FMath::Clamp(CurrentPhase, 1, 5);

    while (PersonalBingoData->CurrentPhase < TargetPhase)
    {
        if (!PersonalBingoData->UnlockNextPhase())
        {
            break;
        }
    }

    for (const FString& ChallengeId : CompletedChallenges)
    {
        PersonalBingoData->CompleteChallenge(ChallengeId);
    }

    // Le solde transmis par le serveur est prioritaire.
    PersonalBingoData->TotalScore = Points;

    UE_LOG(LogTemp, Display,
        TEXT("[Bingo][Client] Progression personnelle recue : phase %d, points %d, defis %d"),
        TargetPhase,
        Points,
        CompletedChallenges.Num());
}
void ABingoRadarPlayerActor::Client_ActivateRadar_Implementation(
    float DurationSeconds)
{
    ActivateLocalRadar(DurationSeconds);
}

void ABingoRadarPlayerActor::ActivateLocalRadar(
    float DurationSeconds)
{
    ClearRadarMarkers();

    if (DurationSeconds <= 0.0f || !GetWorld())
    {
        return;
    }

    AFGActorRepresentationManager* Manager =
        AFGActorRepresentationManager::Get(this);

    if (!IsValid(Manager))
    {
        return;
    }

    const TArray<FBingoRadarResult> Results =
        UBingoRadarScanner::ScanWorld(this);

    for (const FBingoRadarResult& Result : Results)
    {
        FLinearColor MarkerColor;

        switch (Result.Type)
        {
        case EBingoRadarTarget::HardDrive:
            MarkerColor = FLinearColor::Yellow;
            break;

        case EBingoRadarTarget::MercerSphere:
            MarkerColor = FLinearColor::Blue;
            break;

        case EBingoRadarTarget::Somersloop:
            MarkerColor = FLinearColor::Red;
            break;

        default:
            continue;
        }

        UFGActorRepresentation* Marker =
            Manager->CreateAndAddNewRepresentationNoActor(
                Result.Location,
                nullptr,
                MarkerColor,
                DurationSeconds,
                false,
                true
            );

        if (IsValid(Marker))
        {
            RadarMarkers.Add(Marker);
        }
    }

    GetWorldTimerManager().SetTimer(
        RadarExpirationTimer,
        this,
        &ABingoRadarPlayerActor::ClearRadarMarkers,
        DurationSeconds,
        false
    );

    UE_LOG(LogTemp, Display,
        TEXT("[BingoRadar] %d marqueurs temporaires crees"),
        RadarMarkers.Num());
}

void ABingoRadarPlayerActor::ClearRadarMarkers()
{
    GetWorldTimerManager().ClearTimer(RadarExpirationTimer);

    AFGActorRepresentationManager* Manager =
        AFGActorRepresentationManager::Get(this);

    if (IsValid(Manager))
    {
        for (UFGActorRepresentation* Marker : RadarMarkers)
        {
            if (IsValid(Marker))
            {
                Manager->RemoveRepresentation(Marker);
            }
        }
    }

    RadarMarkers.Empty();
}

void ABingoRadarPlayerActor::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    ClearRadarMarkers();
    Super::EndPlay(EndPlayReason);
}

