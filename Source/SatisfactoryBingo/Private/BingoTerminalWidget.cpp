#include "BingoTerminalWidget.h"
#include "Input/Reply.h"
#include "BingoSubsystem.h"
#include "BingoGameData.h"
#include "BingoRadarPlayerActor.h"

#include "EngineUtils.h"
#include "TimerManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/Button.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/FGGameUI.h"
#include "FGPlayerController.h"

namespace
{
    const FLinearColor BingoOrange(1.f, 0.55f, 0.08f, 1.f);
    const FLinearColor BingoDark(0.025f, 0.035f, 0.045f, 0.98f);
    const FLinearColor BingoCell(0.15f, 0.18f, 0.22f, 1.f);
    const FLinearColor BingoGreen(0.06f, 0.48f, 0.22f, 1.f);
}

TSharedRef<SWidget> UBingoTerminalWidget::RebuildWidget()
{
    UE_LOG(LogTemp, Warning,
        TEXT("[BINGO_UI_158] RebuildWidget ENTER"));

    if (WidgetTree &&
        (!WidgetTree->RootWidget || CellButtons.Num() != 25))
    {
        BuildBingoInterface();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[BINGO_UI_158] Root=%s Cells=%d"),
        (WidgetTree && WidgetTree->RootWidget)
            ? TEXT("OK") : TEXT("NULL"),
        CellButtons.Num());

    return Super::RebuildWidget();
}
void UBingoTerminalWidget::NativeConstruct()
{
    Super::NativeConstruct();

    mUseKeyboard = true;
    mUseMouse = true;
    mDisablePlayerActions = true;
    mDisableBuildGunActions = true;
    SetIsFocusable(true);

    SetInputMode();

    // BINGO_PERSONAL_TERMINAL_V1
    BingoData = nullptr;

    APlayerController* LocalController = GetOwningPlayer();

    if (IsValid(LocalController) && GetWorld())
    {
        for (TActorIterator<ABingoRadarPlayerActor> It(GetWorld()); It; ++It)
        {
            ABingoRadarPlayerActor* NetworkActor = *It;

            if (!IsValid(NetworkActor) ||
                NetworkActor->GetOwner() != LocalController)
            {
                continue;
            }

            BingoData = NetworkActor->GetPersonalBingoData();
            break;
        }
    }

    DisplayedPhase = IsValid(BingoData)
        ? BingoData->CurrentPhase
        : 1;

    // Interface construite dans RebuildWidget().
    RefreshPersonalBingoData();

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            BingoRefreshTimer,
            this,
            &UBingoTerminalWidget::RefreshPersonalBingoData,
            1.0f,
            true
        );
    }
}

void UBingoTerminalWidget::RefreshPersonalBingoData()
{
    // BINGO_TERMINAL_LIVE_REFRESH_V1
    if (!GetWorld())
    {
        return;
    }

    APlayerController* LocalController = GetOwningPlayer();

    if (!IsValid(LocalController))
    {
        return;
    }

    UBingoGameData* PersonalData = nullptr;

    for (TActorIterator<ABingoRadarPlayerActor> It(GetWorld()); It; ++It)
    {
        ABingoRadarPlayerActor* NetworkActor = *It;

        UE_LOG(LogTemp, Warning,
            TEXT("[BINGO_OWNER_195] Local=%s Owner=%s Data=%s"),
            *GetNameSafe(LocalController),
            *GetNameSafe(IsValid(NetworkActor) ? NetworkActor->GetOwner() : nullptr),
            *GetNameSafe(IsValid(NetworkActor) ? NetworkActor->GetPersonalBingoData() : nullptr));

        if (!IsValid(NetworkActor) ||
            NetworkActor->GetOwner() != LocalController)
        {
            continue;
        }

        PersonalData = NetworkActor->GetPersonalBingoData();
        break;
    }

    if (IsValid(PersonalData))
    {
        if (BingoData != PersonalData)
        {
            BingoData = PersonalData;
            DisplayedPhase = FMath::Clamp(
                BingoData->CurrentPhase, 1, 5);
            SelectedCell = INDEX_NONE;
        }
    }
    else
    {
        BingoData = nullptr;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[BINGO_DATA_187] Controller=%s Data=%s Phases=%d Phase1=%s"),
        *GetNameSafe(LocalController),
        *GetNameSafe(BingoData),
        IsValid(BingoData) ? BingoData->Phases.Num() : 0,
        IsValid(BingoData) && BingoData->Phases.Num() > 0 &&
            BingoData->Phases[0].bUnlocked
            ? TEXT("DEBLOQUEE") : TEXT("ABSENTE_OU_VERROUILLEE"));

    RefreshBingoInterface();
}
void UBingoTerminalWidget::NativeDestruct()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(BingoRefreshTimer);
    }

    Super::NativeDestruct();
}

void UBingoTerminalWidget::OnEscapePressed_Implementation()
{
    OnCloseClicked();
}

void UBingoTerminalWidget::OnCloseClicked()
{
    AFGPlayerController* PC =
        Cast<AFGPlayerController>(GetOwningPlayer());

    if (PC && PC->GetGameUI())
    {
        PC->GetGameUI()->PopWidget(this);
    }
}

void UBingoTerminalWidget::OnPreviousPhase()
{
    ChangePhase(-1);
}

void UBingoTerminalWidget::OnNextPhase()
{
    ChangePhase(1);
}

void UBingoTerminalWidget::ChangePhase(int32 Direction)
{
    DisplayedPhase = FMath::Clamp(DisplayedPhase + Direction, 1, 5);
    SelectedCell = INDEX_NONE;
    RefreshBingoInterface();
}

void UBingoTerminalWidget::SelectCell(int32 Index)
{
    SelectedCell = Index;
    RefreshBingoInterface();
}

void UBingoTerminalWidget::OnCellClicked()
{
    for (int32 Index = 0; Index < CellButtons.Num(); ++Index)
    {
        UButton* Button = CellButtons[Index];

        if (Button && Button->IsHovered() && Button->GetIsEnabled())
        {
            SelectCell(Index);
            return;
        }
    }
}

void UBingoTerminalWidget::BuildBingoInterface()
{
    if (!WidgetTree)
    {
        return;
    }

    UBorder* Background =
        WidgetTree->ConstructWidget<UBorder>();

    Background->SetBrushColor(BingoDark);
    Background->SetPadding(FMargin(24.f));

    UCanvasPanel* RootCanvas =
        WidgetTree->ConstructWidget<UCanvasPanel>();

    WidgetTree->RootWidget = RootCanvas;

    UCanvasPanelSlot* BingoSlot =
        RootCanvas->AddChildToCanvas(Background);

    BingoSlot->SetAnchors(
        FAnchors(0.5f, 0.5f));

    BingoSlot->SetAlignment(
        FVector2D(0.5f, 0.5f));

    BingoSlot->SetAutoSize(true);

    BingoSlot->SetPosition(
        FVector2D::ZeroVector);

    UVerticalBox* Layout =
        WidgetTree->ConstructWidget<UVerticalBox>();

    Background->SetContent(Layout);

    UHorizontalBox* Header =
        WidgetTree->ConstructWidget<UHorizontalBox>();

    Layout->AddChildToVerticalBox(Header);

    UTextBlock* Title =
        WidgetTree->ConstructWidget<UTextBlock>();

    Title->SetText(FText::FromString(TEXT("FICSIT BINGO")));
    Title->SetColorAndOpacity(FSlateColor(BingoOrange));
    Header->AddChildToHorizontalBox(Title);

    ScoreText = WidgetTree->ConstructWidget<UTextBlock>();
    ScoreText->SetColorAndOpacity(FSlateColor(BingoOrange));
    Header->AddChildToHorizontalBox(ScoreText);

    UButton* CloseButton =
        WidgetTree->ConstructWidget<UButton>();

    UTextBlock* CloseText =
        WidgetTree->ConstructWidget<UTextBlock>();

    CloseText->SetText(FText::FromString(TEXT("X")));
    CloseButton->AddChild(CloseText);
    CloseButton->OnClicked.AddDynamic(
        this, &UBingoTerminalWidget::OnCloseClicked);

    UHorizontalBoxSlot* CloseSlot =
        Header->AddChildToHorizontalBox(CloseButton);

    CloseSlot->SetHorizontalAlignment(HAlign_Right);
    CloseSlot->SetVerticalAlignment(VAlign_Center);

    UHorizontalBoxSlot* HeaderSlot =
        Cast<UHorizontalBoxSlot>(ScoreText->Slot);

    if (HeaderSlot)
    {
        HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    UHorizontalBox* MainContent =
        WidgetTree->ConstructWidget<UHorizontalBox>();

    Layout->AddChildToVerticalBox(MainContent);

    BingoGrid =
        WidgetTree->ConstructWidget<UUniformGridPanel>();

    MainContent->AddChildToHorizontalBox(BingoGrid);

    CellButtons.Empty();
    CellTexts.Empty();

    for (int32 Index = 0; Index < 25; ++Index)
    {
        UButton* Button =
            WidgetTree->ConstructWidget<UButton>();

        USizeBox* Size =
            WidgetTree->ConstructWidget<USizeBox>();

        Size->SetWidthOverride(165.f);
        Size->SetHeightOverride(110.f);

        UTextBlock* CellText =
            WidgetTree->ConstructWidget<UTextBlock>();

        CellText->SetText(FText::GetEmpty());
        CellText->SetJustification(ETextJustify::Center);
        CellText->SetAutoWrapText(true);
        CellText->SetWrapTextAt(150.f);

        FSlateFontInfo Font = CellText->GetFont();
        Font.Size = 14;
        CellText->SetFont(Font);

        CellText->SetColorAndOpacity(
            FSlateColor(FLinearColor::White));

        Size->SetContent(CellText);
        CellTexts.Add(CellText);

        Button->AddChild(Size);
        Button->OnClicked.AddDynamic(
            this, &UBingoTerminalWidget::OnCellClicked);

        BingoGrid->AddChildToUniformGrid(
            Button, Index / 5, Index % 5);

        CellButtons.Add(Button);
    }

    UBorder* DetailPanel =
        WidgetTree->ConstructWidget<UBorder>();

    DetailPanel->SetPadding(FMargin(20.f));
    DetailPanel->SetBrushColor(
        FLinearColor(0.13f, 0.15f, 0.18f, 1.f));

    MainContent->AddChildToHorizontalBox(DetailPanel);

    UVerticalBox* DetailLayout =
        WidgetTree->ConstructWidget<UVerticalBox>();

    DetailPanel->SetContent(DetailLayout);

    UTextBlock* DetailTitle =
        WidgetTree->ConstructWidget<UTextBlock>();

    DetailTitle->SetText(
        FText::FromString(TEXT("DEFI SELECTIONNE")));

    DetailTitle->SetColorAndOpacity(
        FSlateColor(BingoOrange));

    DetailLayout->AddChildToVerticalBox(DetailTitle);

    DescriptionText =
        WidgetTree->ConstructWidget<UTextBlock>();

    DescriptionText->SetAutoWrapText(true);
    DescriptionText->SetWrapTextAt(350.f);
    DetailLayout->AddChildToVerticalBox(DescriptionText);

    UHorizontalBox* PhaseNavigation =
        WidgetTree->ConstructWidget<UHorizontalBox>();

    Layout->AddChildToVerticalBox(PhaseNavigation);

    UButton* Previous =
        WidgetTree->ConstructWidget<UButton>();

    UTextBlock* PreviousText =
        WidgetTree->ConstructWidget<UTextBlock>();

    PreviousText->SetText(FText::FromString(TEXT("<")));
    Previous->AddChild(PreviousText);
    Previous->OnClicked.AddDynamic(
        this, &UBingoTerminalWidget::OnPreviousPhase);

    PhaseNavigation->AddChildToHorizontalBox(Previous);

    PhaseText = WidgetTree->ConstructWidget<UTextBlock>();
    PhaseNavigation->AddChildToHorizontalBox(PhaseText);

    UButton* Next =
        WidgetTree->ConstructWidget<UButton>();

    UTextBlock* NextText =
        WidgetTree->ConstructWidget<UTextBlock>();

    NextText->SetText(FText::FromString(TEXT(">")));
    Next->AddChild(NextText);
    Next->OnClicked.AddDynamic(
        this, &UBingoTerminalWidget::OnNextPhase);

    PhaseNavigation->AddChildToHorizontalBox(Next);

    ProgressText =
        WidgetTree->ConstructWidget<UTextBlock>();

    Layout->AddChildToVerticalBox(ProgressText);

    UE_LOG(LogTemp, Warning,
        TEXT("[BINGO_UI_175] Interface simplifiee construite"));
}
void UBingoTerminalWidget::RefreshBingoInterface()
{
    if (!PhaseText || !ProgressText || !DescriptionText || !ScoreText)
    {
        return;
    }

    PhaseText->SetText(FText::FromString(
        FString::Printf(TEXT("  PHASE %d / 5  "), DisplayedPhase)));

    const FBingoPhase* Phase = nullptr;

    if (IsValid(BingoData))
    {
        for (const FBingoPhase& Candidate : BingoData->Phases)
        {
            if (Candidate.PhaseNumber == DisplayedPhase)
            {
                Phase = &Candidate;
                break;
            }
        }
    }

    const bool bUnlocked = Phase && Phase->bUnlocked;

    int32 Completed = 0;

    for (int32 Index = 0; Index < CellButtons.Num(); ++Index)
    {
        UButton* Button = CellButtons[Index];

        if (!Button)
        {
            continue;
        }

        const bool bValid =
            bUnlocked &&
            Phase->Challenges.IsValidIndex(Index);

        const bool bCompleted =
            bValid && Phase->Challenges[Index].bCompleted;

        if (bCompleted)
        {
            ++Completed;
        }

        // BINGO_NATIVE_CELL_REFRESH_V1
        if (CellTexts.IsValidIndex(Index) &&
            IsValid(CellTexts[Index]))
        {
            UTextBlock* CellText = CellTexts[Index];

            if (Phase && Phase->Challenges.IsValidIndex(Index))
            {
                CellText->SetText(
                    Index == 12
                        ? FText::FromString(TEXT("CASE LIBRE"))
                        : Phase->Challenges[Index].Description);
            }
            else
            {
                CellText->SetText(FText::GetEmpty());
            }

            CellText->SetColorAndOpacity(
                FSlateColor(FLinearColor::White));
        }

        Button->SetIsEnabled(bValid);

        Button->SetBackgroundColor(
            !bValid
                ? FLinearColor(0.08f, 0.08f, 0.08f, 1.f)
                : bCompleted
                    ? BingoGreen
                    : Index == SelectedCell
                        ? BingoOrange
                        : BingoCell);
    }

    ProgressText->SetText(FText::FromString(
        bUnlocked
            ? FString::Printf(
                TEXT("DEFIS : %d / 25 | BINGOS : %d"),
                Completed,
                Phase->BingoCount)
            : TEXT("PHASE VERROUILLEE")));

    if (!bUnlocked)
    {
        DescriptionText->SetText(
            FText::FromString(TEXT("Cette phase n'est pas encore debloquee.")));
    }
    else if (Phase->Challenges.IsValidIndex(SelectedCell))
    {
        const FBingoChallenge& Challenge =
            Phase->Challenges[SelectedCell];

        DescriptionText->SetText(FText::FromString(
            FString::Printf(
                TEXT("%s\n\n%s\n\n%d POINTS | %s"),
                *Challenge.ChallengeId,
                *Challenge.Description.ToString(),
                Challenge.Points,
                Challenge.bCompleted
                    ? TEXT("TERMINE")
                    : TEXT("EN COURS"))));
    }
    else
    {
        DescriptionText->SetText(
            FText::FromString(TEXT("Selectionnez une case pour voir son defi.")));
    }

    ScoreText->SetText(FText::FromString(
        FString::Printf(TEXT("SCORE TOTAL : %d"),
            IsValid(BingoData) ? BingoData->TotalScore : 0)));
}



FReply UBingoTerminalWidget::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
