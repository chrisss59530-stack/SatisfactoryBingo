#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BingoNotificationWidget.generated.h"

class UTextBlock;

UCLASS()
class SATISFACTORYBINGO_API UBingoNotificationWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    virtual void NativeTick(
        const FGeometry& MyGeometry,
        float InDeltaTime) override;

    UFUNCTION(BlueprintCallable, Category="Bingo")
    void ShowChallengeNotification(
        const FText& ChallengeName,
        int32 Points,
        bool bIsBingo = false);

private:
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ChallengeText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PointsText;

    float DisplayTime = 0.0f;
    bool bNotificationActive = false;

    void BuildNotification();
};
