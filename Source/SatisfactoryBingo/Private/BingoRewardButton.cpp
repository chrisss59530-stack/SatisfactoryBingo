#include "BingoRewardButton.h"

void UBingoRewardButton::InitializeReward(const FString& InRewardId)
{
    RewardId = InRewardId;
    OnClicked.AddUniqueDynamic(this, &UBingoRewardButton::HandleClicked);
}

void UBingoRewardButton::HandleClicked()
{
    OnRewardSelected.Broadcast(RewardId);
}
