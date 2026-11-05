#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BingoGameData.h"
#include "BingoRadarPlayerActor.generated.h"

class UFGActorRepresentation;
class UBingoNotificationWidget;

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

    // Only the owning local controller may request a purchase. The server owns
    // all price, phase, inventory and personal balance checks.
    void RequestRewardPurchase(const FString& RewardId);
    bool IsRewardPurchasePending() const { return bRewardPurchasePending; }
    const FText& GetRewardPurchaseMessage() const { return RewardPurchaseMessage; }

protected:
    virtual void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION(Server, Reliable)
    void Server_PurchaseReward(const FString& RewardId);

    UFUNCTION(Client, Reliable)
    void Client_ReceiveRewardPurchaseResult(bool bSucceeded, const FString& Message);

    bool bRewardPurchasePending = false;
    FText RewardPurchaseMessage;
    double NextRewardPurchaseTime = 0.0;

    void ActivateLocalRadar(float DurationSeconds);
    void ClearRadarMarkers();

    UPROPERTY(Transient)
    TArray<TObjectPtr<UFGActorRepresentation>> RadarMarkers;

    UPROPERTY(Transient)
    TObjectPtr<UBingoNotificationWidget> NotificationWidget;

    bool bHasReceivedProgressSnapshot = false;
    void ShowProgressNotification(const FText& Description, int32 Points, bool bIsBingo);

    FTimerHandle RadarExpirationTimer;
};
