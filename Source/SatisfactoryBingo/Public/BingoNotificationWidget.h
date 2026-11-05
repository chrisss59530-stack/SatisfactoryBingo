#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BingoNotificationWidget.generated.h"

class UTextBlock;
class URichTextBlock;
class UWidgetAnimation;
class UWidgetBlueprintGeneratedClass;

struct FBingoPendingNotification
{
    FText Description;
    int32 Points = 0;
    bool bIsBingo = false;
};

UCLASS()
class SATISFACTORYBINGO_API UBingoNotificationWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
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

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SecondaryTitleText;

    UPROPERTY(Transient)
    TObjectPtr<URichTextBlock> NativeMessageText;

    UPROPERTY(Transient)
    TObjectPtr<UWidgetAnimation> NativeSpawnAnimation;

    UPROPERTY(Transient)
    TObjectPtr<UWidgetBlueprintGeneratedClass> NativePresentationClass;

    float DisplayTime = 0.0f;
    float DisplayDuration = 4.0f;
    bool bNotificationActive = false;
    bool bPlayingNativeAnimation = false;
    TArray<FBingoPendingNotification> PendingNotifications;

    void BuildNotification();
    bool BuildNativePresentation();
    void StartNextNotification();
};
