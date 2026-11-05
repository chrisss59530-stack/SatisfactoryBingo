#pragma once

#include "CoreMinimal.h"
#include "Buildables/FGBuildable.h"
#include "BingoTerminalBuildable.generated.h"

class UBoxComponent;
class UTexture2D;
class UFGPowerConnectionComponent;
class UFGPowerInfoComponent;

UCLASS()
class SATISFACTORYBINGO_API ABingoTerminalBuildable : public AFGBuildable
{
    GENERATED_BODY()

public:
    ABingoTerminalBuildable();

    UFUNCTION(BlueprintPure, Category="Bingo|Power")
    bool IsBingoPowered() const;

protected:
    virtual void BeginPlay() override;
    virtual bool IsUseable_Implementation() const override;
    virtual void OnUse_Implementation(class AFGCharacterPlayer* byCharacter, const FUseState& state) override;

private:
    // Hard reference ensures the frame is included when cooking the terminal asset.
    UPROPERTY(EditDefaultsOnly, Category="Bingo|Appearance")
    TObjectPtr<UTexture2D> BingoFrameTexture;

    UPROPERTY(VisibleAnywhere, Category="Bingo|Interaction")
    TObjectPtr<UBoxComponent> InteractionBox;

    UPROPERTY(VisibleAnywhere, Category="Bingo|Power")
    TObjectPtr<UFGPowerConnectionComponent> BingoPowerConnection;

    UPROPERTY(VisibleAnywhere, Category="Bingo|Power")
    TObjectPtr<UFGPowerInfoComponent> BingoPowerInfo;

    UPROPERTY(EditDefaultsOnly, Category="Bingo|Power",
        meta=(ClampMin="0.0"))
    float BingoPowerConsumptionMW = 1.0f;
};
