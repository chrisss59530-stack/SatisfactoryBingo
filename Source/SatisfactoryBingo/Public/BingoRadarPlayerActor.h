#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BingoGameData.h"
#include "BingoRadarPlayerActor.generated.h"

class UFGActorRepresentation;

UCLASS()
class SATISFACTORYBINGO_API ABingoRadarPlayerActor : public AActor
{
    GENERATED_BODY()

public:
    ABingoRadarPlayerActor();

    // BINGO_PERSONAL_NETWORK_DATA_V1
    // Copie locale des cinq grilles du joueur proprietaire.
    UPROPERTY(Transient, BlueprintReadOnly, Category="Bingo|Network")
    TObjectPtr<UBingoGameData> PersonalBingoData;

    UFUNCTION(Client, Reliable)
    void Client_ReceiveBingoProgress(
        int32 CurrentPhase,
        int32 Points,
        const TArray<FString>& CompletedChallenges
    );

    UBingoGameData* GetPersonalBingoData() const
    {
        return PersonalBingoData.Get();
    }

    UFUNCTION(Client, Reliable)
    void Client_ActivateRadar(float DurationSeconds);

protected:
    virtual void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

private:
    void ActivateLocalRadar(float DurationSeconds);
    void ClearRadarMarkers();

    UPROPERTY(Transient)
    TArray<TObjectPtr<UFGActorRepresentation>> RadarMarkers;

    FTimerHandle RadarExpirationTimer;
};
