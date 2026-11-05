#include "BingoTerminalBuildable.h"
#include "BingoTerminalWidget.h"
#include "FGCharacterPlayer.h"
#include "FGPlayerController.h"
#include "UI/FGGameUI.h"
#include "Components/BoxComponent.h"
#include "FGPowerConnectionComponent.h"
#include "FGPowerInfoComponent.h"

ABingoTerminalBuildable::ABingoTerminalBuildable()
{
    InteractionBox = CreateDefaultSubobject<UBoxComponent>(
        TEXT("BingoInteractionBox")
    );

    InteractionBox->SetupAttachment(RootComponent);
    InteractionBox->SetBoxExtent(FVector(160.f, 160.f, 180.f));
    InteractionBox->SetRelativeLocation(FVector(0.f, 0.f, 180.f));

    InteractionBox->SetCollisionEnabled(
        ECollisionEnabled::QueryOnly
    );

    InteractionBox->SetCollisionResponseToAllChannels(
        ECR_Ignore
    );

    InteractionBox->SetCollisionResponseToChannel(
        ECC_GameTraceChannel13,
        ECR_Block
    );

    InteractionBox->SetCollisionResponseToChannel(
        ECC_GameTraceChannel5,
        ECR_Block
    );

    InteractionBox->SetGenerateOverlapEvents(false);

    BingoPowerInfo = CreateDefaultSubobject<UFGPowerInfoComponent>(
        TEXT("BingoPowerInfo")
    );

    BingoPowerConnection =
        CreateDefaultSubobject<UFGPowerConnectionComponent>(
            TEXT("BingoPowerConnection")
        );

    BingoPowerConnection->SetupAttachment(RootComponent);

    // Position provisoire : cote droit du terminal.
    BingoPowerConnection->SetRelativeLocation(
        FVector(0.f, 110.f, 180.f)
    );
}

void ABingoTerminalBuildable::BeginPlay()
{
    Super::BeginPlay();

    if (BingoPowerConnection && BingoPowerInfo)
    {
        BingoPowerConnection->SetPowerInfo(BingoPowerInfo);

        if (HasAuthority())
        {
            BingoPowerInfo->SetMaximumTargetConsumption(
                BingoPowerConsumptionMW
            );

            BingoPowerInfo->SetTargetConsumption(
                BingoPowerConsumptionMW
            );
        }
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[SatisfactoryBingo] Terminal initialized: %s"),
        *GetName()
    );
}

bool ABingoTerminalBuildable::IsBingoPowered() const
{
    return BingoPowerInfo &&
           BingoPowerInfo->IsConnected() &&
           BingoPowerInfo->HasPower();
}

// BINGO_POWER_INTERACTION_FIX_01
bool ABingoTerminalBuildable::IsUseable_Implementation() const
{
    return Super::IsUseable_Implementation() && IsBingoPowered();
}


void ABingoTerminalBuildable::OnUse_Implementation(
    AFGCharacterPlayer* byCharacter,
    const FUseState& state)
{
    UE_LOG(LogTemp, Warning,
        TEXT("[BINGO_DEBUG] ETAPE 1 : OnUse appele"));

    if (!IsBingoPowered() || !byCharacter)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[BINGO_DEBUG] ECHEC : alimentation ou personnage"));
        return;
    }

    AFGPlayerController* PC =
        Cast<AFGPlayerController>(byCharacter->GetController());

    if (!PC)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[BINGO_DEBUG] ECHEC : PlayerController absent"));
        return;
    }

    if (!PC->IsLocalController())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[BINGO_DEBUG] ECHEC : controleur non local"));
        return;
    }

    UFGGameUI* GameUI = PC->GetGameUI();

    if (!GameUI)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[BINGO_DEBUG] ECHEC : GameUI absent"));
        return;
    }

    TSubclassOf<UFGInteractWidget> WidgetClass =
        UBingoTerminalWidget::StaticClass();

    if (!WidgetClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[BINGO_DEBUG] ECHEC : WidgetClass absent"));
        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[BINGO_DEBUG] ETAPE 2 : ouverture demandee"));

        UFGInteractWidget* BingoWidget =
        CreateWidget<UFGInteractWidget>(PC, WidgetClass);

    if (!BingoWidget)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[BINGO_DEBUG] ECHEC : CreateWidget"));
        return;
    }

    if (UBingoTerminalWidget* Terminal = Cast<UBingoTerminalWidget>(BingoWidget))
    {
        Terminal->FrameTexture = BingoFrameTexture;
    }

    BingoWidget->mInteractObject = this;

    UE_LOG(LogTemp, Warning,
        TEXT("[BINGO_DEBUG] Widget cree : %s"),
        *BingoWidget->GetName());

    // BINGO_SINGLE_INTERACT_WIDGET_V1
    GameUI->PushWidget(BingoWidget);

    UE_LOG(LogTemp, Warning,
        TEXT("[BINGO_DEBUG] Ouverture via PushWidget"));

    UE_LOG(LogTemp, Warning,
        TEXT("[BINGO_DEBUG] ETAPE 3 : appel termine"));
}





