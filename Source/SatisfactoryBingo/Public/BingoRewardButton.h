#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "BingoRewardButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBingoRewardSelected, const FString&, RewardId);

/** Carries the exact reward ID for mouse, keyboard and controller activation. */
UCLASS()
class SATISFACTORYBINGO_API UBingoRewardButton : public UButton
{
    GENERATED_BODY()

public:
    void InitializeReward(const FString& InRewardId);

    UPROPERTY()
    FBingoRewardSelected OnRewardSelected;

private:
    UPROPERTY(Transient)
    FString RewardId;

    UFUNCTION()
    void HandleClicked();
};
