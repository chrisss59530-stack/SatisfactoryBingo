#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BingoGameData.generated.h"

class AFGCharacterPlayer;

USTRUCT(BlueprintType)
struct FBingoChallenge
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bingo")
    FString ChallengeId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bingo")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bingo")
    bool bCompleted = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bingo")
    int32 Points = 100;
};

USTRUCT(BlueprintType)
struct FBingoPhase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bingo")
    int32 PhaseNumber = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bingo")
    TArray<FBingoChallenge> Challenges;

    UPROPERTY(BlueprintReadOnly, Category="Bingo")
    TArray<bool> CompletedLines;

    UPROPERTY(BlueprintReadOnly, Category="Bingo")
    int32 BingoCount = 0;

    UPROPERTY(BlueprintReadOnly, Category="Bingo")
    int32 BingoBonus = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bingo")
    bool bUnlocked = false;
};

UCLASS(BlueprintType)
class SATISFACTORYBINGO_API UBingoGameData : public UObject
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadOnly, Category="Bingo")
    TArray<FBingoPhase> Phases;

    UPROPERTY(BlueprintReadOnly, Category="Bingo")
    int32 CurrentPhase = 1;

    UPROPERTY(BlueprintReadOnly, Category="Bingo")
    int32 TotalScore = 0;

    UFUNCTION(BlueprintCallable, Category="Bingo")
    void InitializeGame();

    UFUNCTION(BlueprintCallable, Category="Bingo")
    bool CompleteChallenge(const FString& ChallengeId);

    UFUNCTION(BlueprintCallable, Category="Bingo")
    void EvaluateBingoLines(int32 PhaseNumber);

    UFUNCTION(BlueprintPure, Category="Bingo")
    int32 GetBingoCount(int32 PhaseNumber) const;

    UFUNCTION(BlueprintPure, Category="Bingo")
    int32 GetCompletedCount(int32 PhaseNumber) const;

    UFUNCTION(BlueprintPure, Category="Bingo")
    int32 GetTotalScore() const;

    UFUNCTION(BlueprintCallable, Category="Bingo")
    bool UnlockNextPhase();

    UFUNCTION(BlueprintCallable, Category="Bingo|Rewards")
    bool PurchaseReward(
        const FString& RewardId,
        AFGCharacterPlayer* Player
    );
};
