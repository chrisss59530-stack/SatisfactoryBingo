#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BingoRadarScanner.generated.h"

UENUM(BlueprintType)
enum class EBingoRadarTarget : uint8
{
    HardDrive,
    MercerSphere,
    Somersloop
};

USTRUCT(BlueprintType)
struct FBingoRadarResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EBingoRadarTarget Type = EBingoRadarTarget::HardDrive;

    UPROPERTY(BlueprintReadOnly)
    FVector Location = FVector::ZeroVector;
};

UCLASS()
class SATISFACTORYBINGO_API UBingoRadarScanner : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Bingo|Radar",
        meta = (WorldContext = "WorldContextObject"))
    static TArray<FBingoRadarResult> ScanWorld(
        UObject* WorldContextObject);
};
