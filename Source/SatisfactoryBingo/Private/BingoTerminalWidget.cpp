#include "BingoTerminalWidget.h"
#include "Input/Reply.h"
#include "BingoSubsystem.h"
#include "BingoGameData.h"
#include "BingoRewards.h"
#include "BingoRewardButton.h"
#include "Components/WidgetSwitcher.h"
#include "Components/ScrollBox.h"
#include "BingoRadarPlayerActor.h"

#include "EngineUtils.h"
#include "TimerManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Engine/Texture2D.h"
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
#include "Components/ButtonSlot.h"
#include "Components/ScrollBoxSlot.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/FGGameUI.h"
#include "FGPlayerController.h"

namespace
{
    const FLinearColor BingoOrange(1.f, 0.55f, 0.08f, 1.f);
    const FLinearColor BingoDark(0.025f, 0.035f, 0.045f, 1.f);
    const FLinearColor BingoCell(0.15f, 0.18f, 0.22f, 1.f);
    const FLinearColor BingoGreen(0.06f, 0.48f, 0.22f, 1.f);

    void StyleBingoControl(UButton* Button)
    {
        FButtonStyle ButtonStyle = Button->GetStyle();
        ButtonStyle.Normal = FSlateColorBrush(FLinearColor(0.045f, 0.06f, 0.075f, 1.f));
        ButtonStyle.Hovered = FSlateColorBrush(FLinearColor(0.32f, 0.12f, 0.02f, 1.f));
        ButtonStyle.Pressed = FSlateColorBrush(FLinearColor(0.48f, 0.18f, 0.025f, 1.f));
        ButtonStyle.Disabled = FSlateColorBrush(FLinearColor(0.025f, 0.032f, 0.04f, 1.f));
        ButtonStyle.NormalPadding = FMargin(14.f, 9.f);
        ButtonStyle.PressedPadding = FMargin(14.f, 10.f, 14.f, 8.f);
        Button->SetStyle(ButtonStyle);
        Button->SetBackgroundColor(FLinearColor::White);
    }

    void StyleBingoTab(UButton* Button)
    {
        StyleBingoControl(Button);
        FButtonStyle TabStyle = Button->GetStyle();
        TabStyle.Normal = FSlateColorBrush(BingoOrange);
        TabStyle.Hovered = FSlateColorBrush(FLinearColor(1.f, 0.72f, 0.25f, 1.f));
        TabStyle.Pressed = FSlateColorBrush(FLinearColor(0.85f, 0.35f, 0.02f, 1.f));
        Button->SetStyle(TabStyle);
    }
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
    RewardNetworkActor = nullptr;
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
        RewardNetworkActor = NetworkActor;
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

    // A fixed design canvas scales as one unit, preserving the frame and grid proportions.
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = RootCanvas;
    UScaleBox* Responsive = WidgetTree->ConstructWidget<UScaleBox>();
    Responsive->SetStretch(EStretch::ScaleToFit);
    UCanvasPanelSlot* ResponsiveSlot = RootCanvas->AddChildToCanvas(Responsive);
    ResponsiveSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    ResponsiveSlot->SetOffsets(FMargin(24.f));

    USizeBox* DesignSize = WidgetTree->ConstructWidget<USizeBox>();
    DesignSize->SetWidthOverride(1672.f);
    DesignSize->SetHeightOverride(941.f);
    Responsive->SetContent(DesignSize);
    UCanvasPanel* Design = WidgetTree->ConstructWidget<UCanvasPanel>();
    DesignSize->SetContent(Design);

    // Solid fallback keeps the terminal usable before the frame texture is imported.
    UBorder* Shell = WidgetTree->ConstructWidget<UBorder>();
    Shell->SetBrushColor(BingoOrange);
    Shell->SetVisibility(ESlateVisibility::HitTestInvisible);
    UCanvasPanelSlot* ShellSlot = Design->AddChildToCanvas(Shell);
    ShellSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    ShellSlot->SetOffsets(FMargin(30.f, 14.f, 30.f, 20.f));

    // Opaque backing overlaps the opening underneath the decorative frame.
    // Keep the actual controls inset; only the backing extends under the rim.
    UBorder* InteriorBacking = WidgetTree->ConstructWidget<UBorder>();
    InteriorBacking->SetBrushColor(BingoDark);
    InteriorBacking->SetVisibility(ESlateVisibility::HitTestInvisible);
    UCanvasPanelSlot* BackingSlot = Design->AddChildToCanvas(InteriorBacking);
    BackingSlot->SetPosition(FVector2D(122.f, 118.f));
    BackingSlot->SetSize(FVector2D(1428.f, 704.f));

    if (FrameTexture)
    {
        Shell->SetVisibility(ESlateVisibility::Collapsed);
        UImage* Frame = WidgetTree->ConstructWidget<UImage>();
        Frame->SetBrushFromTexture(FrameTexture);
        Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
        UCanvasPanelSlot* FrameSlot = Design->AddChildToCanvas(Frame);
        FrameSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        FrameSlot->SetOffsets(FMargin(0.f));
    }

    UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
    Background->SetBrushColor(BingoDark);
    Background->SetPadding(FMargin(22.f));
    UCanvasPanelSlot* BingoSlot = Design->AddChildToCanvas(Background);
    // Interior of the supplied 1672 x 941 frame; no controls cover the orange rim.
    BingoSlot->SetPosition(FVector2D(136.f, 132.f));
    BingoSlot->SetSize(FVector2D(1400.f, 676.f));

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
    FSlateFontInfo TitleFont = Title->GetFont();
    TitleFont.Size = 22;
    Title->SetFont(TitleFont);
    Header->AddChildToHorizontalBox(Title)->SetPadding(FMargin(0.f, 0.f, 32.f, 0.f));

    ScoreText = WidgetTree->ConstructWidget<UTextBlock>();
    ScoreText->SetColorAndOpacity(FSlateColor(BingoOrange));
    FSlateFontInfo ScoreFont = ScoreText->GetFont();
    ScoreFont.Size = 20;
    ScoreText->SetFont(ScoreFont);
    ScoreText->SetJustification(ETextJustify::Right);
    Header->AddChildToHorizontalBox(ScoreText);

    UButton* CloseButton =
        WidgetTree->ConstructWidget<UButton>();
    StyleBingoControl(CloseButton);

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
        HeaderSlot->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));
    }

    UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>();
    Layout->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
    UButton* BingoTab = WidgetTree->ConstructWidget<UButton>();
    StyleBingoTab(BingoTab);
    UTextBlock* BingoLabel = WidgetTree->ConstructWidget<UTextBlock>();
    BingoLabel->SetText(FText::FromString(TEXT("BINGO")));
    FSlateFontInfo TabFont = BingoLabel->GetFont();
    TabFont.Size = 18;
    BingoLabel->SetFont(TabFont);
    BingoLabel->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    BingoTab->SetContent(BingoLabel);
    BingoTab->OnClicked.AddDynamic(this, &UBingoTerminalWidget::OnBingoTabClicked);
    Tabs->AddChildToHorizontalBox(BingoTab)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
    UButton* RewardsTab = WidgetTree->ConstructWidget<UButton>();
    StyleBingoTab(RewardsTab);
    UTextBlock* RewardsLabel = WidgetTree->ConstructWidget<UTextBlock>();
    RewardsLabel->SetText(FText::FromString(TEXT("RÉCOMPENSES")));
    RewardsLabel->SetFont(TabFont);
    RewardsLabel->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    RewardsTab->SetContent(RewardsLabel);
    RewardsTab->OnClicked.AddDynamic(this, &UBingoTerminalWidget::OnRewardsTabClicked);
    Tabs->AddChildToHorizontalBox(RewardsTab);

    ContentTabs = WidgetTree->ConstructWidget<UWidgetSwitcher>();
    Layout->AddChildToVerticalBox(ContentTabs)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UVerticalBox* BingoPage = WidgetTree->ConstructWidget<UVerticalBox>();
    ContentTabs->AddChild(BingoPage);
    UScrollBox* RewardsPage = WidgetTree->ConstructWidget<UScrollBox>();
    ContentTabs->AddChild(RewardsPage);
    RewardTexts.Empty();
    RewardButtons.Empty();
    RewardStatusText = WidgetTree->ConstructWidget<UTextBlock>();
    RewardStatusText->SetAutoWrapText(true);
    RewardStatusText->SetColorAndOpacity(FSlateColor(BingoOrange));
    UScrollBoxSlot* RewardStatusSlot = Cast<UScrollBoxSlot>(RewardsPage->AddChild(RewardStatusText));
    RewardStatusSlot->SetPadding(FMargin(0.f, 8.f, 8.f, 12.f));
    for (const FBingoRewardDefinition& Reward : FBingoRewards::GetAll())
    {
        UBingoRewardButton* BuyButton = WidgetTree->ConstructWidget<UBingoRewardButton>();
        StyleBingoControl(BuyButton);
        BuyButton->InitializeReward(Reward.Id);
        BuyButton->OnRewardSelected.AddDynamic(this, &UBingoTerminalWidget::OnRewardPurchaseRequested);
        UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>();
        Row->SetAutoWrapText(true);
        FSlateFontInfo RewardFont = Row->GetFont();
        RewardFont.Size = 18;
        Row->SetFont(RewardFont);
        Row->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        BuyButton->SetContent(Row);
        UScrollBoxSlot* RewardRowSlot = Cast<UScrollBoxSlot>(RewardsPage->AddChild(BuyButton));
        RewardRowSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 8.f));
        RewardButtons.Add(BuyButton);
        RewardTexts.Add(Row);
    }
    ContentTabs->SetActiveWidgetIndex(0);

    UHorizontalBox* MainContent =
        WidgetTree->ConstructWidget<UHorizontalBox>();

    UVerticalBoxSlot* MainSlot = BingoPage->AddChildToVerticalBox(MainContent);
    MainSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    MainSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 12.f));

    BingoGrid =
        WidgetTree->ConstructWidget<UUniformGridPanel>();

    BingoGrid->SetSlotPadding(FMargin(4.f));
    UHorizontalBoxSlot* GridSlot = MainContent->AddChildToHorizontalBox(BingoGrid);
    FSlateChildSize GridWidth(ESlateSizeRule::Fill);
    GridWidth.Value = 0.72f;
    GridSlot->SetSize(GridWidth);

    CellButtons.Empty();
    CellTexts.Empty();

    for (int32 Index = 0; Index < 25; ++Index)
    {
        UButton* Button =
            WidgetTree->ConstructWidget<UButton>();

        UTextBlock* CellText =
            WidgetTree->ConstructWidget<UTextBlock>();

        CellText->SetText(FText::GetEmpty());
        CellText->SetJustification(ETextJustify::Center);
        CellText->SetAutoWrapText(true);
        CellText->SetWrapTextAt(0.f);
        CellText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);

        FSlateFontInfo Font = CellText->GetFont();
        Font.Size = 14;
        CellText->SetFont(Font);

        CellText->SetColorAndOpacity(
            FSlateColor(FLinearColor::White));

        CellTexts.Add(CellText);

        UButtonSlot* CellSlot = Cast<UButtonSlot>(Button->AddChild(CellText));
        CellSlot->SetPadding(FMargin(4.f));
        CellSlot->SetHorizontalAlignment(HAlign_Fill);
        CellSlot->SetVerticalAlignment(VAlign_Fill);
        Button->SetClipping(EWidgetClipping::ClipToBounds);
        Button->OnClicked.AddDynamic(
            this, &UBingoTerminalWidget::OnCellClicked);

        UUniformGridSlot* GridCellSlot = BingoGrid->AddChildToUniformGrid(
            Button, Index / 5, Index % 5);
        GridCellSlot->SetHorizontalAlignment(HAlign_Fill);
        GridCellSlot->SetVerticalAlignment(VAlign_Fill);

        CellButtons.Add(Button);
    }

    UBorder* DetailPanel =
        WidgetTree->ConstructWidget<UBorder>();

    DetailPanel->SetPadding(FMargin(16.f));
    DetailPanel->SetBrushColor(
        FLinearColor(0.13f, 0.15f, 0.18f, 1.f));

    UHorizontalBoxSlot* DetailSlot = MainContent->AddChildToHorizontalBox(DetailPanel);
    FSlateChildSize DetailWidth(ESlateSizeRule::Fill);
    DetailWidth.Value = 0.28f;
    DetailSlot->SetSize(DetailWidth);
    DetailSlot->SetPadding(FMargin(18.f, 0.f, 0.f, 0.f));

    UVerticalBox* DetailLayout =
        WidgetTree->ConstructWidget<UVerticalBox>();

    UScrollBox* DetailScroll = WidgetTree->ConstructWidget<UScrollBox>();
    DetailScroll->AddChild(DetailLayout);
    DetailPanel->SetContent(DetailScroll);

    UTextBlock* DetailTitle =
        WidgetTree->ConstructWidget<UTextBlock>();

    DetailTitle->SetText(
        FText::FromString(TEXT("DEFI SELECTIONNE")));

    DetailTitle->SetColorAndOpacity(
        FSlateColor(BingoOrange));
    FSlateFontInfo DetailFont = DetailTitle->GetFont();
    DetailFont.Size = 18;
    DetailTitle->SetFont(DetailFont);
    DetailTitle->SetAutoWrapText(true);

    DetailLayout->AddChildToVerticalBox(DetailTitle);

    DescriptionText =
        WidgetTree->ConstructWidget<UTextBlock>();

    DescriptionText->SetAutoWrapText(true);
    DescriptionText->SetFont(DetailFont);
    DetailLayout->AddChildToVerticalBox(DescriptionText)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

    UHorizontalBox* PhaseNavigation =
        WidgetTree->ConstructWidget<UHorizontalBox>();

    BingoPage->AddChildToVerticalBox(PhaseNavigation);

    UButton* Previous =
        WidgetTree->ConstructWidget<UButton>();
    StyleBingoControl(Previous);

    UTextBlock* PreviousText =
        WidgetTree->ConstructWidget<UTextBlock>();

    PreviousText->SetText(FText::FromString(TEXT("<")));
    Previous->AddChild(PreviousText);
    Previous->OnClicked.AddDynamic(
        this, &UBingoTerminalWidget::OnPreviousPhase);

    PhaseNavigation->AddChildToHorizontalBox(Previous);

    PhaseText = WidgetTree->ConstructWidget<UTextBlock>();
    PhaseText->SetFont(DetailFont);
    UHorizontalBoxSlot* PhaseSlot = PhaseNavigation->AddChildToHorizontalBox(PhaseText);
    PhaseSlot->SetPadding(FMargin(16.f, 4.f));

    UButton* Next =
        WidgetTree->ConstructWidget<UButton>();
    StyleBingoControl(Next);

    UTextBlock* NextText =
        WidgetTree->ConstructWidget<UTextBlock>();

    NextText->SetText(FText::FromString(TEXT(">")));
    Next->AddChild(NextText);
    Next->OnClicked.AddDynamic(
        this, &UBingoTerminalWidget::OnNextPhase);

    PhaseNavigation->AddChildToHorizontalBox(Next);

    ProgressText =
        WidgetTree->ConstructWidget<UTextBlock>();
    ProgressText->SetFont(DetailFont);

    UVerticalBoxSlot* ProgressSlot = BingoPage->AddChildToVerticalBox(ProgressText);
    ProgressSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
    ProgressText->SetColorAndOpacity(FSlateColor(BingoOrange));

    UE_LOG(LogTemp, Warning,
        TEXT("[BINGO_UI_175] Interface simplifiee construite"));
}
void UBingoTerminalWidget::RefreshBingoInterface()
{
    if (!PhaseText || !ProgressText || !DescriptionText || !ScoreText)
    {
        return;
    }

    const TArray<FBingoRewardDefinition> Rewards = FBingoRewards::GetAll();
    const bool bCanRequest = IsValid(RewardNetworkActor) &&
        RewardNetworkActor->GetOwner() == GetOwningPlayer();
    const bool bPending = bCanRequest && RewardNetworkActor->IsRewardPurchasePending();
    if (RewardStatusText)
    {
        RewardStatusText->SetText(!bCanRequest
            ? FText::FromString(TEXT("Connexion a votre bingo personnel..."))
            : RewardNetworkActor->GetRewardPurchaseMessage().IsEmpty()
                ? FText::FromString(TEXT("Choisissez une recompense. Les points sont debites apres attribution."))
                : RewardNetworkActor->GetRewardPurchaseMessage());
    }
    for (int32 Index = 0; Index < Rewards.Num() && Index < RewardTexts.Num(); ++Index)
    {
        const FBingoRewardDefinition& Reward = Rewards[Index];
        const bool bPhaseReady = IsValid(BingoData) && BingoData->CurrentPhase >= Reward.RequiredPhase;
        const bool bAffordable = bPhaseReady && BingoData->TotalScore >= Reward.Cost;
        RewardTexts[Index]->SetText(FText::FromString(FString::Printf(
            TEXT("%s  |  %d points  |  Phase %d  |  %s"),
            *Reward.Name, Reward.Cost, Reward.RequiredPhase,
            !IsValid(BingoData) ? TEXT("Synchronisation...") : !bPhaseReady ? TEXT("Verrouillée")
                : bPending ? TEXT("Achat en cours...")
                : bAffordable ? TEXT("ACHETER") : TEXT("Points insuffisants"))));
        RewardTexts[Index]->SetColorAndOpacity(FSlateColor(bAffordable
            ? FLinearColor::White : FLinearColor(0.65f, 0.68f, 0.72f, 1.f)));
        if (RewardButtons.IsValidIndex(Index) && IsValid(RewardButtons[Index]))
        {
            RewardButtons[Index]->SetIsEnabled(bCanRequest && !bPending && bAffordable);
        }
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
        Button->SetToolTipText(Phase && Phase->Challenges.IsValidIndex(Index)
            ? Phase->Challenges[Index].Description : FText::GetEmpty());

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
        DescriptionText->SetText(FText::FromString(IsValid(BingoData)
            ? TEXT("Cette phase n'est pas encore debloquee.")
            : TEXT("Synchronisation de votre bingo personnel...")));
    }
    else if (Phase->Challenges.IsValidIndex(SelectedCell))
    {
        const FBingoChallenge& Challenge =
            Phase->Challenges[SelectedCell];

        DescriptionText->SetText(FText::FromString(
            FString::Printf(
                TEXT("%s\n\n%d POINTS | %s"),
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

void UBingoTerminalWidget::OnBingoTabClicked()
{
    if (ContentTabs) ContentTabs->SetActiveWidgetIndex(0);
}

void UBingoTerminalWidget::OnRewardsTabClicked()
{
    if (ContentTabs) ContentTabs->SetActiveWidgetIndex(1);
    RefreshBingoInterface();
}

void UBingoTerminalWidget::OnRewardPurchaseRequested(const FString& RewardId)
{
    if (!IsValid(RewardNetworkActor) ||
        RewardNetworkActor->GetOwner() != GetOwningPlayer())
    {
        return;
    }

    RewardNetworkActor->RequestRewardPurchase(RewardId);
    RefreshPersonalBingoData();
}
