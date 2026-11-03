#include "BingoProgressComponent.h"

UBingoProgressComponent::UBingoProgressComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UBingoProgressComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        InitializePhases();
    }
}

void UBingoProgressComponent::InitializePhases()
{
    if (Phases.Num() == 5)
    {
        return;
    }

    Phases.SetNum(5);

    for (FBingoPhaseState& State : Phases)
    {
        State.Completed.Init(false, 25);
        State.AwardedLines.Init(false, 12);

        // Case centrale gratuite : index 12.
        State.Completed[12] = true;

        State.Score = 0;
        State.BingoCount = 0;
        State.BonusPoints = 0;
    }
}

bool UBingoProgressComponent::CompleteCell(
    int32 Phase,
    int32 Cell)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return false;
    }

    if (!Phases.IsValidIndex(Phase) ||
        Cell < 0 || Cell >= 25)
    {
        return false;
    }

    FBingoPhaseState& State = Phases[Phase];

    if (!State.Completed.IsValidIndex(Cell) ||
        State.Completed[Cell])
    {
        return false;
    }

    State.Completed[Cell] = true;
    State.Score += 1;

    OnCellCompleted.Broadcast(Phase, Cell);

    EvaluateLines(Phase);

    return true;
}

bool UBingoProgressComponent::IsCellCompleted(
    int32 Phase,
    int32 Cell) const
{
    return Phases.IsValidIndex(Phase) &&
        Phases[Phase].Completed.IsValidIndex(Cell) &&
        Phases[Phase].Completed[Cell];
}

int32 UBingoProgressComponent::GetTotalPoints(
    int32 Phase) const
{
    if (!Phases.IsValidIndex(Phase))
    {
        return 0;
    }

    return Phases[Phase].Score +
           Phases[Phase].BonusPoints;
}

int32 UBingoProgressComponent::GetBingoCount(
    int32 Phase) const
{
    return Phases.IsValidIndex(Phase)
        ? Phases[Phase].BingoCount
        : 0;
}

void UBingoProgressComponent::EvaluateLines(int32 Phase)
{
    if (!Phases.IsValidIndex(Phase))
    {
        return;
    }

    FBingoPhaseState& State = Phases[Phase];

    for (int32 Line = 0; Line < 12; ++Line)
    {
        if (State.AwardedLines[Line])
        {
            continue;
        }

        bool bComplete = true;

        for (int32 Offset = 0; Offset < 5; ++Offset)
        {
            int32 Index = 0;

            if (Line < 5)
            {
                // Lignes horizontales.
                Index = Line * 5 + Offset;
            }
            else if (Line < 10)
            {
                // Colonnes verticales.
                Index = Offset * 5 + (Line - 5);
            }
            else if (Line == 10)
            {
                // Diagonale principale.
                Index = Offset * 6;
            }
            else
            {
                // Diagonale secondaire.
                Index = 4 + Offset * 4;
            }

            if (!State.Completed[Index])
            {
                bComplete = false;
                break;
            }
        }

        if (bComplete)
        {
            State.AwardedLines[Line] = true;
            State.BingoCount++;

            // Bonus provisoire, configurable plus tard.
            const int32 Bonus = 5;

            State.BonusPoints += Bonus;

            OnLineCompleted.Broadcast(
                Phase,
                Line,
                Bonus
            );
        }
    }
}
