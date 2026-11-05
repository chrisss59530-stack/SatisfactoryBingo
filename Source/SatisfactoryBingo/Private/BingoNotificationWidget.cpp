#include "BingoNotificationWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Animation/WidgetAnimation.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScaleBox.h"
#include "Components/RichTextBlock.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "UObject/UObjectHash.h"

TSharedRef<SWidget> UBingoNotificationWidget::RebuildWidget()
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
    if (!WidgetTree->RootWidget && !BuildNativePresentation()) BuildNotification();
    return Super::RebuildWidget();
}

void UBingoNotificationWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    SetRenderOpacity(0.0f);
}

bool UBingoNotificationWidget::BuildNativePresentation()
{
    UWidgetBlueprintGeneratedClass* SourceClass = Cast<UWidgetBlueprintGeneratedClass>(
        LoadClass<UUserWidget>(nullptr, TEXT("/Game/FactoryGame/Interface/UI/InGame/Widget_FicsitLogoSplash.Widget_FicsitLogoSplash_C")));
    UWidgetBlueprintGeneratedClass* TreeClass = SourceClass ? SourceClass->FindWidgetTreeOwningClass() : nullptr;
    UWidgetTree* TemplateTree = TreeClass ? TreeClass->GetWidgetTreeArchetype() : nullptr;
    if (!TemplateTree || !TemplateTree->RootWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Bingo][Banniere] Presentation hudOS indisponible, utilisation du fond natif Bingo."));
        return false;
    }

    // Copy only the layout. Never construct the original welcome UserWidget:
    // its Blueprint events and player-name bindings must not run for Bingo.
    UWidgetTree* Copy = DuplicateObject<UWidgetTree>(TemplateTree, this, TEXT("BingoHudOSTree"));
    UTextBlock* Primary = Cast<UTextBlock>(Copy->FindWidget(TEXT("TextBlock_127")));
    UTextBlock* Secondary = Cast<UTextBlock>(Copy->FindWidget(TEXT("TextBlock")));
    URichTextBlock* Message = Cast<URichTextBlock>(Copy->FindWidget(TEXT("mWelcomeMessage")));
    if (!Primary || !Secondary || !Message) return false;

    UWidgetAnimation* Animation = nullptr;
    TArray<UObject*> Objects;
    GetObjectsWithOuter(SourceClass, Objects, true);
    if (TreeClass != SourceClass) GetObjectsWithOuter(TreeClass, Objects, true);
    for (UObject* Object : Objects)
    {
        UWidgetAnimation* Candidate = Cast<UWidgetAnimation>(Object);
        if (Candidate && (Candidate->GetFName() == TEXT("SpawnAnim") ||
            Candidate->GetFName() == TEXT("SpawnAnim_INST")) &&
            Candidate->GetEndTime() > Candidate->GetStartTime())
        {
            Animation = Candidate;
            if (Candidate->GetFName() == TEXT("SpawnAnim_INST")) break;
        }
    }
    if (!Animation) return false;

    WidgetTree = Copy;
    TitleText = Primary;
    SecondaryTitleText = Secondary;
    NativeMessageText = Message;
    NativeSpawnAnimation = Animation;
    NativePresentationClass = SourceClass;
    if (UWidget* Trademark = Copy->FindWidget(TEXT("TextBlock_1")))
        Trademark->SetVisibility(ESlateVisibility::Collapsed);

    FSlateFontInfo Font = Primary->GetFont();
    Font.Size = 60;
    Primary->SetFont(Font);
    Font = Secondary->GetFont();
    Font.Size = 60;
    Secondary->SetFont(Font);
    UE_LOG(LogTemp, Display, TEXT("[Bingo][Banniere] Presentation hudOS et SpawnAnim reutilisees."));
    return true;
}

void UBingoNotificationWidget::BuildNotification()
{
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;

    // HUD-style typography and rules, without replacing the game's welcome UI.
    UScaleBox* Responsive = WidgetTree->ConstructWidget<UScaleBox>();
    Responsive->SetStretch(EStretch::ScaleToFit);
    Responsive->SetStretchDirection(EStretchDirection::DownOnly);
    UCanvasPanelSlot* BannerCanvasSlot = Canvas->AddChildToCanvas(Responsive);
    BannerCanvasSlot->SetAnchors(FAnchors(0.12f, 0.16f, 0.88f, 0.16f));
    BannerCanvasSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 260.f));

    USizeBox* Design = WidgetTree->ConstructWidget<USizeBox>();
    Design->SetWidthOverride(1000.f);
    Design->SetHeightOverride(260.f);
    Responsive->SetContent(Design);

    UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
    Blur->SetBlurStrength(3.f);
    Design->SetContent(Blur);

    UBorder* Banner = WidgetTree->ConstructWidget<UBorder>();
    Banner->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.025f, 0.55f));
    Banner->SetPadding(FMargin(28.f, 14.f));
    Blur->SetContent(Banner);
    UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>();
    Banner->SetContent(Layout);

    const FLinearColor Orange(1.f, 0.55f, 0.08f, 1.f);
    auto MakeText = [this, Layout](int32 Size, const FLinearColor& Color)
    {
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
        Text->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), Size));
        Text->SetColorAndOpacity(FSlateColor(Color));
        Text->SetJustification(ETextJustify::Center);
        Layout->AddChildToVerticalBox(Text);
        return Text;
    };
    auto AddRule = [this, Layout]()
    {
        USizeBox* Height = WidgetTree->ConstructWidget<USizeBox>();
        Height->SetHeightOverride(1.f);
        UBorder* Rule = WidgetTree->ConstructWidget<UBorder>();
        Rule->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 0.65f));
        Height->SetContent(Rule);
        Layout->AddChildToVerticalBox(Height)->SetPadding(FMargin(0.f, 8.f));
    };

    UTextBlock* Brand = MakeText(18, Orange);
    Brand->SetText(FText::FromString(TEXT("FICSIT // BINGO")));
    AddRule();
    TitleText = MakeText(42, FLinearColor::White);
    AddRule();
    ChallengeText = MakeText(20, FLinearColor::White);
    ChallengeText->SetAutoWrapText(false);
    ChallengeText->SetWrapTextAt(920.f);
    PointsText = MakeText(22, Orange);
}

void UBingoNotificationWidget::ShowChallengeNotification(
    const FText& ChallengeName, int32 Points, bool bIsBingo)
{
    FBingoPendingNotification Entry;
    Entry.Description = ChallengeName;
    Entry.Points = Points;
    Entry.bIsBingo = bIsBingo;
    PendingNotifications.Add(MoveTemp(Entry));
    if (!bNotificationActive) StartNextNotification();
}

void UBingoNotificationWidget::StartNextNotification()
{
    if (!TitleText || PendingNotifications.Num() == 0 ||
        (!NativeMessageText && (!ChallengeText || !PointsText)))
    {
        return;
    }

    const FBingoPendingNotification Entry = PendingNotifications[0];
    PendingNotifications.RemoveAt(0);
    const FText Points = FText::FromString(FString::Printf(
        TEXT("+%d POINT%s"), Entry.Points, Entry.Points == 1 ? TEXT("") : TEXT("S")));
    if (NativeMessageText)
    {
        TitleText->SetText(FText::FromString(Entry.bIsBingo ? TEXT("BIN") : TEXT("DÉFI ")));
        SecondaryTitleText->SetText(FText::FromString(Entry.bIsBingo ? TEXT("GO !") : TEXT("RÉUSSI !")));
        TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        SecondaryTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.55f, 0.08f, 1.f)));
        FString Description = Entry.Description.ToString();
        Description.ReplaceInline(TEXT("&"), TEXT("&amp;"));
        Description.ReplaceInline(TEXT("<"), TEXT("&lt;"));
        Description.ReplaceInline(TEXT(">"), TEXT("&gt;"));
        NativeMessageText->SetText(FText::FromString(Description + TEXT("\n") + Points.ToString()));
    }
    else
    {
        TitleText->SetText(FText::FromString(Entry.bIsBingo ? TEXT("BINGO !") : TEXT("DÉFI RÉUSSI !")));
        TitleText->SetColorAndOpacity(FSlateColor(Entry.bIsBingo
            ? FLinearColor(1.f, 0.55f, 0.08f, 1.f) : FLinearColor::White));
        ChallengeText->SetText(Entry.Description);
        PointsText->SetText(Points);
    }
    DisplayTime = 0.f;
    DisplayDuration = Entry.bIsBingo ? 5.f : 4.f;
    bNotificationActive = true;
    bPlayingNativeAnimation = false;
    SetRenderOpacity(0.f);
    SetRenderTranslation(FVector2D::ZeroVector);
    if (NativeSpawnAnimation)
    {
        SetRenderOpacity(1.f);
        const float Duration = NativeSpawnAnimation->GetEndTime() - NativeSpawnAnimation->GetStartTime();
        bPlayingNativeAnimation = PlayAnimation(NativeSpawnAnimation, 0.f, 1,
            EUMGSequencePlayMode::Forward, Duration / DisplayDuration) != nullptr;
    }
}

void UBingoNotificationWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!bNotificationActive)
    {
        if (PendingNotifications.Num() > 0) StartNextNotification();
        return;
    }

    DisplayTime += FMath::Max(0.f, InDeltaTime);
    const float FadeIn = FMath::Clamp(DisplayTime / 0.35f, 0.f, 1.f);
    const float FadeOut = FMath::Clamp((DisplayDuration - DisplayTime) / 0.45f, 0.f, 1.f);
    const float Opacity = FMath::Min(FadeIn, FadeOut);
    if (!bPlayingNativeAnimation)
    {
        SetRenderOpacity(Opacity);
        SetRenderTranslation(FVector2D(0.f, -22.f * (1.f - Opacity)));
    }

    if (DisplayTime >= DisplayDuration)
    {
        bNotificationActive = false;
        if (bPlayingNativeAnimation) StopAnimation(NativeSpawnAnimation);
        bPlayingNativeAnimation = false;
        SetRenderOpacity(0.f);
        StartNextNotification();
    }
}
