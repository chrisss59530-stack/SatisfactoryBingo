#pragma once

#include "CoreMinimal.h"
#include "UI/FGInteractWidget.h"
#include "BingoTerminalWidget.generated.h"

class UBingoGameData;
class UTextBlock;
class UUniformGridPanel;
class UButton;
class UTexture2D;
class UWidgetSwitcher;
class UBingoRewardButton;
class ABingoRadarPlayerActor;

UCLASS()
class SATISFACTORYBINGO_API UBingoTerminalWidget : public UFGInteractWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    virtual void OnEscapePressed_Implementation() override;

public:
    // Import the frame as a UI texture and assign it in the terminal widget defaults.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bingo|Appearance")
    TObjectPtr<UTexture2D> FrameTexture;

private:
    UPROPERTY(Transient)
    TObjectPtr<UBingoGameData> BingoData;

    UPROPERTY(Transient)
    TObjectPtr<UUniformGridPanel> BingoGrid;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PhaseText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ProgressText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DescriptionText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ScoreText;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UButton>> CellButtons;

    // BINGO_NATIVE_CELL_TEXT_V1
    UPROPERTY(Transient)
    TArray<TObjectPtr<UTextBlock>> CellTexts;

    UPROPERTY(Transient)
    TObjectPtr<UWidgetSwitcher> ContentTabs;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UTextBlock>> RewardTexts;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UBingoRewardButton>> RewardButtons;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RewardStatusText;

    UPROPERTY(Transient)
    TObjectPtr<ABingoRadarPlayerActor> RewardNetworkActor;

    UFUNCTION()
    void OnRewardPurchaseRequested(const FString& RewardId);

    UFUNCTION()
    void OnBingoTabClicked();

    UFUNCTION()
    void OnRewardsTabClicked();

    int32 DisplayedPhase = 1;
    int32 SelectedCell = INDEX_NONE;

    void BuildBingoInterface();
    void RefreshBingoInterface();
    // BINGO_TERMINAL_LIVE_REFRESH_V1
    FTimerHandle BingoRefreshTimer;

    void RefreshPersonalBingoData();
    void SelectCell(int32 Index);
    void ChangePhase(int32 Direction);

    UFUNCTION()
    void OnPreviousPhase();

    UFUNCTION()
    void OnNextPhase();

    UFUNCTION()
    void OnCloseClicked();

    UFUNCTION()
    void OnCellClicked();
};

