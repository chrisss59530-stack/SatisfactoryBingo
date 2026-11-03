#include "BingoNotificationWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

void UBingoNotificationWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::HitTestInvisible);

    BuildNotification();
    SetRenderOpacity(0.0f);
}

void UBingoNotificationWidget::BuildNotification()
{
    if (!WidgetTree)
    {
        return;
    }

    UCanvasPanel* Canvas =
        WidgetTree->ConstructWidget<UCanvasPanel>();

    WidgetTree->RootWidget = Canvas;

    UBorder* Banner =
        WidgetTree->ConstructWidget<UBorder>();

    Banner->SetBrushColor(
        FLinearColor(0.015f, 0.02f, 0.025f, 0.85f));

    Banner->SetPadding(FMargin(20.f, 12.f));

    UCanvasPanelSlot* BannerSlot =
        Canvas->AddChildToCanvas(Banner);

    BannerSlot->SetAnchors(
        FAnchors(0.5f, 0.35f));

    BannerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    BannerSlot->SetSize(FVector2D(760.f, 175.f));
    BannerSlot->SetPosition(FVector2D::ZeroVector);

    UVerticalBox* Layout =
        WidgetTree->ConstructWidget<UVerticalBox>();

    Banner->SetContent(Layout);

    UTextBlock* Brand =
        WidgetTree->ConstructWidget<UTextBlock>();

    Brand->SetText(
        FText::FromString(TEXT("FICSIT  BINGO")));

    Brand->SetColorAndOpacity(
        FSlateColor(FLinearColor(1.f, 0.55f, 0.08f)));

    Brand->SetJustification(ETextJustify::Center);
    Brand->SetFont(
        FSlateFontInfo(FCoreStyle::GetDefaultFont(), 18));

    Layout->AddChildToVerticalBox(Brand);

    TitleText =
        WidgetTree->ConstructWidget<UTextBlock>();

    TitleText->SetText(
        FText::FromString(TEXT("DEFI REUSSI !")));

    TitleText->SetColorAndOpacity(
        FSlateColor(FLinearColor::White));

    TitleText->SetJustification(ETextJustify::Center);
    TitleText->SetFont(
        FSlateFontInfo(FCoreStyle::GetDefaultFont(), 34));

    Layout->AddChildToVerticalBox(TitleText);

    ChallengeText =
        WidgetTree->ConstructWidget<UTextBlock>();

    ChallengeText->SetJustification(ETextJustify::Center);
    ChallengeText->SetAutoWrapText(true);
    ChallengeText->SetColorAndOpacity(
        FSlateColor(FLinearColor::White));

    Layout->AddChildToVerticalBox(ChallengeText);

    PointsText =
        WidgetTree->ConstructWidget<UTextBlock>();

    PointsText->SetJustification(ETextJustify::Center);
    PointsText->SetColorAndOpacity(
        FSlateColor(FLinearColor(1.f, 0.55f, 0.08f)));

    Layout->AddChildToVerticalBox(PointsText);
}

void UBingoNotificationWidget::ShowChallengeNotification(
    const FText& ChallengeName,
    int32 Points,
    bool bIsBingo)
{
    if (!TitleText || !ChallengeText || !PointsText)
    {
        return;
    }

    TitleText->SetText(
        FText::FromString(
            bIsBingo
                ? TEXT("BINGO !")
                : TEXT("DEFI REUSSI !")));

    ChallengeText->SetText(ChallengeName);

    PointsText->SetText(
        FText::FromString(
            FString::Printf(TEXT("+%d POINTS"), Points)));

    DisplayTime = 0.0f;
    bNotificationActive = true;

    SetRenderOpacity(0.0f);
}

void UBingoNotificationWidget::NativeTick(
    const FGeometry& MyGeometry,
    float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bNotificationActive)
    {
        return;
    }

    DisplayTime += InDeltaTime;

    constexpr float FadeIn = 0.3f;
    constexpr float TotalDuration = 3.0f;
    constexpr float FadeOut = 0.3f;

    float Opacity = 1.0f;

    if (DisplayTime < FadeIn)
    {
        Opacity = DisplayTime / FadeIn;
    }
    else if (DisplayTime > TotalDuration - FadeOut)
    {
        Opacity =
            (TotalDuration - DisplayTime) / FadeOut;
    }

    SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));

    if (DisplayTime >= TotalDuration)
    {
        bNotificationActive = false;
        SetRenderOpacity(0.0f);
    }
}
