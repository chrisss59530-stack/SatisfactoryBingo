#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Blueprint.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

namespace BingoFigmaAudit
{
    static void Audit()
    {
        UE_LOG(LogTemp, Warning, TEXT("=== BINGO FIGMA AUDIT V2 EXECUTE ==="));

        const FString Path = TEXT("/SatisfactoryBingo/Bingo/UI/FigmaV3/Sans_titre");

        UObject* Asset = LoadObject<UObject>(nullptr, *Path);

        if (!Asset)
        {
            UE_LOG(LogTemp, Error, TEXT("[BINGO AUDIT] Asset introuvable"));
            return;
        }

        UClass* GeneratedClass = nullptr;

        if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
        {
            GeneratedClass = Blueprint->GeneratedClass;
        }

        if (!GeneratedClass)
        {
            UE_LOG(LogTemp, Error, TEXT("[BINGO AUDIT] Classe generee absente"));
            return;
        }

        UWidgetBlueprint* WidgetClass =
            Cast<UWidgetBlueprint>(Asset);

        if (!WidgetClass || !WidgetClass->WidgetTree)
        {
            UE_LOG(LogTemp, Error, TEXT("[BINGO AUDIT] WidgetTree indisponible"));
            return;
        }

        TArray<UWidget*> Widgets;
        WidgetClass->WidgetTree->GetAllWidgets(Widgets);

        int32 TextCount = 0;
        int32 ImageCount = 0;
        int32 ButtonCount = 0;
        int32 ScrollCount = 0;

        for (UWidget* Widget : Widgets)
        {
            if (!Widget) continue;

            const FString Name = Widget->GetName();

            if (Widget->IsA<UTextBlock>()) ++TextCount;
            if (Widget->IsA<UImage>()) ++ImageCount;
            if (Widget->IsA<UButton>()) ++ButtonCount;
            if (Widget->IsA<UScrollBox>()) ++ScrollCount;

            UE_LOG(
                LogTemp,
                Display,
                TEXT("[BINGO AUDIT] %s | %s"),
                *Name,
                *Widget->GetClass()->GetName()
            );
        }

        UE_LOG(LogTemp, Warning, TEXT("=== RESULTATS ==="));
        UE_LOG(LogTemp, Warning, TEXT("Total widgets : %d"), Widgets.Num());
        UE_LOG(LogTemp, Warning, TEXT("TextBlock : %d"), TextCount);
        UE_LOG(LogTemp, Warning, TEXT("Image : %d"), ImageCount);
        UE_LOG(LogTemp, Warning, TEXT("Button : %d"), ButtonCount);
        UE_LOG(LogTemp, Warning, TEXT("ScrollBox : %d"), ScrollCount);
    }

    static FAutoConsoleCommand AuditCommand(
        TEXT("Bingo.AuditFigma"),
        TEXT("Auditer le widget Figma Bingo V3"),
        FConsoleCommandDelegate::CreateStatic(&Audit)
    );
}