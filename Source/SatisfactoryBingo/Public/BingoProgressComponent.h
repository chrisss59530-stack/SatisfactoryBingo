#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BingoProgressComponent.generated.h"

USTRUCT(BlueprintType)
struct FBingoPhaseState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, SaveGame)
    TArray<bool> Completed;

    UPROPERTY(BlueprintReadOnly, SaveGame)
    TArray<bool> AwardedLines;

    UPROPERTY(BlueprintReadOnly, SaveGame)
    int32 Score = 0;

    UPROPERTY(BlueprintReadOnly, SaveGame)
    int32 BingoCount = 0;

    UPROPERTY(BlueprintReadOnly, SaveGame)
    int32 BonusPoints = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FBingoCellCompleted,
    int32, Phase,
    int32, Cell
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FBingoLineCompleted,
    int32, Phase,
    int32, Line,
    int32, Bonus
);

UCLASS(ClassGroup=(Bingo), meta=(BlueprintSpawnableComponent))
class SATISFACTORYBINGO_API UBingoProgressComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:
    UBingoProgressComponent();

    UPROPERTY(BlueprintAssignable, Category="Bingo|Events")
    FBingoCellCompleted OnCellCompleted;

    UPROPERTY(BlueprintAssignable, Category="Bingo|Events")
    FBingoLineCompleted OnLineCompleted;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category="Bingo|Progress")
    TArray<FBingoPhaseState> Phases;

    UFUNCTION(BlueprintCallable, Category="Bingo|Progress")
    bool CompleteCell(int32 Phase, int32 Cell);

    UFUNCTION(BlueprintPure, Category="Bingo|Progress")
    bool IsCellCompleted(int32 Phase, int32 Cell) const;

    UFUNCTION(BlueprintPure, Category="Bingo|Progress")
    int32 GetTotalPoints(int32 Phase) const;

    UFUNCTION(BlueprintPure, Category="Bingo|Progress")
    int32 GetBingoCount(int32 Phase) const;

protected:
    virtual void BeginPlay() override;

private:
    void InitializePhases();
    void EvaluateLines(int32 Phase);
};
