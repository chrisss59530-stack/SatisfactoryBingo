#include "BingoRadarPlayerActor.h"

#include "BingoRadarScanner.h"
#include "BingoNotificationWidget.h"
#include "FGActorRepresentationManager.h"
#include "FGActorRepresentation.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "BingoSubsystem.h"
#include "BingoRewards.h"
#include "FGCharacterPlayer.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

ABingoRadarPlayerActor::ABingoRadarPlayerActor()
{
    bReplicates = true;

    // BINGO_OWNER_RELEVANCY_V1
    // L'acteur reseau reste pertinent pour son proprietaire.
    bOnlyRelevantToOwner = true;
    bAlwaysRelevant = false;

    SetReplicateMovement(false);
}

void ABingoRadarPlayerActor::RequestRewardPurchase(const FString& RewardId)
{
    APlayerController* Controller = Cast<APlayerController>(GetOwner());
    if (!IsValid(Controller) || !Controller->IsLocalController() ||
        bRewardPurchasePending)
    {
        return;
    }

    bRewardPurchasePending = true;
    RewardPurchaseMessage = FText::FromString(TEXT("Achat en cours..."));
    Server_PurchaseReward(RewardId);
}

void ABingoRadarPlayerActor::Server_PurchaseReward_Implementation(const FString& RewardId)
{
    APlayerController* Controller = Cast<APlayerController>(GetOwner());
    AFGCharacterPlayer* Player = IsValid(Controller)
        ? Cast<AFGCharacterPlayer>(Controller->GetPawn()) : nullptr;
    if (!HasAuthority() || !GetWorld() || !IsValid(Player) ||
        Player->GetController() != Controller)
    {
        Client_ReceiveRewardPurchaseResult(false, TEXT("Joueur indisponible."));
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextRewardPurchaseTime)
    {
        Client_ReceiveRewardPurchaseResult(false, TEXT("Veuillez patienter avant le prochain achat."));
        return;
    }
    NextRewardPurchaseTime = Now + 0.5;

    FBingoRewardDefinition Reward;
    if (RewardId.Len() > 64 || !FBingoRewards::Find(RewardId, Reward))
    {
        Client_ReceiveRewardPurchaseResult(false, TEXT("Recompense inconnue."));
        return;
    }

    for (TActorIterator<ABingoSubsystem> It(GetWorld()); It; ++It)
    {
        ABingoSubsystem* Subsystem = *It;
        if (!IsValid(Subsystem)) continue;

        // Reuse the existing authoritative purchase and save-data path.
        const bool bSucceeded = Subsystem->BuyBingoReward(RewardId, Player);
        // Refresh the owner's balance before acknowledging the purchase.
        Subsystem->BingoSendPersonalProgress();
        Client_ReceiveRewardPurchaseResult(bSucceeded, bSucceeded
            ? FString::Printf(TEXT("Achat reussi : %s"), *Reward.Name)
            : TEXT("Achat refuse : verifiez vos points, la phase et la place dans l'inventaire."));
        return;
    }

    Client_ReceiveRewardPurchaseResult(false, TEXT("Service Bingo indisponible."));
}

void ABingoRadarPlayerActor::Client_ReceiveRewardPurchaseResult_Implementation(
    bool bSucceeded, const FString& Message)
{
    bRewardPurchasePending = false;
    RewardPurchaseMessage = FText::FromString(Message);
    UE_LOG(LogTemp, Display, TEXT("[Bingo][Achat] %s : %s"),
        bSucceeded ? TEXT("OK") : TEXT("REFUSE"), *Message);
}

void ABingoRadarPlayerActor::Client_ReceiveBingoProgress_Implementation(
    int32 CurrentPhase,
    int32 Points,
    const TArray<FString>& CompletedChallenges)
{
    // The first snapshot restores history silently. Only compare subsequent
    // authoritative snapshots addressed to this actor's owning player.
    const bool bNotifyChanges = bHasReceivedProgressSnapshot && IsValid(PersonalBingoData);
    TSet<FString> PreviousCompleted;
    TMap<int32, int32> PreviousBingoCounts;
    TMap<int32, int32> PreviousBingoBonuses;
    if (bNotifyChanges)
    {
        for (const FBingoPhase& Phase : PersonalBingoData->Phases)
        {
            PreviousBingoCounts.Add(Phase.PhaseNumber, Phase.BingoCount);
            PreviousBingoBonuses.Add(Phase.PhaseNumber, Phase.BingoBonus);
            for (const FBingoChallenge& Challenge : Phase.Challenges)
            {
                if (Challenge.bCompleted) PreviousCompleted.Add(Challenge.ChallengeId);
            }
        }
    }

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

    if (bNotifyChanges)
    {
        for (const FBingoPhase& Phase : PersonalBingoData->Phases)
        {
            for (const FBingoChallenge& Challenge : Phase.Challenges)
            {
                // The free centre cell is not a newly earned challenge.
                if (Challenge.bCompleted && Challenge.Points > 0 &&
                    !PreviousCompleted.Contains(Challenge.ChallengeId))
                {
                    ShowProgressNotification(Challenge.Description, Challenge.Points, false);
                }
            }

            const int32 NewLines = Phase.BingoCount - PreviousBingoCounts.FindRef(Phase.PhaseNumber);
            if (NewLines > 0)
            {
                const int32 Bonus = Phase.BingoBonus - PreviousBingoBonuses.FindRef(Phase.PhaseNumber);
                const FString Detail = NewLines == 1
                    ? FString::Printf(TEXT("PHASE %d : UNE LIGNE COMPLETEE"), Phase.PhaseNumber)
                    : FString::Printf(TEXT("PHASE %d : %d LIGNES COMPLETEES"), Phase.PhaseNumber, NewLines);
                ShowProgressNotification(FText::FromString(Detail), Bonus, true);
            }
        }
    }
    bHasReceivedProgressSnapshot = true;

    UE_LOG(LogTemp, Display,
        TEXT("[Bingo][Client] Progression personnelle recue : phase %d, points %d, defis %d"),
        TargetPhase,
        Points,
        CompletedChallenges.Num());
}
void ABingoRadarPlayerActor::ShowProgressNotification(
    const FText& Description, int32 Points, bool bIsBingo)
{
    APlayerController* Controller = Cast<APlayerController>(GetOwner());
    if (!IsValid(Controller) || !Controller->IsLocalController()) return;

    if (!IsValid(NotificationWidget))
    {
        NotificationWidget = CreateWidget<UBingoNotificationWidget>(
            Controller, UBingoNotificationWidget::StaticClass());
        if (!IsValid(NotificationWidget)) return;
    }
    if (!NotificationWidget->IsInViewport() && !NotificationWidget->AddToPlayerScreen(100)) return;
    NotificationWidget->ShowChallengeNotification(Description, Points, bIsBingo);
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
    if (IsValid(NotificationWidget))
    {
        NotificationWidget->RemoveFromParent();
        NotificationWidget = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}

