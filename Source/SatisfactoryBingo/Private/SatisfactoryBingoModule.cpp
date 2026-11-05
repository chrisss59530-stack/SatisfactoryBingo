#include "SatisfactoryBingoModule.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"




#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"





#include "Modules/ModuleManager.h"

#if !WITH_EDITOR
#include "Patching/NativeHookManager.h"
#include "BingoSubsystem.h"
#include "FGStatisticsSubsystem.h"
#include "FGBlueprintSubsystem.h"
#include "FGFactoryBlueprintTypes.h"
#include "Buildables/FGBuildableBlueprintDesigner.h"
#include "Hologram/FGBuildableHologram.h"
#include "FGCharacterBase.h"
#include "FGCharacterPlayer.h"
#include "FGHealthComponent.h"
#include "Equipment/FGJetPack.h"
#include "Equipment/FGParachute.h"
#include "Equipment/FGHoverPack.h"
#include "Resources/FGItemDescriptor.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Async/Async.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogSatisfactoryBingo, Log, All);


// BINGO_PROTON_DEBUG_V1
static void BingoProtonTrace(const TCHAR* Message)
{
    const FString LogDir =
        FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Logs"));

    IFileManager::Get().MakeDirectory(*LogDir, true);

    const FString LogFile =
        FPaths::Combine(LogDir, TEXT("Bingo-Proton-Debug.txt"));

    const FString Line = FString::Printf(
        TEXT("%s | %s\n"),
        *FDateTime::UtcNow().ToString(),
        Message
    );

    FFileHelper::SaveStringToFile(
        Line,
        *LogFile,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
        &IFileManager::Get(),
        FILEWRITE_Append
    );

    UE_LOG(LogSatisfactoryBingo, Warning,
        TEXT("[BINGO_PROTON_DEBUG] %s"), Message);
}
#if !WITH_EDITOR
namespace
{
    ABingoSubsystem* FindBingoObserver(const UObject* Context)
    {
        UWorld* World = Context ? Context->GetWorld() : nullptr;
        if (!World || !IsInGameThread()) return nullptr;
        for (TActorIterator<ABingoSubsystem> It(World); It; ++It)
            if (IsValid(*It) && It->HasAuthority()) return *It;
        return nullptr;
    }
}
#endif

void FSatisfactoryBingoModule::StartupModule()
{
    // BINGO_OPTIONAL_HOOK_DIAGNOSTIC_V1
    if (FParse::Param(FCommandLine::Get(), TEXT("BingoSkipNativeHooks")))
    {
        BingoProtonTrace(TEXT("DIAGNOSTIC: native hooks skipped by launch option"));
        return;
    }
    BingoProtonTrace(TEXT("STARTUP BEGIN"));
    UE_LOG(LogSatisfactoryBingo, Log, TEXT("Satisfactory Bingo module started"));

#if !WITH_EDITOR
    // Production statistics hook disabled: incompatible with CL517554.

    // Observation only: each scope call executes the vanilla operation once.
    // Neither its parameters nor its return value are overridden.
        BingoProtonTrace(TEXT("BEFORE SaveBlueprint"));
    SUBSCRIBE_METHOD(AFGBuildableBlueprintDesigner::SaveBlueprint,
        ([](auto& Scope, AFGBuildableBlueprintDesigner* Designer, FBlueprintRecord Record,
            AFGPlayerController* Controller, bool bUpdateLastEditedBy)
        {
            ABingoSubsystem* Observer = FindBingoObserver(Designer);
            if (Observer) Observer->BingoBeginBlueprintSave(Controller);
            Scope(Designer, Record, Controller, bUpdateLastEditedBy);
            if (IsValid(Observer)) Observer->BingoEndBlueprintSave();
        }));
    BingoProtonTrace(TEXT("AFTER SaveBlueprint"));

        BingoProtonTrace(TEXT("BEFORE WriteBlueprintToDisk"));
    SUBSCRIBE_METHOD(AFGBlueprintSubsystem::WriteBlueprintToDisk,
        ([](auto& Scope, AFGBlueprintSubsystem* BlueprintSystem, const FBlueprintRecord& Record)
        {
            const bool bWritten = Scope(BlueprintSystem, Record);
            if (bWritten)
                if (ABingoSubsystem* Observer = FindBingoObserver(BlueprintSystem))
                    Observer->BingoConfirmBlueprintWrite();
        }));
    BingoProtonTrace(TEXT("AFTER WriteBlueprintToDisk"));

        BingoProtonTrace(TEXT("BEFORE Construct"));
    SUBSCRIBE_METHOD_VIRTUAL(AFGBuildableHologram::Construct, GetMutableDefault<AFGBuildableHologram>(),
        ([](auto& Scope, AFGBuildableHologram* Hologram, TArray<AActor*>& Children, FNetConstructionID ConstructionId)
        {
            AActor* Old = Hologram->GetUpgradedActor();
            const bool bWasMk3 = IsValid(Old) && Old->GetClass()->GetName().Equals(
                TEXT("Build_ConveyorBeltMk3_C"), ESearchCase::IgnoreCase);
            AActor* Result = Scope(Hologram, Children, ConstructionId);
            if (bWasMk3 && IsValid(Result) && Result->GetClass()->GetName().Equals(
                TEXT("Build_ConveyorBeltMk5_C"), ESearchCase::IgnoreCase))
                if (ABingoSubsystem* Observer = FindBingoObserver(Hologram))
                    Observer->BingoRecordConveyorUpgrade();
        }));
    BingoProtonTrace(TEXT("AFTER Construct"));

        BingoProtonTrace(TEXT("BEFORE Landed"));
    SUBSCRIBE_METHOD_VIRTUAL(AFGCharacterBase::Landed, GetMutableDefault<AFGCharacterPlayer>(),
        ([](auto& Scope, AFGCharacterBase* Character, const FHitResult& Hit)
        {
            AFGCharacterPlayer* Player = Cast<AFGCharacterPlayer>(Character);
            bool bJetOrParachute = false, bHover = false;
            UFGHealthComponent* Health = IsValid(Player) ? Player->GetHealthComponent() : nullptr;
            const float Before = IsValid(Health) ? Health->GetCurrentHealth() : 0.f;
            if (IsValid(Player) && Player->HasAuthority())
                for (AFGEquipment* Equipment : Player->GetActiveEquipments())
                {
                    if (const AFGJetPack* Jet = Cast<AFGJetPack>(Equipment)) bJetOrParachute |= Jet->IsThrusting();
                    if (const AFGParachute* Chute = Cast<AFGParachute>(Equipment)) bJetOrParachute |= Chute->IsDeployed();
                    if (const AFGHoverPack* Hover = Cast<AFGHoverPack>(Equipment)) bHover |= Hover->IsHoverActive();
                }
            Scope(Character, Hit);
            if (Before > 0.f && IsValid(Health) && Health->GetCurrentHealth() >= Before && (bJetOrParachute || bHover))
                if (ABingoSubsystem* Observer = FindBingoObserver(Player))
                    Observer->BingoRecordSafeLanding(Player, bJetOrParachute, bHover);
        }));
    BingoProtonTrace(TEXT("AFTER Landed"));
#endif
}

void FSatisfactoryBingoModule::ShutdownModule()
{
    UE_LOG(LogSatisfactoryBingo, Log, TEXT("Satisfactory Bingo module stopped"));
}

IMPLEMENT_MODULE(FSatisfactoryBingoModule, SatisfactoryBingo);
